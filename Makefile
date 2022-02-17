all:
	gcc -O3 -o focus_last focus_last.c -lxcb -lxcb-ewmh

clean:
	rm -f focus_last

install:
	strip focus_last
	cp focus_last ~/bin

static:
	ln -sf ${HOME}/build/libxcb/src ${HOME}/build/libxcb/xcb
	cp -r ${HOME}/build/libxcb-wm/ewmh/*.h ${HOME}/build/libxcb/xcb
	# musl-gcc -O3 -static -o focus_last focus_last.c
	# clang -target x86_64-linux-musl -O3 -flto=full -ffunction-sections -Wl,--gc-sections -o focus_last focus_last.c
	python -m ziglang cc -target x86_64-linux-musl -O3 -flto=full -ffunction-sections -Wl,--gc-sections -o focus_last focus_last.c \
		-I${HOME}/build/libxcb \
		-I${HOME}/build/libxau \
		-I${HOME}/build/libxdmcp \
		-I${HOME}/build/libX11 \
		${HOME}/build/libxau/AuDispose.o \
		${HOME}/build/libxau/AuFileName.o \
		${HOME}/build/libxau/AuGetAddr.o \
		${HOME}/build/libxau/AuGetBest.o \
		${HOME}/build/libxau/AuRead.o \
		${HOME}/build/libxau/AuWrite.o \
		${HOME}/build/libxcb-wm/ewmh/ewmh.o \
		${HOME}/build/libxcb/src/bigreq.o \
		${HOME}/build/libxcb/src/xc_misc.o \
		${HOME}/build/libxcb/src/xcb_auth.o \
		${HOME}/build/libxcb/src/xcb_conn.o \
		${HOME}/build/libxcb/src/xcb_ext.o \
		${HOME}/build/libxcb/src/xcb_in.o \
		${HOME}/build/libxcb/src/xcb_list.o \
		${HOME}/build/libxcb/src/xcb_out.o \
		${HOME}/build/libxcb/src/xcb_util.o \
		${HOME}/build/libxcb/src/xcb_xid.o \
		${HOME}/build/libxcb/src/xproto.o \
		${HOME}/build/libxdmcp/Array.o \
		${HOME}/build/libxdmcp/Fill.o \
		${HOME}/build/libxdmcp/Flush.o \
		${HOME}/build/libxdmcp/Key.o \
		${HOME}/build/libxdmcp/Read.o \
		${HOME}/build/libxdmcp/Wrap.o \
		${HOME}/build/libxdmcp/Wraphelp.o
