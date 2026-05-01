# CardioScheduler — Report (LaTeX source)

Portrait-format academic-style project report for the
SL 203 CardioScheduler project. Compiled output is `main.pdf`.

## Layout

```
report/
├── main.tex                  -- master file; \input{}s every section
├── preamble.tex              -- shared style: fonts, colours, layout
├── references.bib            -- bibliography (BibTeX, ieeetr style)
├── sections/
│   ├── 00_titlepage.tex                  -- front matter (unnumbered)
│   ├── 0a_abstract.tex                   -- front matter (unnumbered; keywords folded in)
│   ├── 01_introduction.tex               -- §1
│   ├── 02_background.tex                 -- §2
│   ├── 03_problem_statement.tex          -- §3
│   ├── 04_objectives_contributions.tex   -- §4
│   ├── 05_system_architecture.tex        -- §5
│   ├── 06_methodology.tex                -- §6
│   ├── 07_pipeline_design.tex            -- §7
│   ├── 08_data_flow.tex                  -- §8
│   ├── 09_experimental_setup.tex         -- §9
│   ├── 10_results.tex                    -- §10
│   ├── 11_evaluation_metrics.tex         -- §11
│   ├── 12_comparative_analysis.tex       -- §12
│   ├── 13_ablation_observations.tex      -- §13
│   ├── 14_error_analysis.tex             -- §14
│   ├── 15_engineering_decisions.tex      -- §15
│   ├── 16_scalability_performance.tex    -- §16
│   ├── 17_limitations.tex                -- §17
│   ├── 18_future_work.tex                -- §18
│   ├── 19_conclusion.tex                 -- §19
│   ├── A_appendix.tex                    -- Appendix A
│   └── B_supplementary.tex               -- Appendix B
└── figures/
    └── PLACEHOLDERS.md       -- catalogue of expected figure assets
```

The report compiles to a 42-page A4 portrait PDF.

## Building

The simplest path:

```bash
cd report
latexmk -pdf main.tex
```

Manual sequence (equivalent):

```bash
cd report
pdflatex main.tex
bibtex   main
pdflatex main.tex
pdflatex main.tex
```

`latexmk -c` cleans aux files; `latexmk -C` also removes `main.pdf`.

## Typography

The brief calls for **Spectral** (Production Type's transitional serif
designed for screen reading, distributed via Google Fonts and CTAN).

The preamble (`preamble.tex`) tries to load `\usepackage{spectral}`
first; if the package isn't available it falls back to **Bitstream
Charter** — a Matthew-Carter transitional serif from the same
design lineage, bundled with `texlive-fonts-recommended`. Charter and
Spectral share the same low-contrast, screen-optimised aesthetic, so
the substitution is far closer to the brief than Palatino was.

To switch to genuine Spectral once network access lets you install it:

```bash
tlmgr install spectral
```

The `\IfFileExists{spectral.sty}` guard in `preamble.tex` will pick it
up automatically on the next compile. No source changes needed.

Sans (Helvetica, headings/captions) and monospace (Inconsolata or
beramono fallback) are unchanged.

## Figures

Every figure in the report is currently a **placeholder block** that
records:

- the expected filename (`figures/<label>.pdf`),
- recommended dimensions (height / width fraction),
- a description of the intended content.

To replace a placeholder with a real figure, see
`figures/PLACEHOLDERS.md` for the full list and the exact swap
recipe.

## Dependencies

Packages used that ship with TeX Live `latex-recommended` /
`latex-extra`:

- `geometry, fontenc, inputenc, babel, microtype`
- `mathpazo, helvet` (font substitution; see Typography)
- `xcolor[table], booktabs, tabularx, makecell, multirow, colortbl`
- `graphicx, float, caption, subcaption, wrapfig`
- `listings, tcolorbox, titlesec, fancyhdr, hyperref, enumitem`
- `amsmath, amssymb, mathtools, bm`
- `natbib` with `ieeetr` style for the bibliography

`algorithm2e` is **not** required — the report uses lightweight
listing-style pseudocode blocks instead.

## Reproducibility

The numbers cited in the report come from the captured
`Results.md` writeup dated 2026-04-29, plus the six
`bench_results_*.txt` raw stdout captures in
`CodeBaseRaw/`. To reproduce them, build the C++ binary as
described in §11 of the report and re-run the six bench modes
against `data/sample_ward.json`.
