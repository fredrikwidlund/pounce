FROM alpine:edge
RUN apk add --update alpine-sdk linux-headers

RUN wget https://github.com/fredrikwidlund/pounce/releases/download/v1.0.0/pounce-1.0.0.tar.gz && \
    tar fxz pounce-1.0.0.tar.gz && \
    cd pounce-1.0.0 && \
    ./configure && \
    make && \
    gcc -std=gnu11 -Wall -Wextra -Wpedantic -g -O3 -flto -march=native -I/root/project/pounce/lib/libdynamic/src -I/root/project/pounce/lib/libreactor/src -pthread -o bin/pounce src/main.o src/pounce.o src/url.o src/worker.o src/stats.o src/connection.o src/net.o src/http_client.o  lib/libreactor/.libs/libreactor.a lib/libdynamic/.libs/libdynamic.a -lm -pthread -static && \
    strip bin/pounce
