# CardioScheduler — A Complete Beginner's Guide

**Who this guide is for:**
This guide assumes you have never written a single line of code. Every word is explained.
Every concept is broken into tiny pieces. Nothing is skipped.
Take it one section at a time. There is no rush.

---

## Table of Contents

1. [What Is This Project? The Big Picture](#1-what-is-this-project-the-big-picture)
2. [The Real-World Problem This Project Solves](#2-the-real-world-problem-this-project-solves)
3. [A Tour of the Project Folder](#3-a-tour-of-the-project-folder)
4. [The Data Files — The Information the System Uses](#4-the-data-files--the-information-the-system-uses)
5. [The Models — The "Things" in the System](#5-the-models--the-things-in-the-system)
6. [Loading Data — Reading the Files](#6-loading-data--reading-the-files)
7. [The Graph — Understanding Task Dependencies](#7-the-graph--understanding-task-dependencies)
8. [The Scheduler — The Heart of the System](#8-the-scheduler--the-heart-of-the-system)
9. [Ordering Strategies — Who Goes First?](#9-ordering-strategies--who-goes-first)
10. [Matching Strategies — Who Does Each Task?](#10-matching-strategies--who-does-each-task)
11. [The Scheduling Engine — The Main Loop](#11-the-scheduling-engine--the-main-loop)
12. [Policies — The Priority Rules](#12-policies--the-priority-rules)
13. [Benchmarks — Running Experiments](#13-benchmarks--running-experiments)
14. [The Adversarial Bench — Stress Tests](#14-the-adversarial-bench--stress-tests)
15. [The Web Editor — index.html](#15-the-web-editor--indexhtml)
16. [The Build System — How to Compile](#16-the-build-system--how-to-compile)
17. [The Complete Data Flow — Step by Step](#17-the-complete-data-flow--step-by-step)
18. [How to Run the Program](#18-how-to-run-the-program)
19. [Glossary of Terms](#19-glossary-of-terms)

---

## 1. What Is This Project? The Big Picture

### What is it?

The project is called **CardioScheduler**.

The word "cardio" means related to the heart.
The word "scheduler" means something that decides when and who does each task.

So, **CardioScheduler is a program that automatically decides which hospital worker should do which medical task for which heart patient, and when**.

---

### A real-life analogy

Imagine a very busy hospital ward (a section of a hospital) that has:
- 50 patients with heart problems
- 5 staff members (nurses and doctors)
- Hundreds of medical tests and procedures to be done

The head doctor has to answer questions like:
- Which patient needs help first?
- Which nurse should take which patient's blood?
- Can a doctor be in two places at once? (No!)
- Some tests can only happen AFTER other tests finish — how do we track that?
- Are we going to run out of time in the shift?

Doing this manually is overwhelming. **CardioScheduler does all of this automatically, in milliseconds, using math and computer science.**

---

### Simple summary

> CardioScheduler is a computer program that acts like a very smart hospital manager.
> It reads information about patients, workers, and tasks.
> It figures out the best order and assignment for every task.
> It warns you if workers will have to do overtime.
> It also runs experiments to find the "best" scheduling strategy.

---

## 2. The Real-World Problem This Project Solves

### The problem

When a patient arrives at a cardiac ward, they need multiple medical tests.

For example, a patient with chest pain might need:
1. An ECG (a heart trace test)
2. A blood test (troponin) — **but this can only happen AFTER the ECG**
3. An echocardiogram (heart ultrasound)

Notice: **step 2 depends on step 1.** You cannot do step 2 before step 1.
This is called a **dependency** (more on this later).

Now multiply this by 50 patients, each with their own list of 5–15 tests, each with their own dependencies.
That's potentially 500+ tasks, all tangled together, all needing to be assigned to just 5 workers.

Without a computer, this would take hours to plan and would likely have errors.

---

### What makes it harder?

- **Not all patients are equal.** A patient in cardiac arrest needs help RIGHT NOW. A patient with mild chest tightness can wait a bit.
- **Workers have different roles.** A nurse can take blood but cannot do what a doctor does. The right task must go to the right role.
- **Workers have limited time.** Each worker has a certain number of minutes left in their shift. If you assign too much, they go into overtime.
- **Multiple goals at once.** You want to: (a) help critical patients fast, (b) finish all work quickly, and (c) not overwork any one person. These goals often conflict!

---

### What the project does about it

CardioScheduler solves this by:

1. Reading all patient, worker, and task data from files.
2. Building a map of which tasks depend on which other tasks.
3. Running a **scheduling engine** (a loop that assigns tasks to workers, step by step).
4. Trying different **strategies** (rules for who goes first, who does which task).
5. Comparing the results of those strategies using **benchmarks** (experiments).
6. Reporting which strategy is best, and warning you about overtime.

---

### Simple summary

> The project automates a very complex hospital scheduling problem.
> It respects task order (dependencies), worker roles, shift limits, and patient urgency.
> It tries multiple scheduling approaches and tells you which one works best.

---

## 3. A Tour of the Project Folder

Think of the project folder like a building. Each room (folder) has a purpose.

```
src/                           ← The entire project lives here
│
├── main.cpp                   ← The front door. Program starts here.
├── bench.cpp / bench.h        ← The experiment lab.
├── bench_adversarial.cpp/.h   ← The stress-test lab.
├── index.html                 ← A web page to edit data visually.
├── out.txt                    ← A log of a past run (output).
├── CMakeLists.txt             ← The instruction manual for building the program.
│
├── models/
│   └── models.h               ← Definitions of Patient, Worker, Task, Disease, etc.
│
├── data/
│   ├── loader.cpp / loader.h  ← Code that reads the JSON files.
│   ├── diseases.json          ← List of 418 heart diseases and their required tests.
│   ├── tests.json             ← List of all possible medical tests.
│   ├── test_dependencies.json ← Rules for which test must happen before another.
│   └── sample_ward.json       ← A sample hospital ward: 50 patients + 5 workers.
│
├── data_structures/
│   ├── graph.h / graph.cpp    ← A tool to store and navigate task dependencies.
│
├── scheduler/
│   ├── allocator.cpp/.h       ← The main scheduling engine.
│   ├── strategies.cpp/.h      ← Ordering and matching rules.
│   ├── policies.cpp/.h        ← Priority formulas.
│   ├── topological.cpp/.h     ← A tool to sort tasks respecting dependencies.
│   └── hungarian.cpp/.h       ← A math algorithm for optimal task assignment.
│
├── third_party/nlohmann/
│   └── json.hpp               ← A helper library for reading JSON files.
│
└── graphify-out/              ← Visual reports generated by an analysis tool.
    ├── graph.html             ← An interactive visual map of the code structure.
    ├── graph.json             ← Data behind that visual map.
    └── GRAPH_REPORT.md        ← A written report about the code structure.
```

---

### What is a "folder"?

A folder is like a drawer in a filing cabinet. You put related things together.
- `models/` contains definitions of what a Patient or Worker looks like.
- `data/` contains the actual hospital data and the code to read it.
- `scheduler/` contains all the logic for making decisions about who does what.

---

### What is a `.cpp` file?

A `.cpp` file is a **source code file** written in the C++ programming language.
It contains the actual instructions the computer follows.

Think of it as a recipe. The `.cpp` file is the written recipe.

---

### What is a `.h` file?

A `.h` file (called a **header file**) is like the table of contents of a recipe book.
It lists what functions and data types exist, without writing the full recipe.
Other parts of the program read the `.h` file to know what is available.

Think of it as a menu at a restaurant. The menu lists what you can order.
The kitchen (`.cpp` file) actually makes the food.

---

### What is a `.json` file?

A JSON file is a text file that stores information in a structured way.
It uses `{` curly braces `}` for objects and `[` square brackets `]` for lists.

Example of a small JSON:
```json
{
  "name": "Arun Mehta",
  "age": 55,
  "condition": "heart attack"
}
```

This is how data is stored in this project. The program reads these files at startup.

---

### Simple summary

> The project is organized into neat folders.
> `data/` holds the hospital information.
> `models/` defines what things like Patient and Worker look like.
> `scheduler/` holds all the decision-making logic.
> `main.cpp` is where the program begins.

---

## 4. The Data Files — The Information the System Uses

There are four main data files. Think of them as four different filing cabinets in a hospital.

---

### 4.1 `diseases.json` — The Disease Catalog

**What it is:**
A list of 418 heart diseases. Each disease has an ID, a name, a priority level, and a list of tests that patients with that disease need.

**Why it exists:**
Different diseases require different tests. Atrial fibrillation needs different tests than a heart attack. This file is the "lookup table" that tells the system: "If a patient has disease D001, they need tests T_ECG, T_TROP, T_ECHO."

**What goes in:**
Nothing goes IN. This is a source of information.

**What comes out:**
A lookup table the program can use to say: "Patient P001 has disease D001 → they need tests T_ECG, T_TROP, T_ECHO."

**Example entry:**
```json
{
  "id": "D001",
  "name": "ST-Elevation Myocardial Infarction (STEMI)",
  "base_priority": 1,
  "test_ids": ["T_ECG", "T_TROP", "T_ECHO", "T_HSTROPONIN"]
}
```

- `"id": "D001"` — A unique code for this disease.
- `"name"` — The human-readable name (STEMI = heart attack).
- `"base_priority": 1` — How urgent this disease is (lower number = more urgent).
- `"test_ids"` — The list of tests needed for this disease.

---

### 4.2 `tests.json` — The Test Catalog

**What it is:**
A list of all possible medical tests and procedures. There are around 100+ tests.

**Why it exists:**
Each test has a name, a duration, and a required worker role. This file says: "The ECG test takes 15 minutes and must be done by a nurse."

**What goes in:**
Nothing. It's a reference file.

**What comes out:**
Templates the program uses to create individual task instances for each patient.

**Example entry:**
```json
{
  "id": "T_VITALS",
  "name": "Vitals, SpO₂ & Fluid Balance",
  "duration_minutes": 15,
  "required_role": "NURSE"
}
```

- `"id": "T_VITALS"` — Unique code for this test.
- `"name"` — Human-readable name.
- `"duration_minutes": 15` — This test takes 15 minutes.
- `"required_role": "NURSE"` — Only a nurse can do this.

---

### 4.3 `test_dependencies.json` — The "What Must Come First" Rules

**What it is:**
A list of rules. Each rule says: "Test A must be done BEFORE Test B can start."

**Why it exists:**
In medicine, order matters. You take a blood sample (test A) before you can analyze it in the lab (test B). You cannot analyze it before taking it. These rules encode that medical reality.

**What goes in:**
Nothing. It's a reference file.

**What comes out:**
A set of "edges" (connections/arrows) used to build the dependency graph (more on this in Section 7).

**Example entries:**
```json
{ "from": "T_ECG", "to": "T_TROP" }
```

This means: "The ECG test (`T_ECG`) must be done BEFORE the troponin blood test (`T_TROP`) can start."

Think of it like this:
```
T_ECG  ──────────►  T_TROP
(do this first)      (then do this)
```

---

### 4.4 `sample_ward.json` — The Hospital Ward

**What it is:**
A snapshot of one hospital ward. It contains:
- 50 patients, each with their ID, name, age, complaint, urgency level, and disease.
- 5 workers (nurses and doctors), each with their ID, name, role, and available minutes.

**Why it exists:**
This is the real-world scenario the scheduler uses to do its work. The patients and workers are the inputs. The schedule is the output.

**What goes in:**
Nothing. This is read at startup.

**What comes out:**
The `WardData` object (explained in Section 5) that the rest of the program operates on.

**Example patient entry:**
```json
{
  "id": "P001",
  "name": "Arun Mehta",
  "age": 55,
  "complaint": "crushing chest pain, ST elevation V1-V4",
  "acuity": "CRITICAL",
  "disease_id": "D001"
}
```

- `"id": "P001"` — Unique code for this patient.
- `"name"` — Patient's name.
- `"age": 55` — Patient is 55 years old.
- `"complaint"` — What the patient came in for.
- `"acuity": "CRITICAL"` — How urgently they need help. CRITICAL = most urgent.
- `"disease_id": "D001"` — The disease they have been diagnosed with.

**Example worker entry:**
```json
{
  "id": "W001",
  "name": "Nurse Ravi",
  "role": "NURSE",
  "shift_minutes_left": 480
}
```

- `"id": "W001"` — Unique code for this worker.
- `"role": "NURSE"` — Their job. Only nurses can do nurse tasks.
- `"shift_minutes_left": 480` — They have 480 minutes (8 hours) left in their shift.

---

### The 5 workers in this ward

| ID   | Name                | Role   | Minutes Left |
|------|---------------------|--------|--------------|
| W001 | Nurse Ravi          | NURSE  | 480 min      |
| W002 | Nurse Meera         | NURSE  | 480 min      |
| W003 | Dr. Priya (Intern)  | INTERN | 600 min      |
| W004 | Dr. Kenji (Intern)  | INTERN | 600 min      |
| W005 | Dr. Anand (PGY1)   | PGY1   | 600 min      |

**What is PGY1?**
PGY1 means "Post-Graduate Year 1" — a doctor who has just finished medical school.
They can do more complex tasks than an intern.

**What is an Intern?**
A fresh medical graduate doing their first year of clinical training.

---

### Simple summary

> There are four data files: diseases, tests, dependencies, and the ward.
> The diseases file says which tests each disease needs.
> The tests file says how long each test takes and who can do it.
> The dependencies file says which test must come before another.
> The ward file says who the actual patients and workers are today.

---

## 5. The Models — The "Things" in the System

The file `models/models.h` defines the basic "nouns" of the system.
Think of it as the dictionary. Before you can talk about patients or workers, you need to define what they are.

---

### 5.1 Acuity — How Urgent Is the Patient?

```
enum class Acuity { CRITICAL = 1, HIGH = 2, MEDIUM = 3, LOW = 4 };
```

**What it is:**
A label that describes how urgently a patient needs medical attention.

**What `enum class` means:**
`enum` is short for "enumeration." It's a way to give a fixed set of names to numbers.
Instead of saying "urgency level 1," the code says `Acuity::CRITICAL`. This makes the code much easier to read.

**The four levels:**

| Name     | Number | Meaning                                               |
|----------|--------|-------------------------------------------------------|
| CRITICAL | 1      | Life-threatening. Needs help immediately.            |
| HIGH     | 2      | Serious. Needs help very soon.                       |
| MEDIUM   | 3      | Stable but needs attention within hours.             |
| LOW      | 4      | Not urgent. Can wait.                                |

**Notice:** Lower number = more urgent. This is used in priority calculations.

**Analogy:** Imagine a hospital emergency triage (sorting) system. The nurse at the door stamps your wristband: RED (critical), ORANGE (high), YELLOW (medium), GREEN (low). This tells everyone at a glance how fast to help you.

---

### 5.2 Role — What Kind of Worker Are They?

```
enum class Role { PGY1, INTERN, NURSE };
```

**What it is:**
A label for what type of healthcare worker someone is.

**Why it matters:**
Each medical task can ONLY be done by a specific role. A nurse cannot perform a procedure that requires a PGY1 doctor. The scheduler must match tasks to the correct role.

**The three roles:**
- `NURSE` — A registered nurse. Handles bedside care, blood draws, monitoring, IV lines, etc.
- `INTERN` — A first-year doctor. Handles clinical assessments, history-taking, basic procedures.
- `PGY1` — A more experienced junior doctor. Handles more complex procedures.

---

### 5.3 Status — What Is the State of a Task?

```
enum class Status { PENDING, BLOCKED, IN_PROGRESS, DONE };
```

**What it is:**
A label for where a task currently stands.

**The four states:**

| Name        | Meaning                                                  |
|-------------|----------------------------------------------------------|
| PENDING     | Not started yet, but can start (no unfinished blockers). |
| BLOCKED     | Cannot start yet — a prerequisite task isn't done.      |
| IN_PROGRESS | Currently being worked on (not used by the engine, but defined). |
| DONE        | Finished.                                               |

**Analogy:** Think of a recipe. Step 3 (mix the batter) is BLOCKED until step 2 (melt the butter) is DONE.

---

### 5.4 Task — A Unit of Work

```cpp
struct Task {
  std::string id;
  std::string name;
  Role        required_role;
  int         duration_minutes;
  int         priority;
  std::string assigned_worker;
  Status      status = Status::PENDING;
};
```

**What it is:**
A single medical task — one test or procedure that needs to be done for one patient.

**What `struct` means:**
A `struct` is a container for related pieces of information. Think of it as a form with several fields you fill in.

**The fields, explained one by one:**

- `id` — A unique text label for this task. Example: `"T_ECG_P001"` means "ECG test for patient P001."
- `name` — A human-readable name. Example: `"Vitals, SpO₂ & Fluid Balance"`.
- `required_role` — Which type of worker can do this task. Example: `Role::NURSE`.
- `duration_minutes` — How many minutes this task will take. Example: `15`.
- `priority` — A number showing how important this task is. Lower number = more important.
- `assigned_worker` — Once the task is scheduled, this stores the ID of the worker doing it.
- `status` — Whether the task is PENDING, BLOCKED, IN_PROGRESS, or DONE.

**Analogy:** A task is like a to-do card stuck to a whiteboard.
The card says: "ECG for patient Arun Mehta — 15 mins — must be done by a NURSE."

---

### 5.5 Patient — A Person Receiving Care

```cpp
struct Patient {
  std::string id;
  std::string name;
  int         age;
  std::string complaint;
  Acuity      acuity;
  std::string disease_id;
  std::vector<std::string> test_overrides;
};
```

**The fields:**

- `id` — Unique code. Example: `"P001"`.
- `name` — Full name. Example: `"Arun Mehta"`.
- `age` — Age in years. Example: `55`.
- `complaint` — What the patient described when they arrived. Example: `"crushing chest pain"`.
- `acuity` — Their urgency level. Example: `Acuity::CRITICAL`.
- `disease_id` — Which disease they have. Example: `"D001"` (heart attack / STEMI).
- `test_overrides` — Normally, the disease determines which tests are needed. But if a doctor wants to override and choose specific tests for this particular patient, they list them here.

**What is `std::vector<std::string>`?**
A `vector` is like a list (think: a shopping list). `std::string` means each item in the list is text. So `test_overrides` is a list of test IDs, written as text.

---

### 5.6 Worker — A Hospital Staff Member

```cpp
struct Worker {
  std::string id;
  std::string name;
  Role        role;
  int         shift_minutes_left;
  int         available_at = 0;
};
```

**The fields:**

- `id` — Unique code. Example: `"W001"`.
- `name` — Full name. Example: `"Nurse Ravi"`.
- `role` — Their job type. Example: `Role::NURSE`.
- `shift_minutes_left` — How many minutes they still have before their shift ends. Example: `480` (8 hours). This counts DOWN as tasks are assigned.
- `available_at` — The minute number at which this worker finishes their current task and becomes free. At the start, this is `0` (everyone is free immediately). After being assigned a 15-minute task, this becomes `15`.

**Analogy:** Think of each worker as a taxi driver. `available_at` is when their current passenger gets dropped off. `shift_minutes_left` is the fuel gauge — how much time they have before they clock out.

---

### 5.7 Disease — A Medical Condition

```cpp
struct Disease {
  std::string id;
  std::string name;
  int         base_priority;
  std::vector<std::string> test_ids;
};
```

**The fields:**

- `id` — Unique code. Example: `"D001"`.
- `name` — Full name. Example: `"ST-Elevation Myocardial Infarction (STEMI)"`.
- `base_priority` — The default urgency number for patients with this disease. This gets applied to all their tasks.
- `test_ids` — The list of test codes this disease requires. Example: `["T_ECG", "T_TROP", "T_ECHO"]`.

---

### Simple summary

> Models are like forms or ID cards for the things in the system.
> A `Task` form has fields for name, duration, role needed, and current status.
> A `Patient` form has fields for name, age, urgency level, and disease.
> A `Worker` form has fields for name, role, and how much time they have left.

---

## 6. Loading Data — Reading the Files

The files in `data/loader.h` and `data/loader.cpp` are responsible for reading the JSON files and turning them into the models defined in Section 5.

Think of `loader.cpp` as a translator: it reads the text in the JSON files and converts them into the structured objects (`Disease`, `Task`, `Patient`, `Worker`) that the rest of the program can use.

---

### The `read_file` helper function

```cpp
static string read_file(const string &path) {
  ifstream ifs(path);
  if (!ifs) throw runtime_error("Cannot open file: " + path);
  ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}
```

**Line by line:**

- `ifstream ifs(path);` — Opens the file at the given path for reading. `ifstream` means "input file stream" — a channel to read data from a file.
- `if (!ifs) throw runtime_error(...)` — If the file could not be opened (e.g., it doesn't exist), the program stops and reports an error.
- `ostringstream oss; oss << ifs.rdbuf();` — Reads the entire file content into a text buffer in memory. `oss` is like a temporary notepad.
- `return oss.str();` — Returns all the text as a single string.

**Analogy:** `read_file` is like picking up a physical document, reading every word, and typing it all into a single text message to send to someone else.

---

### `load_diseases` — Reading the Disease Catalog

```cpp
unordered_map<string, Disease> load_diseases(const string &path) {
  nlohmann::json root = nlohmann::json::parse(read_file(path));
  unordered_map<string, Disease> diseases;
  for (const auto &item : root) {
    Disease d;
    d.id            = item.at("id").get<string>();
    d.name          = item.at("name").get<string>();
    d.base_priority = item.at("base_priority").get<int>();
    for (const auto &tid : item.at("test_ids"))
      d.test_ids.push_back(tid.get<string>());
    diseases[d.id] = move(d);
  }
  return diseases;
}
```

**What is `unordered_map`?**
An `unordered_map` is like a dictionary: you look up a word (the "key") and get back its definition (the "value"). Here, the key is the disease ID (like `"D001"`) and the value is a `Disease` object.

**Line by line:**

- `nlohmann::json::parse(read_file(path))` — Reads the file and parses (interprets) it as JSON.
- `for (const auto &item : root)` — Goes through each disease in the list, one at a time.
- `d.id = item.at("id").get<string>()` — Finds the `"id"` field in the current JSON item and stores it as text.
- `for (const auto &tid : item.at("test_ids"))` — Goes through each test ID listed for this disease.
- `d.test_ids.push_back(tid.get<string>())` — Adds each test ID to the disease's test list.
- `diseases[d.id] = move(d)` — Stores the completed `Disease` object in the dictionary, indexed by its ID.

**What comes in:** The file path (location of `diseases.json`).
**What comes out:** A dictionary: `"D001" → Disease{...}`, `"D002" → Disease{...}`, etc.

---

### `load_tests` — Reading the Test Catalog

Same pattern as `load_diseases`, but reads `tests.json`.

**What comes in:** The file path.
**What comes out:** A dictionary: `"T_VITALS" → Task{...}`, `"T_ECG" → Task{...}`, etc.

Note: these tasks from the file are called **templates**. They are not tied to any specific patient yet. They are blueprints.

---

### `load_dependencies` — Reading the Ordering Rules

```cpp
vector<DepEdge> load_dependencies(const string &path) {
  nlohmann::json root = nlohmann::json::parse(read_file(path));
  vector<DepEdge> edges;
  for (const auto &item : root)
    edges.push_back({item.at("from").get<string>(),
                     item.at("to").get<string>()});
  return edges;
}
```

**What is `DepEdge`?**
It's a simple pair: `from` (which test must happen first) and `to` (which test must wait until `from` is done).

**What comes in:** The file path.
**What comes out:** A list of `DepEdge` objects. Example: `{from: "T_ECG", to: "T_TROP"}`.

---

### `load_ward` — Reading the Ward Data

This function reads the list of patients and workers from `sample_ward.json`.

**What comes in:** The file path.
**What comes out:** A `WardData` object containing a list of `Patient` objects and a list of `Worker` objects.

---

### `build_patient_tasks` — Creating Tasks for Each Patient

This is the most important function in the loader. It takes the templates from `tests.json` and creates REAL tasks for each patient.

**The problem it solves:**
The tests.json file contains templates — generic task descriptions. But we need specific tasks: "ECG for Patient P001," "ECG for Patient P002," etc. Each patient needs their own copy of each task.

**How it works, step by step:**

1. For each test the patient needs (based on their disease), the function looks up the test template.
2. It creates a copy of that template.
3. It gives it a unique ID: `test_id + "_" + patient_id`. So `"T_ECG"` for patient `"P001"` becomes `"T_ECG_P001"`.
4. It assigns the patient's disease priority to the task.
5. It adds the task to the shared task list.
6. It adds an entry to the graph (Section 7) showing that this task exists.
7. It records that this task belongs to this patient (in `task_to_patient`).
8. For dependencies: if both "test A" and "test B" are in this patient's task list, and there's a rule saying "A must come before B," then a dependency edge is added to the graph.

**Key insight:** The suffix `_P001` makes every task unique. Two patients can both need `T_ECG`, but they get `T_ECG_P001` and `T_ECG_P002` — separate tasks with separate assignments.

---

### `build_ward` — Building All Tasks for All Patients

This function calls `build_patient_tasks` for every patient in the ward.
After it runs, there is a complete list of all tasks across all patients, with all dependencies encoded in a graph.

---

### Simple summary

> The loader reads the four JSON files and converts them into in-memory objects.
> `load_diseases` builds a dictionary of diseases.
> `load_tests` builds a dictionary of test templates.
> `load_ward` builds the list of patients and workers.
> `build_patient_tasks` creates individual, patient-specific copies of each needed task.
> `build_ward` does this for all patients at once.

---

## 7. The Graph — Understanding Task Dependencies

The files `data_structures/graph.h` and `data_structures/graph.cpp` define the `DirectedGraph` class.

---

### What is a graph?

In computer science, a **graph** is not a bar chart or line graph.
It's a structure made of two things:
- **Nodes** (also called vertices): individual items.
- **Edges**: connections between items.

**Analogy:** Think of a city map.
- Each **city** is a node.
- Each **road** connecting two cities is an edge.

In this project:
- Each **task** is a node.
- Each **dependency rule** is an edge.

---

### What is a "Directed" Graph?

"Directed" means each edge has a direction — an arrow.

Instead of a two-way road, it's a one-way street:
```
T_ECG_P001  ──────►  T_TROP_P001
```

This arrow means: "You must complete the ECG for patient P001 BEFORE you can start the troponin blood test for patient P001."

---

### The `DirectedGraph` class

```cpp
class DirectedGraph {
public:
  void add_node(const std::string &id);
  void add_edge(const std::string &from, const std::string &to);
  void remove_node(const std::string &id);
  const std::vector<std::string> &neighbors(const std::string &id) const;
  std::unordered_map<std::string, int> in_degrees() const;
  bool has_node(const std::string &id) const;
  int node_count() const;
private:
  std::unordered_map<std::string, std::vector<std::string>> adj_;
};
```

**What is a `class`?**
A class is like a blueprint for a machine. It says: "This machine has these buttons (`public` functions) and these internal parts (`private` data)."

**The public functions (buttons):**

- `add_node(id)` — Adds a new task to the graph. Think: "Add this task to our map."
- `add_edge(from, to)` — Adds an arrow from task `from` to task `to`. Think: "Task `from` must happen before task `to`."
- `remove_node(id)` — Removes a task from the graph entirely. Also removes any connections to/from it.
- `neighbors(id)` — Returns the list of tasks that must wait until task `id` is done. Think: "Who am I blocking?"
- `in_degrees()` — Returns, for every task, how many OTHER tasks must be finished before IT can start. A task with `in_degree = 0` can start immediately. A task with `in_degree = 2` must wait for 2 other tasks to finish first.
- `has_node(id)` — Returns `true` if the task exists in the graph.
- `node_count()` — Returns the total number of tasks in the graph.

**The private data:**
- `adj_` — Short for "adjacency list." This is the internal dictionary that stores all the connections.

  For each task ID, it stores the list of tasks that depend on it.
  Example:
  ```
  "T_ECG_P001" → ["T_TROP_P001", "T_BNP_P001", "T_CTPA_P001"]
  ```
  This means: "After the ECG for P001, three more tests become available."

---

### `in_degrees()` — The key concept

This function is crucial for the scheduler.

**What it does:**
It counts, for each task, how many predecessors (tasks that must come before it) haven't been completed yet.

**Example:**
```
T_ECG_P001 ──► T_TROP_P001
T_ECG_P001 ──► T_BNP_P001
T_IV_P001  ──► T_TROP_P001
```

Task `T_TROP_P001` has two arrows pointing TO it: one from `T_ECG_P001` and one from `T_IV_P001`.
So its `in_degree = 2`.

**Why this matters:**
`in_degree = 0` means a task is READY TO START. No one is blocking it.
When a task gets completed, we reduce the `in_degree` of all tasks that depended on it. When any task's `in_degree` drops to `0`, it becomes READY.

---

### Simple summary

> The graph is a map of tasks and the arrows between them (dependencies).
> An arrow from Task A to Task B means: "Do A before B."
> `in_degree` tells you how many unfinished tasks are blocking a given task.
> A task with `in_degree = 0` is ready to be scheduled.
> When a task is finished, we reduce the `in_degree` of its dependents.

---

## 8. The Scheduler — The Heart of the System

The `scheduler/` folder contains the decision-making code.

It is split into several parts:
- **Ordering strategies** — Who should be worked on first?
- **Matching strategies** — Which worker should do a given task?
- **The engine** — The main loop that runs the simulation.
- **Policies** — The formulas for calculating priority scores.
- **Topological sort** — A helper that checks for circular dependencies.
- **Hungarian algorithm** — A math tool for optimal worker-task matching.

We'll explore each part one by one.

---

## 9. Ordering Strategies — Who Goes First?

File: `scheduler/strategies.h` and `scheduler/strategies.cpp`

---

### What is an ordering strategy?

When multiple tasks are READY (their `in_degree = 0`), we need to decide which ones to work on FIRST.

An **ordering strategy** is a rule for making that decision.

Two strategies are available:

1. **FCFS (First Come, First Served)**
2. **Priority (Most Urgent First)**

---

### What is a `ReadyTask`?

```cpp
struct ReadyTask {
  Task*        task;
  std::string  patient_id;
  LexKey       key;
  int          legacy_priority;
};
```

A `ReadyTask` is a "wrapper" around a `Task`. It also stores:
- `patient_id` — Which patient this task belongs to.
- `key` — A sorting key (a list of numbers) used to compare tasks.
- `legacy_priority` — A simple combined urgency number (acuity × 10 + task priority).

**What is `Task*`?**
The `*` means "pointer." A pointer is like a sticky note that says "go look at THIS task in memory" rather than making a full copy. It's efficient.

---

### FCFS Ordering (First Come, First Served)

```cpp
class FcfsOrdering : public OrderingStrategy {
private:
  std::deque<ReadyTask> q_;
};
```

**What it is:**
FCFS means "First Come, First Served." Tasks are processed in the order they become READY.

**Analogy:** A line at a grocery store. First person in line gets served first. No jumping the queue based on how urgent you are.

**Data structure: `deque`**
A `deque` (pronounced "deck") is like a queue or line. Items join at the back (`push_back`) and leave from the front (`pop_front`).

**How FCFS works:**
- When a task becomes ready, it joins the back of the queue.
- The scheduler takes from the front of the queue.
- No priority. Pure order of arrival.

**Pros:** Simple. Fair in the sense that nothing starves.
**Cons:** A low-priority task that became ready early could block critical patients from being served quickly.

---

### Priority Ordering (Most Urgent First)

```cpp
class PriorityOrdering : public OrderingStrategy {
private:
  std::vector<ReadyTask> heap_;
};
```

**What it is:**
Priority ordering always serves the MOST URGENT ready task first.

**Analogy:** An emergency room triage. Regardless of when you arrived, the patient in cardiac arrest goes in first.

**Data structure: min-heap**
A heap is a special arrangement of items where the smallest/most-urgent item is always at the top and can be retrieved instantly.

The urgency score is called a `LexKey` — a list of numbers. Smaller numbers = more urgent.
Example: `[1, 2, hash]` would beat `[1, 3, hash]` because the second number is lower.

**How Priority works:**
- When a task becomes ready, it's placed into the heap at the correct position based on its urgency.
- The scheduler always takes the MOST URGENT task from the heap.

**Pros:** Critical patients are served first.
**Cons:** A low-priority patient might wait a very long time if there are always more urgent tasks (called "starvation").

---

### The `OrderingStrategy` base class

Both `FcfsOrdering` and `PriorityOrdering` share a common interface:

- `seed(roots)` — Load the initial set of ready tasks at the start.
- `push(r)` — Add one more ready task to the queue/heap.
- `empty()` — Returns true if there are no more tasks to do.
- `drain_up_to(k)` — Take up to `k` tasks from the front/top. This is the "batch" for one scheduling round.

---

### Simple summary

> An ordering strategy decides which tasks to work on next.
> FCFS = first task that became ready, gets done first. Like a queue.
> Priority = most urgent task always goes first. Like a triage system.

---

## 10. Matching Strategies — Who Does Each Task?

File: `scheduler/strategies.h` and `scheduler/strategies.cpp`

---

### What is a matching strategy?

Once we have a BATCH of ready tasks (decided by the ordering strategy), we need to decide: **which worker does which task?**

This is called **matching** (matching tasks to workers).

Three matching strategies are available:
1. **Greedy Matcher**
2. **Fair Greedy Matcher**
3. **Hungarian Matcher**

---

### What is `MatchResult`?

```cpp
struct MatchResult {
  int          task_idx;
  int          worker_idx;
  MatchOutcome outcome;
};
```

A `MatchResult` is the decision for one task:
- `task_idx` — Which task in the current batch (its position number, starting from 0).
- `worker_idx` — Which worker was assigned (their position number in the workers list, or `-1` if none).
- `outcome` — What happened:
  - `ASSIGNED` — The task was successfully given to a worker.
  - `DEFERRED` — No available worker right now, but one exists. Try again later.
  - `NO_ROLE_MATCH` — No worker on the roster has the required role. This task can never be done!

---

### Greedy Matcher

```cpp
class GreedyMatcher : public MatchingStrategy {
  // Assigns each task to the worker who will be free soonest.
};
```

**What it does:**
For each task in the batch, it looks at all workers with the CORRECT ROLE, and picks the one who will be free the SOONEST (lowest `available_at`).

**Analogy:** You're managing a fleet of taxis. When someone needs a taxi, you dispatch the taxi that will arrive first (the one closest/most available). You don't worry about distributing rides fairly between drivers — you just want the fastest response.

**Pros:** Fast to compute. Good for throughput (getting tasks done quickly).
**Cons:** May overload one worker while another has nothing to do.

---

### Fair Greedy Matcher

```cpp
class FairGreedyMatcher : public MatchingStrategy {
  double alpha_;  // ranges from 0.0 to 1.0
};
```

**What it does:**
A "smarter" version of Greedy. It balances two goals:
1. Speed (assign to the worker who is free soonest).
2. Fairness (don't overwork one worker if others have more time left).

The `alpha` parameter controls the balance:
- `alpha = 1.0` → Pure greedy (same as Greedy Matcher).
- `alpha = 0.0` → Pure fairness (assign to the worker with the most shift time left).
- `alpha = 0.5` → Equal balance of speed and fairness.

**The formula:**
```
score = alpha × (worker's next free time) + (1 - alpha) × (-(shift time left))
```

The worker with the LOWEST score is chosen.

**Analogy:** You're dividing chores between roommates. With `alpha = 0`, you always give the chore to whoever has the most free time. With `alpha = 1`, you always give it to whoever can start right now. With `alpha = 0.5`, you balance both.

---

### Hungarian Matcher

```cpp
class HungarianMatcher : public MatchingStrategy {
  // Uses the Hungarian algorithm for globally optimal 1-to-1 matching.
};
```

**What it does:**
The Hungarian Matcher solves the "assignment problem" — given N tasks and M workers, find the BEST overall pairing that minimizes total cost.

**Key rule:** Each worker gets AT MOST ONE task per round (1-to-1 matching).
If there are more tasks than workers, the extra tasks are DEFERRED to the next round.

**What is "cost"?**
The cost of assigning task T to worker W is:
`importance(T) × worker_W.available_at`

This means:
- Assigning an IMPORTANT task to a BUSY worker is expensive (bad).
- Assigning an important task to a FREE worker is cheap (good).

The algorithm finds the assignment that minimizes total cost across all pairings.

**The importance score:**
`importance = 50 - legacy_priority`

Legacy priority ranges from 11 to 45, so importance ranges from 5 to 39.
A more urgent task has a HIGHER importance, meaning bad assignments are costlier.

**What is the Hungarian Algorithm?**
It's a well-known mathematical algorithm (from 1955) that finds the optimal pairing in a "cost matrix." It always finds the best possible solution in reasonable time.

**Analogy:** You have 5 delivery drivers and 5 packages to deliver to 5 different addresses. Some drivers are closer to some addresses. The Hungarian algorithm finds the assignment of drivers to addresses that minimizes TOTAL travel time across all 5 deliveries.

**Pros:** Globally optimal matching (the best possible assignment given the available workers).
**Cons:** Slower to compute than greedy, especially with many tasks.

---

### Simple summary

> A matching strategy answers: "Which worker does which task?"
> Greedy = give each task to whoever is free soonest. Fast but can be unfair.
> Fair Greedy = balance speed with fairness. Alpha controls the tradeoff.
> Hungarian = find the mathematically optimal assignment. Slower but globally best.

---

## 11. The Scheduling Engine — The Main Loop

File: `scheduler/allocator.cpp` and `scheduler/allocator.h`

This is the CORE of the entire project. Everything else exists to serve this engine.

---

### What is the engine?

The scheduling engine is a loop that runs until all tasks are done (or there are no more workers available). Each loop is called a **tick**.

Think of each tick like a minute-hand click on a clock. Each time the clock ticks, a batch of tasks gets assigned to workers.

---

### The inputs to `run_engine`

```cpp
std::vector<ScheduleEntry> run_engine(
    std::vector<Patient>&  patients,
    std::vector<Worker>&   workers,
    std::unordered_map<std::string, Task>& tasks,
    const DirectedGraph&   dep_graph,
    const std::unordered_map<std::string, std::string>& task_to_patient,
    EngineConfig           config,
    ...
);
```

- `patients` — The list of all patients.
- `workers` — The list of all workers (with their current availability).
- `tasks` — The dictionary of all tasks (with their current status).
- `dep_graph` — The dependency graph (which task blocks which).
- `task_to_patient` — A lookup: "Which patient does this task belong to?"
- `config` — The configuration for this run: which ordering strategy, which matching strategy, which priority policy to use.

---

### What is `EngineConfig`?

```cpp
struct EngineConfig {
  OrderingStrategy* ordering;
  MatchingStrategy* matching;
  LexPolicyFn       policy;
  std::uint32_t     policy_seed;
  std::string       label;
};
```

This is like a settings panel. Before running the engine, you select:
- `ordering` — FCFS or Priority?
- `matching` — Greedy, FairGreedy, or Hungarian?
- `policy` — Which formula to use for calculating priority scores?
- `policy_seed` — A random number to break ties deterministically.
- `label` — A name for this run (for reporting purposes).

---

### What is `ScheduleEntry`?

```cpp
struct ScheduleEntry {
  std::string task_id;
  std::string task_name;
  std::string worker_id;
  std::string worker_name;
  std::string patient_id;
  int         start_time;
  int         finish_time;
  int         effective_priority;
  bool        over_capacity = false;
};
```

One entry in the final schedule. It records:
- Which task was assigned.
- Which worker was assigned.
- Which patient it's for.
- When it starts and finishes (in minutes from the start of the shift, called T+0).
- Whether this assignment pushed the worker into overtime.

---

### The engine loop — step by step

Here is what happens inside `run_engine`, explained as a story:

**Before the loop starts:**

1. **Check for cycles.** The engine first checks if the dependency graph has any circular dependencies (e.g., "A must come before B, and B must come before A" — impossible!). If there's a cycle, it stops and reports an error.

2. **Pre-unlock already-done tasks.** If some tasks were already marked as DONE before the simulation started, the engine reduces the `in_degree` of their dependents (unblocking them).

3. **Find all "root" tasks.** A root task is one with `in_degree = 0` — nothing is blocking it. These are the tasks that can START IMMEDIATELY. They go into the ordering strategy first.

---

**During each tick:**

**Step 1: Drain a batch.**
The ordering strategy pulls out up to `N` ready tasks (where N = number of workers). This is the "batch" — the work to be done this tick.

**Step 2: Match the batch to workers.**
The matching strategy looks at the batch and the current list of workers and decides who does what. Each decision is a `MatchResult` (ASSIGNED, DEFERRED, or NO_ROLE_MATCH).

**Step 3: Process each result.**

For `NO_ROLE_MATCH`: Nobody on the roster has the required role. This task is added to the "unscheduled" list as permanently stuck. This is a warning.

For `DEFERRED`: The task can't be placed THIS tick (perhaps all workers with the right role are busy). It goes back into the ordering strategy to be tried again later.

For `ASSIGNED`: The task is placed!
- A `ScheduleEntry` is created and added to the output schedule.
- The task's `status` is changed from PENDING to DONE.
- The worker's `available_at` is updated to `current available_at + task duration`.
- The worker's `shift_minutes_left` is reduced by the task's duration.
- If `shift_minutes_left` goes negative, the worker is in OVERTIME. A violation is recorded.

**Step 4: Unlock dependents.**
After a task is marked DONE, the engine looks at all tasks that were waiting for it (the task's neighbors in the graph). For each one, its `in_degree` is reduced by 1. If any task's `in_degree` reaches 0, it becomes READY and is pushed into the ordering strategy for the next tick.

**Step 5: Repeat.**
The loop continues until the ordering strategy is empty (no more ready tasks).

---

### A concrete example (one tick)

Let's say we have:
- Batch: [T_ECG_P001 (CRITICAL, nurse), T_VITALS_P002 (HIGH, nurse)]
- Workers: [W001 = Nurse Ravi (free at T+0), W002 = Nurse Meera (free at T+0)]

**Greedy matching:**
- T_ECG_P001: Both nurses are free at T+0. Pick W001 (first alphabetically). Assigned.
- T_VITALS_P002: W002 is still free at T+0. Assigned.

**After this tick:**
- W001's `available_at` = 0 + 15 = T+15 (ECG takes 15 min).
- W002's `available_at` = 0 + 15 = T+15 (Vitals also takes 15 min).
- T_ECG_P001 is DONE → unblocks T_TROP_P001 (in-degree was 1, now 0 → READY).
- T_VITALS_P002 is DONE → may unblock other tasks for P002.

The next tick's batch will include T_TROP_P001 and any other newly-ready tasks.

---

### The data flow INSIDE the engine

```
Dependency graph
       │
       ▼
In-degree map ──► Find root tasks (in_degree = 0) ──► Seed ordering strategy
                                                              │
                                                              ▼
                                                     TICK LOOP STARTS
                                                              │
                         ┌────────────────────────────────────┘
                         │
                         ▼
             Ordering strategy: drain_up_to(N) → BATCH of ready tasks
                         │
                         ▼
             Matching strategy: match(batch, workers) → List of MatchResults
                         │
                         ▼
             For each result:
             ┌─────────────────────────────────────────────┐
             │ ASSIGNED    → create ScheduleEntry          │
             │             → mark task DONE                │
             │             → update worker.available_at    │
             │             → update worker.shift_left      │
             │             → unlock dependents             │
             │             → push newly-ready to ordering  │
             │                                             │
             │ DEFERRED    → push back to ordering         │
             │                                             │
             │ NO_ROLE_MATCH → add to unscheduled list     │
             └─────────────────────────────────────────────┘
                         │
                         ▼
             Loop back to top (drain next batch)
                         │
                         ▼ (when ordering is empty)
             Finalize capacity report
             Finalize metrics
                         │
                         ▼
             Return schedule (list of ScheduleEntry)
```

---

### `CapacityReport` — Are we going into overtime?

```cpp
struct CapacityReport {
  int  threshold_minutes;
  bool capacity_feasible;
  int  total_overtime_minutes;
  int  over_capacity_task_count;
  std::vector<CapacityViolation> violations;
  std::unordered_map<std::string, int> per_worker_overtime;
};
```

After the engine finishes, a `CapacityReport` is filled in:
- `threshold_minutes` — How many minutes of overtime are "acceptable." Default: 0 (no overtime).
- `capacity_feasible` — Was total overtime within the threshold? If not, the schedule is marked INFEASIBLE.
- `total_overtime_minutes` — Total overtime across all workers.
- `over_capacity_task_count` — How many tasks pushed a worker past their shift end.
- `violations` — A detailed list of each overtime incident.
- `per_worker_overtime` — How many overtime minutes each specific worker has.

---

### `EngineMetrics` — Performance measurements

```cpp
struct EngineMetrics {
  long long scheduler_runtime_ns;
  int       makespan_minutes;
  int       max_per_worker_overtime;
  double    stdev_overtime_minutes;
  int       deferred_event_count;
  int       tick_count;
};
```

- `scheduler_runtime_ns` — How many nanoseconds the engine took to run. (1 millisecond = 1,000,000 nanoseconds.)
- `makespan_minutes` — The finish time of the LAST task. How long did the whole schedule take?
- `max_per_worker_overtime` — The most overtime any single worker worked.
- `stdev_overtime_minutes` — Standard deviation of overtime across workers. Low = fair (everyone does similar overtime). High = unfair (one person works a lot more overtime than others).
- `deferred_event_count` — How many times a task was temporarily put back into the queue because no worker was available.
- `tick_count` — How many ticks (rounds) the engine ran.

---

### Simple summary

> The engine is the main loop.
> Each "tick," it takes a batch of ready tasks, assigns them to workers, records the schedule, and unlocks more tasks.
> It keeps going until there are no more tasks to do.
> It also tracks overtime and performance metrics.

---

## 12. Policies — The Priority Rules

File: `scheduler/policies.h` and `scheduler/policies.cpp`

---

### What is a policy?

A **policy** is a formula that calculates the priority score (called a `LexKey`) for a task.

The `LexKey` tells the ordering strategy: "How urgent is this task right now?"

A smaller `LexKey` means MORE urgent.

---

### What is a `LexKey`?

```
using LexKey = std::vector<int>;
```

A `LexKey` is simply a list of numbers. For example: `[1, 15, 4928374]`.

Two tasks are compared by their keys from left to right:
- First, compare position 1: `[1, ...]` beats `[2, ...]`.
- If position 1 is equal, compare position 2.
- And so on.

This is called **lexicographic comparison** (like alphabetical order, but for numbers).

The reason we use a list instead of a single number: **a single number can't capture all the nuance**. You might want to say "sort primarily by acuity, then by task duration, then by age." A `LexKey` lets you express this naturally as `[acuity, duration, age]`.

---

### The default policy

```cpp
LexKey default_lex_policy(const Task& t, Acuity a, int age, uint32_t seed) {
  return {acuity_int(a) * 10 + t.priority,
          static_cast<int>(tie_hash(seed, t.id))};
}
```

**What it produces:**
A two-number key: `[acuity × 10 + task_priority, random_tie_breaker]`.

Example:
- Patient in CRITICAL condition (acuity = 1), task priority = 3 → key position 1 = 1×10+3 = **13**
- Patient in HIGH condition (acuity = 2), task priority = 1 → key position 1 = 2×10+1 = **21**

Score 13 < 21, so the CRITICAL patient's task goes first. ✓

If two tasks have the same score, the tie-breaker (a hash) ensures deterministic but varied ordering.

---

### The policy catalog — 7 named policies

```cpp
{"acuity_first",       "(acuity, task.priority)"}
{"priority_first",     "(task.priority, acuity)"}
{"acuity_short_first", "(acuity, +duration, task.priority)"}
{"acuity_long_first",  "(acuity, -duration, task.priority)"}
{"acuity_old_first",   "(acuity, -age, task.priority)"}
{"acuity_young_first", "(acuity, +age, task.priority)"}
{"duration_first",     "(+duration, acuity, task.priority)"}
```

Each policy prioritizes tasks differently:

| Policy             | Meaning |
|--------------------|---------|
| `acuity_first`     | Most urgent patient first. Then by task priority. |
| `priority_first`   | Most important task first. Then by acuity. |
| `acuity_short_first` | Most urgent patient first. Among equal acuity, shortest tasks go first ("Short Job First"). |
| `acuity_long_first` | Most urgent patient first. Among equal acuity, longest tasks go first. |
| `acuity_old_first`  | Most urgent patient first. Among equal acuity, oldest patients go first. |
| `acuity_young_first` | Most urgent patient first. Among equal acuity, youngest patients go first. |
| `duration_first`   | Shortest tasks first, regardless of acuity. |

**Why have different policies?**
No single policy is best for every metric. "Acuity first" is great for critical patients but might make the overall schedule longer. "Short first" finishes more tasks faster but might neglect slow-but-critical tasks. The benchmarks explore this.

---

### Objectives — What are we trying to optimize?

```cpp
struct Objectives {
  double acuity_metric;      // Lower = critical patients finished sooner
  int    makespan_minutes;   // Lower = all work done faster
  int    max_overtime_minutes; // Lower = workers don't overwork
};
```

Three objectives are measured:

1. **Acuity metric** — The average time it takes to finish ALL tasks for CRITICAL and HIGH patients. Lower is better (patients get care faster).
2. **Makespan** — The time when the LAST task in the entire schedule finishes. Lower is better (ward finishes sooner).
3. **Max overtime** — The worst overtime of any single worker. Lower is better (fairer and legal).

**The problem:** These three goals CONFLICT.
- To lower acuity metric: rush critical patients, but this may delay everything else → longer makespan.
- To lower makespan: do lots of tasks quickly, but this overloads workers → more overtime.
- No single setting is perfect for all three at once.

This is why the Pareto sweep (Section 13.3) exists.

---

### The Pareto Frontier

**What is "Pareto"?**
A solution is on the **Pareto frontier** if you cannot improve ANY ONE metric without making another metric WORSE.

Example:
- Solution A: acuity=100, makespan=200, overtime=0
- Solution B: acuity=90, makespan=210, overtime=0
- Solution C: acuity=85, makespan=200, overtime=5

Is A on the frontier? Compare A to B: B has better acuity (90 < 100) but worse makespan (210 > 200). Neither dominates the other. So BOTH might be on the frontier.
Is C on the frontier? Compare C to B: C has better acuity (85) AND the same makespan (200) — but worse overtime. Still, neither strictly dominates.

The Pareto frontier is the set of solutions where there's no "free lunch" — every tradeoff requires giving something up.

---

### Simple summary

> A policy is a formula that scores how urgent each task is.
> Different policies prioritize things differently (acuity, task length, patient age, etc.).
> Objectives are the three things we want to minimize: acuity metric, makespan, overtime.
> The Pareto frontier is the set of "best" solutions where no improvement is free.

---

## 13. Benchmarks — Running Experiments

File: `bench.h` and `bench.cpp`

**Benchmarks** are experiments. The program runs the same ward data through different strategies and compares the results.

There are four benchmarks:

---

### 13.1 Legacy Bench (Default Mode)

**What it does:**
Runs the ward using two strategies:
1. Priority ordering + Greedy matching.
2. FCFS ordering + Greedy matching.

Then it prints a detailed report comparing the two.

**What you see in the output:**
- Full schedule timeline for each strategy (each task: who, when, for which patient).
- Per-worker workload summary.
- Comparison: critical patient first-task start times, last-task finish times.
- Overtime and feasibility.

**Question it answers:** "Does priority-based ordering actually help critical patients? By how much?"

---

### 13.2 2×2 Matrix Bench (`--bench-2x2`)

**What it does:**
Runs all four combinations of (ordering) × (matching):
- A: FCFS + Greedy
- B: FCFS + Hungarian
- C: Priority + Greedy
- D: Priority + Hungarian

And puts the results in a 2×2 table (a grid).

```
                   Greedy match    Hungarian match
  FCFS  ordering     A               B
  Priority order     C               D
```

**What you see:**
For each cell: makespan, overtime, runtime, coverage.
Pairwise comparisons: "A→D (worst to best) changed makespan by X minutes."

**Question it answers:** "How much does ordering matter? How much does matching matter? Is the fancy Hungarian algorithm worth the extra computation?"

---

### 13.3 Pareto Sweep (`--bench-pareto`)

**What it does:**
Takes all 7 policies from the catalog, runs each one with 5 different "seeds" (random tie-breakers), giving 35 total runs. Then:
1. Collects the 3 objectives (acuity, makespan, overtime) for each run.
2. Finds the Pareto frontier.
3. Uses a technique called **NSGA-II crowding distance** to pick the 6 most diverse frontier solutions.

**What is NSGA-II crowding distance?**
When many solutions are on the Pareto frontier, you want to show the most "spread out" ones — not 6 solutions that are almost identical. Crowding distance picks solutions that are far apart from each other, giving you a diverse selection of tradeoffs.

**What you see:**
- Per-policy summary: best/median/worst across seeds.
- Top 6 Pareto frontier solutions, with tags like "best acuity" or "lowest max-OT."

**Question it answers:** "Which policy is best for critical patients? Which minimizes overtime? Is there a policy that's reasonably good at all three?"

---

### 13.4 Fairness Bench (`--bench-fair`)

**What it does:**
Keeps Priority ordering FIXED. Varies only the matching strategy. Compares:
- Greedy
- Hungarian
- FairGreedy(α=0.75)
- FairGreedy(α=0.50)
- FairGreedy(α=0.25)
- FairGreedy(α=0.00)

**What is measured:**
- **Throughput:** tasks completed per minute. Higher = better.
- **Intra-role stdev:** how unevenly work is distributed within the same role group (e.g., among nurses). Lower = more fair within the role.
- **Overtime stdev:** how unevenly overtime is spread. Lower = fairer overtime distribution.

**Also shows:** An ASCII scatter plot on the terminal — a small text-art chart showing each matching mode as a dot, with throughput on the X-axis and fairness on the Y-axis. Stars indicate Pareto-optimal modes.

**Question it answers:** "Is there a matching strategy that is both fast (high throughput) and fair (even workload distribution)? What's the tradeoff between speed and fairness?"

---

### Simple summary

> Benchmarks are experiments that test different strategies.
> Legacy bench: compare Priority vs FCFS.
> 2×2 bench: test all four (ordering × matching) combinations.
> Pareto bench: test 7 policies × 5 seeds, find the best tradeoffs.
> Fairness bench: compare 6 matching modes on speed vs fairness.

---

## 14. The Adversarial Bench — Stress Tests

File: `bench_adversarial.h` and `bench_adversarial.cpp`

---

### What is an adversarial test?

A regular test uses real data. An **adversarial test** uses **specially crafted fake data designed to expose weaknesses**.

Think of it this way: a normal patient ward tests if the scheduler USUALLY works. An adversarial ward tests if the scheduler STILL works in tricky edge cases.

---

### What it does:

The adversarial bench creates several synthetic (made-up) ward scenarios in code — no JSON files needed.
Each scenario is designed to stress a specific aspect:
- A scenario with all tasks requiring the same role (to test resource bottlenecks).
- A scenario where one task has a very long chain of dependencies.
- A scenario where workers have different shift lengths.
- And others.

Each scenario is run through multiple strategies, and the results are compared.

---

### Why is this important?

Without adversarial testing, you might think your scheduler works perfectly based only on one real ward. But what if a real ward one day has 100 patients and only 1 nurse? Or all tasks happen to block each other? The adversarial bench catches these corner cases.

---

### Simple summary

> The adversarial bench uses synthetic fake wards to stress-test the scheduler.
> It doesn't need any JSON files — it builds scenarios in code.
> It checks if strategies hold up under deliberately tricky conditions.

---

## 15. The Web Editor — `index.html`

File: `index.html`

---

### What is it?

`index.html` is a **web page** (a file you can open in a browser like Chrome or Safari).

It provides a graphical interface to:
- View and edit the list of patients (add, remove, change fields).
- View and edit the list of workers.
- View and edit the test catalog.
- Load an existing `sample_ward.json` or `tests.json` file.
- Download a modified `sample_ward.json` or `tests.json` file.

---

### Why does it exist?

Editing JSON files by hand is error-prone. If you misplace a comma or forget a quote mark, the file becomes invalid and the program crashes.

The web editor provides a friendly form-based interface where you click, type, and save — without worrying about JSON syntax.

---

### How to use it:

1. Open `index.html` in a web browser.
2. Use the tabs to switch between Patients, Workers, and Tests.
3. Add/edit/delete entries using the buttons.
4. Click "Download" to save the edited file as JSON.
5. Place the downloaded file in the `data/` folder.
6. Re-run the scheduler.

---

### Simple summary

> `index.html` is a web page for visually editing the patient, worker, and test data.
> It's much easier than manually editing JSON files.
> You save changes as a new JSON file and feed it to the scheduler.

---

## 16. The Build System — How to Compile

File: `CMakeLists.txt`

---

### What does "compile" mean?

The `.cpp` files are source code — instructions written for humans to read.
A **compiler** is a program that translates that human-readable source code into machine code (binary instructions that the computer's processor can execute directly).

**Analogy:** A recipe is written in English. A robot chef can't read English. A "translator" converts the recipe into robot-readable instructions. The compiler is that translator.

---

### What is CMake?

**CMake** is a tool that helps manage the compilation process.

Instead of manually typing long compile commands (which can be very complex when there are many files), you write a `CMakeLists.txt` file that describes your project. CMake reads it and generates the correct build commands automatically.

---

### What the `CMakeLists.txt` says:

```cmake
cmake_minimum_required(VERSION 3.20)
project(CardioScheduler)
set(CMAKE_CXX_STANDARD 17)
add_executable(cardio_scheduler
  main.cpp
  bench.cpp
  bench_adversarial.cpp
  data/loader.cpp
  data_structures/graph.cpp
  scheduler/allocator.cpp
  scheduler/hungarian.cpp
  scheduler/policies.cpp
  scheduler/strategies.cpp
  scheduler/topological.cpp
)
target_include_directories(cardio_scheduler PRIVATE src third_party)
```

**Line by line:**

- `cmake_minimum_required(VERSION 3.20)` — This project needs CMake version 3.20 or newer.
- `project(CardioScheduler)` — The project's name is CardioScheduler.
- `set(CMAKE_CXX_STANDARD 17)` — Use the C++17 version of the C++ language. (Languages have versions; C++17 is from 2017 and has many modern features.)
- `add_executable(cardio_scheduler ...)` — Create a program named `cardio_scheduler` from these source files.
- `target_include_directories(...)` — When compiling, look for header files in the `src` and `third_party` folders.

---

### How to build and run (for reference)

```bash
mkdir build
cd build
cmake ..
cmake --build .
./cardio_scheduler
```

**Step by step:**
1. `mkdir build` — Create a new folder called `build` to store compiled output.
2. `cd build` — Go into that folder.
3. `cmake ..` — Run CMake on the parent directory (where `CMakeLists.txt` is).
4. `cmake --build .` — Actually compile everything.
5. `./cardio_scheduler` — Run the compiled program.

---

### What is `nlohmann/json.hpp`?

This is a **third-party library** — code written by someone else that we include in our project.

Specifically, it's the `nlohmann::json` library by Niels Lohmann. It's a single file (`json.hpp`) that makes it easy to read and write JSON in C++.

Without it, parsing JSON would require hundreds of lines of manual code.

It's stored in `third_party/nlohmann/json.hpp`. The `#include <nlohmann/json.hpp>` line in the loader makes it available.

---

### Simple summary

> CMake is the tool that manages the compilation process.
> CMakeLists.txt is the instruction file for CMake.
> You run CMake to build the `cardio_scheduler` executable.
> The `nlohmann/json.hpp` library handles reading JSON files.

---

## 17. The Complete Data Flow — Step by Step

Now let's put everything together. Here is the COMPLETE journey from raw data files to final schedule, explained as a story.

---

### Step 1 — Program starts (`main.cpp`)

The program begins at `main()`. It reads any command-line arguments (like `--bench-2x2`), then announces what it's doing.

---

### Step 2 — Load diseases

```
diseases.json
      │
      ▼
load_diseases()
      │
      ▼
diseases map: { "D001" → Disease{name:"STEMI", priority:1, tests:[T_ECG, T_TROP, ...]}, ... }
```

The program reads `diseases.json` and builds a dictionary of 418 diseases.

---

### Step 3 — Load test templates

```
tests.json
      │
      ▼
load_tests()
      │
      ▼
test_templates map: { "T_ECG" → Task{duration:15, role:NURSE}, ... }
```

The program reads `tests.json` and builds a dictionary of test templates.

---

### Step 4 — Load dependencies

```
test_dependencies.json
      │
      ▼
load_dependencies()
      │
      ▼
dep_edges list: [ {from:T_ECG, to:T_TROP}, {from:T_ECG, to:T_BNP}, ... ]
```

The program reads the dependency rules.

---

### Step 5 — Load ward

```
sample_ward.json
      │
      ▼
load_ward()
      │
      ▼
ward: { patients:[P001, P002, ...P050], workers:[W001, W002, ...W005] }
```

The program reads the 50 patients and 5 workers.

---

### Step 6 — Build all patient tasks (`build_ward`)

For EACH patient:
```
Patient P001 (disease D001 → needs [T_ECG, T_TROP, T_ECHO, ...])
      │
      ▼
build_patient_tasks()
      │
      ▼
Creates: T_ECG_P001, T_TROP_P001, T_ECHO_P001, ...
Adds them to: tasks map, dep_graph, task_to_patient map
```

After `build_ward`:
- `tasks` contains ALL tasks for ALL patients (hundreds of entries).
- `dep_graph` contains ALL dependency edges (showing which tasks block which, per patient).
- `task_to_patient` maps every task back to its patient.

---

### Step 7 — Choose a benchmark and run

The program now picks the benchmark based on the mode.

For `run_legacy_bench` (default):
1. Makes copies of tasks, workers, and patients (so the originals are preserved for the second run).
2. Creates an `EngineConfig` with Priority ordering and Greedy matching.
3. Calls `run_engine(...)`.
4. Gets back a list of `ScheduleEntry` objects (the schedule).
5. Repeats with FCFS ordering.
6. Prints both schedules and the comparison.

---

### Step 8 — Inside `run_engine`

```
dep_graph ──► in_degrees() ──► Find tasks with in_degree=0 ──► seed ordering strategy

TICK 1:
  ordering.drain_up_to(5) → batch of up to 5 tasks (one per worker)
  matching.match(batch, workers) → [ASSIGNED, ASSIGNED, DEFERRED, ...]
  For each ASSIGNED:
    → record ScheduleEntry
    → mark task DONE
    → update worker.available_at and shift_minutes_left
    → reduce in_degree of dependents
    → push newly-ready tasks to ordering

TICK 2, 3, 4, ...:
  repeat until no more ready tasks

Final:
  → finalize CapacityReport (total overtime, feasibility)
  → finalize EngineMetrics (makespan, stdev, runtime)
  → return schedule
```

---

### Step 9 — Print results

The benchmark functions print the schedule and reports to the terminal.

Each task is shown as a block:
```
┌─ [T+0 min]  P001 (CRITICAL) — Arun Mehta
│  Task    : Vitals, SpO₂ & Fluid Balance
│  Worker  : Nurse Ravi
│  Time    : T+0 → T+15 min  (15 min)
│  Priority: 13
└──────────────────────────────────────────
```

This is the final output: the complete schedule showing every task, every assignment, every time slot.

---

### Simple data flow diagram

```
diseases.json ──────────────────────────────────────────┐
tests.json ─────────────────────────────────────────────┤
test_dependencies.json ──────────────────────────────── ├──► build_ward() ──► tasks map
sample_ward.json ──────► load_ward() ──► patients+workers│                    dep_graph
                                                         │                    task_to_patient
                                                         ┘

tasks + dep_graph + patients + workers + EngineConfig
                         │
                         ▼
                    run_engine()
                         │
                    ┌────┴────┐
                    │  TICKS  │
                    └────┬────┘
                         │
              ┌──────────┴──────────┐
              │                     │
         ScheduleEntries        CapacityReport
              │                     │
              ▼                     ▼
          Print schedule       Print capacity
          Print comparison     Print metrics
```

---

### Simple summary

> The data flows from four JSON files into memory, gets expanded into per-patient tasks, goes through the scheduling engine tick by tick, and comes out as a printed schedule with overtime warnings and comparison statistics.

---

## 18. How to Run the Program

---

### Step 1: Open a terminal

A terminal is a text-based interface for your computer. On Mac: open "Terminal" app. On Linux: open a terminal emulator.

---

### Step 2: Navigate to the project folder

```bash
cd /Users/subash.c/Downloads/IISc/IASP/Project/V2/files/src
```

`cd` means "change directory." This moves you into the project folder.

---

### Step 3: Build the program

```bash
mkdir build
cd build
cmake ..
cmake --build .
cd ..
```

This compiles all the source code into an executable named `cardio_scheduler`.

---

### Step 4: Run the program

**Default mode (Legacy benchmark):**
```bash
./build/cardio_scheduler
```
or
```bash
./build/cardio_scheduler src/data
```

**2×2 matrix benchmark:**
```bash
./build/cardio_scheduler --bench-2x2
```

**Pareto sweep:**
```bash
./build/cardio_scheduler --bench-pareto
```

**Fairness sweep:**
```bash
./build/cardio_scheduler --bench-fair
```

**Adversarial stress test:**
```bash
./build/cardio_scheduler --bench-adversarial
```

**Specify a different data directory:**
```bash
./build/cardio_scheduler /path/to/my/data
```

**Set an overtime threshold (allow up to 60 minutes of overtime):**
```bash
./build/cardio_scheduler --overtime-threshold 60
```

---

### Step 5: Read the output

The output is printed to the terminal.
- Each task block shows: patient, task name, worker, start time, finish time, priority.
- The capacity report shows whether overtime occurred.
- The benchmark table compares strategies.

---

### Simple summary

> Build once with CMake. Then run `./build/cardio_scheduler` with optional flags.
> Use `--bench-2x2`, `--bench-pareto`, `--bench-fair`, or `--bench-adversarial` for different experiments.
> All output is printed to the terminal.

---

## 19. Glossary of Terms

This section defines every technical word used in the guide, in plain language.

| Term | Plain-Language Definition |
|------|--------------------------|
| **Algorithm** | A set of step-by-step instructions for solving a problem. Like a recipe. |
| **Acuity** | How urgently a patient needs medical attention. CRITICAL = most urgent. |
| **Adjacency list** | A way to store a graph: for each node, keep a list of all nodes it connects to. |
| **Batch** | A group of tasks taken together for one tick of the scheduling loop. |
| **Benchmark** | An experiment that measures the performance of one or more strategies. |
| **Binary** | The language machines understand: 0s and 1s. Compiled code is in binary. |
| **C++** | A programming language used to write this project. Fast and powerful. |
| **Capacity** | Whether the total work can fit within workers' available shift time. |
| **Class** | A blueprint for an object. It defines what data it holds and what actions it can perform. |
| **CMake** | A tool that generates build instructions for C++ projects. |
| **Compile** | Translate source code into machine-executable binary code. |
| **Crowding distance** | A measure of how "spread out" solutions are on the Pareto frontier (used in NSGA-II). |
| **Cycle (in a graph)** | A circular dependency: A must come before B, B before C, C before A — impossible. |
| **Default** | The behavior when you don't specify otherwise. |
| **Dependency** | A rule saying Task A must be completed before Task B can start. |
| **Dequeue** | Remove an item from the front of a queue. |
| **DirectedGraph** | A graph where edges have a direction (arrows, not two-way roads). |
| **enum** | A fixed set of named values (e.g., Acuity: CRITICAL, HIGH, MEDIUM, LOW). |
| **Edge (in a graph)** | A connection between two nodes, representing a relationship. |
| **Engine** | The core scheduling loop that assigns tasks to workers over time. |
| **FCFS** | First Come, First Served. Process tasks in the order they become ready. |
| **Feasible** | A schedule is feasible if it can be executed without violating constraints (like overtime). |
| **Graph** | A structure of nodes and edges. Used here to represent task dependencies. |
| **Header file (.h)** | A C++ file that declares what functions and types exist. Like a menu. |
| **Heap (data structure)** | A tree-like structure where the smallest (or largest) element is always at the top. |
| **Hungarian algorithm** | A math algorithm for finding the optimal 1-to-1 assignment in a cost matrix. |
| **In-degree** | The number of edges pointing INTO a node. Zero means the task is ready to start. |
| **JSON** | A text format for storing structured data. Uses `{`, `}`, `[`, `]`. |
| **Lex key** | A list of numbers used for multi-dimensional comparison (like sorting by last name, then first name). |
| **Lexicographic order** | Comparing items like a dictionary: compare the first element, then the second if tied, etc. |
| **Makespan** | The time at which the last task in the schedule finishes. |
| **Matching** | Deciding which worker does which task. |
| **Model** | A data structure representing a real-world object (Patient, Worker, Task, etc.). |
| **Node (in a graph)** | An individual item. Here: a task. |
| **Objective** | A goal to minimize or maximize (e.g., makespan, overtime). |
| **Ordering** | Deciding which ready tasks to work on first. |
| **Overtime** | Time worked beyond the end of a worker's scheduled shift. |
| **Pareto frontier** | The set of solutions where improving one objective requires worsening another. |
| **Pointer** | A reference to a location in memory. Like a sticky note that says "look here." |
| **Policy** | A formula for calculating a task's urgency score (LexKey). |
| **Priority** | A number representing urgency. Lower = more urgent (in this project). |
| **Queue** | A line where items join at the back and leave from the front. |
| **Role** | The type of healthcare worker: NURSE, INTERN, or PGY1. |
| **Root task** | A task with no prerequisites (in_degree = 0). Ready to start immediately. |
| **Scheduler** | The system that decides who does what task, and when. |
| **Simulation** | Running the scheduling engine on a set of inputs to see what schedule results. |
| **Source code** | The human-readable instructions written by programmers (.cpp files). |
| **Standard deviation (stdev)** | A measure of how spread out numbers are. Low stdev = everyone is similar. High stdev = large variation. |
| **Status** | The current state of a task: PENDING, BLOCKED, IN_PROGRESS, or DONE. |
| **Strategy** | A choice of algorithm or rule. The system has multiple interchangeable strategies. |
| **Struct** | A container for related data fields. Like a form with named boxes to fill in. |
| **Task template** | A generic task from tests.json, not yet tied to any specific patient. |
| **Threshold** | A limit or cutoff point. Overtime above the threshold makes the schedule "infeasible." |
| **Tick** | One iteration of the scheduling loop. One round of batch assignment. |
| **Topological sort** | Ordering tasks so that every prerequisite appears before the task that needs it. |
| **Unordered map** | A fast dictionary (lookup by key) with no guaranteed ordering. Like a real dictionary. |
| **Vector** | A resizable list. Can grow or shrink. Items can be accessed by position. |
| **Ward** | A section of a hospital dedicated to one type of care (here: cardiac/heart patients). |

---

## Final Overview Diagram

Here is the entire project summarized in one diagram:

```
                    ┌────────────────────────────────────────────┐
                    │            THE PROJECT (big picture)       │
                    └────────────────────────────────────────────┘
                                         │
              ┌──────────────────────────┼──────────────────────────┐
              │                          │                          │
              ▼                          ▼                          ▼
         INPUT DATA               CORE ENGINE                   OUTPUT
         (4 JSON files)           (C++ code)                   (terminal)
              │                          │                          │
    ┌─────────┴────────┐      ┌──────────┴──────────┐    ┌─────────┴────────┐
    │ diseases.json    │      │ Ordering Strategy   │    │ Schedule entries │
    │ tests.json       │─────►│ (FCFS or Priority)  │───►│ (who, what, when)│
    │ test_deps.json   │      │                     │    │                  │
    │ sample_ward.json │      │ Matching Strategy   │    │ Capacity report  │
    └──────────────────┘      │ (Greedy/Hungarian/  │    │ (overtime info)  │
                              │  FairGreedy)        │    │                  │
                              │                     │    │ Benchmark tables │
                              │ Policy              │    │ (comparisons)    │
                              │ (acuity/priority/   │    └──────────────────┘
                              │  duration/age...)   │
                              │                     │
                              │ Dependency Graph    │
                              │ (task order rules)  │
                              └─────────────────────┘
```

---

*This guide was written to be read slowly. If any section is unclear, re-read it once more before moving on. Every concept is here — nothing was left out.*
