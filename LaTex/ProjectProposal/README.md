# Proposal

This directory contains the LaTeX source for the capstone project proposal.

## Structure

```
Proposal/
├── proposal.tex          ← Main file (compile this)
├── references.bib        ← BibTeX bibliography
├── Objective/            ← Section: What you intend to do
├── Rationale/            ← Section: Why this project matters
├── Approach/             ← Section: Your methodology
├── Timeline/             ← Section: Milestones and deadlines
└── PossibleIssues/       ← Section: Anticipated challenges
```

## How to Compile

```bash
cd Proposal/
pdflatex proposal
bibtex proposal
pdflatex proposal
pdflatex proposal
```

The final output is `proposal.pdf`.
