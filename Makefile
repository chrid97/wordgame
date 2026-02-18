.PHONY: build

build:
	@bash ./build.sh

run:
	@bash ./build.sh && ./build/wordgame.exe
