# Pipeline Studio — C + raylib study notes

These notes explain the current project source. Read them alongside the code, then use the exercises and questions at the end to check your understanding. The application source and tests are C, not C++.

## 1. What you have actually built

Pipeline Studio is a desktop dashboard for a local build-and-test pipeline. The user selects or drops C source files, the application asks the computer's C compiler to build them, and it optionally runs the resulting executable as a test program.

There are three stages:

| Stage | What the current implementation checks |
|---|---|
| Inspect | Queued files are readable, and at least one `.c` file exists |
| Build | The C compiler can compile and link the source files into an executable |
| Test | If enabled, that executable runs and exits with status zero |

A successful build does **not** prove the calculations are correct. A successful test run only tells you that the program's checks, if any, did not report a failure. The dashboard does not invent tests for an arbitrary file.

This prototype runs real compiler and test processes. It does not yet watch GitHub commits, run in a hosted CI service, or deploy an application. The project name describes the dashboard's intended area; its implemented scope is local build and test.

### Two different builds

This distinction makes the rest of the project easier to understand:

1. **Building the dashboard:** `make` compiles `src/main.c` and `src/pipeline.c`, linking them with raylib. The result is the windowed application.
2. **Building dropped code:** inside that application, **Run pipeline** invokes `cc` on your selected `.c` files. The result is a separate temporary executable.

Your dropped test program does not become part of the dashboard itself.

## 2. The files and how they connect

| File | Responsibility |
|---|---|
| [src/pipeline.h](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/src/pipeline.h>) | Shared data types, size limits, and function declarations |
| [src/pipeline.c](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/src/pipeline.c>) | Validation, compiler processes, test processes, status updates, logs, cleanup |
| [src/main.c](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/src/main.c>) | Window, drawing, buttons, file dropping, and the frame loop |
| [examples/passing.c](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/examples/passing.c>) | Correct addition function and three checks |
| [examples/failing.c](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/examples/failing.c>) | Deliberately incorrect addition function and a failing check |
| [examples/build_error.c](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/examples/build_error.c>) | Deliberate syntax error to demonstrate build failure |
| [tests/test_pipeline.c](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/tests/test_pipeline.c>) | Tests of the dashboard's runner itself, without the graphical interface |
| [Makefile](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/Makefile>) | Commands and dependencies for building, running, testing, and setup |
| [Start Pipeline Studio.command](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/Start Pipeline Studio.command>) | Small shell launcher for macOS; it is not application source |
| [.gitignore](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/.gitignore>) | Tells Git to ignore generated builds and local dependency files |
| [README.md](</Users/taemourbasharat/Documents/Codex/2026-10-06/wha/outputs/pipeline-studio/README.md>) | Setup, usage, limitations, and suggested development plan |
| `vendor/` | Local raylib dependency files; you do not need to explain raylib's internals |
| `build/` | Generated executables; not hand-written source |

Conceptually:

```text
Mouse / dropped files
        |
        v
main.c: user interface
        |
        v
pipeline.c: runner -----> C compiler -----> temporary executable
        |                                          |
        |                                  optional test execution
        v                                          |
Pipeline struct <----- status and log updates <----+
        |
        v
main.c draws the latest state
```

`pipeline.h` is the agreement between the interface and the runner: both include it, so both understand the same `Pipeline` structure and function signatures.

## 3. C concepts used throughout

### Variables, types, and functions

`int` stores whole numbers, `double` stores elapsed seconds with fractions, `bool` stores `true` or `false`, and `char` stores character values. In this project, `<stdbool.h>` makes `bool`, `true`, and `false` available in C11.

A function has a return type, a name, and parameters:

```c
bool pipeline_add(Pipeline *p, const char *path);
```

Read this as: “Try to add a path to a pipeline, then return whether it was added.” `void` as a return type means a function returns no value.

### Structs

A `struct` groups related values together. Instead of maintaining dozens of separate variables for a pipeline, the project groups them into `Pipeline`.

```c
Pipeline pipeline;
pipeline.active = false;
```

The dot accesses a member of a structure variable.

### Pointers: `&`, `*`, and `->`

```c
pipeline_init(&pipeline);
```

`&pipeline` means “the address of this structure.” Passing its address lets a function modify the existing structure without copying the large file-path and log arrays.

Inside a function:

```c
void pipeline_clear(Pipeline *p) {
    p->file_count = 0;
}
```

`Pipeline *p` declares a pointer to a `Pipeline`. `p->file_count` accesses the member of the structure that `p` points to. It means the same thing as `(*p).file_count`.

In `logs(const char *text, int *scroll)`, the pointer `scroll` lets the drawing function update the caller's scroll position. `*scroll` reads or changes that integer.

### Arrays and strings

```c
char files[8][4096];
```

This reserves eight arrays, each holding 4,096 characters. Each row stores one path. A C string ends with a zero byte, written `'\0'`, so space must be left for that terminator.

`files[0]` is the first path. Array indexes start at zero. `stages[2]` is the third stage.

`NULL` is a null pointer. `'\0'` is a zero character. They have different purposes even though both involve a zero value.

### `const` and `static`

- `const char *path`: the function should not modify the characters through this pointer.
- File-scope `static` on a function: that helper is private to its `.c` file.
- `static const Color BG`: a private colour value that should not change.
- A local `static` array, such as the status names, remains available after the function returns.

### Operators you will see

| Expression | Meaning |
|---|---|
| `a == b` | Compare for equality |
| `a = b` | Assign a value |
| `a != b` | Compare for inequality |
| `!value` | Logical NOT |
| `a && b` | Both conditions must be true |
| `a || b` | At least one condition must be true |
| `condition ? a : b` | Choose `a` if true, otherwise `b` |
| `count++` | Use the current value, then increment |
| `++count` | Increment, then use the new value |
| `i / 4` | Integer division when both operands are integers |
| `i % 4` | Remainder after division |

Logical `&&` and `||` short-circuit: they skip the right side when the result is already known. This matters when the right side would use a pointer that could be null.

### Useful string and memory functions

| Function | Meaning in this project |
|---|---|
| `strlen(s)` | Number of characters before the terminator |
| `strcmp(a, b)` | Returns **zero** if the strings are equal |
| `strrchr(s, '.')` | Finds the last dot, or returns `NULL` |
| `strstr(s, "FAIL")` | Finds a substring, or returns `NULL` |
| `strcpy(dst, src)` | Copies a string; the destination must be large enough |
| `snprintf(dst, capacity, ...)` | Writes formatted text within a specified capacity |
| `memset(p, 0, bytes)` | Sets that many bytes to zero |

`!strcmp(extension, ".c")` means the extension **equals** `.c`.

`sizeof *p` in this project is the size of the complete `Pipeline` structure. `sizeof p` would only be the size of its pointer. That distinction matters in `memset`.

## 4. `pipeline.h`: the shared model

### Include guard and includes

```c
#ifndef PIPELINE_H
#define PIPELINE_H
/* declarations */
#endif
```

This prevents repeated declarations if the header is included more than once in one compilation unit. A compilation unit is a source file after its included headers have been processed.

`<stdbool.h>` provides Boolean support. `<sys/types.h>` provides the process-ID type `pid_t`. `<stddef.h>` supplies common definitions such as `size_t`; it is not directly needed by the declarations currently in this header.

### Limits

- `MAX_FILES` is 8: maximum queued files, including headers.
- `PATH_CAP` is 4,096: storage capacity for each path.
- `LOG_CAP` is 32,768: storage for the in-memory tail of the output.

These are preprocessor macros: their names are replaced before compilation.

### Stage statuses

```c
typedef enum { WAITING, RUNNING, PASSED, FAILED, SKIPPED } StageStatus;
```

The default numeric values are 0, 1, 2, 3, 4. Named values make conditions readable. Because `WAITING` is zero, zeroing the stage array sets every stage to waiting in this implementation.

### Every field of `Pipeline`

| Field | What it holds |
|---|---|
| `files` | The queued source/header paths |
| `file_count` | How many entries in `files` are currently used |
| `stages[0]` | Inspect status |
| `stages[1]` | Build status |
| `stages[2]` | Test status |
| `active` | Whether a run is currently in progress |
| `run_tests` | Whether this particular run should execute the built program |
| `success` | Whether the most recently completed run succeeded |
| `child` | Operating-system ID of the active compiler or test process; zero when none is tracked |
| `started` | Time the overall run started |
| `child_started` | Time the current compiler/test process started |
| `elapsed` | Overall duration displayed in the interface |
| `directory` | Temporary build directory |
| `executable` | Path of the compiled test program |
| `log_path` | Path of the output file |
| `log` | Recent output copied into memory for drawing |
| `message` | Short status sentence below the output panel |
| `runs` | Completed runs in this application session |
| `passed_runs` | Successful runs in this session, including successful build-only runs |

The declarations below the structure announce the public functions. Their bodies live in `pipeline.c`. A declaration says how to call a function; a definition implements it.

## 5. `pipeline.c`: the runner, function by function

### Platform definitions and headers

`_DARWIN_C_SOURCE` and `_POSIX_C_SOURCE 200809L` request operating-system API declarations from the system headers. They must appear before those headers. The runner uses POSIX APIs, which is why this implementation targets macOS/Linux rather than Windows.

| Header | Why it is included |
|---|---|
| `errno.h` | Error information, including interrupted system calls |
| `fcntl.h` | File-opening flags |
| `signal.h` | Stopping a process with a signal |
| `spawn.h` | Starting child processes |
| `stdio.h` | File streams and formatted text |
| `stdlib.h` | Temporary directory creation on this platform |
| `string.h` | String and memory functions |
| `sys/stat.h` | File type and size information |
| `sys/wait.h` | Checking child-process completion |
| `time.h` | Monotonic timing |
| `unistd.h` | File access, current directory, removal, and related POSIX functions |

`extern char **environ` refers to the process's existing environment, an array of strings such as `PATH=...`. The child receives that environment. `extern` declares something defined elsewhere; it does not create a new environment.

### `pipeline_now()`

Calls `clock_gettime(CLOCK_MONOTONIC, &t)` and converts seconds plus nanoseconds to a `double` number of seconds. A monotonic clock is appropriate for elapsed time because changing the calendar clock should not change a duration.

```c
elapsed = current_time - start_time;
```

### `pipeline_status(status)`

Uses the enum's numeric value as an index into five text labels. `PASSED` selects `"Passed"`. It assumes the caller supplies a valid enum value; it does not bounds-check an arbitrary integer.

### `message(p, s)`

Copies a short sentence into `p->message` using `snprintf`. It changes the status line, not the compiler log. Text longer than the buffer would be truncated.

### `note(p, s)`

Opens the log file in append mode (`"a"`), writes a line prefixed with `[studio]`, then closes it. Existing output stays intact. If the file cannot be opened, it simply skips writing the note.

### `read_log(p)`

1. Open the output file for binary reading.
2. Seek to its end and get its size.
3. Seek back to the start, or to the start of its last 32,767 bytes if it is larger.
4. Read that data into `p->log`.
5. Add the string terminator and close the file.

The subtraction of one leaves room for `'\0'`. This is a bounded tail, not a permanent full log archive. A large tail can begin in the middle of a line. The current code does not comprehensively report seek/read errors.

### `cleanup(p)`

Deletes the temporary executable and log with `unlink`, removes the temporary directory with `rmdir`, and empties their path strings. It does not delete the user's selected source files.

`if (*p->executable)` checks the first character of the string: zero means an empty string. Cleanup ignores removal errors; extra files created by a test could prevent the directory from being removed.

### `pipeline_init(p)`

Zeroes the structure and sets the initial instruction message. This establishes a known initial state: empty queue, no active run, no child PID, zero counters, and waiting stages.

### `pipeline_add(p, path)`

This is the validation and queueing function:

1. Refuse changes while a pipeline is active.
2. Find the final dot and accept only lowercase `.c` or `.h`.
3. Check that the path refers to a readable regular file rather than a directory.
4. Check path capacity.
5. If necessary, prefix the current working directory to form an absolute path.
6. Reject an exact duplicate path string.
7. Reject the addition when eight slots are already used.
8. Copy the path into the queue, increment the count, reset stages, and update the message.

The duplicate check compares strings. It does not resolve symbolic links or normalise every `..` segment, so two differently written paths could refer to the same file.

```c
strcpy(p->files[p->file_count++], absolute);
```

If the count is 2, this copies into slot 2, then changes the count to 3. Earlier capacity checks make this particular copy fit.

The function does not examine C syntax or prove a `main()` exists. Compilation/linking handles those issues later.

### `pipeline_clear(p)`

If idle, sets the queue count to zero, resets stages, and changes the message. It does not erase the session counters or previous log. Old bytes can remain in `files`; the count determines which entries are valid.

### `spawn_command(p, arguments)` — starting another program

A **process** is a running program managed by the operating system. Here the dashboard is the parent; the compiler or test executable is its child. These are separate processes, not extra C threads in the dashboard.

This helper prepares a child process:

1. Create file-action and process-attribute objects.
2. Connect standard output to the log file in append mode.
3. Connect standard error to that same destination.
4. Connect standard input to `/dev/null`, so interactive input is unavailable.
5. Request a new process group, allowing cancellation to target ordinary descendants as well as the direct child.
6. Call `posix_spawnp` to start the requested executable.
7. Destroy the setup objects and record the start time.

The file descriptors are conventional: input is 0, output is 1, error is 2. The constants `STDIN_FILENO`, `STDOUT_FILENO`, and `STDERR_FILENO` name them.

`O_WRONLY | O_APPEND | O_CREAT` combines flags using bitwise OR. `0600` is an octal permissions value: owner read/write. It is not the decimal number six hundred.

The `p` in `posix_spawnp` means it can search `PATH` for a command such as `cc`.

Arguments are separate array entries:

```c
char *args[] = {"cc", "-std=c11", "/path/my file.c", "-o", "/tmp/program", NULL};
```

The last `NULL` marks the end. A path containing spaces stays one argument. No shell interprets punctuation in that path. This avoids problems associated with building one shell command string, but it does not sandbox code that is later executed.

Most setup failures return false; spawn/setup errors reaching the final error handler also get a log message. The first two initialisation failures return without a detailed note.

### `finish(p, success)`

Marks the run inactive, records its result and elapsed time, increments session counters, and refreshes the log. It does not immediately remove the build output; cleanup happens before a later run or at normal shutdown.

### `pipeline_start(p, run_tests)`

This begins a run rather than waiting for it to finish:

1. Reject a start if already active.
2. Recheck queued file readability and count `.c` files.
3. Reject the request if there are no C sources.
4. Clean up the previous temporary build.
5. Create a unique `/tmp/pipeline-studio-XXXXXX` directory using `mkdtemp`.
6. Construct executable and output-file paths.
7. Store the run configuration and timing.
8. Set Inspect to passed, Build to running, and Test to waiting.
9. Assemble the compiler arguments and launch the compiler.

The compiler flags are:

| Flag | Purpose |
|---|---|
| `-std=c11` | Use the C11 language standard |
| `-Wall -Wextra` | Enable common and additional warnings |
| `-pedantic` | Request further standard-conformance diagnostics |
| `-fdiagnostics-color=never` | Avoid terminal colour escapes in captured output |
| `-o path` | Choose the output executable path |

Warnings are not automatically failures: `-Werror` is not enabled. Headers are not separate compilation arguments; source files include them using `#include`. The starter does not add arbitrary external libraries or custom include paths.

The return value means the start request progressed, **not that compilation passed**. In particular, a compiler-launch failure can be recorded as a completed failed run while `pipeline_start` still returns true. The reliable outcome is in `active`, `success`, and the stages.

### `stop_child(p)`

If a child is tracked, sends `SIGKILL` to its process group, waits for the direct child to be collected, then clears its PID. A negative PID in `kill` means a process group. `waitpid` retries if interrupted with `EINTR`.

Collecting a finished child prevents an unreaped child process. This function can briefly wait during cancellation; normal progress checking is non-blocking.

### `pipeline_update(p)` — the state machine

Called once each frame. A **state machine** changes behaviour according to the current state.

If no run is active, return immediately. Otherwise update elapsed time, read output, and determine whether Build or Test is running.

Before checking completion, enforce limits:

- More than 1 MiB of logged output: stop and fail.
- Build running longer than 30 seconds: stop and fail.
- Test running longer than 5 seconds: stop and fail.

These limits are checked on updates, not imposed as exact kernel limits. Output can overshoot before the next frame. Because limit checks happen before exit checking, a just-finished process can still be marked failed if its output already exceeded the limit.

Then:

```c
pid_t result = waitpid(p->child, &status, WNOHANG);
```

| Result | Interpretation |
|---|---|
| `0` | Child has not finished; return and draw another frame |
| Positive PID | Child finished; inspect its status |
| `-1` | Error; an interruption is retried on a later update |

`WNOHANG` is the key to keeping the interface responsive. The code does not sit waiting for the compiler during ordinary frames.

`status` is encoded operating-system information, not a plain exit code. `WIFEXITED(status)` checks normal exit. Only then does `WEXITSTATUS(status)` provide the exit code. Short-circuit evaluation prevents reading an exit code as though a crashed process had exited normally.

The transitions are:

| Situation | Build | Test | Result |
|---|---|---|---|
| Compiler still running | Running | Waiting | Continue |
| Compile/link error | Failed | Skipped | Failed run |
| Build passes, execution disabled | Passed | Skipped | Successful build-only run |
| Build passes, execution enabled | Passed | Running | Start executable |
| Executable exits 0 | Passed | Passed | Successful run |
| Executable exits nonzero or crashes | Passed | Failed | Failed run |
| Running process exceeds a limit | Current stage fails | Later stage skipped if needed | Failed run |

After the direct child finishes, the code also attempts to kill remaining members of its process group. This is basic cleanup, not isolation against a deliberately evasive program.

### `pipeline_cancel(p)` and `pipeline_dispose(p)`

`pipeline_cancel` stops the child, marks the running stage failed and waiting stages skipped, records cancellation, and finishes the run. There is no separate `CANCELLED` enum in this version; the message distinguishes cancellation from a normal error.

`pipeline_dispose` cancels an active run if necessary, then removes temporary output. It is called when the window closes.

## 6. `main.c`: the interface, function by function

### Includes, dimensions, colours, and font

`raylib.h` declares drawing and input functions. `pipeline.h` declares the runner interface. The standard headers provide formatting and string comparisons.

The window is 1,280 by 860 pixels. It uses fixed coordinates; this version is not a responsive/resizable layout.

A raylib `Color` stores red, green, blue, and alpha values. For example:

```c
static const Color MINT = {111, 232, 186, 255};
```

Alpha 255 is fully opaque. `BG`, `PANEL`, `LINE`, `TEXT`, `MUTED`, `MINT`, and `ERROR_COLOR` provide a consistent palette. Change these values to restyle the application.

The file-level `font` is shared by the drawing helpers in this source file.

### Coordinates and compound literals

The top-left corner is `(0, 0)`. X increases to the right; Y increases downward.

```c
(Rectangle){240, 246, 620, 157}
```

This is a **C compound literal**: a rectangle value with x=240, y=246, width=620, height=157. It is valid C, not C++.

`(Vector2){x, y}` similarly creates a two-component position.

### `label(s, x, y, size, color)`

A small wrapper around `DrawTextEx`. It uses the shared font and consistent extra character spacing. Calling one helper avoids repeating the font argument everywhere.

### `box(r, color)`

Draws a rounded rectangle. Its roundness and segment arguments control the appearance of the corners. This is reused for cards, buttons, panels, and the checkbox.

### `hover(r)`

Gets the mouse position and checks whether it is inside rectangle `r`. Despite raylib's “collision” terminology, this is simply a point-inside-rectangle test.

### `button(r, s, primary, enabled)`

This function both draws a button and reports a click:

1. Choose a mint primary fill or darker secondary fill.
2. Brighten an enabled button when hovered.
3. Use muted styling when disabled.
4. Draw the background.
5. Measure and centre the text.
6. Change the cursor when an enabled button is hovered.
7. Return true only for an enabled, hovered button on the frame the left mouse button was pressed.

`IsMouseButtonPressed` detects a new press. It differs from `IsMouseButtonDown`, which stays true while held and could repeatedly trigger an action.

The centring calculation is:

```c
text_x = button_x + (button_width - text_width) / 2;
```

The caller decides what a click does:

```c
if (button(/* rectangle, label, appearance, enabled */)) {
    /* perform the action */
}
```

This style redraws controls and checks their input each frame. There are no persistent button objects or event-listener classes.

### `status_color(s)`

Maps passed to mint, failed to pink/red, running to amber, and waiting/skipped to muted grey. It chooses appearance only; it does not decide whether a run passes.

### `stage_card(index, title, subtitle, status)`

Draws one of the three stage cards. The horizontal position is `240 + index * 340`, giving x=240, 580, and 920.

It draws a status dot, status word, title, description, and an animated line for a running stage. The animation uses the fractional part of `GetTime()` and repeats each second.

**The line is an activity indicator, not a measured build percentage.** The application cannot determine compiler progress from elapsed time alone.

### `stat_card(x, caption, value, detail, accent)`

Draws one summary card: queued file count, completed run count, or duration. The caller prepares strings; this helper places them consistently.

### `load_demo(p, name)`

Forms a sample path from `PROJECT_ROOT`, clears the queue, and adds the sample file. `PROJECT_ROOT` is inserted by the Makefile at compile time.

It does **not** start a run or tick the execution checkbox. That is why selecting **Passing tests** does not automatically execute tests.

Moving the project folder can make the embedded sample location stale. Rebuild after moving it; if Make thinks the output is already current, use `make clean` followed by `make`.

### `logs(text, scroll)`

Turns the output string into display lines, then draws a visible slice:

1. Read characters until the end of the string or the line-storage limit.
2. Start a new line at a newline or after 110 characters.
3. Replace non-printable/non-ASCII bytes with spaces for display.
4. Adjust scroll with the mouse wheel when the pointer is over the panel.
5. Clamp scroll to valid bounds.
6. Draw up to nine lines inside a clipping rectangle.

`BeginScissorMode` and `EndScissorMode` prevent drawing outside the output panel. They do not modify the stored log.

The local `lines[1024][121]` is temporary storage, rebuilt each call. Scroll 0 selects the bottom of the parsed lines. If there are more than roughly 1,024 very short lines in the in-memory log, parsing stops at the limit and may omit later lines. This is a useful future improvement, not an unlimited log viewer.

Lines containing `error:` or `FAIL` appear red; `[studio]` lines appear mint. This is only text highlighting. **The test outcome comes from the process exit status, not searching for the word “PASS.”**

### `main(argc, argv)`: startup

`argc` is the command-line argument count. `argv` is the array of argument strings, beginning with the program name at index 0.

Startup does the following:

1. Check whether the first extra argument is `--screenshot`.
2. Request anti-aliasing and create the window.
3. Target 60 frames per second; this is a target, not a guarantee.
4. Use raylib's default font, or load Arial from the Mac's system font folder when present.
5. Set bilinear texture filtering to smooth font rendering.
6. Initialise one `Pipeline` structure.
7. Set `execute = false`, scroll to zero, and the screenshot frame count to zero.
8. Queue any command-line file paths when not in screenshot mode.

`Font custom = {0}` starts the optional font record with zero-valued members. The intended fallback is the default font when the system font file is absent. A present file that fails to load is not separately handled by this code.

### `main`: the frame loop

```c
while (!WindowShouldClose()) {
    pipeline_update(&pipeline);
    /* handle file drops */
    BeginDrawing();
    /* draw controls, handle clicks, draw results */
    EndDrawing();
}
```

The loop repeatedly updates state and redraws the screen. It is not a loop that runs the compiler repeatedly: `pipeline_update` checks an already-started process.

The actual code handles several button clicks while drawing the controls. A click can therefore start a process during that drawing pass; its completion is checked on a following loop iteration.

### Drag-and-drop, step by step

```c
if (IsFileDropped()) {
    FilePathList dropped = LoadDroppedFiles();
    /* copy accepted paths into Pipeline */
    UnloadDroppedFiles(dropped);
}
```

- `IsFileDropped` detects a completed file drop into the window.
- `LoadDroppedFiles` returns the dropped path list.
- Each path goes through `pipeline_add`.
- A rejection count becomes a short status message.
- `UnloadDroppedFiles` releases raylib's list memory after the paths have been copied.

Keeping a raw pointer from the released list would be incorrect. The code avoids this by copying accepted strings into `Pipeline.files` first.

**Current behaviour:** drops anywhere in the application window are accepted. The drawn drop-zone rectangle highlights on mouse hover, but does not restrict the input handler to that rectangle. Dropped files append to the queue. Use Clear for an unrelated project.

### Drawing the dashboard sections

| Drawing block | Purpose |
|---|---|
| Sidebar | Logo, active-page label, sample buttons, local-runner label |
| Heading | Project identity and instructions |
| Summary cards | File count, completed runs, duration |
| Drop panel | Drag-and-drop hint and queued file names |
| Run configuration | Execution checkbox, Run/Stop button, Clear button |
| Stage cards | Inspect, Build, and Test state |
| Output panel | Compiler/test messages |
| Footer | Short current message and prototype scope |

The sample buttons are disabled while active, protecting the current file queue.

`snprintf(value, ..., "%02d", count)` gives at least two digits, so 3 displays as `03`. `"%.1fs"` displays one decimal place followed by `s`.

The dashed drop border comes from repeatedly drawing small separated line segments. Queued file names use `GetFileName` to hide their directory portion and `%.29s` to limit the displayed name to 29 characters. The full path remains stored.

For queue positions, `i / 4` chooses a column and `i % 4` chooses a row. With eight slots, this creates two columns of four entries.

### The checkbox and why tests can be skipped

```c
bool execute = false;
```

The default is build-only. Clicking the checkbox area while idle performs:

```c
execute = !execute;
```

Clicking Run passes that value into:

```c
pipeline_start(&pipeline, execute);
```

The runner copies it into `p->run_tests`. When compilation succeeds:

```text
run_tests is true  -> start the built executable
run_tests is false -> mark Test as skipped
```

There are two variables because `execute` is the interface's selected setting, while `run_tests` records the setting used by the active run. The checkbox cannot change mid-run.

### Run, Stop, and Clear

The same button shows **Run pipeline** while idle and **Stop run** while active. A Run click resets log scrolling and calls `pipeline_start`; a Stop click calls `pipeline_cancel`.

The Run button becomes available when at least one file is queued, including a header. The runner performs the stronger check for at least one C source and reports an error for a header-only queue.

Clear is available only when idle with queued files. It changes the queue, not the user's files on disk.

### Screenshot and shutdown

Screenshot mode waits for three drawing passes, saves `../dashboard-preview.png` relative to the current working directory, and leaves the loop. It is a development helper, not part of a normal run.

On normal exit, the application disposes of the pipeline, unloads the optional font, closes the raylib window, and returns 0. Default window handling also allows Escape to request closing.

## 7. The three example programs

### `examples/passing.c`

```c
static int add(int a, int b) { return a + b; }
```

This function returns the sum. `main` checks three cases:

| Inputs | Expected answer | Purpose |
|---|---|---|
| `2, 3` | `5` | Positive addition |
| `-2, 2` | `0` | Opposite signs |
| `0, 0` | `0` | Zero case |

Each `if` compares actual and expected results. If wrong, the program prints a failure and immediately returns 1. If all checks succeed, it prints a summary and returns 0.

`puts` writes a string followed by a newline. The parent captures it in the log. Printing `PASS` alone would not make the process successful if it then returned 1.

### `examples/failing.c`

The function deliberately returns `a - b`. This is valid C syntax, so compilation succeeds. For `2, 3`, it returns -1 instead of 5. The check catches that logic error and returns 1.

Expected dashboard: Inspect passed, Build passed, Test failed—provided execution is enabled.

### `examples/build_error.c`

A `puts(...)` statement is missing its terminating semicolon. The compiler reports a syntax error, so there is no successful executable to test.

Expected dashboard: Inspect passed, Build failed, Test skipped.

The examples each have their own `main`. They are alternatives: do not drop all three into one run, because one executable must have one entry point. The sample buttons clear the queue before adding their chosen example.

## 8. `tests/test_pipeline.c`: testing the runner itself

There are **two levels of tests**:

- The sample program checks an addition function.
- This test file checks whether the dashboard runner builds, runs, reports, and stops programs correctly.

`make test` compiles this file with `pipeline.c`, without `main.c` or raylib. That is a practical benefit of keeping drawing and execution separate.

### Includes and `assert`

`assert(condition)` terminates the test executable with a diagnostic if a condition is false. Assertions are enabled in the provided build because it does not define `NDEBUG`.

Some assertions here also call functions, such as `assert(pipeline_start(...))`. Defining `NDEBUG` would remove those calls entirely, so do not build this test file with assertions disabled. Separating calls from checks would improve a future test-harness revision.

### `wait_for_run(p)`

Repeatedly calls `pipeline_update`, sleeping for 10 milliseconds between calls, until the run finishes or an outer 35-second guard expires. It then asserts that the run is no longer active.

This replaces the GUI's frame loop. The test helper can sleep because it has no window to redraw. It is not how the graphical application waits for a compiler.

### `run_example(p, path, execute)`

Clears the queue, adds a file, starts a run, and waits for completion. It reduces repetition in the following checks.

### What the test `main` checks

| Test | What it proves about the runner |
|---|---|
| Empty queue | Cannot start without C source |
| `.cpp` input | Unsupported C++ extension rejected |
| Missing `.c` path | Unreadable/missing input rejected |
| Passing example | Real compilation, execution, success states, and output capture work |
| Duplicate example | Same path string cannot be queued twice |
| Failing example | Successful compilation can be followed by failed tests |
| Build-error example | Compilation failure skips testing |
| Build-only failing example | Disabled execution genuinely skips the failing program |
| Unusual filename | Spaces, apostrophes, and semicolons remain part of a filename |
| Infinite loop | Test process is stopped by its time limit |
| Excessive output | Output limit is detected and the run fails |
| Cancellation | Active run stops and reports cancellation |
| Queue mutation during run | Cannot add files while active |
| Disposal | Temporary build directory is removed in the tested case |

Several fixtures are created temporarily with `fopen`, `fputs`, and `fclose`. `getpid()` adds the test process ID to their names, reducing collisions. `unlink` removes them afterward.

These checks do not prove every interface feature works. They do not automate Finder drag-and-drop, button clicks, fonts, or visual layout. They also do not currently cover every combination of headers, multiple sources, maximum queue size, or missing compilers.

## 9. Makefile, launcher, and Git ignore rules

### What Make does

Make looks at targets and dependencies. If a source dependency is newer than its output, Make executes the recipe to rebuild it.

```make
build/pipeline-studio: src/main.c src/pipeline.c src/pipeline.h | build
```

The three source/header files are normal prerequisites. `build` after `|` is an order-only prerequisite: ensure the directory exists without rebuilding merely because its directory timestamp changed.

Recipe lines begin with an actual tab. `$@` means the current target's name.

### Variables and flags

| Item | Explanation |
|---|---|
| `CC = cc` | Compiler command |
| `CFLAGS` | C11, warnings, conformance diagnostics, and `-O2` optimisation |
| `RAYLIB_DIR ?= ...` | Default dependency path, unless already supplied |
| `UNAME_S := $(shell uname -s)` | Evaluate the operating-system name while reading the Makefile |
| `ifeq (...,Darwin)` | Choose macOS framework libraries; otherwise use the listed Linux libraries |
| `-I...` | Directory to search for included headers |
| `libraylib.a` | Static library linked into the dashboard |
| `-DPROJECT_ROOT=...` | Define a source macro containing the project directory |
| `$(CURDIR)` | Current project directory as seen by Make |

Makefile syntax and the shell launcher are build/support languages. They are not C++ application code. The Mac frameworks and raylib provide platform support under the C application.

### Targets

| Command | Effect |
|---|---|
| `make` | Build the dashboard using the default `all` target |
| `make setup` | Download raylib 5.5, extract it, and build the dependency |
| `make run` | Build if necessary, then launch the window |
| `make test` | Build and execute the runner test suite |
| `make clean` | Remove the generated `build` directory |

`.PHONY` marks command-like targets so a file coincidentally named `test` or `clean` does not prevent their recipes from running.

The deliberate syntax-error example is not in the dashboard's own build target. It only goes to the compiler when selected as an input fixture.

### `Start Pipeline Studio.command`

- `#!/bin/sh`: run this launcher with a shell.
- `dirname "$0"`: find the directory containing the launcher.
- `cd ... || exit 1`: move there or stop if that fails.
- `[ ! -x build/pipeline-studio ]`: check whether the application is missing or not executable.
- `printf`: show the build instruction if needed.
- `exec ./build/pipeline-studio`: replace the launcher process with the application.

The quotation marks preserve paths containing spaces. This launcher does not download dependencies or compile the project automatically.

### `.gitignore`

Ignores `build/`, `.DS_Store`, and `vendor/` so generated executables, Finder metadata, and local dependency copies are not accidentally committed. It does not delete those files and does not automatically untrack files Git already tracks.

## 10. Trace a complete successful run

Follow this sequence in the source until you can describe it without looking:

1. `main` creates the window and initialises `Pipeline`.
2. You click **Passing tests**.
3. `button` returns true; `load_demo` clears the queue and adds `passing.c`.
4. You tick **Run trusted code as tests**; `execute` changes to true.
5. You click **Run pipeline**.
6. `pipeline_start` checks inputs, creates temporary paths, records `run_tests = true`, and launches `cc`.
7. The interface draws Build as running while subsequent frames call `pipeline_update`.
8. `waitpid(..., WNOHANG)` eventually reports that the compiler exited successfully.
9. Build becomes passed and `spawn_command` launches the temporary executable.
10. The example's three comparisons succeed. It prints its messages and returns 0.
11. A later `pipeline_update` sees that exit status, marks Test passed, and calls `finish`.
12. The next drawing pass shows the results and updated run count. Closing the app later removes temporary output.

Typical states:

```text
                           Inspect     Build       Test
Before running             Waiting     Waiting     Waiting
Compiler started           Passed      Running     Waiting
Compiler passed            Passed      Passed      Running
Test executable exited 0   Passed      Passed      Passed
```

If you leave execution unchecked, the third row is replaced by Passed / Passed / Skipped, and the run finishes there.

## 11. What this version does not yet do

Know these boundaries for code review rather than claiming unsupported features:

- No automatic trigger on a Git commit, hosted runner, or deployment stage.
- No persistent run history: counters reset when the app closes.
- No Windows process implementation yet.
- Fixed-size layout and basic font fallback.
- No directory/ZIP project import, package management, or arbitrary build-system support.
- Exact path-string duplicate checks rather than full path canonicalisation.
- Basic log clipping/highlighting rather than a full terminal or unlimited history.
- Basic process-group termination, not a security sandbox.
- No test discovery: the dropped program must contain its own checks.
- No requirement that a successful executable prints a particular phrase.

The separation between interface and runner makes these potential extensions easier to isolate. For example, persistent history would store completed run records, while replacing colours is mostly an interface change.

## 12. Small exercises that build understanding

Do one change at a time. Predict the result before running it.

### Exercise 1: Change the accent colour

Change the `MINT` RGBA values in `main.c`, rebuild with `make`, and run. Identify all controls affected by that shared value. Restore it if you prefer the original palette.

### Exercise 2: Add a fourth addition check

In `passing.c`, check that `add(-3, -4)` equals `-7`, and update the printed summary. Run it through the dashboard.

Then inspect the runner test: it searches for the exact text `3 tests passed.`. Update that expectation if you change the summary. This shows how a test can depend on a message as well as behaviour.

The dashboard itself does not need rebuilding for a changed example: it compiles the current example file on each run.

### Exercise 3: Repair the broken build

Add the missing semicolon in `build_error.c` and run it. Predict whether Test passes or is skipped based on the checkbox. Restore the deliberate error afterward, because the runner test suite expects this fixture to fail compilation.

### Exercise 4: Repair the logic bug

Change subtraction to addition in `failing.c`. Observe that Build still passes, while the Test result changes. Restore the deliberate bug afterward so the failure demonstration and test suite remain meaningful.

### Exercise 5: Add an intentional failure

Create a new C program whose `main` prints `PASS` but returns 1. Run it with execution enabled. It must fail: exit status, not the printed word, decides the result.

### Exercise 6: Explain build-only mode

Load the failing example and leave execution off. Before clicking Run, predict all three stage statuses. Explain why a successful build-only result says nothing about the faulty arithmetic.

### Exercise 7: Trace a pointer

Start at `pipeline_init(&pipeline)` in `main.c`, then follow `p` into the function. Explain why changing `p->message` affects what the UI later displays.

### Exercise 8: Plan a small extension

Sketch how you would add a Remove button for each queued file. Consider shifting later entries down, decrementing the count, rejecting changes while active, and resetting stage state. Write this plan before editing code.

## 13. Code-review questions and answers

**1. Why use raylib?**  
It handles window creation, drawing, mouse input, file-drop events, and timing for the desktop interface. The runner's compiler logic is separate.

**2. Why split the project into multiple source files?**  
It separates drawing from execution, gives each file a clear responsibility, and lets the runner be tested without a window.

**3. Why pass a `Pipeline *`?**  
So functions can update the original state and avoid copying its large arrays.

**4. Why use an enum?**  
Named statuses make state transitions clearer than unexplained integers.

**5. Why doesn't the window freeze during compilation?**  
The compiler runs as another process, and the frame loop polls with non-blocking `waitpid` rather than waiting for completion on each frame.

**6. What makes a test pass?**  
The executable exits normally with code zero. Its source is responsible for implementing meaningful checks.

**7. Why might tests be skipped?**  
Execution is disabled, or the build failed. Cancellation during the build also skips a waiting Test stage.

**8. What is the difference between a compile error and a logic error?**  
A syntax/compile error can prevent an executable from being created. A logic error can compile successfully but produce an incorrect answer, which a test may catch.

**9. How are file names containing spaces handled?**  
They are passed as individual argument-array entries to `posix_spawnp`, not concatenated into a shell command.

**10. How does drag-and-drop avoid dangling pointers?**  
Accepted paths are copied into the pipeline before the raylib path list is unloaded.

**11. Why do test programs need `main()`?**  
The runner creates an executable, so one entry point is required. A helper function file alone is not a complete test executable.

**12. What happens if there are two `main()` definitions?**  
Linking normally fails because the executable has duplicate entry-point definitions.

**13. Are warnings treated as errors?**  
No. The runner enables warnings but not `-Werror`.

**14. Is the activity bar a real percentage?**  
No. It animates while the stage is running; the runner does not measure compiler completion percentage.

**15. Is this a complete CI/CD service?**  
No. It is a local build-and-test dashboard prototype. Automated repository triggers and deployment would be additional features.

**16. Where do logs come from?**  
The compiler and test process have output/error redirected to a temporary file. The runner appends its own notes and reads recent output into memory for the UI.

**17. What does `make test` test?**  
The runner's real build, execution, failure, timeout, cancellation, and cleanup behaviours. It does not click the graphical interface.

**18. What would you improve next?**  
Choose one change you understand, such as persistent history, removing individual files, clearer test-mode controls, or better log navigation. Explain the data and functions you would change.

## 14. Suggested study sessions

| Session | What to read and do | Check you can answer |
|---|---|---|
| 1 — 30 minutes | Sections 1–4 and the passing example | What is stored in `Pipeline`, and why? |
| 2 — 45 minutes | Section 6 while looking at `main.c` | What happens when I click a button or drop a file? |
| 3 — 45 minutes | Section 5 and the successful-run trace | How does the compiler run without freezing the UI? |
| 4 — 30 minutes | Examples, runner tests, and exercises | How do build failures differ from test failures? |
| 5 — 20 minutes | Review questions, then rehearse your demo | Can I explain a full run without reading a script? |

For your development journal, record what you actually changed, what AI supplied, what you tested, and what you can now explain. Do not present suggested exercises as completed work until you have done them.
