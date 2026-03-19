.PHONY: build

build:
	@bash ./build.sh

run:
	@bash ./build.sh && ./build/wordgame.exe

web:
	mkdir -p build
	emcc ./src/main.c \
	  -I./lib/raylib/src \
	  ./lib/raylib/src/libraylib.web.a \
	  -DPLATFORM_WEB \
	  -s USE_GLFW=3 \
	  -s ASYNCIFY \
	  -s SINGLE_FILE=1 \
	  --preload-file assets \
	  --preload-file all_words.txt \
		-sINITIAL_MEMORY=33554432 \
		-sALLOW_MEMORY_GROWTH=1 \
	  -o build/index.html
