#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("wunzip: file1 [file2 ...]\n");
        exit(EXIT_FAILURE);
    }

    FILE *stream;
    size_t nread;
    int num;
    int run_length = 5;
    char *buffer = malloc(run_length * sizeof(char));
    for (int i = 1; i < argc; i++) {
        stream = fopen(argv[i], "rb");
        if (stream == NULL) {
            printf("wunzip: cannot open file\n");
            exit(EXIT_FAILURE);
        }

        while ((nread = fread(buffer, sizeof(char), run_length, stream)) == run_length) {
            num = (unsigned char)(buffer[3]) << 24 |
                  (unsigned char)(buffer[2]) << 16 |
                  (unsigned char)(buffer[1]) << 8 |
                  (unsigned char)(buffer[0]);
            for (int j = 0; j < num; j++) {
                printf("%c", buffer[4]);
            }
        }
        fclose(stream);
    }
    return 0;
}
