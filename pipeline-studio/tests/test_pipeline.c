#define _POSIX_C_SOURCE 200809L
#include "pipeline.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void wait_for_run(Pipeline *p) {
    double start = pipeline_now();
    while (p->active && pipeline_now() - start < 35) {
        pipeline_update(p);
        struct timespec pause = {0, 10000000};
        nanosleep(&pause, NULL);
    }
    assert(!p->active);
}

static void run_example(Pipeline *p, const char *path, bool execute) {
    pipeline_clear(p);
    assert(pipeline_add(p, path));
    assert(pipeline_start(p, execute));
    wait_for_run(p);
}

int main(void) {
    Pipeline p;
    pipeline_init(&p);
    assert(!pipeline_start(&p, false));
    assert(!pipeline_add(&p, "example.cpp"));
    assert(!pipeline_add(&p, "missing.c"));
    puts("PASS: empty queue, C++ rejection, missing input");

    run_example(&p, "examples/passing.c", true);
    assert(p.success && p.stages[1] == PASSED && p.stages[2] == PASSED);
    assert(strstr(p.log, "3 tests passed."));
    assert(!pipeline_add(&p, "examples/passing.c"));
    puts("PASS: real build, test execution, output capture, duplicate rejection");

    run_example(&p, "examples/failing.c", true);
    assert(!p.success && p.stages[1] == PASSED && p.stages[2] == FAILED);
    assert(strstr(p.log, "FAIL:"));
    puts("PASS: test failure is reported correctly");

    run_example(&p, "examples/build_error.c", true);
    assert(!p.success && p.stages[1] == FAILED && p.stages[2] == SKIPPED);
    puts("PASS: compilation failure skips tests");

    run_example(&p, "examples/failing.c", false);
    assert(p.success && p.stages[2] == SKIPPED);
    assert(!strstr(p.log, "FAIL:"));
    puts("PASS: build-only never executes the program");

    char odd_path[PATH_CAP];
    snprintf(odd_path, sizeof odd_path, "/tmp/studio quote' spaces ; %ld.c", (long)getpid());
    FILE *f = fopen(odd_path, "w");
    assert(f); fputs("int main(void) { return 0; }\n", f); fclose(f);
    run_example(&p, odd_path, true);
    assert(p.success); unlink(odd_path);
    puts("PASS: file names with spaces, quotes and shell punctuation");

    snprintf(odd_path, sizeof odd_path, "/tmp/studio-timeout-%ld.c", (long)getpid());
    f = fopen(odd_path, "w");
    assert(f); fputs("int main(void) { for (;;) {} }\n", f); fclose(f);
    run_example(&p, odd_path, true);
    assert(!p.success && p.stages[2] == FAILED && strstr(p.log, "Time limit"));
    unlink(odd_path);
    puts("PASS: infinite test stops after timeout");

    snprintf(odd_path, sizeof odd_path, "/tmp/studio-output-%ld.c", (long)getpid());
    f = fopen(odd_path, "w");
    assert(f);
    fputs("#include <stdio.h>\nint main(void) { for (int i=0; i<200000; ++i) puts(\"excessive output\"); return 0; }\n", f);
    fclose(f);
    run_example(&p, odd_path, true);
    assert(!p.success && p.stages[2] == FAILED && strstr(p.log, "Output limit"));
    unlink(odd_path);
    puts("PASS: excessive output fails the run");

    pipeline_clear(&p);
    assert(pipeline_add(&p, "examples/passing.c"));
    assert(pipeline_start(&p, true));
    assert(!pipeline_add(&p, "examples/failing.c"));
    pipeline_cancel(&p);
    assert(!p.active && !p.success && strstr(p.log, "cancelled"));
    puts("PASS: cancellation and queue protection during run");

    char temp_dir[PATH_CAP];
    strcpy(temp_dir, p.directory);
    pipeline_dispose(&p);
    assert(access(temp_dir, F_OK) != 0);
    puts("PASS: temporary build files cleaned up");
    puts("All pipeline checks passed.");
    return 0;
}
