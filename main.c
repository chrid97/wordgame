#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define VIRTUAL_WIDTH 640
#define VIRTUAL_HEIGHT 360

#define LETTER_COUNT 26
#define LETTER_BAG_SIZE 100
#define BOARD_COL 4
#define BOARD_ROW 4
#define BOARD_COUNT (BOARD_COL * BOARD_ROW)
#define TILE_SIZE 40
#define TILE_NONE 0xFF // Sentinel Value
#define MAX_CHAR_SELECTION 10

//----------------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------------
typedef enum { TILE } EntityType;
typedef struct {
  EntityType type;
  Vector2 position;
  int width;
  int height;
  char tile_value;
} Entity;

typedef struct {
  Entity tiles[LETTER_BAG_SIZE];
  uint8_t remaining;
  uint8_t size; // total number of tiles
} LetterBag;

//----------------------------------------------------------------------------------
// State
//----------------------------------------------------------------------------------
char *words;
int total_words = 0;
LetterBag letter_bag = {0};
// do we want to include null terminator?
uint8_t selection[MAX_CHAR_SELECTION]; // index to tile in letter bag
int selection_length = 0;
uint8_t board[BOARD_COUNT] = {TILE_NONE}; // index to tiles in bag
Texture2D letter_textures[LETTER_COUNT] = {0};

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------
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
          // .tile_location = BAG,
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

  load_letter_textures();
  Texture2D tile_texture = LoadTexture("assets/tile.png");

  load_words();
  srand(time(NULL));
  init_letter_bag();
  shuffle_bag();

  const int padding = 10;
  const int board_width = BOARD_ROW * (TILE_SIZE + padding);
  const int board_height = BOARD_COL * (TILE_SIZE + padding);
  const float board_origin_x = (VIRTUAL_WIDTH / 2.0) - board_width / 2.0;
  const float board_origin_y = VIRTUAL_HEIGHT - board_height;
  const Rectangle board_rect = {board_origin_x, board_origin_y, board_width,
                                board_height};
  const int selection_origin_x = 100;
  const int selection_origin_y = board_height - 200;

  bool fill_board = true;
  while (!WindowShouldClose()) {
    //----------------------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------------------
    if (fill_board) {
      for (int i = 0; i < BOARD_COUNT; i++) {
        Entity *letter = &letter_bag.tiles[letter_bag.remaining - i - 1];
        board[i] = letter_bag.remaining - i - 1;
        letter_bag.remaining--;
      }
      fill_board = false;
    }

    // Maybe store most recent mouse_pos then i can get rid of this statement
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse_pos = GetMousePosition();

      // If click on board
      if (selection_length < MAX_CHAR_SELECTION &&
          CheckCollisionPointRec(mouse_pos, board_rect)) {
        for (int i = 0; i < BOARD_COUNT; i++) {
          const uint8_t row = i / 4;
          const uint8_t col = i % 4;
          const int x = board_origin_x + (col * (TILE_SIZE + padding));
          const int y = board_origin_y + (row * (TILE_SIZE + padding));

          if (CheckCollisionPointRec(mouse_pos,
                                     (Rectangle){x, y, TILE_SIZE, TILE_SIZE})) {
            const uint8_t cell = col + (row * BOARD_COL);
            printf("Selected tile %c in (%i, %i) \n",
                   letter_bag.tiles[board[cell]].tile_value, col, row);

            bool already_chosen = false;
            for (int i = 0; i < selection_length; i++) {
              if (board[cell] == selection[i]) {
                already_chosen = true;
                printf("Tile already chosen\n");
              }
            }
            if (!already_chosen) {
              selection[selection_length++] = board[cell];
            }
          }
        }
      }

      if (CheckCollisionPointRec(
              mouse_pos, (Rectangle){selection_origin_x, selection_origin_y,
                                     (TILE_SIZE + padding) * selection_length,
                                     TILE_SIZE})) {
        const float relative_x = mouse_pos.x - selection_origin_x;
        const int cell = relative_x / (TILE_SIZE + padding);
        printf("Selected chosen tile %c\n",
               letter_bag.tiles[selection[cell]].tile_value);

        selection_length = selection_length - (selection_length - cell);
      }
    }

    //----------------------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(ORANGE);

    DrawRectangle(board_origin_x, board_origin_y, board_width, board_height,
                  BROWN);

    for (int i = 0; i < BOARD_COUNT; i++) {
      Entity *tile = &letter_bag.tiles[board[i]];
      const uint8_t row = i / 4;
      const uint8_t col = i % 4;
      const int x = board_origin_x + (col * (TILE_SIZE + padding));
      const int y = board_origin_y + (row * (TILE_SIZE + padding));
      DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, RED);
      // DrawTexture(tile_texture, x, y, WHITE);
      bool already_chosen = false;
      for (int j = 0; j < selection_length; j++) {
        if (board[i] == selection[j]) {
          already_chosen = true;
        }
      }
      const Color color = already_chosen ? GRAY : WHITE;
      DrawTextureEx(tile_texture, (Vector2){x, y}, 0, 1.3, color);
      DrawTexture(letter_textures[tile->tile_value - 'a'], x, y, WHITE);
    }

    for (int i = 0; i < selection_length; i++) {
      Entity *tile = &letter_bag.tiles[selection[i]];
      const int x = selection_origin_x + (i * (TILE_SIZE + padding));
      DrawTextureEx(tile_texture, (Vector2){x, selection_origin_y}, 0, 1.3,
                    WHITE);
      DrawTexture(letter_textures[tile->tile_value - 'a'], x,
                  selection_origin_y, WHITE);
    }

    EndDrawing();
  }

  //----------------------------------------------------------------------------------
  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
