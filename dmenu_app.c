#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include "dmenu_helper.h"
#include "ini.h"

int main(int argc, char **argv)
{
    struct raw_desktop_entry rde;
    struct stat st;
    void *memory;
    int fd;
    int rc = EXIT_FAILURE;

    const char **file = NULL;

    if (argc < 2) {
        fprintf(stderr, "usage: fmenu_app <desktop_file> <args...>\n");
        return 1;
    }

    file = (const char **)(argv + 2);

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("unable to open desktop file");
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        perror("unable to stat desktop file");
        goto _close;
    }

    memory = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (memory == MAP_FAILED) {
        perror("unable to map desktop file");
        goto _close;
    }

    memset(&rde, 0, sizeof(rde));

    if (ini_parse_string(memory, st.st_size, fh_ini_callback, &rde) == false) {
        fprintf(stderr, "unable to parse ini file\n");
        goto _munmap;
    }

    if (rde.exec.size == 0) {
        fprintf(stderr, "No exec section in desktop file\n");
        goto _munmap;
    }

    int arg_max = sysconf(_SC_ARG_MAX);
    if (arg_max < 0) {
        perror("sysconf failed");
        goto _munmap;
    }

    char *cmdline = malloc(arg_max + 1);
    if (cmdline == NULL) {
        perror("malloc failed");
        goto _munmap;
    }

    int cmdline_length = 0;

    const char *b[3];
    size_t b_length[3];

    const char **append;
    size_t *append_length;

    for (size_t i = 0; i < rde.exec.size; i++) {
        memset(b, 0, sizeof(b));
        append = b;
        append_length = b_length;

        if (rde.exec.data[i] == '%' && i + 1 < rde.exec.size) {
            switch (rde.exec.data[i + 1]) {
            case 'i':
                if (rde.icon.size != 0) {
                    append[0] = "--icon";
                    append_length[0] = strlen(append[0]);

                    append[1] = rde.icon.data;
                    append_length[1] = rde.icon.size;
                }
                break;
            case 'c':
                append[0] = rde.name.data;
                append_length[0] = rde.name.size;
                break;
            case 'k':
                append[0] = argv[1];
                append_length[0] = strlen(argv[1]);
                break;
            case 'f':
                append[0] = file[0];
                append[1] = NULL;
                append_length = NULL;
                break;
            case 'F':
                append = file;
                append_length = NULL;
                break;
            // ignore everything else
            default:
                break;
            }

            i++;
        } else {
            append[0] = &rde.exec.data[i];
            append_length[0] = 1;
        }

        for (int j = 0; append[j]; j++) {
            size_t len = append_length ? append_length[j] : strlen(append[j]);

            if (cmdline_length + len + !j > (size_t)arg_max) {
                fprintf(stderr, "cmdline length exceeds ARG_MAX\n");
                exit(EXIT_FAILURE);
            }

            if (j != 0) {
                memcpy(cmdline + cmdline_length, " ", 1);
                cmdline_length += 1;
            }

            memcpy(cmdline + cmdline_length, append[j], len);
            cmdline_length += len;
        }
    }

    cmdline[cmdline_length] = '\0';

    char *shell = (char *)getenv("SHELL");
    if (shell == NULL) shell = "/bin/sh";

    shell = strdup(shell);
    if (shell == NULL) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }

    char *const *args;
    char *const term_args[] = {"alacritty", "-e", shell, "-c", cmdline, NULL};
    char *const shell_args[] = {shell, "-c", cmdline, NULL};

    args = fh_strbool(rde.terminal) ? term_args : shell_args;

    execvp(args[0], args);

    perror("exec failed");

    free(shell);
    free(cmdline);

_munmap:
    munmap(memory, st.st_size);

_close:
    close(fd);
    return rc;
}
