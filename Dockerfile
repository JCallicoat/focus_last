FROM alpine:3.15 AS base
RUN apk add --no-cache git build-base clang xorgproto libxcb-dev libxcb-static libxdmcp-dev xcb-util-wm-dev libxau-dev

FROM base as build
WORKDIR focus_last
RUN git clone https://github.com/JCallicoat/focus_last /focus_last && \
    clang -O3 -c focus_last.c -I/usr/include/xcb -DFILTER_NORMAL_WINDOWS=false && \
    clang -static -o focus_last focus_last.o -lxcb -lxcb-ewmh -lXau -lXdmcp && \
    strip focus_last

FROM scratch AS export
COPY --from=build /focus_last/focus_last .

