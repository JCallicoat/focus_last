all:
	gcc -O3 -o focus_last focus_last.c -lxcb -lxcb-ewmh

install:
	cp focus_last ~/bin

clean:
	rm -f focus_last
