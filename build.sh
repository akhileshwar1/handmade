#!/bin/sh
set -e

clang \
    -g -O0 \
    -Wall -Wextra \
    -fsanitize=address \
    linux_handmade.cpp \
    -o handmade.out \
    -lX11 \
    -lasound
