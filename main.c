#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#define VIRTUAL_WIDTH 640
#define VIRTUAL_HEIGHT 360

#define LETTER_COUNT 26
#define LETTER_BAG_SIZE 100
#define BOARD_COL 4
#define BOARD_ROW 4
#define BOARD_COUNT (BOARD_COL * BOARD_ROW)
#define TILE_SIZE 32
// #define TILE_NONE 0xFF // Sentinel Value
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

uint8_t selection[MAX_CHAR_SELECTION] = {0}; // index to tile in letter bag
uint8_t selection_length = 0;

char selected_word[MAX_CHAR_SELECTION + 1] = {0}; // actual word

uint8_t board[BOARD_COUNT] = {0};                 // index to tiles in bag
uint8_t selected_cells[MAX_CHAR_SELECTION] = {0}; // Selected board index

bool is_selected[LETTER_BAG_SIZE] = {0};
bool valid_word = false;

char **word_pointers;
char *current_word;

Texture2D tile_texture;
Texture2D letter_textures[LETTER_COUNT] = {0};

// UI
const uint8_t padding = 3;
const uint8_t pitch = TILE_SIZE + padding;
const int board_width = BOARD_ROW * (TILE_SIZE + padding);
const int board_height = BOARD_COL * (TILE_SIZE + padding);
const float board_origin_x = (VIRTUAL_WIDTH / 2.0) - board_width / 2.0;
const float board_origin_y = VIRTUAL_HEIGHT - board_height;
const Rectangle board_rect = {board_origin_x, board_origin_y, board_width,
                              board_height};
const float selection_origin_x = 100;
const float selection_origin_y = board_height - 200;
Vector2 submit_button_pos = {board_origin_x + board_width + 25,
                             VIRTUAL_HEIGHT - 25};
float submit_button_radius = 20;
//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------
bool binary_search_word(char **words, const char *target, int count) {
  int high = count - 1;
  int low = 0;
  while (low <= high) {
    int mid = (low + high) / 2;

    int result = strcmp(target, words[mid]);
    if (result == 0) {
      return true;
    }

    if (result > 0) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }

  return false;
}

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
  int i = 0;
  for (int letter = 0; letter < 26; letter++) {
    for (int j = 0; j < letter_occurrence[letter]; j++) {
      letter_bag.tiles[i++] = (Entity){
          .type = TILE,
          .position = {},
          .height = 50,
          .width = 50,
          .tile_value = 97 + letter,
      };
    }
  }
  letter_bag.remaining = 99;
}

// update this to go up to remaining tiles
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

void draw_tile(int tile_idx, Rectangle rect, bool selected) {
  Color tint = selected ? GRAY : WHITE;
  DrawTexture(tile_texture, rect.x, rect.y, tint);

  char tile_value = letter_bag.tiles[tile_idx].tile_value;
  Texture2D letter_tex = letter_textures[tile_value - 'a'];

  float x = rect.x + (rect.width - letter_tex.width) / 2.0f;
  float y = rect.y + (rect.height - letter_tex.height) / 2.0f;
  DrawTexture(letter_tex, (int)x, (int)y, tint);
}

void update_draw(void) {
  //----------------------------------------------------------------------------------
  // Update
  //----------------------------------------------------------------------------------
  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    // i wonder if this goto is sus
    goto draw;
  }

  Vector2 mouse_pos = GetMousePosition();
  if (selection_length < MAX_CHAR_SELECTION &&
      CheckCollisionPointRec(mouse_pos, board_rect)) {
    const float relative_x = (mouse_pos.x - board_origin_x);
    const float relative_y = (mouse_pos.y - board_origin_y);

    const uint8_t col = relative_x / pitch;
    const uint8_t row = relative_y / pitch;
    const uint8_t cell = col + (row * BOARD_COL);

    printf("[click] cell=%u row=%u col=%u tile='%c' (idx=%u)\n", cell, row, col,
           letter_bag.tiles[board[cell]].tile_value, board[cell]);

    uint8_t tile_idx = board[cell];
    if (!is_selected[tile_idx]) {
      selection[selection_length] = tile_idx;
      is_selected[tile_idx] = true;

      selected_word[selection_length] = letter_bag.tiles[tile_idx].tile_value;
      selected_word[selection_length + 1] = '\0';
      selected_cells[selection_length] = cell;

      selection_length++;
    } else {
      printf("Tile already selected\n");
    }

    // (TODO) only binary search when we select an unselected tile
    valid_word = binary_search_word(word_pointers, selected_word, total_words);
    printf("Current word: %s\n", selected_word);
  }

  if (CheckCollisionPointRec(
          mouse_pos,
          (Rectangle){selection_origin_x, selection_origin_y,
                      (TILE_SIZE + padding) * selection_length, TILE_SIZE})) {
    const float relative_x = mouse_pos.x - selection_origin_x;
    const int cell = relative_x / (TILE_SIZE + padding);
    printf("[click] Deselect tile %c\n",
           letter_bag.tiles[selection[cell]].tile_value);

    // Set selected tiles to false
    for (int i = cell; i < selection_length; i++) {
      is_selected[selection[i]] = false;
    }
    selection_length = cell;
    selected_word[selection_length] = '\0';

    // (TODO) only binary search if selection_length > 0
    valid_word = binary_search_word(word_pointers, selected_word, total_words);
    printf("Current word: %s\n", selected_word);
  }

  if (valid_word && CheckCollisionPointCircle(mouse_pos, submit_button_pos,
                                              submit_button_radius)) {
    printf("Submitted word %s!\n", selected_word);
    for (int i = 0; i < selection_length; i++) {
      uint8_t cell = selected_cells[i];
      board[cell] = letter_bag.remaining;
      is_selected[selection[i]] = false;
      letter_bag.remaining--;
      printf("Refilled bag with tile %c\n",
             letter_bag.tiles[letter_bag.remaining].tile_value);
    };
    selection_length = 0;
    selected_word[0] = '\0';
  }

  //----------------------------------------------------------------------------------
  // Draw
  //----------------------------------------------------------------------------------
draw:
  BeginDrawing();
  ClearBackground(ORANGE);

  DrawRectangle(board_origin_x, board_origin_y, board_width, board_height,
                BROWN);

  for (int i = 0; i < BOARD_COUNT; i++) {
    const uint8_t row = i / BOARD_ROW;
    const uint8_t col = i % BOARD_COL;
    const int x = board_origin_x + (col * (TILE_SIZE + padding));
    const int y = board_origin_y + (row * (TILE_SIZE + padding));
    draw_tile(board[i], (Rectangle){x, y, TILE_SIZE, TILE_SIZE},
              is_selected[board[i]]);
  }

  for (int i = 0; i < selection_length; i++) {
    const int x = selection_origin_x + (i * (TILE_SIZE + padding));
    const int y = selection_origin_y + 100;
    draw_tile(selection[i], (Rectangle){x, y, TILE_SIZE, TILE_SIZE}, false);
  }

  // Word Submit Button
  DrawCircleV(submit_button_pos, submit_button_radius,
              valid_word ? GREEN : GRAY);
  DrawText("Submit", submit_button_pos.x - 15, submit_button_pos.y - 5, 10,
           BLACK);
  DrawCircleLinesV(submit_button_pos, submit_button_radius, BLACK);

  EndDrawing();
}

int main(int argc, char *argv[]) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Wordgame");
  SetTargetFPS(0);

  load_letter_textures();
  tile_texture = LoadTexture("assets/tile.png");

  load_words();
  word_pointers = malloc(total_words * sizeof(char *));
  current_word = words;
  for (int i = 0; i < total_words; i++) {
    word_pointers[i] = current_word;
    current_word += strlen(current_word) + 1;
  }

  srand(time(NULL));
  init_letter_bag();
  shuffle_bag();

  for (int i = 0; i < BOARD_COUNT; i++) {
    board[i] = letter_bag.remaining;
    letter_bag.remaining--;
  }
#ifdef PLATFORM_WEB
  emscripten_set_main_loop(update_draw, 0, 1);
#else
  while (!WindowShouldClose()) {
    update_draw();
  }
#endif

  //----------------------------------------------------------------------------------
  // De-Initialization
  //--------------------------------------------------------------------------------------
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
