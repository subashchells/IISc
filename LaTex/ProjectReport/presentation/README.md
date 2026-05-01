# CardioScheduler — Presentation (LaTeX source)

Landscape Beamer presentation that mirrors the report's narrative for
in-class delivery (and is straightforward to convert to PowerPoint).
Compiled output is `slides.pdf` — 24 slides, 16:9 aspect ratio.

## Layout

```
presentation/
├── slides.tex          -- master file; all 24 frames live here
├── beamer-config.tex   -- shared style: theme, colours, macros
└── figures/            -- (place real figure assets here when ready)
```

## Slide flow (24 frames)

1.  Title
2.  Problem motivation
3.  Real-world need
4.  Existing gaps
5.  Project goals
6.  System overview
7.  Architecture (engine core)
8.  Pipeline walkthrough
9.  Key modules (per-file LOC)
10. Data processing
11. Algorithms 1/2 — ordering & lex policy
12. Algorithms 2/2 — matching
13. Infrastructure & stack
14. Experimental setup — production ward
15. Results 1 — legacy bench
16. Results 2 — 2×2 matrix
17. Results 3 — Pareto sweep
18. Results 4 — urgency bench
19. Key insights
20. Failure cases & adversarial
21. Limitations
22. Future improvements
23. Conclusion
24. Thank you / Q&A

## Building

Simplest:

```bash
cd presentation
latexmk -pdf slides.tex
```

Manual:

```bash
cd presentation
pdflatex slides.tex
pdflatex slides.tex          # second pass for cross-references
```

The presentation has **no bibliography**, so no `bibtex` pass is
needed.

## Aspect ratio

The class is loaded as
`\documentclass[aspectratio=169, 11pt]{beamer}`. To switch to 4:3,
change `aspectratio=169` to `aspectratio=43`.

## Converting to PowerPoint

Two common paths, in increasing fidelity:

1.  **PDF → editable PPT (Acrobat / Keynote / Google Slides):**
    open `slides.pdf` in Acrobat Pro and "Export to PowerPoint",
    or open in Keynote / Google Slides and re-save.

2.  **`pdf2pptx` (Linux):** preserves vector text where possible:

    ```bash
    pdf2pptx slides.pdf slides.pptx
    ```

3.  **PNG-rasterised slides for embedding:**

    ```bash
    pdftoppm -r 200 -png slides.pdf slide
    ```

    produces `slide-01.png … slide-24.png`, ready to drop into any
    deck tool.

## Typography

Same Spectral-substitution policy as the report
(`mathpazo` Palatino with `helvet` for sans). Beamer's frame titles
and structure colours use the project palette (`ruleblue`, `accent`,
`slate`).

## Figures

Slide 2, 6, 7, 8 use `\slidefig{label}{height-cm}{width-frac}{description}`
placeholders. Drop a real PDF/PNG into `figures/` named to match the
label and replace the macro call with `\includegraphics`.

## Dependencies

- `beamer` (with default theme)
- `mathpazo, helvet, microtype`
- `booktabs, tabularx, array, makecell`
- `xcolor, tikz, graphicx`
- `listings`
- `amsmath, amssymb, bm`

All ship with `texlive-latex-recommended` / `texlive-latex-extra` /
`texlive-pictures`.
