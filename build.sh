#!/bin/sh
set -e

clang \
    -g -O0 \
    -Wall -Wextra \
    -fsanitize=address \
    main.cpp \
    -o handmade.out \
    -lX11 \
    -lasound
