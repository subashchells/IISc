# CardioScheduler

An algorithmic task scheduler for cardiac wards that prioritises urgent patients while respecting clinical dependencies, worker roles, and shift limits.
Built for **SL 203 — Introduction to Algorithms and Software Programming**, IISc Bangalore (2026).

## Structure

```
IISc/
├── Documents/
│   ├── FinalReport.pdf          # Compiled final project report
│   ├── Proposal.pdf             # Project proposal document
│   └── SlideDeck.pdf            # Presentation slide deck
│
├── LaTex/
│   ├── ProjectProposal/         # LaTeX source for proposal
│   └── ProjectReport/
│       ├── presentation/        # LaTeX source for slides
│       └── report/              # LaTeX source for final report
│
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp · bench*.cpp    # CLI + bench drivers
│   ├── index.html               # Ward editor (browser UI)
│   ├── data/                    # 4 JSON catalogues + loader
│   ├── data_structures/         # DirectedGraph
│   ├── scheduler/               # Allocator · strategies · hungarian · policies · topological
│   ├── models/                  # Patient / Task / Worker / Disease
│   └── third_party/nlohmann/    # Vendored JSON library
│
└── README.md
```
