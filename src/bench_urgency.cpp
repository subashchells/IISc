#include "bench_urgency.h"
#include "scheduler/allocator.h"
#include "scheduler/strategies.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace {

struct PerPatient {
  string patient_id;
  string name;
  Acuity acuity;
  int    first_start;
  int    last_finish;
  bool   scheduled;
};

struct TierStats {
  int    n          = 0;
  double mean_first = 0;
  double mean_last  = 0;
  int    p50_first = 0, p90_first = 0, max_first = 0;
  int    p50_last  = 0, p90_last  = 0, max_last  = 0;
};

struct StrategyResult {
  string             label;
  vector<PerPatient> per_patient;
  TierStats          critical, high, medium, low;
};

static const char* tier_label(Acuity a) {
  switch (a) {
    case Acuity::CRITICAL: return "CRIT";
    case Acuity::HIGH:     return "HIGH";
    case Acuity::MEDIUM:   return "MED";
    case Acuity::LOW:      return "LOW";
  }
  return "?";
}

// One row per patient: (first_start, last_finish) folded across the
// patient's tasks. Patients absent from the schedule appear with
// scheduled=false (won't fold into stats).
static vector<PerPatient> collect_per_patient(
    const vector<ScheduleEntry> &schedule,
    const vector<Patient>       &patients) {
  unordered_map<string, PerPatient> by_pid;
  for (const auto &p : patients) {
    by_pid[p.id] = {p.id, p.name, p.acuity, 0, 0, false};
  }
  for (const auto &e : schedule) {
    auto it = by_pid.find(e.patient_id);
    if (it == by_pid.end()) continue;
    auto &s = it->second;
    if (!s.scheduled) {
      s.first_start = e.start_time;
      s.last_finish = e.finish_time;
      s.scheduled   = true;
    } else {
      s.first_start = min(s.first_start, e.start_time);
      s.last_finish = max(s.last_finish, e.finish_time);
    }
  }
  vector<PerPatient> out;
  out.reserve(by_pid.size());
  for (auto &kv : by_pid) out.push_back(kv.second);
  return out;
}

static int percentile(const vector<int> &sorted, double frac) {
  if (sorted.empty()) return 0;
  size_t idx = static_cast<size_t>((sorted.size() - 1) * frac);
  return sorted[idx];
}

static TierStats compute_tier(const vector<PerPatient> &pps, Acuity tier) {
  vector<int> firsts, lasts;
  for (const auto &p : pps) {
    if (!p.scheduled || p.acuity != tier) continue;
    firsts.push_back(p.first_start);
    lasts .push_back(p.last_finish);
  }
  TierStats t;
  t.n = static_cast<int>(firsts.size());
  if (t.n == 0) return t;
  sort(firsts.begin(), firsts.end());
  sort(lasts .begin(), lasts .end());
  long long sum_f = 0, sum_l = 0;
  for (int x : firsts) sum_f += x;
  for (int x : lasts)  sum_l += x;
  t.mean_first = static_cast<double>(sum_f) / t.n;
  t.mean_last  = static_cast<double>(sum_l) / t.n;
  t.p50_first  = percentile(firsts, 0.50);
  t.p90_first  = percentile(firsts, 0.90);
  t.max_first  = firsts.back();
  t.p50_last   = percentile(lasts,  0.50);
  t.p90_last   = percentile(lasts,  0.90);
  t.max_last   = lasts.back();
  return t;
}

static StrategyResult run_one(
    const string                       &label,
    MatchingStrategy                   &matching,
    const WardData                     &ward,
    const unordered_map<string, Task>  &tasks,
    const DirectedGraph                &dep_graph,
    const unordered_map<string, string>&task_to_patient,
    int                                 ot_threshold) {
  PriorityOrdering ordering;
  EngineConfig     cfg;
  cfg.ordering = &ordering;
  cfg.matching = &matching;
  cfg.label    = label;

  auto t   = tasks;
  auto w   = ward.workers;
  auto pts = ward.patients;
  CapacityReport         cap;
  cap.threshold_minutes = ot_threshold;
  vector<UnscheduledTask> missed;

  auto schedule = run_engine(pts, w, t, dep_graph, task_to_patient,
                             cfg, &missed, &cap, nullptr);

  StrategyResult r;
  r.label       = label;
  r.per_patient = collect_per_patient(schedule, ward.patients);
  r.critical    = compute_tier(r.per_patient, Acuity::CRITICAL);
  r.high        = compute_tier(r.per_patient, Acuity::HIGH);
  r.medium      = compute_tier(r.per_patient, Acuity::MEDIUM);
  r.low         = compute_tier(r.per_patient, Acuity::LOW);
  return r;
}

static void print_tier_row(const char *tier, const TierStats &t) {
  if (t.n == 0) return;
  cout << "       " << left << setw(5) << tier
       << " n="   << right << setw(3) << t.n
       << "    first  mean=" << fixed << setprecision(0) << setw(5) << t.mean_first
       <<  "  p50=" << setw(5) << t.p50_first
       <<  "  p90=" << setw(5) << t.p90_first
       <<  "  max=" << setw(5) << t.max_first
       <<  "    last  mean=" << setw(5) << t.mean_last
       <<  "  p50=" << setw(5) << t.p50_last
       <<  "  p90=" << setw(5) << t.p90_last
       <<  "  max=" << setw(5) << t.max_last
       << "\n" << defaultfloat;
}

static void print_strategy(const StrategyResult &r) {
  cout << "\n  ── " << r.label << " "
       << string(max(0, 60 - static_cast<int>(r.label.size())), '-') << "\n";
  print_tier_row(tier_label(Acuity::CRITICAL), r.critical);
  print_tier_row(tier_label(Acuity::HIGH),     r.high);
  print_tier_row(tier_label(Acuity::MEDIUM),   r.medium);
  print_tier_row(tier_label(Acuity::LOW),      r.low);

  // Worst-served CRITICAL patient: highest last_finish among CRIT.
  const PerPatient *worst = nullptr;
  for (const auto &p : r.per_patient) {
    if (!p.scheduled || p.acuity != Acuity::CRITICAL) continue;
    if (!worst || p.last_finish > worst->last_finish) worst = &p;
  }
  if (worst) {
    cout << "       worst CRIT: " << worst->patient_id << " " << worst->name
         << "   first=T+" << worst->first_start
         << "   last=T+"  << worst->last_finish << "\n";
  }
}

static void print_delta_table(const vector<StrategyResult> &rs) {
  if (rs.size() < 2) return;
  const auto &base = rs[0];
  cout << "\n  ── Δ vs " << base.label
       << "  (negative = better; row = strategy minus baseline) ──\n";
  cout << "       strategy              "
       <<        "ΔCRIT.first  ΔCRIT.p90L  ΔCRIT.maxL    "
       <<        "ΔHIGH.first  ΔHIGH.p90L\n";
  cout << fixed << setprecision(0);
  for (size_t i = 1; i < rs.size(); ++i) {
    const auto &r = rs[i];
    auto df = [](double a, double b) { return static_cast<int>(a - b); };
    cout << "       " << left << setw(22) << r.label
         << right
         << setw(10) << df(r.critical.mean_first, base.critical.mean_first)
         << setw(13) << (r.critical.p90_last - base.critical.p90_last)
         << setw(13) << (r.critical.max_last - base.critical.max_last)
         << setw(15) << df(r.high.mean_first, base.high.mean_first)
         << setw(13) << (r.high.p90_last - base.high.p90_last)
         << "\n";
  }
  cout << defaultfloat;
}

} // namespace

void run_urgency_bench(
    const WardData                     &ward,
    const unordered_map<string, Task>  &tasks,
    const DirectedGraph                &dep_graph,
    const unordered_map<string, string>&task_to_patient,
    int                                 overtime_threshold_minutes) {
  cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║  Urgency Bench — per-acuity timing across 5 matchers         ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  All times in minutes; lower = better.\n"
          "  \"first\" = first-task-start (when care begins).\n"
          "  \"last\"  = last-task-finish (when workup is complete).\n"
          "  Priority ordering held fixed across all 5 matchers.\n";

  GreedyMatcher    greedy;
  HungarianMatcher hungarian;
  FairGreedyMatcher fair_075(0.75), fair_050(0.50), fair_025(0.25), fair_000(0.00);

  vector<StrategyResult> results;
  results.reserve(6);
  results.push_back(run_one("Greedy",       greedy,    ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  results.push_back(run_one("Hungarian",    hungarian, ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  results.push_back(run_one("Fair(α=0.75)", fair_075,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  results.push_back(run_one("Fair(α=0.50)", fair_050,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  results.push_back(run_one("Fair(α=0.25)", fair_025,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));
  results.push_back(run_one("Fair(α=0.00)", fair_000,  ward, tasks, dep_graph, task_to_patient, overtime_threshold_minutes));

  for (const auto &r : results) print_strategy(r);
  print_delta_table(results);

  cout << "\n  Reading: per acuity tier you see how many patients are in it (n),\n"
          "  the time-to-first-task (mean/p50/p90/max) — i.e. how long until\n"
          "  any care starts for them — and last-task-finish — when the workup\n"
          "  is complete. The 'worst CRIT' line names the single critical patient\n"
          "  who waited longest under each strategy (i.e. the person most affected\n"
          "  by your scheduling choice). The Δ table shows each non-baseline\n"
          "  strategy's gap vs Greedy on the urgent-care axes.\n";
}
