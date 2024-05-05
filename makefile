MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

CFLAGS := -O3 -g0 -march=native -mtune=native -Werror -Wall -Wextra
CFLAGS += -MMD
CFLAGS += -I ./simdini

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
	$(CC) $^ -o $@

dmenu_desktop: dmenu_desktop.o dmenu_helper.o simdini/ini.o
	$(CC) $^ -o $@

$(obj): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(dep)
