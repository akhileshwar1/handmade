#!/bin/sh
set -e

# rand=$RANDOM
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

mv libhandmade_temp.so libhandmade.so

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
