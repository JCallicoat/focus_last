Focus Last Window
====

A window manager agnostic program to switch between the last focused windows using libxcb

The program works by creating a lock file named `focus_last.lock` in `$XDG_RUNTIME_DIR` (or `/tmp` if that fails), then listening to X11 events on the root window for changes to the `_NET_ACTIVE_WINDOW` property.

The last two active windows along with their desktops are saved in binary format named `focus_last.state` in `$XDG_RUNTIME_DIR` (or `/tmp` if that fails).

When the program is run again and detects a lock on the lock file, it reads the state file and switches to the last active window (changing desktops if necessary).

So the basic usage is:

```
focus_last & # obtain lock and start listening for window changes

focus_last # called from a keybinding in your wm for example to switch to last window
```

The only configuration options are the X11 screen number to use (default 0), the time to sleep after sending a request to change the active desktop/window (default 250 ms), and whether to only remember normal windows (`_NET_WM_WINDOW_TYPE_NORMAL`).

These are currently hard coded at the top of `focus_last.c` put I plan to add cli flags for them in the future.

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

The `static_binary` branch tries to build a static binary using `zig cc` and linking with musl.

This assumes you have compiled the source for libX11, libxau, libxdmcp, libxcb and libxcb-wm in their respective directories in `~/build'.

The 'focus_last` static binary file is included in the repo.

The lastest is built with:

- libX11 master (623b77d4)
- libxau (tags/libXau-1.0.9)
- libxdmcp (tags/libXdmcp-1.1.3)
- libxcb (tags/libxcb-1.14)
- libxcb-wm (tags/0.4.1)

TODO: Add more details about compiling statically.
