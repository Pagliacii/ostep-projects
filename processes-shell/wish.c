#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void error(int err_code) {
    char error_message[30] = "An error has occured\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    if (err_code != 0) {
        exit(err_code);
    }
}

// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated. The return
// value must NOT be deallocated using free() etc.
char *trim_whitespace(char *str) {
    if (str == NULL) {
        return NULL;
    }

    char *end;
    // Trim leading spaces
    while (isspace((unsigned char)*str)) {
        str++;
    }
    // All spaces?
    if (*str == 0) {
        return str;
    }
    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    // Write new null terminator character
    end[1] = '\0';

    return str;
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        error(EXIT_FAILURE);
    }

    bool batch_mode = false;
    char prompt[7] = "wish> ";

    int rc, pid;
    char *token;
    char *delimiter = " \t";

    FILE *stream;
    ssize_t nread;
    size_t n;
    char *line = NULL;

    if (argc == 2) {
        batch_mode = true;
        stream = fopen(argv[1], "r");
        if (stream == NULL) error(EXIT_FAILURE);
    } else {
        batch_mode = false;
        stream = stdin;
        printf("%s", prompt);
    }

    while ((nread = getline(&line, &n, stream)) != -1) {
        token = trim_whitespace(strsep(&line, delimiter));
        line = trim_whitespace(line);

        if (strcmp(token, "exit") == 0) {
            free(line);
            exit(EXIT_SUCCESS);
        }

        pid = fork();
        if (pid < 0) {
            // fork failed, print error message
            error(batch_mode);
        } else if (pid == 0) {
            // child (new process)
            char *arguments[3];
            arguments[0] = strdup(token);
            arguments[1] = strdup(line);
            arguments[2] = NULL; // mark end of array
            rc = execvp(token, arguments);
            if (rc == -1) {
                error(batch_mode);
            }
        } else {
            // parent goes down this path
            rc = waitpid(pid, NULL, 0);
            if (rc == -1) {
                error(batch_mode);
            }
        }

        if (!batch_mode) {
            printf("%s", prompt);
        }
    }

    free(line);
    return 0;
}
