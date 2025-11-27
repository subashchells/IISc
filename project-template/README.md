# Project Template 

This is the project template for the Ethics in AI (DS 307) at Indian Institute of Science. 


# Goals

### Proposal Stage 

The total page limit is 4 pages. You would lose 2 grades if your report is longer than that. 
References, appendices and some sections (wherever mentioned) do not count towards the page limit. 

The proposal stage should have: (a) abstract, (b) introduction, (c) related work, and (d) action plan.

**Abstract** introduces the problem at a high level. **Introduction** 
includes further details about (a) problem statement, 
(b) why is it important, (c) how are things done currently (closely related work),
and (d) expected outcomes from the project, along with the broader impact it may have.
**Related work** covers all relevant work, with clear emphasis on how it is relevant to what you want to do, 
and for closely related studies, how is your study different?
Lastly, you should include an **action plan** outlining 
specific tasks with associated timelines.



Rubric for Grading:

**A+ (Outstanding)**:
- Problem is clearly formulated, specified and motivated
- Comprehensive literature review (well synthesized and the relationship between the proposed work and past work is clearly established)
- Thorough feasibility analysis (establishing that you have all the ingredients to solve the problem)
- Concrete and actionable plan of attack
- Beautifully-written report demonstrating strong understanding  
- Goes beyond most submissions


**A (Excellent)**:
- Problem is well formulated, justified and motivated
- Literature survey is almost complete 
- Feasibility has been established 
- Plan is  concrete and actionable 
- Well-written report 
- Meets all expectations


**B+ (Good)**
- Problem statement is almost there, but there is room to be clearer
- Literature survey covers many important pieces but misses some key pieces
- A reasonable attempt to assess feasibility
- Fair plan 
- Reasonable report 
- Does not meet one one or more expectations but largely on track



**B (Satisfactory)**
- Problem statement is somewhat there, but room to improve 
- Literature survey exists but misses several important studies, and does not connect to the proposed work
- Half-decent attempt to assess feasibility 
- Action plan is somewhat there 
- Mediocre report that could easily be better 
- Does not meet two one or more expectations and needs work to be on track 



**C or below (Needs Improvement)**
- Problem statement is unclear, vague, or unfocused 
- Literature survey misses several important papers
- Shallow attempts to assess feasibility  
- Not a clear action plan
- Poor or confused writing 
- Needs serious work to be on track



### Mid-term Report

The total page limit is 6 pages. You would lose 2 grades if your report is longer than that. 
References, appendices and some sections (wherever mentioned) do not count towards the page limit. 

The mid-term stage should have:  abstract,  introduction,  related work, and 
sections corresponding to approach, experiments and results/progress made so far. 
The specific names of sections are less important, 
but 
we would like to see whether you have made progress 
on the action plan outlined in the proposal stage. 
Any major roadblocks or risks should be identified by this point of time.  



# Organization


## Paper structure

    .
    ├── project_template.tex    # main tex file 
    ├── project_template.sty    # main sty file 
    ├── project_template.bib    # main bib file
    ├── project_template.bst    # main bst file
    ├── Makefile                # make file to build LaTeX code locally
    ├── sections                # Folder for all sections 
    │   ├── abstract.tex        # abstract
    │   ├── intro.tex           # introduction
    │   ├── related.tex         # related work
    │   ├── methods.tex         # Methodology (where our contributions start)
    │   ├── experiments.tex     # experimental setup and results 
    │   ├── conclusion.tex      # conclusion 
    ├── figures                 # All figure files
    │   ├── overall_figure.pdf  # Image 1
    │   ├── comparison.png      # Image 2
    ├── tables                  # All Table files
    │   ├── overall_table.tex   # Table LaTeX file
    └── README.md               # can contain TODOs, notes, etc.


Feel free to include more sections as and when necessary.

## How to build?

If you have LaTeX installed locally you'd have to run `make` in your terminal (assuming you have a `Makefile`). Please see the sample `Makefile` in this folder for more details.

If you use `vscode` or `cursor` you can build directly from there. You can use the "LaTeX Workshop" package to do so.
