all:
	gcc -O3 -o focus_last focus_last.c -I/usr/liclude/cxb -lxcb -lxcb-ewmh -lXau -Xdmcp

clean:
	rm -f focus_last

install:
	strip focus_last
	cp focus_last ~/bin

static:
	DOCKER_BUILDKIT=1 docker build --force-rm --file Dockerfile --output . .

