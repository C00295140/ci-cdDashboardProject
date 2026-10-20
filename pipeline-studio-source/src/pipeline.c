#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "pipeline.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

double pipeline_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1000000000.0;
}

const char *pipeline_status(StageStatus status) {
    static const char *names[] = {"Waiting", "Running", "Passed", "Failed", "Skipped"};
    return names[status];
}

static void message(Pipeline *p, const char *s) {
    snprintf(p->message, sizeof p->message, "%s", s);
}

static void note(Pipeline *p, const char *s) {
    FILE *f = fopen(p->log_path, "a");
    if (f) { fprintf(f, "[studio] %s\n", s); fclose(f); }
}

static void read_log(Pipeline *p) {
    FILE *f = fopen(p->log_path, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, size > LOG_CAP - 1 ? size - (LOG_CAP - 1) : 0, SEEK_SET);
    size_t n = fread(p->log, 1, LOG_CAP - 1, f);
    p->log[n] = '\0';
    fclose(f);
}

static void cleanup(Pipeline *p) {
    if (*p->executable) unlink(p->executable);
    if (*p->log_path) unlink(p->log_path);
    if (*p->directory) rmdir(p->directory);
    p->directory[0] = p->executable[0] = p->log_path[0] = '\0';
}

void pipeline_init(Pipeline *p) {
    memset(p, 0, sizeof *p);
    message(p, "Drop C source files to get started.");
}

bool pipeline_add(Pipeline *p, const char *path) {
    if (p->active) { message(p, "Stop the current run before changing files."); return false; }
    const char *extension = strrchr(path, '.');
    if (!extension || (strcmp(extension, ".c") && strcmp(extension, ".h"))) {
        message(p, "Only .c and .h files are supported. C++ is not accepted."); return false;
    }
    struct stat st;
    if (stat(path, &st) || !S_ISREG(st.st_mode) || access(path, R_OK)) {
        message(p, "That file is missing or cannot be read."); return false;
    }
    if (strlen(path) >= PATH_CAP - 2) { message(p, "File path is too long."); return false; }
    char absolute[PATH_CAP];
    if (path[0] == '/') snprintf(absolute, sizeof absolute, "%s", path);
    else {
        if (!getcwd(absolute, sizeof absolute)) return false;
        size_t len = strlen(absolute);
        if (len + strlen(path) + 2 > sizeof absolute) return false;
        absolute[len++] = '/';
        strcpy(absolute + len, path);
    }
    for (int i = 0; i < p->file_count; i++)
        if (!strcmp(p->files[i], absolute)) { message(p, "This file is already in the queue."); return false; }
    if (p->file_count == MAX_FILES) { message(p, "Queue full: maximum 8 files per run."); return false; }
    strcpy(p->files[p->file_count++], absolute);
    memset(p->stages, 0, sizeof p->stages);
    message(p, "Ready. Include one C file containing main().");
    return true;
}

void pipeline_clear(Pipeline *p) {
    if (p->active) return;
    p->file_count = 0;
    memset(p->stages, 0, sizeof p->stages);
    message(p, "Queue cleared. Drop a new set of C files.");
}

/* Argument arrays avoid shell interpolation of dropped file names. */
static bool spawn_command(Pipeline *p, char *const arguments[]) {
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attrs;
    int error = posix_spawn_file_actions_init(&actions);
    if (error) return false;
    error = posix_spawnattr_init(&attrs);
    if (error) { posix_spawn_file_actions_destroy(&actions); return false; }
    error = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO,
                                            p->log_path, O_WRONLY | O_APPEND | O_CREAT, 0600);
    if (!error) error = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
    if (!error) error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    if (!error) error = posix_spawnattr_setflags(&attrs, POSIX_SPAWN_SETPGROUP);
    if (!error) error = posix_spawnattr_setpgroup(&attrs, 0);
    if (!error) error = posix_spawnp(&p->child, arguments[0], &actions, &attrs, arguments, environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attrs);
    if (error) {
        char detail[256];
        snprintf(detail, sizeof detail, "Could not start process: %s", strerror(error));
        note(p, detail); p->child = 0; return false;
    }
    p->child_started = pipeline_now();
    return true;
}

static void finish(Pipeline *p, bool success) {
    p->active = false;
    p->success = success;
    p->elapsed = pipeline_now() - p->started;
    p->runs++;
    if (success) p->passed_runs++;
    read_log(p);
}

bool pipeline_start(Pipeline *p, bool run_tests) {
    if (p->active) return false;
    int sources = 0;
    for (int i = 0; i < p->file_count; i++) {
        if (access(p->files[i], R_OK)) { message(p, "A queued file is no longer readable."); return false; }
        if (!strcmp(strrchr(p->files[i], '.'), ".c")) sources++;
    }
    if (!sources) { message(p, "Add at least one .c source file containing main()."); return false; }
    cleanup(p);
    strcpy(p->directory, "/tmp/pipeline-studio-XXXXXX");
    if (!mkdtemp(p->directory)) { p->directory[0] = '\0'; message(p, "Cannot create temporary build folder."); return false; }
    snprintf(p->executable, sizeof p->executable, "%s/program", p->directory);
    snprintf(p->log_path, sizeof p->log_path, "%s/output.log", p->directory);
    p->log[0] = '\0';
    p->started = pipeline_now(); p->elapsed = 0;
    p->run_tests = run_tests; p->active = true; p->success = false;
    p->stages[0] = PASSED; p->stages[1] = RUNNING; p->stages[2] = WAITING;
    note(p, "Input checked. Compiling and linking as C11 with warnings enabled.");
    char *args[32] = {"cc", "-std=c11", "-Wall", "-Wextra", "-pedantic", "-fdiagnostics-color=never"};
    int count = 6;
    for (int i = 0; i < p->file_count; i++)
        if (!strcmp(strrchr(p->files[i], '.'), ".c")) args[count++] = p->files[i];
    args[count++] = "-o"; args[count++] = p->executable; args[count] = NULL;
    if (!spawn_command(p, args)) {
        p->stages[1] = FAILED; p->stages[2] = SKIPPED;
        message(p, "Compiler could not start. Install a C compiler (cc)."); finish(p, false);
    } else message(p, "Building your C project...");
    return true;
}

static void stop_child(Pipeline *p) {
    if (p->child > 0) {
        kill(-p->child, SIGKILL);
        while (waitpid(p->child, NULL, 0) < 0 && errno == EINTR) {}
        p->child = 0;
    }
}

void pipeline_update(Pipeline *p) {
    if (!p->active) return;
    p->elapsed = pipeline_now() - p->started;
    read_log(p);
    int stage = p->stages[1] == RUNNING ? 1 : 2;
    double limit = stage == 1 ? 30.0 : 5.0;
    struct stat output;
    if (stat(p->log_path, &output) == 0 && output.st_size > 1024 * 1024) {
        stop_child(p); p->stages[stage] = FAILED;
        if (stage == 1) p->stages[2] = SKIPPED;
        note(p, "Output limit exceeded; process stopped.");
        message(p, "Run stopped: output exceeded 1 MB."); finish(p, false); return;
    }
    if (pipeline_now() - p->child_started > limit) {
        stop_child(p); p->stages[stage] = FAILED;
        if (stage == 1) p->stages[2] = SKIPPED;
        note(p, "Time limit exceeded; process stopped.");
        message(p, "Run exceeded its time limit."); finish(p, false); return;
    }
    int status = 0;
    pid_t result = waitpid(p->child, &status, WNOHANG);
    if (!result || (result < 0 && errno == EINTR)) return;
    if (result > 0) kill(-p->child, SIGKILL); /* Clean up leftover children. */
    p->child = 0;
    if (result < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        p->stages[stage] = FAILED;
        if (stage == 1) p->stages[2] = SKIPPED;
        note(p, stage == 1 ? "Build failed. Test stage skipped." : "Test process returned failure or crashed.");
        message(p, stage == 1 ? "Build failed. Read the compiler output below." : "Tests failed. Read the output below.");
        finish(p, false); return;
    }
    p->stages[stage] = PASSED;
    if (stage == 1 && p->run_tests) {
        note(p, "Build passed. Running test program (5 second limit).");
        char *args[] = {p->executable, NULL};
        p->stages[2] = RUNNING;
        if (!spawn_command(p, args)) { p->stages[2] = FAILED; message(p, "Test process could not start."); finish(p, false); }
        else message(p, "Running tests. Exit code 0 means passed.");
    } else {
        if (stage == 1) {
            p->stages[2] = SKIPPED;
            note(p, "Build passed. Execution disabled; no tests were run.");
            message(p, "Build passed. Tests were not run.");
        } else { note(p, "Test program exited successfully."); message(p, "Build and test program passed."); }
        finish(p, true);
    }
}

void pipeline_cancel(Pipeline *p) {
    if (!p->active) return;
    stop_child(p);
    for (int i = 0; i < 3; i++) {
        if (p->stages[i] == RUNNING) p->stages[i] = FAILED;
        else if (p->stages[i] == WAITING) p->stages[i] = SKIPPED;
    }
    note(p, "Run cancelled by user."); message(p, "Run cancelled."); finish(p, false);
}

void pipeline_dispose(Pipeline *p) { pipeline_cancel(p); cleanup(p); }
