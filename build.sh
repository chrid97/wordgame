mkdir -p build/

# (TODO) Add release build flags
# -Wall -Wextra -Wpedantic -O0

gcc ./main.c \
  -I./lib/raylib/src/ \
  ./lib/raylib/src/libraylib.a \
  -lglfw -lGL -lopenal -lm -lpthread -ldl -lrt -lX11 \
  -o ./build/wordgame.exe
