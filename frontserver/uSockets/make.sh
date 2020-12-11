#! /bin/bash

gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/socket.c
gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/context.c
gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/loop.c

gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/crypto/openssl.c
gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/crypto/wolfssl.c

gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/eventing/epoll_kqueue.c
gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/eventing/gcd.c
gcc -O0 -c -fPIC -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV -I src/ src/eventing/libuv.c

mv *.o obj/

ls -al obj/
sleep 2s

g++ -O0 -shared -fPIC -o lib/libuSockets.so obj/socket.o obj/loop.o obj/context.o obj/libuv.o obj/gcd.o obj/epoll_kqueue.o obj/openssl.o obj/wolfssl.o -lpthread -lssl -lcrypto -luv