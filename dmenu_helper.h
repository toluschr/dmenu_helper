#ifndef _FMENU_DESKTOP_H_
#define _FMENU_DESKTOP_H_

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

struct string {
    const char *data;
    size_t size;
};

struct raw_desktop_entry {
    struct string exec, icon, name, type, terminal, no_display;
};

bool fh_strcaseeq_pl(const char *p, const char *o, size_t ol);
bool fh_streq_pl(const char *p, const char *o, size_t ol);

int fh_ini_callback(const char *s, size_t sl, const char *k, size_t kl, const char *v, size_t vl, void *user);
ssize_t fh_malloc_readlink(const char *link_path, char **output_path, size_t *size);

bool fh_strbool(struct string s);

#endif // _FMENU_DESKTOP_H_
