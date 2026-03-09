.PHONY: build

build:
	@bash ./build.sh

run:
	@bash ./build.sh && ./build/wordgame.exe

web:
	mkdir -p build
	emcc ./main.c \
	  -I./lib/raylib/src \
	  ./lib/raylib/src/libraylib.web.a \
	  -DPLATFORM_WEB \
	  -s USE_GLFW=3 \
	  -s ASYNCIFY \
	  -s SINGLE_FILE=1 \
	  --preload-file assets \
	  --preload-file all_words.txt \
	  -o build/index.html
