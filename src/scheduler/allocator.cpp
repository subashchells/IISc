#include "allocator.h"
#include "policies.h"
#include "strategies.h"
#include "topological.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

using namespace std;
static int legacy_priority_of(int task_priority, Acuity acuity) {
    return static_cast<int>(acuity) * 10 + task_priority;
}
static LexKey build_lex_key(const EngineConfig& cfg, const Task& task,
                            Acuity acuity, int age) {
    if (cfg.policy) return cfg.policy(task, acuity, age, cfg.policy_seed);
    return default_lex_policy(task, acuity, age, cfg.policy_seed);
}
static void record_violation(CapacityReport* report,
                             const Task& task, const Worker& worker,
                             const string& patient_id,
                             int shift_remaining_before) {
    if (!report) return;
    int billable_shift  = max(0, shift_remaining_before);
    int overtime_added  = task.duration_minutes - billable_shift;
    if (overtime_added < 0) overtime_added = 0;
    report->violations.push_back({
        task.id, task.name, worker.id, worker.name, patient_id,
        shift_remaining_before, task.duration_minutes, overtime_added,
    });
    report->over_capacity_task_count++;
}
static void finalise_capacity_report(CapacityReport* report,
                                     const vector<Worker>& workers) {
    if (!report) return;
    int total_ot = 0;
    for (const auto& w : workers) {
        int ot = -min(0, w.shift_minutes_left);
        if (ot > 0) {
            report->per_worker_overtime[w.id] = ot;
            total_ot += ot;
        }
    }
    report->total_overtime_minutes = total_ot;
    report->capacity_feasible      = (total_ot <= report->threshold_minutes);
}
static void finalise_engine_metrics(EngineMetrics* metrics,
                                    const vector<ScheduleEntry>& schedule,
                                    const vector<Worker>& workers) {
    if (!metrics) return;

    int makespan = 0;
    for (const auto& e : schedule)
        if (e.finish_time > makespan) makespan = e.finish_time;
    metrics->makespan_minutes = makespan;

    // Stdev includes zero-OT workers — fairness across the whole roster.
    vector<double> ot_each;
    ot_each.reserve(workers.size());
    int max_ot = 0;
    for (const auto& w : workers) {
        int ot = -min(0, w.shift_minutes_left);
        if (ot > max_ot) max_ot = ot;
        ot_each.push_back(static_cast<double>(ot));
    }
    metrics->max_per_worker_overtime = max_ot;

    if (ot_each.empty()) {
        metrics->stdev_overtime_minutes = 0.0;
    } else {
        double sum = 0.0;
        for (double v : ot_each) sum += v;
        const double mean = sum / static_cast<double>(ot_each.size());
        double sq = 0.0;
        for (double v : ot_each) { const double d = v - mean; sq += d * d; }
        metrics->stdev_overtime_minutes = sqrt(sq / static_cast<double>(ot_each.size()));
    }
}
vector<ScheduleEntry> run_engine(
    vector<Patient>&                     patients,
    vector<Worker>&                      workers,
    unordered_map<string, Task>&         tasks,
    const DirectedGraph&                 dep_graph,
    const unordered_map<string, string>& task_to_patient,
    EngineConfig                         config,
    vector<UnscheduledTask>*             unscheduled,
    CapacityReport*                      capacity_report,
    EngineMetrics*                       metrics)
{
    auto t_start = chrono::steady_clock::now();

    config.matching->prepare(workers);

    unordered_map<string, Acuity> patient_acuity;
    unordered_map<string, int>    patient_age;
    for (const auto& p : patients) {
        patient_acuity[p.id] = p.acuity;
        patient_age[p.id]    = p.age;
    }

    auto in_deg = dep_graph.in_degrees();

    // Refuse to schedule on cyclic dep data.
    try {
        (void)topological_sort(dep_graph);
    } catch (const std::runtime_error& e) {
        if (unscheduled) unscheduled->push_back({"", "", "", e.what()});
        return {};
    }

    // Pre-pass: unlock dependents of already-DONE tasks at T+0.
    for (const auto& [task_id, task] : tasks) {
        if (task.status != Status::DONE) continue;
        if (!dep_graph.has_node(task_id)) continue;
        for (const auto& next_id : dep_graph.neighbors(task_id))
            if (in_deg.count(next_id)) in_deg[next_id]--;
    }

    auto make_ready = [&](Task* t, const string& pid) {
        const Acuity a   = patient_acuity[pid];
        const int    age = patient_age.count(pid) ? patient_age.at(pid) : 0;
        ReadyTask r;
        r.task            = t;
        r.patient_id      = pid;
        r.legacy_priority = legacy_priority_of(t->priority, a);
        r.key             = build_lex_key(config, *t, a, age);
        return r;
    };

    vector<ReadyTask> roots;
    for (auto& [task_id, task] : tasks) {
        if (task.status != Status::PENDING) continue;
        if (in_deg.count(task_id) && in_deg[task_id] > 0) continue;
        auto pit = task_to_patient.find(task_id);
        if (pit == task_to_patient.end()) continue;
        roots.push_back(make_ready(&task, pit->second));
    }
    config.ordering->seed(roots);

    vector<ScheduleEntry> schedule;
    int tick_count      = 0;
    int deferred_events = 0;
    const size_t batch_cap = max<size_t>(1, workers.size());

    while (!config.ordering->empty()) {
        vector<ReadyTask> batch = config.ordering->drain_up_to(batch_cap);
        if (batch.empty()) break;

        vector<MatchResult> results = config.matching->match(batch, workers);
        ++tick_count;

        for (const auto& res : results) {
            if (res.task_idx < 0 ||
                static_cast<size_t>(res.task_idx) >= batch.size()) continue;
            ReadyTask& rt   = batch[res.task_idx];
            Task&      task = *rt.task;

            if (res.outcome == MatchOutcome::NO_ROLE_MATCH) {
                if (unscheduled)
                    unscheduled->push_back({task.id, task.name, rt.patient_id,
                                            "no worker with role on roster"});
                continue;
            }
            if (res.outcome == MatchOutcome::DEFERRED) {
                config.ordering->push(rt);
                ++deferred_events;
                continue;
            }
            if (res.worker_idx < 0 ||
                static_cast<size_t>(res.worker_idx) >= workers.size()) continue;
            Worker& w = workers[res.worker_idx];

            const int start        = w.available_at;
            const int finish       = start + task.duration_minutes;
            const int shift_before = w.shift_minutes_left;
            const bool over        = shift_before < task.duration_minutes;

            schedule.push_back({task.id, task.name, w.id, w.name,
                                rt.patient_id, start, finish,
                                rt.legacy_priority, over});

            if (over)
                record_violation(capacity_report, task, w,
                                 rt.patient_id, shift_before);

            task.status            = Status::DONE;
            task.assigned_worker   = w.id;
            w.available_at         = finish;
            w.shift_minutes_left  -= task.duration_minutes;

            // Sorted neighbours → deterministic order across runs.
            if (dep_graph.has_node(task.id)) {
                vector<string> next_ids = dep_graph.neighbors(task.id);
                sort(next_ids.begin(), next_ids.end());
                for (const auto& next_id : next_ids) {
                    auto it = in_deg.find(next_id);
                    if (it == in_deg.end()) continue;
                    if (it->second > 0) it->second--;
                    if (it->second != 0) continue;

                    auto t_it = tasks.find(next_id);
                    if (t_it == tasks.end()) continue;
                    if (t_it->second.status != Status::PENDING) continue;

                    auto p_it = task_to_patient.find(next_id);
                    if (p_it == task_to_patient.end()) continue;

                    config.ordering->push(make_ready(&t_it->second, p_it->second));
                }
            }
        }
    }

    finalise_capacity_report(capacity_report, workers);
    if (metrics) {
        auto t_end = chrono::steady_clock::now();
        metrics->scheduler_runtime_ns =
            chrono::duration_cast<chrono::nanoseconds>(t_end - t_start).count();
        metrics->tick_count           = tick_count;
        metrics->deferred_event_count = deferred_events;
        finalise_engine_metrics(metrics, schedule, workers);
    }
    return schedule;
}

// ── Backward-compatible wrappers ─────────────────────────────────────────────

vector<ScheduleEntry> run_simulation(
    vector<Patient>&                     patients,
    vector<Worker>&                      workers,
    unordered_map<string, Task>&         tasks,
    const DirectedGraph&                 dep_graph,
    const unordered_map<string, string>& task_to_patient,
    vector<UnscheduledTask>*             unscheduled,
    CapacityReport*                      capacity_report)
{
    PriorityOrdering ordering;
    GreedyMatcher    matcher;
    EngineConfig     cfg;
    cfg.ordering = &ordering;
    cfg.matching = &matcher;
    cfg.label    = "Priority×Greedy";
    return run_engine(patients, workers, tasks, dep_graph, task_to_patient,
                      cfg, unscheduled, capacity_report, nullptr);
}

vector<ScheduleEntry> run_simulation_fcfs(
    vector<Patient>&                     patients,
    vector<Worker>&                      workers,
    unordered_map<string, Task>&         tasks,
    const DirectedGraph&                 dep_graph,
    const unordered_map<string, string>& task_to_patient,
    vector<UnscheduledTask>*             unscheduled,
    CapacityReport*                      capacity_report)
{
    FcfsOrdering  ordering;
    GreedyMatcher matcher;
    EngineConfig  cfg;
    cfg.ordering = &ordering;
    cfg.matching = &matcher;
    cfg.label    = "FCFS×Greedy";
    return run_engine(patients, workers, tasks, dep_graph, task_to_patient,
                      cfg, unscheduled, capacity_report, nullptr);
}
