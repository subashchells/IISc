# Figure placeholders

This directory holds the actual figure assets referenced by the report.
Each filename below corresponds to a `\figplaceholder{...}` invocation
in `sections/*.tex`. Drop the real PDF/PNG into this folder and the
report will render the image automatically (by replacing the `fbox`
placeholder block in `preamble.tex` with `\includegraphics`).

| Filename | Section | Recommended size | Content |
| --- | --- | --- | --- |
| `architecture_overview.pdf` | System Architecture | 0.9 \textwidth, ~7 cm tall | Five-layer stack diagram. |
| `module_dependency_graph.pdf` | System Architecture | 0.85 \textwidth | Per-source-file DAG. |
| `pipeline_overview.pdf` | Pipeline Design | 0.95 \textwidth | Four-stage horizontal pipeline. |
| `sequence_diagram.pdf` | Data Flow | 0.85 \textwidth | UML sequence for one ASSIGNED commit. |
| `pareto_frontier_3d.pdf` | Results | 0.9 \textwidth | 3D Pareto scatter (35 candidates). |
| `fairness_pareto_2d.pdf` | Comparative Analysis | 0.75 \textwidth | 2D throughput vs. intra-role stdev. |
| `hungarian_regression.pdf` | Error Analysis | 0.85 \textwidth | Two timeline strips, Greedy vs. Hungarian. |
| `scaling_curves.pdf` | Scalability & Performance | 0.85 \textwidth | Log-log wall-clock vs. \|W\|. |
| `iisc_crest.pdf` | Title page | ~3 cm | IISc institutional crest. |

The `\figplaceholder{label}{caption}{height}{width}{description}` macro
in `preamble.tex` produces a labelled `fbox` that records the expected
filename inline. To swap in a real figure, replace the macro call with:

```latex
\begin{figure}[H]
  \centering
  \includegraphics[width=0.9\textwidth]{figures/architecture_overview}
  \caption{...}
  \label{fig:architecture_overview}
\end{figure}
```
