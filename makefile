CFLAGS := -O3 -g0 -march=native -mtune=native
CFLAGS += -MMD
CFLAGS += -I ./simdini

CC := gcc

bin = dmenu_app dmenu_desktop
obj = dmenu_app.o dmenu_desktop.o dmenu_helper.o
dep = $(obj:%.o=%.d)

.PHONY: all
all: simdini dmenu_app dmenu_desktop

.PHONY: clean
clean:
	$(MAKE) -C simdini $@
	-rm -f $(obj) $(bin) $(dep)

.PHONY: simdini
simdini:
	$(MAKE) -C $@

dmenu_app: dmenu_app.o dmenu_helper.o simdini/ini.o
	$(CC) $^ -o $@

dmenu_desktop: dmenu_desktop.o dmenu_helper.o simdini/ini.o
	$(CC) $^ -o $@

$(obj): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(dep)
