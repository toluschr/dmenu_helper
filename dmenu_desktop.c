#include <wordexp.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>

#include "dmenu_helper.h"
#include "ini.h"

static int process_entry(int dirfd, const char *path);
static int process_file(int fd, struct stat *st);
static int process_dir(int fd, struct stat *st);

static int process_dir(int fd, struct stat *st)
{
    (void)(st);

    DIR *d = fdopendir(fd);
    if (d == NULL) {
        return -1;
    }

    int rc = 0;

    struct dirent *entry;
    while (errno = 0, (entry = readdir(d))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // a single broken file should not mess up the menu
        if (process_entry(fd, entry->d_name) == -1) {
            rc = -1;
        }
    }

    if (errno != 0) {
        closedir(d);
        return -1;
    }

    if (closedir(d) < 0) {
        return -1;
    }

    return rc;
}

static int process_file(int fd, struct stat *st)
{
    int rc = -1;
    void *map;
    struct raw_desktop_entry ent;

    if (st->st_size == 0) {
        return 0;
    }

    map = mmap(NULL, st->st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        goto _out_close;
    }

    memset(&ent, 0, sizeof(ent));

    if (!ini_parse_string(map, st->st_size, fh_ini_callback, &ent)) {
        goto _out_munmap;
    }

    if (ent.name.size == 0 || ent.exec.size == 0 || fh_strbool(ent.no_display)) {
        rc = 0;
        goto _out_munmap;
    }

    int size = snprintf(NULL, 0, "/proc/self/fd/%d", fd);

    char *path = malloc(size + 1);
    if (path == NULL) {
        goto _out_munmap;
    }

    snprintf(path, size + 1, "/proc/self/fd/%d", fd);

    char *realpath = NULL;
    size_t realpath_size = 0;
    ssize_t realpath_length;

    realpath_length = fh_malloc_readlink(path, &realpath, &realpath_size);
    if (realpath_length < 0) {
        goto _out_free_realpath;
    }

    printf("%.*s\rdmenu_app %.*s\n", (int)ent.name.size, ent.name.data, (int)realpath_length, realpath);
    rc = 0;

_out_free_realpath:
    free(realpath);

// _out_free_path:
    free(path);

_out_munmap:
    munmap(map, st->st_size);

_out_close:
    close(fd);

    return rc;
}

static int process_entry(int dirfd, const char *path)
{
    struct stat st;
    int fd;

    fd = openat(dirfd, path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    if (fstat(fd, &st) < 0) {
        close(fd);
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        return process_dir(fd, &st);
    }

    return process_file(fd, &st);
}

int main()
{
    int rc = EXIT_SUCCESS;
    // @fixme: use paths specified in $XDG_DATA_DIRS
    char application_path[] = "/usr/share/applications:/usr/local/share/applications:~/.local/share/applications";
    const char *tok;

    tok = strtok(application_path, ":;");
    while (tok) {
        wordexp_t p;

        if (wordexp(tok, &p, 0) != 0) {
            fprintf(stderr, "Unable to shell expand '%s'\n", tok);
        }

        for (size_t i = 0; i < p.we_wordc; i++) {
            if (process_entry(AT_FDCWD, p.we_wordv[i]) == -1) {
                fprintf(stderr, "Processing one or more entries failed\n");
                rc = EXIT_FAILURE;
            }
        }

        wordfree(&p);

        tok = strtok(NULL, ":;");
    }

    return rc;
}
