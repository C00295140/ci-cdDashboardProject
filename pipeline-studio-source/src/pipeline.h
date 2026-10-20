#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_FILES 8
#define PATH_CAP 4096
#define LOG_CAP 32768

typedef enum { WAITING, RUNNING, PASSED, FAILED, SKIPPED } StageStatus;
typedef struct {
    char files[MAX_FILES][PATH_CAP];
    int file_count;
    StageStatus stages[3];
    bool active, run_tests, success;
    pid_t child;
    double started, child_started, elapsed;
    char directory[PATH_CAP], executable[PATH_CAP], log_path[PATH_CAP];
    char log[LOG_CAP];
    char message[256];
    int runs, passed_runs;
} Pipeline;

void pipeline_init(Pipeline *p);
bool pipeline_add(Pipeline *p, const char *path);
void pipeline_clear(Pipeline *p);
bool pipeline_start(Pipeline *p, bool run_tests);
void pipeline_update(Pipeline *p);
void pipeline_cancel(Pipeline *p);
void pipeline_dispose(Pipeline *p);
double pipeline_now(void);
const char *pipeline_status(StageStatus status);

#endif
