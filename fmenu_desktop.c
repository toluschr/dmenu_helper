#include <wordexp.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>

#include "ini.h"

struct string {
    const char *data;
    size_t size;
};

struct raw_desktop_entry {
    struct string version, type, name, generic_name, comment, try_exec, exec, categories;
};

bool streq_pl(const char *p, const char *o, size_t ol)
{
    size_t pl = strlen(p);
    if (ol != pl) return false;

    return memcmp(p, o, ol) == 0;
}

int cb(const char *s, size_t sl, const char *k, size_t kl, const char *v, size_t vl, void *user)
{
    // printf("['%.*s' (%d)] '%.*s' (%d) = '%.*s' (%d)\n", sl, s, sl, kl, k, kl, vl, v, vl);

    if (!streq_pl("Desktop Entry", s, sl)) {
        return 1;
    }

    struct string *into = NULL;

    if (streq_pl("Type", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->type;
    }

    if (streq_pl("Exec", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->exec;
    }

    if (streq_pl("Name", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->name;
    }

    if (into == NULL) {
        return 1;
    }

    into->data = v;
    into->size = vl;
    return 1;
}

static int process_entry(int dirfd, const char *path);

static int process_dir(int fd, struct stat *st)
{
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
    struct raw_desktop_entry ent;
    memset(&ent, 0, sizeof(ent));

    void *map;

    if (st->st_size == 0) {
        return 0;
    }

    map = mmap(NULL, st->st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        return -1;
    }

    if (!ini_parse_string(map, st->st_size, cb, &ent)) {
        return -1;
    }

    if (ent.exec.size > 0) {
        printf("%.*s\rfmenu_app %.*s\n", ent.name.size, ent.name.data, ent.exec.size, ent.exec.data);
    }

    if (munmap(map, st->st_size) < 0) {
        return -1;
    }

    return 0;
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
    char application_path[] = "/usr/share/applications:~/.local/share/applications";
    const char *tok;

    tok = strtok(application_path, ":;");
    while (tok) {
        wordexp_t p;

        if (wordexp(tok, &p, 0) != 0) {
            printf("error!\n");
        }

        for (int i = 0; i < p.we_wordc; i++) {
            if (process_entry(AT_FDCWD, p.we_wordv[i]) == -1) {
                fprintf(stderr, "Processing one or more entries failed\n");
            }
        }

        tok = strtok(NULL, ":;");
    }
}
