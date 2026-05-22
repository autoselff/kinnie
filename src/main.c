#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "kinnie.h"
#include "lexer.h"
#include "codegen.h"

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000.0) + (ts.tv_nsec / 1000000.0);
}

static void print_stats(CompileStats *stats) {
    printf("\n=== kinnie Compilation Statistics ===\n");
    printf("Tokenization:     %.2f ms\n", stats->tokenize_time);
    printf("Process includes: %.2f ms\n", stats->includes_time);
    printf("Parse functions:  %.2f ms\n", stats->parse_time);
    printf("C++ code gen:     %.2f ms\n", stats->codegen_time);
    printf("C++ compilation:  %.2f ms\n", stats->compile_time);
    double total = stats->tokenize_time + stats->includes_time
                 + stats->parse_time + stats->codegen_time + stats->compile_time;
    printf("Total:            %.2f ms\n", total);
}

int main(int argc, char **argv) {
    int compile_only = 0;
    int remove_cpp   = 0;
    int show_stats   = 0;
    char *input_file = NULL;

    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--version") == 0) {
            printf("kinnie " KINNIE_VERSION "\n");
            return 0;
        } else if (strcmp(argv[a], "--compile") == 0) {
            compile_only = 1;
        } else if (strcmp(argv[a], "--remcpp") == 0) {
            remove_cpp = 1;
        } else if (strcmp(argv[a], "--stime") == 0) {
            show_stats = 1;
        } else {
            input_file = argv[a];
        }
    }

    if (!input_file) {
        fprintf(stderr, "Usage: %s [--version] [--compile] [--remcpp] [--stime] file.kn\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(input_file, "rb");
    if (!f) { perror("fopen"); return 1; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0) {
        fprintf(stderr, "The file is empty\n");
        fclose(f);
        return 1;
    }

    char *source = malloc(size + 1);
    if (!source) { perror("malloc"); fclose(f); return 1; }
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    CompileStats stats = {0};
    double t0, t1;

    t0 = get_time_ms();
    Token  tokens[MAX_TOKENS];
    size_t token_count = tokenize(source, tokens);
    t1 = get_time_ms();
    stats.tokenize_time = t1 - t0;

    t0 = get_time_ms();
    Token expanded[MAX_EXPANDED_TOKENS];
    Token *final_tokens = tokens;
    size_t final_count  = token_count;
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOK_ADD) {
            memset(expanded, 0, sizeof(expanded));
            final_count  = process_includes(tokens, token_count, expanded, MAX_EXPANDED_TOKENS);
            final_tokens = expanded;
            break;
        }
    }
    t1 = get_time_ms();
    stats.includes_time = t1 - t0;

#if HAVE_SDL == 0
    fprintf(stderr, "Warning: SDL2 not found. Install it with: sudo apt install libsdl2-dev\n");
#endif

    char output_path[256];
    strncpy(output_path, input_file, sizeof(output_path) - 1);
    output_path[sizeof(output_path) - 1] = '\0';
    size_t len = strlen(output_path);
    if (len > 3 && strcmp(output_path + len - 3, ".kn") == 0)
        strcpy(output_path + len - 3, ".cpp");
    else
        strncat(output_path, ".cpp", sizeof(output_path) - len - 1);

    t0 = get_time_ms();
    convert_to_cpp(final_tokens, final_count, output_path, show_stats ? &stats : NULL);
    t1 = get_time_ms();
    stats.codegen_time = t1 - t0;

    char basename[256];
    strncpy(basename, output_path, sizeof(basename) - 1);
    basename[sizeof(basename) - 1] = '\0';
    len = strlen(basename);
    if (len > 4 && strcmp(basename + len - 4, ".cpp") == 0)
        basename[len - 4] = '\0';

    char compile_cmd[512];
    snprintf(compile_cmd, sizeof(compile_cmd),
        "g++ -std=c++17 %s -o %s $(pkg-config --cflags --libs sdl2 SDL2_ttf 2>/dev/null)",
        output_path, basename);

    fprintf(stderr, "Compiling...\n");
    t0 = get_time_ms();
    int ret = system(compile_cmd);
    t1 = get_time_ms();
    stats.compile_time = t1 - t0;

    if (show_stats)
        print_stats(&stats);

    if (!compile_only && ret == 0) {
        fprintf(stderr, "Running...\n");
        char run_cmd[256];
        snprintf(run_cmd, sizeof(run_cmd), "./%s", basename);
        system(run_cmd);
    }

    if (remove_cpp) remove(output_path);

    free(source);
    return ret != 0 ? 1 : 0;
}
