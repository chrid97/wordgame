# Follow this to step 4 https://github.com/raysan5/raylib/wiki/Working-on-Windows#manual-setup-with-w64devkit

mkdir -p build/

gcc -o ./build/wordgame.exe ./main.c -I include -L lib -lraylib -lgdi32 -lwinmm