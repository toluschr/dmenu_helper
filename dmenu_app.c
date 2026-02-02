#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "dmenu_helper.h"
#include "ini.h"

int
main(int argc, char **argv)
{
    struct raw_desktop_entry rde = {0};
    struct stat st;
    void *memory = MAP_FAILED;
    char *cmdline = NULL;
    char *shell = NULL;
    int arg_max;
    int rc = EXIT_FAILURE;
    int cmdline_length = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: fmenu_app <desktop_file> <args...>\n");
        exit(EXIT_FAILURE);
    }

    const char *desktop_file = argv[1];
    argv += 2, argc -= 2;

    GUri *uris[argc];
    char *uri_strings[argc];
    const char *path_strings[argc];

    for (int i = 0; i < argc; i++) {
        uris[i] = g_uri_parse(argv[i], G_URI_FLAGS_PARSE_RELAXED | G_URI_FLAGS_SCHEME_NORMALIZE, NULL);
        char *tmp = uris[i] ? g_uri_to_string(uris[i]) : NULL;

        uri_strings[i] = tmp ? tmp : g_strdup(argv[i]);
        path_strings[i] = uris[i] ? g_uri_get_path(uris[i]) : argv[i];
    }

    // mmap file
    {
        int fd = open(desktop_file, O_RDONLY);
        if (fd < 0) {
            perror("unable to open desktop file");
            goto cleanup;
        }

        if (fstat(fd, &st) < 0) {
            perror("unable to stat desktop file");
            goto cleanup;
        }

        memory = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        int eno = errno;
        close(fd);
        errno = eno;

        if (memory == MAP_FAILED) {
            perror("unable to map desktop file");
            goto cleanup;
        }
    }

    // parse file
    if (ini_parse_string(memory, st.st_size, fh_ini_callback, &rde) == false) {
        fprintf(stderr, "unable to parse ini file\n");
        goto cleanup;
    }

    if (rde.exec.size == 0) {
        fprintf(stderr, "No exec section in desktop file\n");
        goto cleanup;
    }

    // create cmdline
    shell = (char *)getenv("SHELL");
    if (shell == NULL) shell = "/bin/sh";

    shell = strdup(shell);
    if (shell == NULL) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }

    if ((arg_max = sysconf(_SC_ARG_MAX)) < 0) {
        perror("sysconf failed");
        goto cleanup;
    }

    if ((cmdline = malloc(arg_max + 1)) == NULL) {
        perror("malloc failed");
        goto cleanup;
    }

    char *const term_args[] = {"alacritty", "-e", shell, "-c", cmdline, NULL};
    char *const shell_args[] = {shell, "-c", cmdline, NULL};

    for (int argument_no = 0; argument_no < argc + !argc; argument_no++) {
        bool f_or_u_used = false;

        for (size_t i = 0; i < rde.exec.size; i++) {
            const char *b[3] = {0};
            size_t b_length[3] = {0};

            const char **append = b;
            size_t *append_length = b_length;

            if (i + 1 < rde.exec.size && rde.exec.data[i] == '%') {
                char flag = rde.exec.data[i + 1];
                switch (flag) {
                case 'i':
                    if (rde.icon.size == 0) break;

                    append[0] = "--icon";
                    append_length[0] = strlen(append[0]);

                    append[1] = rde.icon.data;
                    append_length[1] = rde.icon.size;
                    break;
                case 'k':
                    append[0] = desktop_file;
                    append_length = NULL;
                    break;
                case 'c':
                    append[0] = rde.name.data;
                    append_length[0] = rde.name.size;
                    break;
                case 'u':
                    append[0] = uri_strings[argument_no];
                    append_length = NULL;
                    f_or_u_used = true;
                    break;
                case 'U':
                    append = (const char **)uri_strings;
                    append_length = NULL;
                    break;
                case 'f':
                    append[0] = path_strings[argument_no];
                    append_length = NULL;
                    f_or_u_used = true;
                    break;
                case 'F':
                    append = path_strings;
                    append_length = NULL;
                    break;
                case '%':
                    append[0] = "%";
                    append_length[0] = 1;
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

                if (cmdline_length + len + !!j > (size_t)arg_max) {
                    fprintf(stderr, "cmdline length exceeds ARG_MAX\n");
                    exit(EXIT_FAILURE);
                }

                if (j) cmdline[cmdline_length++] = ' ';
                memcpy(cmdline + cmdline_length, append[j], len);
                cmdline_length += len;
            }
        }

        cmdline[cmdline_length] = '\0';

        char *const *args = fh_strbool(rde.terminal) ? term_args : shell_args;
        if (!f_or_u_used || fork() == 0) {
            execvp(args[0], args);
            perror("exec failed");
            goto cleanup;
        }
    }

    while (wait(NULL) != -1 || errno != ECHILD)
        ;

    rc = EXIT_SUCCESS;

cleanup:
    for (int i = 0; i < argc; i++) {
        g_free(uri_strings[i]);
        g_free(uris[i]);
    }

    free(shell);
    free(cmdline);
    if (memory != MAP_FAILED) munmap(memory, st.st_size);
    return rc;
}
