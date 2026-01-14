#!/bin/sh
set -e

clang \
    -g -O0 \
    -Wall -Wextra \
    main.cpp \
    -o handmade.out \
    -lX11
