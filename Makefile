MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

CFLAGS := -O0 -ggdb -march=native -mtune=native -Werror -Wall -Wextra
CFLAGS += -MMD
CFLAGS += -I ./simdini

CFLAGS += `pkg-config --cflags glib-2.0`
LDFLAGS += `pkg-config --libs glib-2.0`

CC := gcc

bin = dmenu_app dmenu_desktop
obj = dmenu_app.o dmenu_desktop.o dmenu_helper.o
dep = $(obj:%.o=%.d)

.PHONY: all
all: simdini/ini.o $(bin)

.PHONY: simdini
simdini:
	$(MAKE) -C simdini all

.PHONY: clean
clean:
	$(MAKE) -C simdini $@
	-rm -f $(obj) $(bin) $(dep)

simdini/ini.o: simdini

dmenu_app: dmenu_app.o dmenu_helper.o simdini/ini.o
	$(CC) $(LDFLAGS) $^ -o $@

dmenu_desktop: dmenu_desktop.o dmenu_helper.o simdini/ini.o
	$(CC) $(LDFLAGS) $^ -o $@

$(obj): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(dep)
