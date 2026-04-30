#include "bench.h"
#include "bench_adversarial.h"
#include "bench_urgency.h"
#include "data/loader.h"
#include "data_structures/graph.h"
#include "models/models.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

namespace {

void print_usage(std::ostream& os) {
  os << "Usage: cardio_scheduler [options] [data_dir]\n"
        "\n"
        "  data_dir                 directory of the four JSON files\n"
        "                           (default: ./data, falls back to ./src/data)\n"
        "\n"
        "Bench modes (default is the legacy Priority-vs-FCFS comparison):\n"
        "  --bench-2x2              FCFS/Priority × Greedy/Hungarian matrix\n"
        "  --bench-pareto           Pareto sweep over lex policies × seeds\n"
        "  --bench-fair             throughput vs worker-load stdev\n"
        "  --bench-adversarial      synthetic wards built to break each algo\n"
        "  --bench-urgency          per-acuity timing across 5 matchers\n"
        "\n"
        "  --overtime-threshold N   OT budget (min) before INFEASIBLE\n"
        "  -h, --help               this message\n"
        "\n"
        "Edit patients, workers, and tests in src/index.html.\n";
}

enum class Mode { LEGACY, BENCH_2X2, BENCH_PARETO, BENCH_FAIR, BENCH_ADVERSARIAL, BENCH_URGENCY };

struct Args {
  std::string data_dir           = "";  // empty → resolve_data_dir picks one
  int         overtime_threshold = kDefaultOvertimeThresholdMinutes;
  Mode        mode               = Mode::LEGACY;
};

// When data_dir wasn't passed on the CLI, try the two layouts that exist
// in this project: ./data (binary launched from src/) and ./src/data
// (binary launched from the project root). First one with diseases.json
// present wins. Throws if neither exists.
std::string resolve_data_dir(const std::string& cli_value) {
  auto exists = [](const std::string& dir) {
    std::ifstream f(dir + "/diseases.json");
    return f.good();
  };
  if (!cli_value.empty()) return cli_value;
  if (exists("data"))      return "data";
  if (exists("src/data"))  return "src/data";
  std::cerr << "  (no diseases.json found in ./data or ./src/data — "
               "pass an explicit data_dir as the positional argument)\n";
  std::exit(2);
}

Args parse_args(int argc, char* argv[]) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string s = argv[i];
    if      (s == "--overtime-threshold" && i + 1 < argc) {
      try { a.overtime_threshold = std::stoi(argv[++i]); }
      catch (...) {
        std::cerr << "  (ignoring non-integer --overtime-threshold "
                  << argv[i] << ")\n";
      }
    }
    else if (s == "--bench-2x2")         a.mode = Mode::BENCH_2X2;
    else if (s == "--bench-pareto")      a.mode = Mode::BENCH_PARETO;
    else if (s == "--bench-fair")        a.mode = Mode::BENCH_FAIR;
    else if (s == "--bench-adversarial") a.mode = Mode::BENCH_ADVERSARIAL;
    else if (s == "--bench-urgency")     a.mode = Mode::BENCH_URGENCY;
    else if (s == "-h" || s == "--help") { print_usage(std::cout); std::exit(0); }
    else if (!s.empty() && s[0] != '-') a.data_dir = s;
    else {
      std::cerr << "Unknown option: " << s << "\n";
      print_usage(std::cerr);
      std::exit(2);
    }
  }
  return a;
}

} // namespace

int main(int argc, char* argv[]) {
  const Args args = parse_args(argc, argv);

  // Adversarial bench is fully synthetic — no data files needed.
  if (args.mode == Mode::BENCH_ADVERSARIAL) {
    run_adversarial_bench();
    return 0;
  }

  const std::string data_dir = resolve_data_dir(args.data_dir);

  std::cout << "=== CardioScheduler V2 ===\n"
            << "Loading data from: " << data_dir << "/\n"
            << "Overtime threshold: " << args.overtime_threshold << " min\n";

  auto diseases       = load_diseases(data_dir + "/diseases.json");
  auto test_templates = load_tests(data_dir + "/tests.json");
  auto dep_edges      = load_dependencies(data_dir + "/test_dependencies.json");
  auto ward           = load_ward(data_dir + "/sample_ward.json");

  std::unordered_map<std::string, Task>        tasks;
  DirectedGraph                                dep_graph;
  std::unordered_map<std::string, std::string> task_to_patient;
  build_ward(ward, diseases, test_templates, dep_edges, tasks, dep_graph,
             task_to_patient);

  std::cout << "  Diseases: " << diseases.size()
            << "   Tests: "   << test_templates.size()
            << "   Patients: " << ward.patients.size()
            << "   Workers: "  << ward.workers.size() << "\n";

  switch (args.mode) {
    case Mode::BENCH_2X2:
      run_2x2_bench(ward, tasks, dep_graph, task_to_patient, args.overtime_threshold);
      break;
    case Mode::BENCH_PARETO:
      run_pareto_sweep(ward, tasks, dep_graph, task_to_patient, args.overtime_threshold);
      break;
    case Mode::BENCH_FAIR:
      run_fairness_bench(ward, tasks, dep_graph, task_to_patient, args.overtime_threshold);
      break;
    case Mode::BENCH_URGENCY:
      run_urgency_bench(ward, tasks, dep_graph, task_to_patient, args.overtime_threshold);
      break;
    case Mode::LEGACY:
      run_legacy_bench(ward, tasks, dep_graph, task_to_patient, args.overtime_threshold);
      break;
    case Mode::BENCH_ADVERSARIAL:
      break;  // handled before data load
  }
  return 0;
}
