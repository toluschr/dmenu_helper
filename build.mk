obj += ../simdini/ini.o

bin += fmenu_desktop
fmenu_desktop-obj += fmenu_desktop.o
fmenu_desktop-obj += $(obj)

bin += fmenu_app
fmenu_app-obj += fmenu_app.o
fmenu_app-obj += $(obj)

cflags += -mavx -mavx2 -I ../simdini

.PHONY: install
install:
	cp -f $(PWD)/fmenu_app $(PWD)/fmenu_desktop ~/bin
