#include "jv.c"

int main(int argc, char **argv) {
    FILE *fp;
    const char *path;
    struct json_stream_t stream;
    struct output_t out;

    if (argc == 2) {
        // Stream from stdin
        fp = stdin;
        path = argv[1];
    }
    else if (argc == 3) {
        // Stream in a file.
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "Error opening file %s.\n", argv[1]);
            exit(2);
        }
        path = argv[2];
    }
    else {
        fprintf(stderr, "Usage: ga <file> <attr>\n");
        fprintf(stderr, "Usage: jv main.browser.whatever < config.json\n");
        exit(1);
    }

    if (init_stream(&stream, fp) != OK) {
        fprintf(stderr, "Problem initializing stream.\n");
        exit(1);
    }

    path = scan_value(&stream, path);
    if (path == NULL) {
        // Error bomb. For now, just exit with code.
        exit(stream.code);
    }
    if (path[0] != '\0') {
        // Successful scan, no match. Return nothing.
        exit(OK);
    }

    // Match! Let's pipe.
    init_output(&out, stdout, NULL, 0);
    if (pipe_value(&stream, &out) != OK) {
        exit(stream.code);
    }

    exit(OK);
}
