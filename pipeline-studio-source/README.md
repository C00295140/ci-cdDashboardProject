# Pipeline Studio

A C11 + raylib course-project starter: a dark desktop dashboard for dropping C source files, building them, and optionally running a test program. All application and test source is C. No C++.

## Start on this Mac

Open Terminal in this folder and run:

```sh
make run
```

The compiled application is also in `build/pipeline-studio`. The initial build embeds this project's directory so sidebar samples work when launched from elsewhere. Rebuild after moving the project.

You can also double-click `Start Pipeline Studio.command`. The separate source ZIP excludes compiled binaries and raylib; run `make setup` and `make` after extracting that ZIP.

1. Choose **Passing tests** in the sidebar, or drag `.c` and `.h` files from Finder into the window.
2. Turn on **Run trusted code as tests** to execute your test program. Leave it off for a build-only check.
3. Press **Run pipeline**. Inspect, Build and Test show the actual outcomes.
4. Try **Failing tests** and **Build error** to demonstrate the difference between a logic bug and a compilation error.
5. Use **Clear** before dropping an unrelated project: dropped files are added to the current queue.

## Set up on another computer

Requires a C compiler named `cc`, Make, and raylib 5.5. This first runner implementation supports **macOS and Linux** using POSIX process APIs. It has been checked on macOS; Windows needs a separate process runner. The UI itself uses raylib's cross-platform API.

On macOS, install Apple's command-line tools with `xcode-select --install` if needed. To fetch and build raylib, then build this project:

```sh
make setup
make
make run
```

On Linux, install a compiler and the desktop development libraries required by raylib before `make setup`; see the official [raylib Linux guide](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux). No raylib library installation outside this project is needed.

You can also point Make at an existing raylib source build:

```sh
make RAYLIB_DIR=/absolute/path/to/raylib/src
```

## What can I drop?

Up to eight `.c` and `.h` files belonging to **one small command-line C project**. Include exactly one `main()`. Standard C library code works out of the box. Headers should remain beside the source files that include them; headers are listed in the queue but are not separate compilation units. Source files are read in place, not copied or changed.

This starter does not discover a project directory, resolve external libraries, build arbitrary raylib projects, accept ZIP files, or interpret Makefiles. It rejects C++ files. Those would be separate extensions to scope with your lecturer.

## What does “passed” mean?

- **Inspect:** input files are readable and at least one C source file exists.
- **Build:** `cc -std=c11 -Wall -Wextra -pedantic` compiles and links the submitted sources. Warnings are shown but do not fail a build.
- **Test:** when enabled, the resulting program runs. Exit code zero means success; a nonzero exit code or crash means failure. Your C program must contain actual checks, like the provided samples. A successful exit does not prove arbitrary code is correct.
- **Build-only:** the Test stage is explicitly skipped; successful-run counts include successful build-only runs.

Tests run locally with your account's permissions, not in a sandbox. Dropping files does not execute them. Only enable test execution for code you trust. Programs requiring interactive input are not supported. Build timeout: 30 seconds; test timeout: 5 seconds. Output over 1 MB is detected on the next frame and stops the run. **Stop run** cancels the active process group. The UI stays responsive because the runner checks process completion each frame instead of blocking.

This is a **local build-and-test dashboard prototype**, not a hosted CI service or deployment system. Run counters last for the current session. The log viewer shows the most recent 32 KB of output; temporary compiler output and executables are removed on a new run or normal exit.

## How to understand the code

Read the project in this order:

1. `examples/passing.c`: a tiny, complete C test program.
2. `src/pipeline.h`: structs, enums and the runner's public functions.
3. `src/pipeline.c`: file validation, compilation, test execution and state transitions.
4. `src/main.c`: raylib drawing, buttons, drag-and-drop, logs and frame loop.
5. `tests/test_pipeline.c`: automated checks of the actual runner, without opening a window.

The frame loop handles input, updates the runner, and draws the screen. Drag-and-drop uses `IsFileDropped`, `LoadDroppedFiles`, and `UnloadDroppedFiles`, as documented in the [raylib cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html).

The runner passes file names as separate process arguments rather than joining them into a shell command. This handles spaces and punctuation safely. It uses `posix_spawnp` and `waitpid`; there are no C++ constructs or C++ build targets.

## Verify your work

```sh
make test
```

This checks success, failing assertions, compiler errors, build-only mode, duplicate/invalid inputs, unusual file names, timeout, cancellation, and temporary-file cleanup. The intentionally invalid `examples/build_error.c` is a fixture; do not add it to the dashboard's own compilation sources.

Manual UI checklist for your journal:

- Drop a passing example from Finder and verify the file name appears.
- Drop a `.cpp`, a directory, a duplicate, and more than eight files; check the messages.
- Run the passing sample with execution enabled and verify all three stages pass.
- Run the failing sample; Build passes and Test fails.
- Run the broken sample; Build fails and Test is skipped.
- Leave execution disabled; Test must be skipped even for a successful build.
- Scroll lengthy logs and click Stop during a run.

## Course development plan

**Week 1:** Understand the supplied source, run the samples, test drag-and-drop, choose your final layout, and create your group's GitHub repository. Record which parts were AI generated and what each member has reviewed.

**Week 2:** Add persistent run history and individual file removal. One person leads runner/data changes; the other leads UI work. Review each other's code.

**Week 3:** Test edge cases, fix bugs, polish the demo, and finish journals and screencasts. Demonstrate a working run, a compile failure, a test failure and a corrected rerun.

Future extensions: export logs, store history in CSV, configure warning policy, or read GitHub Actions results. Keep deployment out of the first version unless your lecturer requires it.
