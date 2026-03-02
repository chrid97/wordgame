#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define VIRTUAL_WIDTH 640
#define VIRTUAL_HEIGHT 360

//----------------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------------
typedef enum { TILE } EntityType;
typedef enum { BOARD, BAG, USED, IN_PLAY } TileLocation;

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
uint8_t input[10]; // index to tile in letter bag
int input_length = 0;

const uint8_t padding = 38;
const int board_width = 4 * padding;
const int board_height = 4 * padding;
const int half_width = VIRTUAL_WIDTH / 2;
const int half_height = VIRTUAL_HEIGHT / 2;
const int board_origin_x = half_width - (board_width / 2);
const int board_origin_y = half_width - board_height;

// Rectangle hand = {board_origin_x, board_origin_y, board_width, board_height};
#define BOARD_COUNT 16
uint8_t board[BOARD_COUNT]; // index to tiles in bag
uint8_t board_length;
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------
#define LETTER_COUNT 26

Texture2D letter_textures[LETTER_COUNT] = {0};

void load_letter_textures(void) {
  for (int i = 0; i < LETTER_COUNT; i++) {
    char path[256];
    snprintf(path, sizeof(path), "assets/letters/%c.png", 'a' + i);

    letter_textures[i] = LoadTexture(path);

    if (letter_textures[i].id == 0) {
      TraceLog(LOG_ERROR, "Failed to load %s", path);
    }
  }
}

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

void draw_tile(Entity *letter, Texture2D tile, Color color) {
  DrawTexture(tile, letter->position.x, letter->position.y, color);
  DrawTexture(letter_textures[letter->tile_value - 'a'], letter->position.x,
              letter->position.y, WHITE);
}

int main(int argc, char *argv[]) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Wordgame");
  SetTargetFPS(0);

  load_letter_textures();
  Texture2D tile_texture = LoadTexture("assets/tile.png");

  load_words();
  srand(time(NULL));
  init_letter_bag();
  shuffle_bag();

  bool draw_new_hand = true;
  while (!WindowShouldClose()) {
    // Update
    //----------------------------------------------------------------------------------

    if (draw_new_hand) {
      for (int i = 0; i < BOARD_COUNT; i++) {
        Entity *letter = &letter_bag.tiles[letter_bag.remaining - i - 1];
        letter->tile_location = BOARD;
        board[i] = letter_bag.remaining - i - 1;
        board_length++;
      }
      draw_new_hand = false;
    }
    assert(board_length != 0);

    for (int i = 0; i < letter_bag.remaining; i++) {
      Entity *tile = &letter_bag.tiles[i];
      switch (tile->tile_location) {
      case BOARD: {
        tile->position.x = 0;
        tile->position.y = 0;
      } break;
      case IN_PLAY: {
        tile->position.x = 0;
        tile->position.y = 0;
      } break;
      case BAG: {
      } break;
      case USED: {
      } break;
      }
    }

    // if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    //   Vector2 mouse_pos = GetMousePosition();
    //   Rectangle mouse_box = {mouse_pos.x, mouse_pos.y, 0, 0};
    //
    //   for (int i = 0; i < 16; i++) {
    //     Entity *letter = &letter_bag.tiles[letter_bag.remaining - i - 1];
    //
    //     if (letter->tile_location == BOARD &&
    //         mouse_pos.x >= letter->position.x &&
    //         mouse_pos.x <= letter->position.x + letter->width &&
    //         mouse_pos.y >= letter->position.y &&
    //         mouse_pos.y <= letter->position.y + letter->height) {
    //
    //       letter->tile_location = IN_PLAY;
    //       input[input_length++] = letter_bag.remaining - i - 1;
    //     }
    //   }
    //
    //   for (int i = 0; i < input_length; i++) {
    //     Entity *letter = &letter_bag.tiles[input[i]];
    //
    //     if (letter->tile_location == IN_PLAY &&
    //         CheckCollisionRecs(
    //             mouse_box, (Rectangle){letter->position.x,
    //             letter->position.y,
    //                                    letter->width, letter->height})) {
    //       letter->tile_location = BOARD;
    //       input[i] = 0;
    //       input_length--;
    //     }
    //   }
    //
    //   Rectangle button = {400, 200, 140, 40};
    //   char *word_pointer = words;
    //
    //   char submit_word[10] = {0};
    //   for (int i = 0; i < input_length; i++) {
    //     submit_word[i] = letter_bag.tiles[input[i]].tile_value;
    //     printf("%c\n", submit_word[i]);
    //   }
    //
    //   if (CheckCollisionRecs(mouse_box, button)) {
    //     for (int i = 0; i < total_words; i++) {
    //       int result = strcmp(submit_word, word_pointer);
    //
    //       if (result == 0) {
    //         printf("Match found\n");
    //         break;
    //       }
    //       word_pointer += strlen(word_pointer) + 1;
    //     }
    //
    //     printf("Match not found\n");
    //   }
    // }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(ORANGE);

    for (int i = 0; i < BOARD_COUNT; i++) {
      Entity *tile = &letter_bag.tiles[board[i]];
      assert(tile->tile_location == BOARD);
      draw_tile(tile, tile_texture, WHITE);
    }

    // const uint8_t padding = 38;
    // const int board_width = 4 * padding;
    // const int board_height = 4 * padding;
    // const int half_width = VIRTUAL_WIDTH / 2;
    // const int half_height = VIRTUAL_HEIGHT / 2;
    // const int board_origin_x = half_width - (board_width / 2);
    // const int board_origin_y = half_width - board_height;
    //
    // for (int i = 0; i < 16; i++) {
    //   uint8_t row = i / 4;
    //   uint8_t col = i % 4;
    //   Color color = WHITE;
    //
    //   Entity *letter = &letter_bag.tiles[letter_bag.remaining - i - 1];
    //   letter->width = 32;
    //   letter->height = 32;
    //   letter->position.x = col * padding + board_origin_x;
    //   letter->position.y = row * padding + board_origin_y;
    //   if (letter->tile_location == IN_PLAY) {
    //     color = GRAY;
    //   }
    //   draw_tile(letter, tile, color);
    // }
    //
    // for (int i = 0; i < input_length; i++) {
    //   Entity *t = &letter_bag.tiles[input[i]];
    //   t->position = (Vector2){250 + i * 38, 20};
    //   draw_tile(t, tile, WHITE);
    // }
    //
    // // submit button
    // DrawRectangle(400, 200, 140, 40, RED);
    // DrawText("Submit", 400, 200, 30, BLACK);

    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
