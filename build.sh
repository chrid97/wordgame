#!/bin/bash
mkdir -p build/
# Ensure raylib is built for system platform
if [ ! -f ./lib/raylib/src/libraylib.a ]; then
  echo "📦 Building Raylib library..."
  (cd lib/raylib/src && make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC)
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
  clang ./main.c \
    -I./lib/raylib/src/ \
    ./lib/raylib/src/libraylib.a \
    -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework OpenGL \
    -o ./build/wordgame.exe
else
  gcc ./main.c \
    -I./lib/raylib/src/ \
    ./lib/raylib/src/libraylib.a \
    -lglfw -lGL -lopenal -lm -lpthread -ldl -lrt -lX11 \
    -o ./build/wordgame.exe
fi

# (TODO) Add release build flags
# -Wall -Wextra -Wpedantic -O0
