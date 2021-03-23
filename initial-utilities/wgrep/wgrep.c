#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE *stream;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    if (argc == 1) {
        printf("wgrep: searchterm [file ...]\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 2; i < argc; i++) {
        stream = fopen(argv[i], "r");
        if (stream == NULL) {
            printf("wgrep: cannot open file\n");
            exit(EXIT_FAILURE);
        }

        while ((nread = getline(&line, &len, stream)) != -1) {
            if (strstr(line, argv[1]) != NULL) {
                fprintf(stdout, "%s", line);
            }
        }

        free(line);
        fclose(stream);
    }

    if (argc == 2) {
        while ((nread = getline(&line, &len, stdin)) != -1) {
            if (strstr(line, argv[1]) != NULL) {
                fprintf(stdout, "%s", line);
            }
        }
        free(line);
    }
    return 0;
}
