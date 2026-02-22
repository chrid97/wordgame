#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VIRTUAL_WIDTH 640
#define VIRTUAL_HEIGHT 360

//----------------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------------
typedef enum { TILE } EntityType;
typedef enum { HAND, BAG, USED, IN_PLAY } TileLocation;

typedef struct {
  EntityType type;
  Vector2 position;
  int width;
  int height;

  char tile_value;
  bool tile_location;
} Entity;

#define HAND_SIZE 16
typedef struct {
  Entity index[HAND_SIZE]; // index into letter bag
  uint8_t size;
} Hand;

#define LETTER_BAG_SIZE 100
typedef struct {
  Entity tiles[LETTER_BAG_SIZE];
  uint8_t size;
} LetterBag;
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// State
//----------------------------------------------------------------------------------
char *words;
int total_words = 0;

Entity letter_bag[LETTER_BAG_SIZE] = {0};
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------
void init_letter_bag() {
  uint8_t letter_occurrence[26] = {
      8,  // a
      2,  // b
      4,  // c
      4,  // d
      11, // e
      1,  // f
      3,  // g
      2,  // h
      8,  // i
      1,  // j
      1,  // k
      5,  // l
      3,  // m
      7,  // n
      6,  // o
      3,  // p
      1,  // q
      7,  // r
      8,  // s
      5,  // t
      4,  // u
      1,  // v
      1,  // w
      1,  // x
      2,  // y
      1   // z
  };
  Entity entity = {.position = {}, .type = TILE};

  int i = 0;
  for (int letter = 0; letter < 26; letter++) {
    for (int j = 0; j < letter_occurrence[letter]; j++) {
      letter_bag[i++] = (Entity){
          .type = TILE,
          .tile_location = BAG,
          .position = {},
          .height = 50,
          .width = 50,
          .tile_value = 97 + letter,
      };
    }
  }
}

void draw_hand(Hand *hand) {
  for (int i = 0; i < hand->size; i++) {
    // hand->tiles[i] = ;
  }
}

void shuffle_bag() {
  for (int i = 99; i > 0; i--) {
    int j = rand() % (i + 1);
    Entity temp = letter_bag[i];
    letter_bag[i] = letter_bag[j];
    letter_bag[j] = temp;
  }
}

void load_words() {
  char filename[32] = "all_words.txt";
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "ERROR: Failed to load %s\n", filename);
    exit(1);
  }

  // handle fseek error
  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  words = malloc(file_size + 1);
  int elements_read = fread(words, 1, file_size, file);
  words[elements_read] = '\0';

  for (int i = 0; i < elements_read; i++) {
    if (words[i] == '\n') {
      words[i] = '\0';
      total_words++;
    }
  }

  fclose(file);
}

int main(int argc, char *argv[]) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Wordgame");
  SetTargetFPS(0);

  load_words();
  srand(time(NULL));
  init_letter_bag();
  shuffle_bag();

  while (!WindowShouldClose()) {
    // Update
    //----------------------------------------------------------------------------------
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse_pos = GetMousePosition();
      // printf("%d", mouse_pos.x);
      // printf("%d", mouse_pos.y);
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(ORANGE);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
