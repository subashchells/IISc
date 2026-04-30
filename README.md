# CardioScheduler

An algorithmic task scheduler for cardiac wards that prioritises urgent patients while respecting clinical dependencies, worker roles, and shift limits.
Built for **SL 203 — Introduction to Algorithms and Software Programming**, IISc Bangalore (2026).

## Structure

```
src/
├── CMakeLists.txt
├── main.cpp · bench*.cpp        # CLI + bench drivers
├── index.html                   # ward editor
├── data/                        # 4 JSON catalogues + loader
├── data_structures/             # DirectedGraph
├── scheduler/                   # allocator · strategies · hungarian · policies · topological
├── models/                      # Patient / Task / Worker / Disease
└── third_party/nlohmann/        # vendored JSON
```
