#include "bench_adversarial.h"
#include "data/loader.h"
#include "data_structures/graph.h"
#include "models/models.h"
#include "scheduler/allocator.h"
#include "scheduler/strategies.h"
#include <climits>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace {

struct Scenario {
  string                        name;
  string                        description;
  string                        takeaway;
  WardData                      ward;
  unordered_map<string, Task>   tasks;
  DirectedGraph                 dep_graph;
  unordered_map<string, string> task_to_patient;
  int                           total_work = 0;
};

// ── synthesis helpers ────────────────────────────────────────

static string padded(const char* prefix, int n, int width = 3) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%s%0*d", prefix, width, n);
  return buf;
}

static void add_worker(Scenario& s, int idx, Role role, int shift) {
  s.ward.workers.push_back({padded("W", idx), "Worker " + to_string(idx),
                            role, shift, 0});
}

static void add_patient(Scenario& s, int idx, Acuity acuity, int age = 50) {
  Patient p;
  p.id         = padded("P", idx);
  p.name       = "Patient " + to_string(idx);
  p.age        = age;
  p.complaint  = "synthetic";
  p.acuity     = acuity;
  s.ward.patients.push_back(p);
}

static string add_task(Scenario& s, int patient_idx, const string& base_id,
                       Role role, int duration, int priority = 1) {
  const string pid = padded("P", patient_idx);
  Task t;
  t.id               = base_id + "_" + pid;
  t.name             = base_id;
  t.required_role    = role;
  t.duration_minutes = duration;
  t.priority         = priority;
  t.status           = Status::PENDING;
  s.tasks[t.id]      = t;
  s.dep_graph.add_node(t.id);
  s.task_to_patient[t.id] = pid;
  s.total_work += duration;
  return t.id;
}

// ── scenario builders ────────────────────────────────────────

static Scenario all_critical(int n_patients = 20, int tasks_per_patient = 5) {
  Scenario s;
  s.name        = "all-critical";
  s.description = "Every patient CRITICAL — legacy_priority collapses to a single value (11)";
  s.takeaway    = "Priority ordering degenerates to deterministic tie-hash;"
                  " ordering becomes effectively FIFO-ish.";
  // 5 workers, balanced across roles.
  Role roles[] = { Role::NURSE, Role::INTERN, Role::PGY1 };
  for (int i = 1; i <= 5; ++i) add_worker(s, i, roles[(i - 1) % 3], 480);
  for (int p = 1; p <= n_patients; ++p) {
    add_patient(s, p, Acuity::CRITICAL);
    for (int t = 1; t <= tasks_per_patient; ++t) {
      char tb[8]; snprintf(tb, sizeof(tb), "T%02d", t);
      add_task(s, p, tb, roles[(t - 1) % 3], 15);
    }
  }
  return s;
}

static Scenario long_chain(int chain_length = 25) {
  Scenario s;
  s.name        = "long-chain";
  s.description = to_string(chain_length) +
                  " serially-dependent tasks — parallelism = 1";
  s.takeaway    = "Every tick has batch=1; matchers can't differentiate."
                  " Runtime gap is pure per-decision overhead.";
  for (int i = 1; i <= 5; ++i) add_worker(s, i, Role::NURSE, 480);
  add_patient(s, 1, Acuity::HIGH);
  string prev;
  for (int t = 1; t <= chain_length; ++t) {
    char tb[8]; snprintf(tb, sizeof(tb), "T%03d", t);
    string cur = add_task(s, 1, tb, Role::NURSE, 15);
    if (!prev.empty()) s.dep_graph.add_edge(prev, cur);
    prev = cur;
  }
  return s;
}

static Scenario single_role_bottleneck(int n_patients = 15) {
  Scenario s;
  s.name        = "single-role-bottleneck";
  s.description = to_string(n_patients) +
                  " patients all need PGY1, only 1 PGY1 on roster (4 idle peers)";
  s.takeaway    = "All work funnels onto one worker; matcher choice is moot."
                  " Hungarian wastes ticks deferring tasks it can't place.";
  add_worker(s, 1, Role::PGY1,   480);
  add_worker(s, 2, Role::NURSE,  480);
  add_worker(s, 3, Role::NURSE,  480);
  add_worker(s, 4, Role::INTERN, 480);
  add_worker(s, 5, Role::INTERN, 480);
  for (int p = 1; p <= n_patients; ++p) {
    add_patient(s, p, p % 3 == 0 ? Acuity::CRITICAL : Acuity::HIGH);
    for (int t = 1; t <= 3; ++t) {
      char tb[8]; snprintf(tb, sizeof(tb), "T%02d", t);
      add_task(s, p, tb, Role::PGY1, 30);
    }
  }
  return s;
}

static Scenario tight_shift(int n_patients = 12) {
  Scenario s;
  s.name        = "tight-shift";
  s.description = "Per-worker shift = perfect-split share, 0 min slack";
  s.takeaway    = "Perfect balance → 0 OT. Any imbalance → immediate OT."
                  " Exposes which matchers actually balance vs just ride slack.";
  // 4 tasks/patient: 2 NURSE + 2 PGY1, each 20 min.
  // Per role: n_patients × 2 tasks × 20 min = 40·n minutes total per role.
  // 2 workers per role → 20·n per worker is the perfect-split share.
  const int shift = 20 * n_patients;
  for (int i = 1; i <= 2; ++i) add_worker(s, i, Role::NURSE, shift);
  for (int i = 3; i <= 4; ++i) add_worker(s, i, Role::PGY1,  shift);
  for (int p = 1; p <= n_patients; ++p) {
    add_patient(s, p, p <= n_patients / 2 ? Acuity::HIGH : Acuity::MEDIUM);
    add_task(s, p, "TN1", Role::NURSE, 20);
    add_task(s, p, "TN2", Role::NURSE, 20);
    add_task(s, p, "TP1", Role::PGY1,  20);
    add_task(s, p, "TP2", Role::PGY1,  20);
  }
  return s;
}

// ── runner ───────────────────────────────────────────────────

struct Result {
  string    label;
  int       makespan;
  int       total_ot;
  int       max_ot;
  int       breaches;
  size_t    scheduled;
  size_t    total_tasks;
  long long runtime_ns;
  int       ticks;
  int       deferred;
  bool      complete;
};

static Result run_with(const string& label,
                       OrderingStrategy& ord, MatchingStrategy& matc,
                       const Scenario& s) {
  auto t = s.tasks;
  auto w = s.ward.workers;
  auto p = s.ward.patients;
  CapacityReport cap;
  cap.threshold_minutes = 0;
  vector<UnscheduledTask> miss;
  EngineMetrics m;

  EngineConfig cfg;
  cfg.ordering = &ord;
  cfg.matching = &matc;
  cfg.label    = label;

  auto sched = run_engine(p, w, t, s.dep_graph, s.task_to_patient,
                          cfg, &miss, &cap, &m);

  return {label, m.makespan_minutes, cap.total_overtime_minutes,
          m.max_per_worker_overtime, cap.over_capacity_task_count,
          sched.size(), s.tasks.size(), m.scheduler_runtime_ns,
          m.tick_count, m.deferred_event_count,
          sched.size() == s.tasks.size()};
}

static vector<Result> run_strategies(const Scenario& s) {
  vector<Result> rs;
  rs.reserve(5);
  { FcfsOrdering    o; GreedyMatcher    m; rs.push_back(run_with("FCFS × Greedy",        o, m, s)); }
  { FcfsOrdering    o; HungarianMatcher m; rs.push_back(run_with("FCFS × Hungarian",     o, m, s)); }
  { PriorityOrdering o; GreedyMatcher    m; rs.push_back(run_with("Priority × Greedy",    o, m, s)); }
  { PriorityOrdering o; HungarianMatcher m; rs.push_back(run_with("Priority × Hungarian", o, m, s)); }
  { PriorityOrdering o; FairGreedyMatcher m(0.0);
    rs.push_back(run_with("Priority × Fair(α=0)", o, m, s)); }
  return rs;
}

// ── presentation ─────────────────────────────────────────────

static void print_scenario(const Scenario& s, const vector<Result>& rs) {
  cout << "\n  ── " << s.name << " "
       << string(max(0, 56 - (int)s.name.size()), '-') << "\n";
  cout << "     " << s.description << "\n";
  cout << "     patients=" << s.ward.patients.size()
       << "  workers="   << s.ward.workers.size()
       << "  tasks="     << s.tasks.size()
       << "  total_work=" << s.total_work << " min\n\n";

  // Best on each axis among complete runs.
  int       best_ms = INT_MAX, best_ot = INT_MAX;
  long long best_rt = LLONG_MAX;
  for (const auto& r : rs) {
    if (!r.complete) continue;
    best_ms = min(best_ms, r.makespan);
    best_ot = min(best_ot, r.total_ot);
    best_rt = min(best_rt, r.runtime_ns);
  }

  cout << "     strategy                 ms    OT  max-OT  brch    cov     runtime   ticks\n";
  cout << "     ─────────────────────────────────────────────────────────────────────────────\n";
  for (const auto& r : rs) {
    cout << "     " << left << setw(22) << r.label
         << right << setw(6) << r.makespan
         << "  "  << setw(5) << r.total_ot
         << "  "  << setw(5) << r.max_ot
         << "  "  << setw(4) << r.breaches
         << "  "  << setw(7) << (to_string(r.scheduled) + "/" + to_string(r.total_tasks))
         << fixed << setprecision(1)
         << setw(8)  << (r.runtime_ns / 1000.0) << " µs"
         << "  " << setw(4) << r.ticks;
    if (r.deferred > 0) cout << " (+" << r.deferred << "d)";
    cout << defaultfloat;

    // Tag axis-winners (only meaningful when complete).
    string tag;
    if (r.complete) {
      if (r.makespan   == best_ms) tag = "best ms";
      if (r.total_ot   == best_ot && best_ot < INT_MAX)
        tag = tag.empty() ? "lowest OT" : tag + " / lowest OT";
      if (r.runtime_ns == best_rt) tag = tag.empty() ? "fastest" : tag + " / fastest";
    } else {
      tag = "INCOMPLETE";
    }
    if (!tag.empty()) cout << "  ← " << tag;
    cout << "\n";
  }
  cout << "\n     " << s.takeaway << "\n";
}

} // namespace

void run_adversarial_bench() {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║  Adversarial Bench — synthetic wards designed to break algos ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Each scenario is built from scratch in-memory (no JSON).\n"
          "  Five strategies run on identical input; per-axis winners are\n"
          "  tagged so the surviving strategy per attack is visible at a glance.\n";

  vector<Scenario> scenarios;
  scenarios.push_back(all_critical());
  scenarios.push_back(long_chain());
  scenarios.push_back(single_role_bottleneck());
  scenarios.push_back(tight_shift());

  for (const auto& s : scenarios) print_scenario(s, run_strategies(s));

  cout << "\n  Reading the table:\n"
          "    ms      = makespan, minutes\n"
          "    OT      = total overtime across all workers, minutes\n"
          "    max-OT  = worst single-worker overtime, minutes\n"
          "    brch    = number of commits that pushed a worker past their shift\n"
          "    cov     = scheduled / total tasks (incomplete = INCOMPLETE tag)\n"
          "    ticks   = match() invocations; (+Nd) = Hungarian re-queue events\n";
}
