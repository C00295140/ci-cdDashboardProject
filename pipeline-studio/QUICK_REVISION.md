# Pipeline Studio — quick revision

## Explain the project in one sentence

“My project is a C and raylib desktop dashboard that accepts C source files, compiles them, optionally runs their test program, and shows the actual results and output.”

## Remember the three main files

| File | Think of it as | What it does |
|---|---|---|
| `main.c` | The screen | Draws the interface and handles clicks and drops |
| `pipeline.h` | The shared model | Declares the `Pipeline` structure, statuses, and functions |
| `pipeline.c` | The runner | Starts the compiler and test program, then checks their results |

Raylib draws the screen. The operating system runs the compiler. Your dropped program contains the tests.

## Follow the data

```text
Choose/drop files
    -> pipeline_add stores the paths
Click Run
    -> pipeline_start launches cc
Each frame
    -> pipeline_update checks completion
    -> main.c draws the stored status
Build passes + execution enabled
    -> run the resulting executable
Executable exits
    -> record pass/fail and display output
```

The compiler and test program run as child processes. `waitpid(..., WNOHANG)` checks them without waiting for completion, so the normal frame loop stays responsive.

## The test checkbox

1. **Passing tests** loads a sample only.
2. **Run trusted code as tests** enables execution.
3. **Run pipeline** begins the build.

The checkbox starts off. Build success with the checkbox off means **Test: Skipped**.

## Predict the result

| Input and setting | Build | Test |
|---|---|---|
| Passing example, execution on | Passed | Passed |
| Failing example, execution on | Passed | Failed |
| Broken syntax example | Failed | Skipped |
| Passing or failing example, execution off | Passed | Skipped |

A missing semicolon is a compile error. Subtracting when you meant to add is a logic error. Valid syntax can still produce an incorrect answer.

## C vocabulary to know

| Code | Meaning |
|---|---|
| `Pipeline pipeline;` | A structure variable containing the state |
| `&pipeline` | Address of that variable |
| `Pipeline *p` | A pointer to a pipeline |
| `p->active` | Access a field through a pointer |
| `stages[2]` | Third element: Test stage |
| `const char *path` | Pointer used to read path characters |
| `!strcmp(a, b)` | True when the strings are equal |
| `return 0;` in test `main` | Successful process exit |
| `return 1;` in test `main` | Failure process exit |
| `static` on a helper function | Private to that source file |
| `(Rectangle){x,y,w,h}` | A C rectangle value; not C++ |

The word `PASS` in output does not decide success. The program's exit status does.

## Build commands

Run these from the project folder:

```sh
make       # build the dashboard
make run   # build if necessary, then open it
make test  # test the runner without opening the window
```

Editing dashboard source requires rebuilding. Editing a sample requires rerunning the pipeline, which compiles the sample again.

## Limits to explain honestly

Up to eight `.c`/`.h` files, one executable with one `main`, build timeout 30 seconds, test timeout 5 seconds, basic output limits, session-only counters, and no hosted CI or deployment yet. Execution is local with your user permissions. The activity animation is not a percentage.

## Rehearse these five questions

1. How does a dropped path reach the compiler?
2. Why can the build pass but the tests fail?
3. Why does the window stay responsive?
4. What does passing a structure by pointer achieve?
5. What would you change to save run history between sessions?

Read the full study notes for the function-by-function explanations and answers.
