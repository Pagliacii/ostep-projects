#include <stdio.h>
#include <stdlib.h>

#define MAX 1024

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return 0;
    }
    FILE *fp;
    char buffer[MAX];
    for (int i = 1; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wcat: cannot open file\n");
            exit(1);
        }
        while (fgets(buffer, MAX, fp) != NULL) {
            printf("%s", buffer);
        }
        fclose(fp);
    }
    return 0;
}
