obj += ../simdini/ini.o
obj += dmenu_helper.o

bin += dmenu_desktop
dmenu_desktop-obj += dmenu_desktop.o
dmenu_desktop-obj += $(obj)

bin += dmenu_app
dmenu_app-obj += dmenu_app.o
dmenu_app-obj += $(obj)

cflags += -mavx -mavx2 -I ../simdini -ggdb -Og

.PHONY: install
install:
	cp -f $(PWD)/dmenu_app $(PWD)/dmenu_desktop ~/bin
