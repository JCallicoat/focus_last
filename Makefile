all:
	gcc -O3 -o focus_last focus_last.c -lxcb -lxcb-ewmh
	# gcc -O3 -static -o focus_last focus_last.c /home/sage/build/libxcb/src/bigreq.o /home/sage/build/libxcb/src/xcb_list.o  /home/sage/build/libxcb/src/xc_misc.o /home/sage/build/libxcb/src/xproto.o /home/sage/build/libxcb/src/xcb_auth.o /home/sage/build/libxcb/src/xcb_in.o /home/sage/build/libxcb/src/xcb_out.o /home/sage/build/libxcb/src/xcb_ext.o /home/sage/build/libxcb/src/xcb_xid.o /home/sage/build/libxcb/src/xcb_conn.o /home/sage/build/libxcb-wm/ewmh/ewmh.o /home/sage/build/libxau/AuDispose.o /home/sage/build/libxau/AuGetBest.o /home/sage/build/libxau/AuGetAddr.o /home/sage/build/libxau/AuFileName.o  /home/sage/build/libxau/AuRead.o /home/sage/build/libxau/AuWrite.o /home/sage/build/libxdmcp/Wrap.o /home/sage/build/libxdmcp/Read.o /home/sage/build/libxdmcp/Key.o /home/sage/build/libxdmcp/Array.o /home/sage/build/libxdmcp/Fill.o /home/sage/build/libxdmcp/Flush.o /home/sage/build/libxdmcp/Wraphelp.o /home/sage/build/libxcb/src/xcb_util.o 

install:
	cp focus_last ~/bin

clean:
	rm -f focus_last
