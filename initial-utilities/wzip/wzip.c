#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    FILE *stream;
    char current_char;
    char last_char;
    int num = 0;

    for (int i = 1; i < argc; i++) {
        stream = fopen(argv[i], "r");
        if (stream == NULL) {
            printf("wzip: cannot open file\n");
            exit(EXIT_FAILURE);
        }

        while ((current_char = fgetc(stream)) != -1) {
            if (last_char != 0 && strncmp(&last_char, &current_char, 1) != 0) {
                fwrite(&num, sizeof(int), 1, stdout);
                fwrite(&last_char, sizeof(char), 1, stdout);
                num = 1;
            } else {
                num++;
            }
            strncpy(&last_char, &current_char, 1);
        }
        fclose(stream);
    }
    fwrite(&num, sizeof(int), 1, stdout);
    fwrite(&last_char, sizeof(char), 1, stdout);

    return 0;
}
