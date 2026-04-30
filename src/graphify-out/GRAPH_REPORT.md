# Graph Report - src  (2026-04-29)

## Corpus Check
- 21 files · ~104,706 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 249 nodes · 785 edges · 13 communities detected
- Extraction: 77% EXTRACTED · 23% INFERRED · 0% AMBIGUOUS · INFERRED: 178 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 0|Community 0]]
- [[_COMMUNITY_Community 1|Community 1]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]
- [[_COMMUNITY_Community 5|Community 5]]
- [[_COMMUNITY_Community 6|Community 6]]
- [[_COMMUNITY_Community 7|Community 7]]
- [[_COMMUNITY_Community 8|Community 8]]
- [[_COMMUNITY_Community 9|Community 9]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 11|Community 11]]
- [[_COMMUNITY_Community 12|Community 12]]

## God Nodes (most connected - your core abstractions)
1. `push_back()` - 46 edges
2. `size()` - 46 edges
3. `end()` - 40 edges
4. `begin()` - 33 edges
5. `run_engine()` - 27 edges
6. `empty()` - 26 edges
7. `namespace()` - 22 edges
8. `find()` - 22 edges
9. `basic_json()` - 20 edges
10. `is_object()` - 19 edges

## Surprising Connections (you probably didn't know these)
- `run_pareto_sweep()` --calls--> `policy_catalog()`  [INFERRED]
  bench.cpp → scheduler/policies.cpp
- `add_worker()` --calls--> `push_back()`  [INFERRED]
  bench_adversarial.cpp → third_party/nlohmann/json.hpp
- `add_patient()` --calls--> `push_back()`  [INFERRED]
  bench_adversarial.cpp → third_party/nlohmann/json.hpp
- `add_task()` --calls--> `add_node()`  [INFERRED]
  bench_adversarial.cpp → data_structures/graph.cpp
- `long_chain()` --calls--> `empty()`  [INFERRED]
  bench_adversarial.cpp → third_party/nlohmann/json.hpp

## Communities

### Community 0 - "Community 0"
Cohesion: 0.13
Nodes (38): begin(), cbegin(), clear(), count(), crend(), decode(), empty(), front() (+30 more)

### Community 1 - "Community 1"
Cohesion: 0.1
Nodes (28): in_degrees(), node_count(), add(), back(), json_sax_dom_callback_parser, max_size(), push_back(), start_array() (+20 more)

### Community 2 - "Community 2"
Cohesion: 0.06
Nodes (7): get_ptr(), get_ref_impl(), is_number_unsigned(), items(), iterator_wrapper(), json_sax_acceptor, patch()

### Community 3 - "Community 3"
Cohesion: 0.17
Nodes (28): at(), emplace(), emplace_back(), end_array(), end_object(), erase(), erase_internal(), get_impl_ptr() (+20 more)

### Community 4 - "Community 4"
Cohesion: 0.2
Nodes (23): build_patient_tasks(), build_ward(), add_edge(), add_node(), has_node(), cend(), contains(), convert() (+15 more)

### Community 5 - "Community 5"
Cohesion: 0.33
Nodes (13): acuity_first(), acuity_int(), acuity_long_first(), acuity_old_first(), acuity_short_first(), acuity_young_first(), default_lex_policy(), dominates() (+5 more)

### Community 6 - "Community 6"
Cohesion: 0.43
Nodes (13): to_string(), add_patient(), add_task(), add_worker(), all_critical(), long_chain(), padded(), print_scenario() (+5 more)

### Community 7 - "Community 7"
Cohesion: 0.23
Nodes (13): binary(), boolean(), handle_value(), json_pointer, namespace(), null(), number_float(), number_integer() (+5 more)

### Community 8 - "Community 8"
Cohesion: 0.35
Nodes (10): load_dependencies(), load_diseases(), load_tests(), load_ward(), parse_acuity(), parse_role(), read_file(), main() (+2 more)

### Community 9 - "Community 9"
Cohesion: 0.33
Nodes (11): accept(), array(), basic_json(), from_bjdata(), from_bson(), from_cbor(), from_msgpack(), from_ubjson() (+3 more)

### Community 10 - "Community 10"
Cohesion: 0.29
Nodes (7): data(), dump_integer(), get_token_string(), hex_bytes(), is_negative_number(), NLOHMANN_JSON_NAMESPACE_BEGIN(), remove_sign()

### Community 11 - "Community 11"
Cohesion: 0.5
Nodes (4): scan(), skip_bom(), skip_whitespace(), unget()

### Community 12 - "Community 12"
Cohesion: 0.5
Nodes (4): from_json(), get_impl(), get_to(), noexcept()

## Knowledge Gaps
- **1 isolated node(s):** `json_sax_acceptor`
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `push_back()` connect `Community 1` to `Community 0`, `Community 2`, `Community 3`, `Community 4`, `Community 5`, `Community 6`, `Community 8`, `Community 9`, `Community 10`?**
  _High betweenness centrality (0.154) - this node is a cross-community bridge._
- **Why does `size()` connect `Community 0` to `Community 1`, `Community 2`, `Community 3`, `Community 4`, `Community 5`, `Community 6`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.150) - this node is a cross-community bridge._
- **Why does `end()` connect `Community 4` to `Community 0`, `Community 1`, `Community 2`, `Community 3`, `Community 6`, `Community 7`, `Community 9`?**
  _High betweenness centrality (0.063) - this node is a cross-community bridge._
- **Are the 24 inferred relationships involving `push_back()` (e.g. with `add_worker()` and `add_patient()`) actually correct?**
  _`push_back()` has 24 INFERRED edges - model-reasoned connections that need verification._
- **Are the 29 inferred relationships involving `size()` (e.g. with `run_with()` and `print_scenario()`) actually correct?**
  _`size()` has 29 INFERRED edges - model-reasoned connections that need verification._
- **Are the 18 inferred relationships involving `end()` (e.g. with `print_schedule()` and `per_patient_timing()`) actually correct?**
  _`end()` has 18 INFERRED edges - model-reasoned connections that need verification._
- **Are the 12 inferred relationships involving `begin()` (e.g. with `print_schedule()` and `print_capacity_report()`) actually correct?**
  _`begin()` has 12 INFERRED edges - model-reasoned connections that need verification._