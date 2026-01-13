#!/bin/sh
set -e

clang \
    -g -O0 \
    -Wall -Wextra \
    main.c \
    -o handmade.out \
    -lX11
