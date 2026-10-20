#include "raylib.h"
#include "pipeline.h"
#include <stdio.h>
#include <string.h>

#define WIDTH 1280
#define HEIGHT 860

static const Color BG = {12, 17, 23, 255};
static const Color PANEL = {20, 27, 35, 255};
static const Color LINE = {39, 50, 62, 255};
static const Color TEXT = {230, 237, 242, 255};
static const Color MUTED = {135, 153, 169, 255};
static const Color MINT = {111, 232, 186, 255};
static const Color ERROR_COLOR = {255, 130, 143, 255};
static Font font;

static void label(const char *s, float x, float y, float size, Color color) {
    DrawTextEx(font, s, (Vector2){x, y}, size, 0.3f, color);
}

static void box(Rectangle r, Color color) { DrawRectangleRounded(r, 0.12f, 12, color); }

static bool hover(Rectangle r) { return CheckCollisionPointRec(GetMousePosition(), r); }

static bool button(Rectangle r, const char *s, bool primary, bool enabled) {
    Color fill = primary ? MINT : (Color){31, 42, 52, 255};
    if (enabled && hover(r)) fill = primary ? (Color){156, 249, 213, 255} : (Color){45, 60, 72, 255};
    if (!enabled) fill = (Color){30, 39, 46, 255};
    box(r, fill);
    Vector2 measure = MeasureTextEx(font, s, 16, 0.3f);
    label(s, r.x + (r.width - measure.x)/2, r.y + (r.height - 16)/2, 16,
          !enabled ? MUTED : primary ? BG : TEXT);
    if (enabled && hover(r)) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return enabled && hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static Color status_color(StageStatus s) {
    return s == PASSED ? MINT : s == FAILED ? ERROR_COLOR : s == RUNNING ? (Color){239, 199, 120, 255} : MUTED;
}

static void stage_card(int index, const char *title, const char *subtitle, StageStatus status) {
    float x = 240 + index * 340;
    box((Rectangle){x, 429, 316, 117}, PANEL);
    Color color = status_color(status);
    DrawCircle((int)x + 26, 455, 5, color);
    label(pipeline_status(status), x + 40, 447, 14, color);
    label(title, x + 20, 476, 21, TEXT);
    label(subtitle, x + 20, 510, 13, MUTED);
    if (status == RUNNING) {
        float amount = 0.3f + 0.7f * (float)(GetTime() - (int)GetTime());
        DrawRectangle((int)x + 20, 539, (int)(276 * amount), 2, color);
    }
}

static void stat_card(float x, const char *caption, const char *value, const char *detail, Color accent) {
    box((Rectangle){x, 134, 316, 96}, PANEL);
    label(caption, x + 18, 149, 13, MUTED);
    label(value, x + 18, 172, 28, accent);
    label(detail, x + 140, 185, 12, MUTED);
}

static void load_demo(Pipeline *p, const char *name) {
    char path[PATH_CAP];
    snprintf(path, sizeof path, "%s/examples/%s", PROJECT_ROOT, name);
    pipeline_clear(p);
    pipeline_add(p, path);
}

/* Scrollable output, wrapped at a fixed column width for the embedded font. */
static void logs(const char *text, int *scroll) {
    Rectangle viewport = {256, 601, 958, 174};
    char lines[1024][121];
    int count = 0, col = 0;
    for (const unsigned char *c = (const unsigned char *)text; *c && count < 1023; c++) {
        if (*c == '\n' || col == 110) {
            lines[count][col] = '\0'; count++; col = 0;
            if (*c == '\n') continue;
        }
        lines[count][col++] = *c >= 32 && *c < 127 ? (char)*c : ' ';
    }
    if (col) { lines[count][col] = '\0'; count++; }
    if (hover(viewport)) *scroll += (int)(GetMouseWheelMove() * 3);
    int max_scroll = count > 9 ? count - 9 : 0;
    if (*scroll < 0) *scroll = 0;
    if (*scroll > max_scroll) *scroll = max_scroll;
    int first = max_scroll - *scroll;
    BeginScissorMode(256, 601, 958, 174);
    if (!count) label("Compiler output and test results will appear here.", 260, 610, 14, MUTED);
    for (int i = first; i < count && i < first + 9; i++) {
        Color color = strstr(lines[i], "error:") || strstr(lines[i], "FAIL") ? ERROR_COLOR :
                      strstr(lines[i], "[studio]") ? MINT : MUTED;
        label(lines[i], 260, 608 + (i - first)*18, 13, color);
    }
    EndScissorMode();
}

int main(int argc, char **argv) {
    bool capture = argc > 1 && !strcmp(argv[1], "--screenshot");
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WIDTH, HEIGHT, "Pipeline Studio | C + raylib");
    SetTargetFPS(60);
    font = GetFontDefault();
    /* Optional local font; the project remains asset-free and portable. */
    Font custom = {0};
    const char *font_path = "/System/Library/Fonts/Supplemental/Arial.ttf";
    if (FileExists(font_path)) { custom = LoadFontEx(font_path, 40, NULL, 0); font = custom; }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    Pipeline pipeline;
    pipeline_init(&pipeline);
    bool execute = false;
    int scroll = 0, frames = 0;
    if (!capture) for (int i = 1; i < argc; i++) pipeline_add(&pipeline, argv[i]);

    while (!WindowShouldClose()) {
        pipeline_update(&pipeline);
        if (IsFileDropped()) {
            FilePathList dropped = LoadDroppedFiles();
            if (!pipeline.active) {
                int rejected = 0;
                for (unsigned int i = 0; i < dropped.count; i++)
                    if (!pipeline_add(&pipeline, dropped.paths[i])) rejected++;
                if (rejected) snprintf(pipeline.message, sizeof pipeline.message,
                                       "%d file(s) not added. Use readable .c/.h files; maximum 8, no duplicates.", rejected);
            } else snprintf(pipeline.message, sizeof pipeline.message, "Stop the current run before dropping more files.");
            UnloadDroppedFiles(dropped);
        }
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        BeginDrawing();
        ClearBackground(BG);
        DrawRectangle(0, 0, 208, HEIGHT, (Color){16, 23, 30, 255});
        DrawLine(208, 0, 208, HEIGHT, LINE);

        box((Rectangle){22, 28, 32, 32}, MINT);
        label("P", 31, 32, 23, BG);
        label("pipeline", 66, 27, 23, TEXT);
        label("S T U D I O", 67, 53, 10, MUTED);
        label("WORKSPACE", 24, 114, 11, MUTED);
        box((Rectangle){16, 138, 176, 42}, (Color){30, 52, 48, 255});
        DrawRectangle(16, 150, 3, 18, MINT);
        label("Pipeline overview", 32, 151, 16, MINT);

        label("TRY A SAMPLE", 24, 221, 11, MUTED);
        if (button((Rectangle){20, 245, 168, 38}, "Passing tests", false, !pipeline.active)) load_demo(&pipeline, "passing.c");
        if (button((Rectangle){20, 291, 168, 38}, "Failing tests", false, !pipeline.active)) load_demo(&pipeline, "failing.c");
        if (button((Rectangle){20, 337, 168, 38}, "Build error", false, !pipeline.active)) load_demo(&pipeline, "build_error.c");
        label("Select a sample, then", 24, 397, 13, MUTED);
        label("start your pipeline.", 24, 416, 13, MUTED);
        DrawLine(24, 752, 184, 752, LINE);
        DrawCircle(29, 779, 4, MINT);
        label("LOCAL RUNNER", 42, 772, 12, TEXT);
        label("C11 / raylib / prototype", 24, 802, 11, MUTED);

        label("WORKSPACE  /  LOCAL PIPELINE", 240, 27, 11, MUTED);
        label("Good code starts here.", 240, 53, 32, TEXT);
        label("Drop your C files. Build with confidence. See every result.", 241, 96, 15, MUTED);
        box((Rectangle){1080, 39, 156, 31}, (Color){24, 47, 41, 255});
        DrawCircle(1096, 55, 4, MINT);
        label("C ONLY PROJECT", 1108, 48, 11, MINT);

        char value[48], detail[64];
        snprintf(value, sizeof value, "%02d", pipeline.file_count);
        stat_card(240, "QUEUED FILES", value, "8 files maximum", TEXT);
        snprintf(value, sizeof value, "%02d", pipeline.runs);
        snprintf(detail, sizeof detail, "%d successful this session", pipeline.passed_runs);
        stat_card(580, "COMPLETED RUNS", value, detail, MINT);
        snprintf(value, sizeof value, "%.1fs", pipeline.elapsed);
        stat_card(920, "RUN DURATION", value, pipeline.active ? "Live elapsed time" : "Most recent run", TEXT);

        Rectangle drop = {240, 246, 620, 157};
        box(drop, hover(drop) ? (Color){23, 43, 40, 255} : (Color){19, 34, 33, 255});
        for (int x = 254; x < 846; x += 14) {
            DrawLine(x, 248, x + 7, 248, (Color){63, 108, 91, 255});
            DrawLine(x, 401, x + 7, 401, (Color){63, 108, 91, 255});
        }
        label("+", 270, 265, 33, MINT);
        label("Drop C files here", 309, 266, 22, TEXT);
        label(".c and .h  /  one main() per project  /  no C++", 309, 296, 13, MUTED);
        if (!pipeline.file_count) label("Or choose a sample from the sidebar to explore.", 264, 357, 14, MUTED);
        else {
            BeginScissorMode(260, 329, 575, 65);
            for (int i = 0; i < pipeline.file_count; i++) {
                const char *name = GetFileName(pipeline.files[i]);
                label(TextFormat("%d  %.29s", i + 1, name), 264 + (i/4)*288, 330 + (i%4)*16, 12, TEXT);
            }
            EndScissorMode();
        }

        box((Rectangle){876, 246, 360, 157}, PANEL);
        label("RUN CONFIGURATION", 894, 260, 11, MUTED);
        Rectangle checkbox = {894, 286, 18, 18};
        if (!pipeline.active && hover((Rectangle){890, 280, 325, 33}) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) execute = !execute;
        box(checkbox, execute ? MINT : LINE);
        if (execute) label("x", 899, 286, 16, BG);
        label("Run trusted code as tests", 922, 287, 15, TEXT);
        label("Runs locally with your user permissions.", 894, 313, 12, MUTED);
        if (button((Rectangle){894, 346, 204, 39}, pipeline.active ? "Stop run" : "Run pipeline", true, pipeline.active || pipeline.file_count > 0)) {
            if (pipeline.active) pipeline_cancel(&pipeline);
            else { scroll = 0; pipeline_start(&pipeline, execute); }
        }
        if (button((Rectangle){1108, 346, 110, 39}, "Clear", false, !pipeline.active && pipeline.file_count > 0)) pipeline_clear(&pipeline);

        stage_card(0, "01  Inspect", "Readable C source files", pipeline.stages[0]);
        stage_card(1, "02  Build", "Compile + link with cc", pipeline.stages[1]);
        stage_card(2, "03  Test", "Your assertions / exit code", pipeline.stages[2]);

        box((Rectangle){240, 562, 996, 225}, (Color){9, 14, 19, 255});
        label("OUTPUT", 260, 578, 11, MUTED);
        label("Scroll to review  /  follows latest output", 973, 578, 11, MUTED);
        DrawLine(260, 599, 1215, 599, LINE);
        logs(pipeline.log, &scroll);
        DrawCircle(247, 814, 4, pipeline.active ? MINT : MUTED);
        label(pipeline.message, 260, 806, 14, TEXT);
        label("Local build + test prototype. No deployment or hosted CI integration.", 240, 837, 11, MUTED);
        EndDrawing();
        if (capture && ++frames == 3) { TakeScreenshot("../dashboard-preview.png"); break; }
    }
    pipeline_dispose(&pipeline);
    if (custom.texture.id) UnloadFont(custom);
    CloseWindow();
    return 0;
}
