#!/bin/sh
set -e

# Build game library.
clang \
    -g -O0 \
    -DHANDMADE_SLOW=1 \
    -Wall -Wextra \
    -fsanitize=address \
    -fPIC \
    -shared \
    handmade.cpp \
    -o libhandmade.so

# Build platform executable.
clang \
    -g -O0 \
    -DHANDMADE_SLOW=1 \
    -Wall -Wextra \
    -fsanitize=address \
    linux_handmade.cpp \
    -o handmade.out \
    -lX11 \
    -ldl \
    -lasound
