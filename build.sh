mkdir -p build/

gcc ./main.c \
  -I./lib/raylib/src/ \
  ./lib/raylib/src/libraylib.a \
  -lglfw -lGL -lopenal -lm -lpthread -ldl -lrt -lX11 \
  -o ./build/wordgame.exe
