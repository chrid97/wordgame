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
  TileLocation tile_location;
} Entity;

#define LETTER_BAG_SIZE 100
typedef struct {
  Entity tiles[LETTER_BAG_SIZE];
  // uint8_t *deck;     // pointer to tiles
  uint8_t remaining; // tiles in the bag
  uint8_t size;      // total number of tiles
} LetterBag;
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// State
//----------------------------------------------------------------------------------
char *words;
int total_words = 0;

LetterBag letter_bag = {0};

// do we want to include null terminator?
char input[10];
int input_length = 0;
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
      letter_bag.tiles[i++] = (Entity){
          .type = TILE,
          .tile_location = BAG,
          .position = {},
          .height = 50,
          .width = 50,
          .tile_value = 97 + letter,
      };
    }
  }
  letter_bag.remaining = 100;
}

void shuffle_bag() {
  for (int i = 99; i > 0; i--) {
    int j = rand() % (i + 1);
    Entity temp = letter_bag.tiles[i];
    letter_bag.tiles[i] = letter_bag.tiles[j];
    letter_bag.tiles[j] = temp;
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

  bool draw_new_hand = true;
  while (!WindowShouldClose()) {
    // Update
    //----------------------------------------------------------------------------------

    // draw hand
    if (draw_new_hand) {
      for (int i = 0; i < 16; i++) {
        Entity *letter = &letter_bag.tiles[letter_bag.remaining - i - 1];
        letter->tile_location = HAND;
      }
      draw_new_hand = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse_pos = GetMousePosition();

      for (int i = 0; i < 16; i++) {
        Entity *letter = &letter_bag.tiles[letter_bag.remaining - i - 1];

        if (letter->tile_location == HAND &&
            mouse_pos.x >= letter->position.x &&
            mouse_pos.x <= letter->position.x + letter->width &&
            mouse_pos.y >= letter->position.y &&
            mouse_pos.y <= letter->position.y + letter->height) {

          letter->tile_location = IN_PLAY;
          input[input_length++] = letter->tile_value;
          printf("%s\n", input);
        }
      }
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(ORANGE);

    for (int i = 0; i < 16; i++) {
      uint8_t row = i / 4;
      uint8_t col = i % 4;
      uint8_t padding = 45;

      Entity *current_letter = &letter_bag.tiles[letter_bag.remaining - i - 1];
      current_letter->width = 40;
      current_letter->height = 40;
      current_letter->position.x = col * padding;
      current_letter->position.y = row * padding;
      DrawRectangle(current_letter->position.x, current_letter->position.y,
                    current_letter->width, current_letter->height, WHITE);
      const char *letter = TextFormat("%c", current_letter->tile_value);
      DrawText(letter, current_letter->position.x, current_letter->position.y,
               30, BLACK);
    }

    for (int i = 0; i < input_length; i++) {
      const char *letter = TextFormat("%c", input[i]);
      DrawText(letter, 250 + i * 30, 20, 30, BLACK);
    }

    // submit button
    DrawRectangle(400, 200, 140, 40, RED);
    DrawText("Submit", 400, 200, 30, BLACK);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
