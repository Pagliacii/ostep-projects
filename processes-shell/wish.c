#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX 256

void show_error_msg() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
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
        show_error_msg();
        exit(EXIT_FAILURE);
    }

    bool batch_mode = false;
    char prompt[7] = "wish> ";
    char *path[MAX] = { "/bin", "/usr/bin", NULL };

    int rc, p = 0;
    pid_t pid[MAX];
    char *token, *command;
    char *delimiter = " \t", *redirect_token = ">", *parallel_token = "&";

    FILE *stream;
    int output_fd;
    size_t n;
    char *line = NULL;

    if (argc == 2) {
        batch_mode = true;
        stream = fopen(argv[1], "r");
        if (stream == NULL) {
            show_error_msg();
            exit(EXIT_FAILURE);
        }
    } else {
        batch_mode = false;
        stream = stdin;
        printf("%s", prompt);
    }

    while (getline(&line, &n, stream) != -1) {
        line = trim_whitespace(line);
        if (strcmp(line, "") == 0) {
            goto PROMPT;
        }
        while ((command = trim_whitespace(strsep(&line, parallel_token))) != NULL) {
            token = trim_whitespace(strsep(&command, delimiter));
            command = trim_whitespace(command);

            if (strcmp(token, "") == 0) {
                break;
            } else if (strcmp(token, "exit") == 0) {
                if (command != NULL) {
                    show_error_msg();
                    continue;
                }
                exit(EXIT_SUCCESS);
            } else if (strcmp(token, "cd") == 0) {
                if (command == NULL) {
                    show_error_msg();
                    continue;
                }
                char *dest = trim_whitespace(strsep(&command, delimiter));
                rc = chdir(dest);
                if (rc == -1) {
                    show_error_msg();
                }
            } else if (strcmp(token, "path") == 0) {
                if (command == NULL) {
                    path[0] = NULL;
                }
                for (int i = 0; i < MAX; i++) {
                    if (path[i] == NULL) {
                        break;
                    }

                    path[i] = strdup(trim_whitespace(strsep(&command, delimiter)));
                    if (command == NULL) {
                        path[i + 1] = NULL;
                        break;
                    }
                }
            } else {
                pid[p] = fork();
                if (pid[p] < 0) {
                    // fork failed, print error message
                    show_error_msg();
                } else if (pid[p] == 0) {
                    // child (new process)
                    char *arguments[MAX];
                    arguments[0] = strdup(token);
                    if (command != NULL) {
                        char *output_file;
                        char *found = trim_whitespace(strsep(&command, redirect_token));
                        // check if contains arguments
                        if (strcmp(found, "") != 0) {
                            int i = 1;
                            char *argument;
                            while ((argument = trim_whitespace(strsep(&found, delimiter))) != NULL && i < MAX - 1) {
                                arguments[i] = strdup(argument);
                                i++;
                            }
                            arguments[i++] = NULL; // mark end of array
                        } else {
                            arguments[1] = NULL;
                        }

                        // check if contains the redicrect token
                        if (command) {
                            // check if contains another redirect token
                            found = trim_whitespace(strsep(&command, redirect_token));
                            if (command) {
                                show_error_msg();
                                exit(EXIT_FAILURE);
                            }

                            // check if specifies more than one file
                            output_file = trim_whitespace(strsep(&found, delimiter));
                            if (found) {
                                show_error_msg();
                                exit(EXIT_FAILURE);
                            }

                            // redirect stdout and stderr to an output file
                            output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
                            if (output_fd < 0) {
                                show_error_msg();
                                exit(EXIT_FAILURE);
                            }
                            // redirect the child's stdout to the output file
                            if (dup2(output_fd, STDOUT_FILENO) < 0) {
                                close(output_fd);
                                show_error_msg();
                                exit(EXIT_FAILURE);
                            }
                            // redirect the child's stderr to the output file
                            if (dup2(output_fd, STDERR_FILENO) < 0) {
                                close(output_fd);
                                show_error_msg();
                                exit(EXIT_FAILURE);
                            }
                        }
                    } else {
                        arguments[1] = NULL; // mark end of array
                    }

                    for (int i = 0; i < MAX; i++) {
                        if (path[i] == NULL) {
                            rc = -1;
                            break;
                        }
                        char *exec_path;
                        if (startswith("/", token)) {
                            exec_path = malloc(sizeof(token));
                            strcpy(exec_path, token);
                        } else if (endswith("/", path[i])) {
                            exec_path = malloc(sizeof(path[i]) + sizeof(token) + 1);
                            strcpy(exec_path, path[i]);
                            strcat(exec_path, token);
                        } else {
                            exec_path = malloc(sizeof(path[i]) + sizeof(token) + sizeof("/") + 1);
                            strcpy(exec_path, path[i]);
                            strcat(exec_path, "/");
                            strcat(exec_path, token);
                        }
                        if (access(exec_path, X_OK) == 0) {
                            // child process will block here if it can be invoked normally
                            rc = execv(exec_path, arguments);
                            break; // finish iterate if child process failed
                        }
                    }
                    if (rc == -1) {
                        show_error_msg();
                        exit(EXIT_FAILURE);
                    }
                } else {
                    if (++p >= MAX) {
                        show_error_msg();
                        if (batch_mode) {
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }

        // parent goes down this path
        while (p > 0) {
            rc = waitpid(pid[--p], NULL, 0);
            if (rc == -1) {
                show_error_msg();
                if (batch_mode) {
                    exit(EXIT_FAILURE);
                }
            }
        }
PROMPT:
        if (!batch_mode) {
            printf("%s", prompt);
        }
    }

    fclose(stream);
    close(output_fd);
    return 0;
}
