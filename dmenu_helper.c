#include "dmenu_helper.h"

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

// Stolen https://github.com/gagern/gnulib/blob/master/lib/memcasecmp.c
static int memcasecmp (const void *vs1, const void *vs2, size_t n)
{
    size_t i;
    char const *s1 = vs1;
    char const *s2 = vs2;

    for (i = 0; i < n; i++) {
        unsigned char u1 = s1[i];
        unsigned char u2 = s2[i];

        int U1 = toupper (u1);
        int U2 = toupper (u2);

        int diff = (UCHAR_MAX <= INT_MAX ? U1 - U2 : U1 < U2 ? -1 : U2 < U1);
        if (diff) return diff;
    }

    return 0;
}

bool fh_strcaseeq_pl(const char *p, const char *o, size_t ol)
{
    size_t pl = strlen(p);
    if (ol != pl) return false;

    return memcasecmp(p, o, ol) == 0;
}

bool fh_streq_pl(const char *p, const char *o, size_t ol)
{
    size_t pl = strlen(p);
    if (ol != pl) return false;

    return memcmp(p, o, ol) == 0;
}

int fh_ini_callback(const char *s, size_t sl, const char *k, size_t kl, const char *v, size_t vl, void *user)
{
    if (!s || !fh_streq_pl("Desktop Entry", s, sl)) {
        return 1;
    }

    struct string *into = NULL;

    if (fh_streq_pl("Exec", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->exec;
    }

    if (fh_streq_pl("Icon", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->icon;
    }

    if (fh_streq_pl("Name", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->name;
    }

    if (fh_streq_pl("Type", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->type;
    }

    if (fh_streq_pl("Terminal", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->terminal;
    }

    if (fh_streq_pl("NoDisplay", k, kl)) {
        into = &((struct raw_desktop_entry *)user)->no_display;
    }

    if (into == NULL) {
        return 1;
    }

    into->data = v;
    into->size = vl;
    return 1;
}

ssize_t fh_malloc_readlink(const char *link_path, char **output_path, size_t *size)
{
    ssize_t realpath_length = 0;

    for (;;) {
        if ((size_t)realpath_length <= *size) {
            *size += 4096;
        }

        char *new_realpath = realloc((*output_path), *size);
        if (new_realpath == NULL) {
            break;;
        }

        realpath_length = readlink(link_path, ((*output_path) = new_realpath), *size);
        if (realpath_length < 0 || (size_t)realpath_length != *size) {
            break;
        }
    }

    return realpath_length;
}

bool fh_strbool(struct string s)
{
    return fh_strcaseeq_pl("true", s.data, s.size) || fh_strcaseeq_pl("1", s.data, s.size) || fh_strcaseeq_pl("yes", s.data, s.size);
}
