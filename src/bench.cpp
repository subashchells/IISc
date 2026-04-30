#include "bench.h"
#include "scheduler/allocator.h"
#include "scheduler/policies.h"
#include "scheduler/strategies.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <unordered_set>

using namespace std;

static string acuity_label(Acuity a) {
  switch (a) {
  case Acuity::CRITICAL: return "CRITICAL";
  case Acuity::HIGH:     return "HIGH";
  case Acuity::MEDIUM:   return "MEDIUM";
  case Acuity::LOW:      return "LOW";
  }
  return "?";
}

static string role_label(Role r) {
  switch (r) {
  case Role::PGY1:   return "PGY1";
  case Role::INTERN: return "Intern";
  case Role::NURSE:  return "Nurse";
  }
  return "?";
}

// ── Schedule / capacity printers ─────────────────────────────────────────────

static void print_schedule(
    const vector<ScheduleEntry> &schedule,
    const unordered_map<string, Acuity> &patient_acuity,
    const unordered_map<string, string> &patient_name,
    const string &title) {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║  " << title << "\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";

  auto sorted = schedule;
  sort(sorted.begin(), sorted.end(),
       [](const ScheduleEntry &a, const ScheduleEntry &b) {
         if (a.start_time != b.start_time) return a.start_time < b.start_time;
         return a.effective_priority < b.effective_priority;
       });

  for (const auto &e : sorted) {
    auto ait = patient_acuity.find(e.patient_id);
    auto nit = patient_name.find(e.patient_id);
    string ac = ait != patient_acuity.end() ? acuity_label(ait->second) : "?";
    string pn = nit != patient_name.end() ? nit->second : "?";
    cout << "┌─ [T+" << e.start_time << " min]  " << e.patient_id << " ("
         << ac << ") — " << pn << "\n";
    cout << "│  Task    : " << e.task_name << "\n";
    cout << "│  Worker  : " << e.worker_name
         << (e.over_capacity ? "   ⚠ OVER CAPACITY" : "") << "\n";
    cout << "│  Time    : T+" << e.start_time << " → T+" << e.finish_time
         << " min  (" << (e.finish_time - e.start_time) << " min)\n";
    cout << "│  Priority: " << e.effective_priority << "\n";
    cout << "└──────────────────────────────────────────────────────────\n";
  }
}

static void print_unscheduled(const vector<UnscheduledTask> &unscheduled) {
  if (unscheduled.empty()) return;
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n"
          "║  ⚠ Unscheduled Tasks (no qualified worker on roster)         ║\n"
          "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  These tasks could not be assigned. Every task that depends\n"
          "  on one of them was also blocked.\n\n";
  for (const auto &u : unscheduled) {
    cout << "  • " << u.task_id << " (" << u.task_name << ")"
         << "  patient=" << u.patient_id
         << "   — " << u.reason << "\n";
  }
}

// Per-patient first-start / last-finish; absent patients had no scheduled
// tasks and must NOT be folded into per-acuity averages as zeros.
struct PatientTiming {
  int first_start;
  int last_finish;
};

static unordered_map<string, PatientTiming> per_patient_timing(
    const vector<ScheduleEntry> &sched) {
  unordered_map<string, PatientTiming> out;
  for (const auto &e : sched) {
    auto it = out.find(e.patient_id);
    if (it == out.end()) {
      out[e.patient_id] = {e.start_time, e.finish_time};
    } else {
      it->second.first_start = min(it->second.first_start, e.start_time);
      it->second.last_finish = max(it->second.last_finish, e.finish_time);
    }
  }
  return out;
}

static constexpr int kCapacityTopK = 5;

static void print_capacity_report(const CapacityReport &rep,
                                  const string &label) {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║  Capacity Report — " << left << setw(42) << label << "║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";

  cout << "  threshold       : " << rep.threshold_minutes << " min\n";
  cout << "  total overtime  : " << rep.total_overtime_minutes << " min\n";
  cout << "  breached tasks  : " << rep.over_capacity_task_count << "\n";
  cout << "  workers in OT   : " << rep.per_worker_overtime.size() << "\n";

  if (rep.capacity_feasible) {
    cout << "  status          : ✓ feasible (overtime within threshold)\n";
    return;
  }

  cout << "\n  ╔══════════════════════════════════════════════════════════╗\n"
          "  ║  ⚠ SCHEDULE INFEASIBLE — overtime exceeds threshold      ║\n"
          "  ╚══════════════════════════════════════════════════════════╝\n";
  cout << "  The schedule above lists every task as 'placed', but executing\n"
          "  it would require " << rep.total_overtime_minutes
       << " minutes of unbudgeted overtime\n"
          "  across " << rep.per_worker_overtime.size()
       << " worker(s). Add staff, extend shifts, or drop work.\n";

  vector<pair<string, int>> by_worker(rep.per_worker_overtime.begin(),
                                      rep.per_worker_overtime.end());
  sort(by_worker.begin(), by_worker.end(),
       [](const pair<string,int>& a, const pair<string,int>& b) {
         return a.second > b.second;
       });
  cout << "\n  Worst overtime by worker:\n";
  int n_show = min<int>(by_worker.size(), kCapacityTopK);
  for (int i = 0; i < n_show; i++) {
    cout << "    • " << by_worker[i].first
         << "  +" << by_worker[i].second << " min over shift\n";
  }
  if ((int)by_worker.size() > n_show)
    cout << "    (... " << (by_worker.size() - n_show) << " more)\n";

  auto by_task = rep.violations;
  sort(by_task.begin(), by_task.end(),
       [](const CapacityViolation& a, const CapacityViolation& b) {
         return a.overtime_added > b.overtime_added;
       });
  cout << "\n  Worst breached assignments:\n";
  n_show = min<int>(by_task.size(), kCapacityTopK);
  for (int i = 0; i < n_show; i++) {
    const auto &v = by_task[i];
    cout << "    • " << v.task_id << " (" << v.task_name << ")"
         << "  worker=" << v.worker_name
         << "  +" << v.overtime_added << " min OT"
         << "  (shift left before: " << v.shift_remaining_before
         << " min, task: " << v.task_duration << " min)\n";
  }
  if ((int)by_task.size() > n_show)
    cout << "    (... " << (by_task.size() - n_show) << " more)\n";
}

struct GroupStat { int n_total; int n_scheduled; float first_mean; float last_mean; };

static GroupStat compute_group(
    const vector<Patient> &patients,
    const unordered_map<string, PatientTiming> &timing,
    bool critical_or_high) {
  GroupStat g{0, 0, 0.0f, 0.0f};
  long sum_first = 0, sum_last = 0;
  for (const auto &p : patients) {
    bool is_crit = (p.acuity == Acuity::CRITICAL || p.acuity == Acuity::HIGH);
    if (is_crit != critical_or_high) continue;
    g.n_total++;
    auto it = timing.find(p.id);
    if (it == timing.end()) continue;
    g.n_scheduled++;
    sum_first += it->second.first_start;
    sum_last  += it->second.last_finish;
  }
  if (g.n_scheduled > 0) {
    g.first_mean = static_cast<float>(sum_first) / g.n_scheduled;
    g.last_mean  = static_cast<float>(sum_last)  / g.n_scheduled;
  }
  return g;
}

static void print_benchmark(
    const vector<ScheduleEntry> &sched_prio,
    const vector<ScheduleEntry> &sched_fcfs,
    const CapacityReport &cap_prio,
    const CapacityReport &cap_fcfs,
    const vector<UnscheduledTask> &missed_prio,
    const vector<UnscheduledTask> &missed_fcfs,
    int total_tasks,
    const vector<Patient> &patients) {
  auto timing_p = per_patient_timing(sched_prio);
  auto timing_f = per_patient_timing(sched_fcfs);

  GroupStat crit_p = compute_group(patients, timing_p, true);
  GroupStat crit_f = compute_group(patients, timing_f, true);
  GroupStat med_p  = compute_group(patients, timing_p, false);
  GroupStat med_f  = compute_group(patients, timing_f, false);

  int incomplete_p = 0, incomplete_f = 0;
  for (const auto &p : patients) {
    if (timing_p.find(p.id) == timing_p.end()) incomplete_p++;
    if (timing_f.find(p.id) == timing_f.end()) incomplete_f++;
  }

  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║               FCFS vs Priority Benchmarking                  ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Both schedulers respect task dependencies. They differ only in\n"
          "  ready-task ordering: FCFS = FIFO, Priority = acuity-weighted heap.\n"
          "  Per-acuity averages exclude patients with zero scheduled tasks.\n\n";

  cout << fixed << setprecision(1);
  auto report = [&](const string &label, const GroupStat &p, const GroupStat &f) {
    cout << "  [" << label << "]\n";
    cout << "    patients (total / scheduled under Priority / under FCFS): "
         << p.n_total << " / " << p.n_scheduled << " / " << f.n_scheduled << "\n";
    if (p.n_scheduled > 0 && f.n_scheduled > 0) {
      cout << "    first task start  — FCFS: " << f.first_mean
           << " min   Priority: " << p.first_mean << " min\n";
      cout << "    last task finish  — FCFS: " << f.last_mean
           << " min   Priority: " << p.last_mean << " min\n";
    } else {
      cout << "    (insufficient scheduled patients to compare)\n";
    }
    cout << "\n";
  };
  if (crit_p.n_total > 0) report("CRITICAL/HIGH", crit_p, crit_f);
  if (med_p.n_total  > 0) report("MEDIUM/LOW",    med_p,  med_f);

  cout << "  [Roster utilisation]\n";
  cout << "    overtime minutes      — FCFS: " << cap_fcfs.total_overtime_minutes
       << "       Priority: " << cap_prio.total_overtime_minutes << "\n";
  cout << "    over-capacity tasks   — FCFS: " << cap_fcfs.over_capacity_task_count
       << "         Priority: " << cap_prio.over_capacity_task_count << "\n\n";

  cout << "  [Coverage]\n";
  cout << "    scheduled / total tasks — FCFS: " << sched_fcfs.size()
       << " / " << total_tasks
       << "    Priority: " << sched_prio.size() << " / " << total_tasks << "\n";
  cout << "    unscheduled tasks       — FCFS: " << missed_fcfs.size()
       << "             Priority: " << missed_prio.size() << "\n";
  cout << "    incomplete patients     — FCFS: " << incomplete_f
       << "             Priority: " << incomplete_p << "\n\n";

  cout << "  [Feasibility @ threshold=" << cap_prio.threshold_minutes << " min]\n";
  cout << "    FCFS    : " << (cap_fcfs.capacity_feasible ? "✓ feasible" : "⚠ INFEASIBLE")
       << "   (overtime " << cap_fcfs.total_overtime_minutes << " min)\n";
  cout << "    Priority: " << (cap_prio.capacity_feasible ? "✓ feasible" : "⚠ INFEASIBLE")
       << "   (overtime " << cap_prio.total_overtime_minutes << " min)\n";

  if (!cap_prio.capacity_feasible || !cap_fcfs.capacity_feasible) {
    cout << "\n  Headline: priority scheduling re-orders the same set of\n"
            "  tasks; it does not create capacity. The current roster cannot\n"
            "  cover this workload under either policy without overtime.\n";
  }
  cout << defaultfloat << "\n";
}

static void print_worker_summary(
    const vector<Worker> &workers,
    const vector<ScheduleEntry> &schedule,
    const string &label) {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║              Worker Load Summary (" << left << setw(26) << label << ")║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  for (const auto &w : workers) {
    int load = 0;
    for (const auto &e : schedule)
      if (e.worker_id == w.id) load += (e.finish_time - e.start_time);
    cout << "  " << w.name << " (" << role_label(w.role) << ")"
         << "  total load: " << load << " min"
         << "   shift remaining: " << w.shift_minutes_left << " min"
         << (w.shift_minutes_left < 0 ? "  ⚠ OVERTIME" : "") << "\n";
  }
}

// ── 2×2 matrix bench ─────────────────────────────────────────────────────────

struct CellResult {
  string                  cell_id;
  string                  ordering_name;
  string                  matching_name;
  vector<ScheduleEntry>   schedule;
  vector<UnscheduledTask> unscheduled;
  CapacityReport          capacity;
  EngineMetrics           metrics;
  size_t                  total_tasks = 0;
};

static CellResult run_one_cell(
    const string &cell_id,
    OrderingStrategy &ordering,
    MatchingStrategy &matching,
    const WardData &ward,
    const unordered_map<string, Task> &tasks,
    const DirectedGraph &dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int overtime_threshold_minutes) {
  CellResult r;
  r.cell_id        = cell_id;
  r.ordering_name  = ordering.name();
  r.matching_name  = matching.name();
  r.total_tasks    = tasks.size();
  r.capacity.threshold_minutes = overtime_threshold_minutes;

  auto t   = tasks;
  auto w   = ward.workers;
  auto pts = ward.patients;

  EngineConfig cfg;
  cfg.ordering = &ordering;
  cfg.matching = &matching;
  cfg.label    = cell_id;
  r.schedule = run_engine(pts, w, t, dep_graph, task_to_patient,
                          cfg, &r.unscheduled, &r.capacity, &r.metrics);
  return r;
}

static void print_cell_summary(const CellResult &r, const string &header) {
  cout << "\n  " << r.cell_id << " — " << header << "\n";
  cout << "      ordering         : " << r.ordering_name << "\n";
  cout << "      matching         : " << r.matching_name << "\n";
  cout << "      makespan         : " << r.metrics.makespan_minutes << " min\n";
  cout << "      total overtime   : " << r.capacity.total_overtime_minutes
       << " min   (max worker: "    << r.metrics.max_per_worker_overtime
       << ", stdev: "                 << fixed << setprecision(1)
       << r.metrics.stdev_overtime_minutes << ")\n";
  cout << defaultfloat;
  cout << "      capacity breach  : " << r.capacity.over_capacity_task_count
       << " task(s)\n";
  cout << "      runtime          : " << fixed << setprecision(3)
       << (r.metrics.scheduler_runtime_ns / 1000.0) << " µs"
       << "   (" << r.metrics.tick_count << " ticks";
  if (r.metrics.deferred_event_count > 0)
    cout << ", " << r.metrics.deferred_event_count << " re-queues";
  cout << ")\n" << defaultfloat;
  cout << "      coverage         : " << r.schedule.size() << " / "
       << r.total_tasks << " tasks ("
       << r.unscheduled.size() << " unscheduled)\n";
}

static void print_2x2_matrix(const CellResult &A, const CellResult &B,
                             const CellResult &C, const CellResult &D) {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║              2×2 Strategy Matrix Benchmark                   ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Same input across all four cells. Ordering decides the BATCH;\n"
          "  Matching decides WHO does each task in the batch.\n\n";

  cout << "                       Greedy match           Hungarian match\n";
  cout << "  ─────────────────────────────────────────────────────────────\n";
  cout << "  FCFS  ordering   "
       << "  ms=" << setw(4) << A.metrics.makespan_minutes
       << "  ot=" << setw(4) << A.capacity.total_overtime_minutes
       << "       "
       << "  ms=" << setw(4) << B.metrics.makespan_minutes
       << "  ot=" << setw(4) << B.capacity.total_overtime_minutes << "\n";
  cout << "  Priority order   "
       << "  ms=" << setw(4) << C.metrics.makespan_minutes
       << "  ot=" << setw(4) << C.capacity.total_overtime_minutes
       << "       "
       << "  ms=" << setw(4) << D.metrics.makespan_minutes
       << "  ot=" << setw(4) << D.capacity.total_overtime_minutes << "\n";

  print_cell_summary(A, "FCFS × Greedy        (current FCFS baseline)");
  print_cell_summary(B, "FCFS × Hungarian     (matching effect alone)");
  print_cell_summary(C, "Priority × Greedy    (current main scheduler)");
  print_cell_summary(D, "Priority × Hungarian (best of both)");

  auto dms = [](const CellResult &lo, const CellResult &hi) {
    return hi.metrics.makespan_minutes - lo.metrics.makespan_minutes;
  };
  auto dot = [](const CellResult &lo, const CellResult &hi) {
    return hi.capacity.total_overtime_minutes - lo.capacity.total_overtime_minutes;
  };
  auto dbreach = [](const CellResult &lo, const CellResult &hi) {
    return hi.capacity.over_capacity_task_count - lo.capacity.over_capacity_task_count;
  };

  cout << "\n  ── Pairwise comparisons (Δ = right − left) ─────────────────\n";
  cout << "  A→D  (worst → best)              Δ makespan = " << dms(A, D)
       << " min,  Δ overtime = " << dot(A, D) << " min,"
       << "  Δ breaches = " << dbreach(A, D) << "\n";
  cout << "  A→B  (ordering held FCFS)        Δ makespan = " << dms(A, B)
       << " min,  Δ overtime = " << dot(A, B) << " min     ← matching effect alone\n";
  cout << "  A→C  (matching held Greedy)      Δ makespan = " << dms(A, C)
       << " min,  Δ overtime = " << dot(A, C) << " min     ← ordering effect alone\n";
  cout << "  B→D  (ordering effect under H.)  Δ makespan = " << dms(B, D)
       << " min,  Δ overtime = " << dot(B, D) << " min     ← does priority still help?\n";
  cout << "  C→D  (Hungarian over Priority)   Δ makespan = " << dms(C, D)
       << " min,  Δ overtime = " << dot(C, D) << " min     ← worth the complexity?\n";

  cout << "\n  ── Scheduler wall-clock (lower = cheaper) ──────────────────\n";
  auto us = [](long long ns) { return ns / 1000.0; };
  cout << fixed << setprecision(3);
  cout << "  A " << setw(8) << us(A.metrics.scheduler_runtime_ns) << " µs"
       << "    B " << setw(8) << us(B.metrics.scheduler_runtime_ns) << " µs"
       << "    C " << setw(8) << us(C.metrics.scheduler_runtime_ns) << " µs"
       << "    D " << setw(8) << us(D.metrics.scheduler_runtime_ns) << " µs\n";
  cout << defaultfloat;
}

void run_2x2_bench(
    const WardData &ward,
    const unordered_map<string, Task> &tasks,
    const DirectedGraph &dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int overtime_threshold_minutes) {
  // Each cell needs its own strategy objects — orderings hold per-run state.
  FcfsOrdering     ord_a;  GreedyMatcher    mat_a;
  FcfsOrdering     ord_b;  HungarianMatcher mat_b;
  PriorityOrdering ord_c;  GreedyMatcher    mat_c;
  PriorityOrdering ord_d;  HungarianMatcher mat_d;

  CellResult A = run_one_cell("A", ord_a, mat_a, ward, tasks, dep_graph,
                              task_to_patient, overtime_threshold_minutes);
  CellResult B = run_one_cell("B", ord_b, mat_b, ward, tasks, dep_graph,
                              task_to_patient, overtime_threshold_minutes);
  CellResult C = run_one_cell("C", ord_c, mat_c, ward, tasks, dep_graph,
                              task_to_patient, overtime_threshold_minutes);
  CellResult D = run_one_cell("D", ord_d, mat_d, ward, tasks, dep_graph,
                              task_to_patient, overtime_threshold_minutes);
  print_2x2_matrix(A, B, C, D);
}

// ── Pareto sweep ─────────────────────────────────────────────────────────────

struct Candidate {
  string                cell_id;
  string                policy_name;
  uint32_t              seed;
  Objectives            obj;
  vector<ScheduleEntry> schedule;
  CapacityReport        capacity;
  EngineMetrics         metrics;
  size_t                total_tasks = 0;
};

static Candidate run_one_candidate(
    int                                  policy_idx,
    const NamedPolicy &                  policy,
    uint32_t                             seed,
    const WardData &                     ward,
    const unordered_map<string, Task> &  tasks,
    const DirectedGraph &                dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int                                  overtime_threshold_minutes) {
  Candidate c;
  ostringstream id;
  id << "P" << (policy_idx + 1) << ".s" << seed;
  c.cell_id     = id.str();
  c.policy_name = policy.name;
  c.seed        = seed;
  c.total_tasks = tasks.size();

  // Sweep holds Priority × Greedy; LexPolicy is the variable.
  PriorityOrdering ordering;
  GreedyMatcher    matcher;
  EngineConfig     cfg;
  cfg.ordering    = &ordering;
  cfg.matching    = &matcher;
  cfg.policy      = policy.fn;
  cfg.policy_seed = seed;
  cfg.label       = c.cell_id;

  auto t   = tasks;
  auto w   = ward.workers;
  auto pts = ward.patients;
  CapacityReport cap;
  cap.threshold_minutes = overtime_threshold_minutes;
  vector<UnscheduledTask> missed;

  c.schedule = run_engine(pts, w, t, dep_graph, task_to_patient,
                          cfg, &missed, &cap, &c.metrics);
  c.capacity = std::move(cap);
  c.obj      = compute_objectives(c.schedule, ward.patients, w, c.capacity);
  return c;
}

// Per-axis-extreme tags for the surfaced picks.
static vector<string> annotate_corners(const vector<Candidate> &cands,
                                       const vector<int> &picks) {
  vector<string> tags(picks.size(), "");
  if (picks.empty()) return tags;
  auto best_on = [&](auto getter) {
    int best_i = picks.front();
    double best_v = getter(cands[best_i].obj);
    for (size_t k = 1; k < picks.size(); ++k) {
      const int idx = picks[k];
      const double v = getter(cands[idx].obj);
      if (v < best_v) { best_v = v; best_i = idx; }
    }
    return best_i;
  };
  const int best_acu = best_on([](const Objectives &o) { return o.acuity_metric; });
  const int best_ms  = best_on([](const Objectives &o) {
    return static_cast<double>(o.makespan_minutes); });
  const int best_ot  = best_on([](const Objectives &o) {
    return static_cast<double>(o.max_overtime_minutes); });

  for (size_t k = 0; k < picks.size(); ++k) {
    string tag;
    if (picks[k] == best_acu) tag = "best acuity";
    if (picks[k] == best_ms)  tag = tag.empty() ? "best makespan"  : tag + " / best makespan";
    if (picks[k] == best_ot)  tag = tag.empty() ? "lowest max-OT"  : tag + " / lowest max-OT";
    tags[k] = tag;
  }
  return tags;
}

// Per-policy best/median/worst across seeds — useful even when the strict
// frontier collapses to a single point.
struct PolicyRollup {
  string policy_name;
  double acu_best, acu_med, acu_worst;
  int    ms_best,  ms_med,  ms_worst;
  int    ot_best,  ot_med,  ot_worst;
  int    n_seeds;
};

static vector<PolicyRollup> rollup_by_policy(const vector<Candidate> &cands) {
  vector<string> order;
  unordered_map<string, vector<int>> bucket;
  for (size_t i = 0; i < cands.size(); ++i) {
    const string &name = cands[i].policy_name;
    if (!bucket.count(name)) order.push_back(name);
    bucket[name].push_back(static_cast<int>(i));
  }
  auto pick = [](vector<double> v, double frac) {
    sort(v.begin(), v.end());
    return v[min(static_cast<size_t>(v.size() * frac), v.size() - 1)];
  };
  auto picki = [](vector<int> v, double frac) {
    sort(v.begin(), v.end());
    return v[min(static_cast<size_t>(v.size() * frac), v.size() - 1)];
  };
  vector<PolicyRollup> out;
  out.reserve(order.size());
  for (const auto &name : order) {
    PolicyRollup r;
    r.policy_name = name;
    r.n_seeds     = static_cast<int>(bucket[name].size());
    vector<double> acu;
    vector<int>    ms, ot;
    for (int idx : bucket[name]) {
      acu.push_back(cands[idx].obj.acuity_metric);
      ms.push_back(cands[idx].obj.makespan_minutes);
      ot.push_back(cands[idx].obj.max_overtime_minutes);
    }
    r.acu_best  = pick(acu, 0.0);
    r.acu_med   = pick(acu, 0.5);
    r.acu_worst = pick(acu, 1.0);
    r.ms_best   = picki(ms, 0.0); r.ms_med = picki(ms, 0.5); r.ms_worst = picki(ms, 1.0);
    r.ot_best   = picki(ot, 0.0); r.ot_med = picki(ot, 0.5); r.ot_worst = picki(ot, 1.0);
    out.push_back(r);
  }
  return out;
}

static void print_pareto_frontier(const vector<Candidate> &cands,
                                  const vector<int> &frontier_idx,
                                  const vector<int> &picks) {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║   Pareto Frontier — multi-objective schedule selection       ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Candidates : " << cands.size()
       << "   Frontier : " << frontier_idx.size()
       << "   Surfaced : " << picks.size() << "\n";
  cout << "\n  Acuity   = mean last-finish (min) for CRITICAL+HIGH patients\n"
          "  Makespan = max finish_time (min) across the schedule\n"
          "  Max-OT   = worst single-worker overtime (min)\n"
          "  All three: smaller is better.\n";

  cout << "\n  ── Per-policy roll-up (best/median/worst across seeds) ────────────────\n";
  cout << "    policy                  acuity (best/med/worst)    ms       max-OT\n";
  cout << fixed << setprecision(1);
  for (const auto &r : rollup_by_policy(cands)) {
    cout << "    " << left << setw(22) << r.policy_name
         << " " << right << setw(7)  << r.acu_best
         << " / " << setw(7) << r.acu_med
         << " / " << setw(7) << r.acu_worst
         << "   " << setw(4)  << r.ms_best  << "/" << setw(4) << r.ms_worst
         << "   " << setw(4)  << r.ot_best  << "/" << setw(4) << r.ot_worst
         << "\n";
  }
  cout << defaultfloat;

  cout << "\n  ── Pareto frontier (Top-" << picks.size()
       << " by NSGA-II crowding distance) ──────────\n";
  cout << "   #   policy (seed)              acuity   makespan   max-OT   notes\n";
  cout << "   ──────────────────────────────────────────────────────────────────────\n";
  cout << fixed << setprecision(1);
  const auto tags = annotate_corners(cands, picks);
  for (size_t k = 0; k < picks.size(); ++k) {
    const Candidate &c = cands[picks[k]];
    ostringstream label;
    label << c.policy_name << " (s" << c.seed << ")";
    cout << "  [" << (k + 1) << "]  "
         << left  << setw(26) << label.str()
         << right << setw(8)  << c.obj.acuity_metric
         << "   "  << setw(7)  << c.obj.makespan_minutes
         << "    " << setw(6)  << c.obj.max_overtime_minutes;
    if (!tags[k].empty()) cout << "   ← " << tags[k];
    cout << "\n";
  }
  cout << defaultfloat;

  if (frontier_idx.size() == 1) {
    cout << "\n  Note: single-point frontier means one schedule dominates on all three\n"
            "  axes. On this dataset, makespan / max-OT are bottleneck-limited\n"
            "  (total work ≫ capacity); see the per-policy roll-up for the full picture.\n";
  }
}

void run_pareto_sweep(
    const WardData &                     ward,
    const unordered_map<string, Task> &  tasks,
    const DirectedGraph &                dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int                                  overtime_threshold_minutes) {
  const auto catalog = policy_catalog();
  const int  S       = kPolicySeedCount;
  vector<Candidate> cands;
  cands.reserve(catalog.size() * static_cast<size_t>(S));
  for (size_t i = 0; i < catalog.size(); ++i) {
    for (int s = 0; s < S; ++s) {
      cands.push_back(run_one_candidate(
          static_cast<int>(i), catalog[i], static_cast<uint32_t>(s),
          ward, tasks, dep_graph, task_to_patient,
          overtime_threshold_minutes));
    }
  }
  vector<Objectives> objs;
  objs.reserve(cands.size());
  for (const auto &c : cands) objs.push_back(c.obj);
  const auto frontier = pareto_frontier(objs);
  const auto picks    = top_k_by_crowding(objs, frontier, kFrontierTopK);
  print_pareto_frontier(cands, frontier, picks);
}

// ── Fairness sweep ───────────────────────────────────────────────────────────

struct FairnessRow {
  string label;
  double throughput;
  double stdev_load;
  double stdev_overtime;
  // Mean of per-role-group load stdevs — the variance the matcher can
  // actually move (across-role variance is structural, not fixable).
  double stdev_load_intra_role;
  int    max_overtime;
  int    mean_overtime;
  int    makespan;
  long long runtime_ns;
  vector<int> per_worker_load;
};

static void per_worker_load_ot(const vector<Worker>& initial,
                               const vector<Worker>& final_w,
                               vector<int>& out_load,
                               vector<int>& out_ot) {
  out_load.clear(); out_ot.clear();
  out_load.reserve(initial.size()); out_ot.reserve(initial.size());
  for (size_t i = 0; i < initial.size() && i < final_w.size(); ++i) {
    const int load = initial[i].shift_minutes_left - final_w[i].shift_minutes_left;
    out_load.push_back(load < 0 ? 0 : load);
    const int ot = -min(0, final_w[i].shift_minutes_left);
    out_ot.push_back(ot);
  }
}

static double stdev(const vector<int>& xs) {
  if (xs.empty()) return 0.0;
  double sum = 0.0;
  for (int x : xs) sum += x;
  const double mean = sum / xs.size();
  double sq = 0.0;
  for (int x : xs) { const double d = x - mean; sq += d * d; }
  return std::sqrt(sq / xs.size());
}

static double stdev_within_role(const vector<Worker>& initial,
                                const vector<int>&    load) {
  if (initial.empty() || load.size() != initial.size()) return 0.0;
  unordered_map<int, vector<int>> by_role;
  for (size_t i = 0; i < initial.size(); ++i) {
    by_role[static_cast<int>(initial[i].role)].push_back(load[i]);
  }
  double sum = 0.0;
  int    groups = 0;
  for (const auto& [_, ls] : by_role) {
    if (ls.size() < 2) continue;
    sum += stdev(ls);
    ++groups;
  }
  return groups == 0 ? 0.0 : sum / groups;
}

static FairnessRow run_fairness_row(
    const string &label,
    MatchingStrategy &matching,
    const WardData &ward,
    const unordered_map<string, Task> &tasks,
    const DirectedGraph &dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int overtime_threshold_minutes) {
  PriorityOrdering ordering;
  EngineConfig     cfg;
  cfg.ordering = &ordering;
  cfg.matching = &matching;
  cfg.label    = label;

  auto t   = tasks;
  auto w   = ward.workers;
  auto pts = ward.patients;
  CapacityReport cap;
  cap.threshold_minutes = overtime_threshold_minutes;
  vector<UnscheduledTask> missed;
  EngineMetrics m;

  auto schedule = run_engine(pts, w, t, dep_graph, task_to_patient,
                             cfg, &missed, &cap, &m);

  vector<int> load, ot;
  per_worker_load_ot(ward.workers, w, load, ot);

  FairnessRow r;
  r.label                 = label;
  r.makespan              = m.makespan_minutes;
  r.throughput            = (m.makespan_minutes > 0)
      ? static_cast<double>(schedule.size()) / m.makespan_minutes : 0.0;
  r.stdev_load            = stdev(load);
  r.stdev_overtime        = stdev(ot);
  r.stdev_load_intra_role = stdev_within_role(ward.workers, load);
  r.max_overtime          = m.max_per_worker_overtime;
  r.mean_overtime         = ot.empty() ? 0
      : static_cast<int>(std::round(static_cast<double>(
          accumulate(ot.begin(), ot.end(), 0)) / ot.size()));
  r.runtime_ns            = m.scheduler_runtime_ns;
  r.per_worker_load       = load;
  return r;
}

// 2D Pareto on (throughput ↑, intra-role stdev ↓).
static vector<int> pareto_2d(const vector<FairnessRow>& rows) {
  vector<int> out;
  const int n = static_cast<int>(rows.size());
  for (int i = 0; i < n; ++i) {
    bool dominated = false;
    for (int j = 0; j < n; ++j) {
      if (i == j) continue;
      const bool j_ge_thr = rows[j].throughput            >= rows[i].throughput;
      const bool j_le_std = rows[j].stdev_load_intra_role <= rows[i].stdev_load_intra_role;
      const bool any_strict =
          rows[j].throughput            >  rows[i].throughput  ||
          rows[j].stdev_load_intra_role <  rows[i].stdev_load_intra_role;
      if (j_ge_thr && j_le_std && any_strict) { dominated = true; break; }
    }
    if (!dominated) out.push_back(i);
  }
  return out;
}

static void print_ascii_scatter(const vector<FairnessRow>& rows,
                                const vector<int>& frontier) {
  if (rows.empty()) return;
  const int W = 60, H = 12;
  double thr_lo = rows.front().throughput, thr_hi = thr_lo;
  double std_lo = rows.front().stdev_load_intra_role, std_hi = std_lo;
  for (const auto& r : rows) {
    thr_lo = min(thr_lo, r.throughput);
    thr_hi = max(thr_hi, r.throughput);
    std_lo = min(std_lo, r.stdev_load_intra_role);
    std_hi = max(std_hi, r.stdev_load_intra_role);
  }
  const double thr_pad = max(1e-9, (thr_hi - thr_lo) * 0.05);
  const double std_pad = max(1e-9, (std_hi - std_lo) * 0.05);
  thr_lo -= thr_pad; thr_hi += thr_pad;
  std_lo -= std_pad; std_hi += std_pad;

  vector<string> grid(H, string(W, ' '));
  unordered_set<int> on_frontier(frontier.begin(), frontier.end());
  for (size_t k = 0; k < rows.size(); ++k) {
    const auto& r = rows[k];
    int x = static_cast<int>(((r.throughput - thr_lo) / (thr_hi - thr_lo)) * (W - 1));
    int y = static_cast<int>(((std_hi - r.stdev_load) / (std_hi - std_lo)) * (H - 1));
    x = max(0, min(W - 1, x));
    y = max(0, min(H - 1, y));
    char marker = on_frontier.count(static_cast<int>(k))
        ? static_cast<char>('A' + static_cast<int>(k))
        : static_cast<char>('a' + static_cast<int>(k));
    grid[y][x] = marker;
  }
  cout << "\n  ASCII scatter — throughput (→) vs intra-role load-stdev (↓ better)\n";
  cout << "    high stdev ↑ (less fair)\n";
  for (int y = 0; y < H; ++y) cout << "                | " << grid[y] << "\n";
  cout << "    low  stdev    +" << string(W, '-') << "→ throughput\n";
  cout << "                  low                                                 high\n";
  cout << "    Legend: capital = on Pareto frontier; lowercase = dominated.\n";
  cout << "    A=" << rows[0].label;
  for (size_t k = 1; k < rows.size(); ++k)
    cout << "  " << static_cast<char>('A' + static_cast<int>(k))
         << "=" << rows[k].label;
  cout << "\n";
}

void run_fairness_bench(
    const WardData &ward,
    const unordered_map<string, Task> &tasks,
    const DirectedGraph &dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int overtime_threshold_minutes) {
  GreedyMatcher    greedy;
  HungarianMatcher hungarian;
  FairGreedyMatcher fair_075(0.75), fair_050(0.50), fair_025(0.25), fair_000(0.00);

  vector<FairnessRow> rows;
  rows.reserve(6);
  rows.push_back(run_fairness_row("Greedy",       greedy,    ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  rows.push_back(run_fairness_row("Hungarian",    hungarian, ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  rows.push_back(run_fairness_row("Fair(α=0.75)", fair_075,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  rows.push_back(run_fairness_row("Fair(α=0.50)", fair_050,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  rows.push_back(run_fairness_row("Fair(α=0.25)", fair_025,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  rows.push_back(run_fairness_row("Fair(α=0.00)", fair_000,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));

  const auto frontier = pareto_2d(rows);
  unordered_set<int> on_frontier(frontier.begin(), frontier.end());

  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║  Fairness Sweep — throughput vs worker-load fairness         ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Priority ordering held fixed; only the matching rule varies.\n"
          "  α=1.0 → pure earliest-free greedy; α=0.0 → max-min water-filling.\n\n";
  cout << "  Mode                throughput   load-stdev  intra-role  OT-stdev"
          "   max-OT  mean-OT  makespan   runtime\n";
  cout << "  ──────────────────────────────────────────────────────────────────"
          "─────────────────────────────────────────────\n";
  cout << fixed;
  for (size_t i = 0; i < rows.size(); ++i) {
    const auto& r = rows[i];
    cout << "  " << left << setw(20) << r.label
         << right << setprecision(4) << setw(8) << r.throughput  << "    "
         << setprecision(1)
         << setw(8) << r.stdev_load            << "    "
         << setw(8) << r.stdev_load_intra_role << "    "
         << setw(7) << r.stdev_overtime        << "    "
         << setw(5) << r.max_overtime          << "    "
         << setw(5) << r.mean_overtime         << "    "
         << setw(6) << r.makespan              << "    "
         << setw(7) << (r.runtime_ns / 1000.0) << " µs"
         << (on_frontier.count(static_cast<int>(i)) ? "  ★" : "")
         << "\n";
  }
  cout << defaultfloat;
  cout << "\n  ★ = on the Pareto frontier of (throughput ↑, intra-role stdev ↓).\n"
          "  intra-role = mean of per-role-group load stdevs — the variance a\n"
          "               fairness matcher can actually move.\n";

  auto print_worker_dist = [&](const FairnessRow& r) {
    cout << "    " << left << setw(20) << r.label << ":";
    for (size_t i = 0; i < r.per_worker_load.size() && i < ward.workers.size(); ++i) {
      cout << "  " << ward.workers[i].id
           << "(" << role_label(ward.workers[i].role) << ")="
           << r.per_worker_load[i];
    }
    cout << "\n";
  };
  cout << "\n  Per-worker total minutes (first / last mode):\n";
  if (!rows.empty())   print_worker_dist(rows.front());
  if (rows.size() > 1) print_worker_dist(rows.back());

  print_ascii_scatter(rows, frontier);

  // Diagnostic: when intra-role stdev is essentially zero across modes, the
  // matcher had nothing to redistribute (uniform initial state, ≤2/role).
  double intra_max = 0.0;
  for (const auto& r : rows) intra_max = max(intra_max, r.stdev_load_intra_role);
  if (intra_max <= 5.0) {
    cout << "\n  Diagnostic: intra-role load-stdev ≈ 0 across every mode (max "
         << fixed << setprecision(1) << intra_max << " min). Workers within\n"
            "  each role start uniform, so greedy already balances within-role —\n"
            "  water-filling has nothing to redistribute. The fairness curve\n"
            "  opens up with pre-loaded workers or larger role groups.\n"
         << defaultfloat;
  }
}

// ── Legacy bench (default mode) ──────────────────────────────────────────────

void run_legacy_bench(
    const WardData &                     ward,
    const unordered_map<string, Task> &  tasks,
    const DirectedGraph &                dep_graph,
    const unordered_map<string, string> &task_to_patient,
    int                                  overtime_threshold_minutes) {
  unordered_map<string, Acuity> patient_acuity;
  unordered_map<string, string> patient_name;
  for (const auto &p : ward.patients) {
    patient_acuity[p.id] = p.acuity;
    patient_name[p.id]   = p.name;
  }

  vector<UnscheduledTask> missed_prio, missed_fcfs;
  CapacityReport cap_prio, cap_fcfs;
  cap_prio.threshold_minutes = overtime_threshold_minutes;
  cap_fcfs.threshold_minutes = overtime_threshold_minutes;

  auto t_p = tasks; auto w_p = ward.workers; auto pts_p = ward.patients;
  auto sched_prio = run_simulation(pts_p, w_p, t_p, dep_graph,
                                   task_to_patient, &missed_prio, &cap_prio);
  print_schedule(sched_prio, patient_acuity, patient_name,
                 "Priority Ward Scheduling — Full Timeline                 ║");
  print_worker_summary(w_p, sched_prio, "Priority");
  print_unscheduled(missed_prio);
  print_capacity_report(cap_prio, "Priority");

  auto t_f = tasks; auto w_f = ward.workers; auto pts_f = ward.patients;
  auto sched_fcfs = run_simulation_fcfs(pts_f, w_f, t_f, dep_graph,
                                        task_to_patient, &missed_fcfs, &cap_fcfs);
  print_schedule(sched_fcfs, patient_acuity, patient_name,
                 "FCFS Ward Scheduling — Full Timeline                     ║");
  print_worker_summary(w_f, sched_fcfs, "FCFS");
  print_unscheduled(missed_fcfs);
  print_capacity_report(cap_fcfs, "FCFS");

  print_benchmark(sched_prio, sched_fcfs, cap_prio, cap_fcfs,
                  missed_prio, missed_fcfs,
                  static_cast<int>(tasks.size()), ward.patients);
}
