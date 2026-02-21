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
    -o libhandmade_temp.so

mv libhandmade_temp.so libhandmade.so
