#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct Node {
    struct Node *prev;
    struct Node *next;
    char *line;
} Node;

int same_file(FILE *fp1, FILE *fp2) {
    struct stat stat1, stat2;
    if (fstat(fileno(fp1), &stat1) < 0) return -1;
    if (fstat(fileno(fp2), &stat2) < 0) return -1;
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        // Too many arguments passed to program
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(EXIT_FAILURE);
    }
    if (argc == 3 && strcmp(argv[1], argv[2]) == 0) {
        // Input is the same as output
        fprintf(stderr, "reverse: input and output file must differ\n");
        exit(EXIT_FAILURE);
    }

    FILE *input, *output;
    size_t nread, n = 0;
    Node *head = malloc(sizeof(Node));

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            // Invalid file
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    } else {
        // When invoked without any arguments, your reversing program shoud read from standard input,
        // which is the input that a user types in, and write to standard output.
        input = stdin;
    }
    if (argc == 3) {
        output = fopen(argv[2], "w");
        if (output == NULL) {
            // Invalid file
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    } else {
        output = stdout;
    }
    if (same_file(input, output)) {
        // Input is the same as output (hardlinked)
        fprintf(stderr, "reverse: input and output file must differ\n");
        exit(EXIT_FAILURE);
    }

    while ((nread = getline(&head->line, &n, input)) != -1) {
        head->prev = malloc(sizeof(Node));
        head->prev->next = head;
        head = head->prev;
    }
    fclose(input);

    while (head != NULL) {
        fprintf(output, "%s", head->line);
        Node *temp = malloc(sizeof(Node));
        temp = head;
        head = head->next;
        free(temp);
    }
    fclose(output);
    return 0;
}
