#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PATH 256

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

bool startswith(const char *prefix, const char *str) {
    size_t len_pre = strlen(prefix),
           len_str = strlen(str);
    return len_str < len_pre ? false : strncmp(str, prefix, len_pre) == 0;
}

bool endswith(const char *suffix, const char *str) {
    size_t len_suf = strlen(suffix),
           len_str = strlen(str);
    return len_str < len_suf ? false : strcmp(str + len_str - len_suf, suffix) == 0;
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        error(EXIT_FAILURE);
    }

    bool batch_mode = false;
    char prompt[7] = "wish> ";
    char *path[MAX_PATH] = { "/bin", "/usr/bin", NULL };

    int rc, pid;
    char *token, *command;
    char *delimiter = " \t", *redirect_token = ">", *parallel_token = "&";

    FILE *stream;
    int output_fd;
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

    while (getline(&line, &n, stream) != -1) {
        while ((command = trim_whitespace(strsep(&line, parallel_token))) != NULL) {
            token = trim_whitespace(strsep(&command, delimiter));
            command = trim_whitespace(command);

            if (strcmp(token, "exit") == 0) {
                if (command != NULL) {
                    error(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            } else if (strcmp(token, "cd") == 0) {
                if (command == NULL) {
                    error(batch_mode);
                }
                char *dest = trim_whitespace(strsep(&command, delimiter));
                rc = chdir(dest);
                if (rc == -1) {
                    error(batch_mode);
                }
            } else if (strcmp(token, "path") == 0) {
                for (int i = 0; i < MAX_PATH; i++) {
                    if (command != NULL) {
                        path[i] = strdup(trim_whitespace(strsep(&command, delimiter)));
                        if (command == NULL) {
                            path[i + 1] = NULL;
                            break;
                        }
                    } else if (path[i] == NULL) {
                        break;
                    } else {
                        printf("%s\n", path[i]);
                    }
                }
            } else {
                pid = fork();
                if (pid < 0) {
                    // fork failed, print error message
                    error(batch_mode);
                } else if (pid == 0) {
                    // child (new process)
                    char exec_path[MAX_PATH];
                    char *arguments[3];
                    arguments[0] = strdup(token);
                    if (command != NULL) {
                        char *output_file;
                        char *found = trim_whitespace(strsep(&command, redirect_token));
                        // check if contains arguments
                        if (strcmp(found, "") != 0) {
                            arguments[1] = strdup(found);
                            arguments[2] = NULL; // mark end of array
                        } else {
                            arguments[1] = NULL;
                        }

                        // check if contains the redicrect token
                        if (command) {
                            // check if contains another redirect token
                            found = trim_whitespace(strsep(&command, redirect_token));
                            if (command) {
                                free(found);
                                error(EXIT_FAILURE);
                            }

                            // check if specifies more than one file
                            output_file = trim_whitespace(strsep(&found, delimiter));
                            if (found) {
                                free(found);
                                error(EXIT_FAILURE);
                            }

                            // redirect stdout and stderr to an output file
                            output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
                            free(output_file);
                            if (output_fd < 0) {
                                error(EXIT_FAILURE);
                            }
                            // redirect the child's stdout to the output file
                            if (dup2(output_fd, STDOUT_FILENO) < 0) {
                                close(output_fd);
                                error(EXIT_FAILURE);
                            }
                            // redirect the child's stderr to the output file
                            if (dup2(output_fd, STDERR_FILENO) < 0) {
                                close(output_fd);
                                error(EXIT_FAILURE);
                            }
                        }
                    } else {
                        arguments[1] = NULL; // mark end of array
                    }

                    for (int i = 0; i < MAX_PATH; i++) {
                        if (path[i] == NULL) {
                            rc = -1;
                            break;
                        }
                        if (startswith("/", token)) {
                            sprintf(exec_path, "%s", token);
                        } else if (endswith("/", path[i])) {
                            sprintf(exec_path, "%s%s", path[i], token);
                        } else {
                            sprintf(exec_path, "%s/%s", path[i], token);
                        }
                        if (access(exec_path, X_OK) == 0) {
                            // child process will block here if it can be invoked normally
                            rc = execv(exec_path, arguments);
                            break; // finish iterate if child process failed
                        }
                    }
                    if (rc == -1) {
                        error(EXIT_FAILURE);
                    }
                } else {
                    // parent goes down this path
                    rc = waitpid(pid, NULL, 0);
                    if (rc == -1) {
                        error(batch_mode);
                    }
                }
            }
        }

        if (!batch_mode) {
            printf("%s", prompt);
        }
    }

    fclose(stream);
    close(output_fd);
    free(token);
    free(command);
    free(line);
    return 0;
}
