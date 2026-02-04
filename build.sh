#!/bin/sh
set -e

clang \
    -g -O0 \
    -DHANDMADE_SLOW=1 \
    -Wall -Wextra \
    -fsanitize=address \
    linux_handmade.cpp \
    -o handmade.out \
    -lX11 \
    -lasound
