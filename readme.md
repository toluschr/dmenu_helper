# dmenu_helper

## dmenu_desktop

Parse xdg desktop files in a dmenu friendly format into stdout

Requires the dmenu `separator` patch found [here](https://tools.suckless.org/dmenu/patches/separator/).
The delimiter is a carriage return symbol ('`\r`').

```
$ dmenu_desktop | dmenu -d $'\r'
```

## dmenu_app

Execute a desktop file from the commandline

```
$ dmenu_app /usr/share/applications/xyz.desktop
```
