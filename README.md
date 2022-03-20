Focus Last Window
====

A window manager agnostic program to switch between the last focused windows using libxcb

The program works by creating a lock file named `focus_last.lock` in `$XDG_RUNTIME_DIR` (or `/tmp` if that fails), then listening to X11 events on the root window for changes to the `_NET_ACTIVE_WINDOW` property.

The last two active windows are saved in binary format in a file named `focus_last.state` in `$XDG_RUNTIME_DIR` (or `/tmp` if that fails).

When the program is run again and detects a lock on the lock file, it reads the state file and switches to the last active window (changing desktops if necessary).

So the basic usage is:

```
# obtain lock and start listening for window changes in a background process
focus_last &

# now call it from a keybinding in your wm for example to switch to last window
focus_last
```

The only configuration options are the X11 screen number to use (default 0), the time to sleep after sending a request to change the active desktop/window (default 250 ms), and whether to only remember normal windows (`_NET_WM_WINDOW_TYPE_NORMAL` default true).

These can be configured using compiler flags (e.g., `-DFILTER_NORMAL_WINDOWS=false`) for the time being. I may add a config file in the future.

Building from source
----

You will need gcc as well as libcxb and libxcb-wm libraries and headers installed. 

```
git clone https://github.com/JCallicoat/focus_last
cd focus_last
make
```

The binary will be named `focus_last`. You can also install it to `~/bin` with `make install`.


Static binary
----

The `make static` builds a static binary using docker to download and build in a container using an alpine 3.15 base and output the static binary to this directory.

The `focus_last` static binary file from this is included in the repo for people who cannot build their own and should run on an x86_64 linux system.

