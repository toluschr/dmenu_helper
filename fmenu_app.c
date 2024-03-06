#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

int main(int argc, char **argv)
{
    int arg_max = sysconf(_SC_ARG_MAX);
    char *cmdline = malloc(arg_max + 1);

    int len = 0;
    for (int i = 1; i < argc; i++) {
        size_t l = strlen(argv[i]);

        if (len + l + 1 > arg_max) {
            fprintf(stderr, "invalid arguments: length exceeds limit\n");
            return 1;
        }

        // skip all specifiers
        if (l > 0 && argv[i][0] == '%') {
            if (i != argc - 1) {
                fprintf(stderr, "invalid cmdline: % not last argument\n");
                return 1;
            }

            continue;
        }

        memcpy(&cmdline[len], argv[i], l);
        len += l;

        memcpy(&cmdline[len], " ", 1);
        len += 1;
    }

    char *shell = (char *)getenv("SHELL");
    if (shell == NULL) shell = "/bin/sh";

    shell = strdup(shell);

    execv(shell, (char *const []){shell, "-c", cmdline, NULL});
    perror("exec failed");
    return 1;
}
