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

#define TURN_TRANSITION_DURATION 1.0f

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

  uint8_t max_health_points;
  uint8_t health_points;
} Entity;

typedef struct {
  Entity tiles[LETTER_BAG_SIZE];
  uint8_t remaining;
  uint8_t size; // total number of tiles
} LetterBag;

// (TODO) Think of a better name
typedef enum { IDLE, ATTACK, HURT } Action;

typedef enum {
  TURN_TRANSITION_TO_PLAYER,
  PLAYER_TURN,
  // PLAYER_ACTION_RESOLVE,
  TURN_TRANSITION_TO_ENEMY,
  ENEMY_TURN,
  // ENEMY_ACTION_RESOLVE
} Phase;

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
Texture2D background;

Texture2D mushroom_idle;
Texture2D mushroom_hit;
Texture2D mushroom_attack;

Texture2D knight_idle;
Texture2D knight_attack;
Texture2D knight_hurt;

Sound punch_sound;
Sound keystroke_sound;
Sound backspace_sound;

Music upbeat_music;

Action enemy_action = IDLE;
Action player_action = IDLE;

Phase phase = TURN_TRANSITION_TO_PLAYER;

int player_pending_damage = 0;
float player_attack_start_time = 0.0f;
// start time for animation
float player_damage_start_time = 0.0f;
bool player_hit_applied = false;

float enemy_attack_start_time = 0.0f;
float enemy_damage_start_time = 0.0f;
bool enemy_hit_applied = false;

float turn_transition_start_time = 1.0f;

// UI
const uint8_t padding = 3;
const uint8_t pitch = TILE_SIZE + padding;
const int board_width = BOARD_ROW * (TILE_SIZE + padding);
const int board_height = BOARD_COL * (TILE_SIZE + padding);
const float board_origin_x = (VIRTUAL_WIDTH / 2.0) - board_width / 2.0;
const float board_origin_y = VIRTUAL_HEIGHT - board_height;
const Rectangle board_rect = {board_origin_x, board_origin_y, board_width,
                              board_height};
const float selection_origin_x = 200;
const float selection_origin_y = board_height;
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

void draw_health_bar(Rectangle bar, uint8_t current_hp, uint8_t max_hp) {
  DrawRectangleRec(bar, GRAY);
  DrawRectangleRoundedLinesEx(bar, 100, 4, 1, BLACK);

  Rectangle fill = bar;
  fill.width = ((float)current_hp / max_hp) * bar.width;
  DrawRectangleRec(fill, RED);

  char hp_text[32];
  snprintf(hp_text, sizeof(hp_text), "%d/%d", current_hp, max_hp);

  const int font_size = 18;
  const int text_width = MeasureText(hp_text, font_size);

  int text_x = bar.x + (bar.width - text_width) / 2;
  int text_y = bar.y - (float)font_size / 3;

  // --- BLACK (draw multiple times) ---
  DrawText(hp_text, text_x - 2, text_y, font_size, BLACK);
  DrawText(hp_text, text_x + 2, text_y, font_size, BLACK);
  DrawText(hp_text, text_x, text_y - 2, font_size, BLACK);
  DrawText(hp_text, text_x, text_y + 2, font_size, BLACK);

  DrawText(hp_text, text_x - 1, text_y - 1, font_size, BLACK);
  DrawText(hp_text, text_x + 1, text_y - 1, font_size, BLACK);
  DrawText(hp_text, text_x - 1, text_y + 1, font_size, BLACK);
  DrawText(hp_text, text_x + 1, text_y + 1, font_size, BLACK);

  // --- main text ---
  DrawText(hp_text, text_x, text_y, font_size, WHITE);
}

Entity player = {.health_points = 20, .max_health_points = 20};
Entity enemy = {.health_points = 20, .max_health_points = 20};

void update_draw(void) {
  //----------------------------------------------------------------------------------
  // Update
  //----------------------------------------------------------------------------------
  UpdateMusicStream(upbeat_music);
  SetMusicVolume(upbeat_music, 0.1f);

  if (phase == TURN_TRANSITION_TO_PLAYER || phase == TURN_TRANSITION_TO_ENEMY) {
    if (turn_transition_start_time > 0) {
      double elapsed = GetTime() - turn_transition_start_time;
      if (elapsed >= TURN_TRANSITION_DURATION) {
        phase = phase == TURN_TRANSITION_TO_PLAYER ? PLAYER_TURN : ENEMY_TURN;
        turn_transition_start_time = 0.0f;
      }
    }
  }

  switch (player_action) {
  case IDLE: {
  } break;
  case ATTACK: {
    double elapsed = GetTime() - player_attack_start_time;
    int frame = (int)(elapsed * 15);
    if (!player_hit_applied && frame == 3) {
      enemy_damage_start_time = GetTime();
      enemy_action = HURT;
      PlaySound(punch_sound);
      player_hit_applied = true;

      if (player_pending_damage >= enemy.health_points) {
        enemy.health_points = 0;
      } else {
        enemy.health_points -= player_pending_damage;
      }
      player_pending_damage = 0;
    }

    if (frame >= 6) {
      player_action = IDLE;
      // (TODO) Should maybe be player turn end
      // phase = TURN_TRANSITION_TO_ENEMY;
    }
  } break;
  case HURT: {
    double elapsed = GetTime() - player_damage_start_time;
    int frame = (int)(elapsed * 15);
    if (frame >= 4) {
      player_action = IDLE;
    }
  } break;
  }

  switch (enemy_action) {
  case ATTACK: {
    const int frames = 9;
    float fps = 10.0f;
    float elapsed = GetTime() - enemy_attack_start_time;
    int frame = (int)(elapsed * fps);
    if (!enemy_hit_applied && frame == 6) {
      // (TODO) I should probably play the damage sound when the target is hit?
      // I could even pass a sound effect to it if i wanted it to be different
      // based on the players attack
      PlaySound(punch_sound);
      player_action = HURT;
      enemy_hit_applied = true;
      player_damage_start_time = GetTime();
      int damage = rand() % 5;
      if (damage >= player.health_points) {
        player.health_points = 0;
      } else {
        player.health_points -= damage;
      }
    }

    if (frame >= frames) {
      enemy_action = IDLE;
      phase = TURN_TRANSITION_TO_PLAYER;
      turn_transition_start_time = GetTime();
    }
  } break;
  case IDLE: {
  } break;
  case HURT: {
    const int frames = 5;
    const float fps = 15.0f;
    float elapsed = GetTime() - enemy_damage_start_time;
    int frame = (int)(elapsed * fps);
    if (frame >= frames) {
      enemy_action = IDLE;
      phase = TURN_TRANSITION_TO_ENEMY;
      turn_transition_start_time = GetTime();
    }
  } break;
  }

  if (phase == ENEMY_TURN) {
    if (enemy_action != ATTACK) {
      enemy_action = ATTACK;
      enemy_attack_start_time = GetTime();
      enemy_hit_applied = false;
    }
    goto draw;
  }

  if (phase != PLAYER_TURN) {
    goto draw;
  }

  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

      valid_word =
          selection_length > 0 &&
          binary_search_word(word_pointers, selected_word, total_words);
      PlaySound(keystroke_sound);
    } else {
      printf("Tile already selected\n");
    }

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

    valid_word = selection_length > 0 &&
                 binary_search_word(word_pointers, selected_word, total_words);
    printf("Current word: %s\n", selected_word);

    PlaySound(backspace_sound);
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

    player_pending_damage = selection_length;

    selection_length = 0;
    selected_word[0] = '\0';
    valid_word = false;

    player_action = ATTACK;
    player_attack_start_time = GetTime();
    player_hit_applied = false;
  }

  // (TODO) Right click should clear selection
  // (MAYBE) right click on selected tile to insert before or after

  //----------------------------------------------------------------------------------
  // Draw
  //----------------------------------------------------------------------------------
draw:
  BeginDrawing();
  Rectangle src = {0, 0, background.width, background.height};
  Rectangle background_dest = {0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT};
  DrawTexturePro(background, src, background_dest, (Vector2){0, 0}, 0, WHITE);

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
    draw_tile(selection[i],
              (Rectangle){x, selection_origin_y, TILE_SIZE, TILE_SIZE}, false);
  }

  // Word Submit Button
  DrawCircleV(submit_button_pos, submit_button_radius,
              valid_word ? GREEN : GRAY);
  DrawText("Submit", submit_button_pos.x - 15, submit_button_pos.y - 5, 10,
           BLACK);
  DrawCircleLinesV(submit_button_pos, submit_button_radius, BLACK);

  const float scale = 2.5f;
  switch (player_action) {
  case ATTACK: {
    const int frame_width = 60;
    const int frame_height = 36;
    const int frame_count = 6;
    const float fps = 15.0f;
    float elapsed = GetTime() - player_attack_start_time;
    int frame = (int)(elapsed * fps);
    float dest_w = frame_width * scale;
    float dest_h = frame_height * scale;
    float ground_y = board_origin_y - 10;
    Rectangle dest = {100 - dest_w / 2.0f, ground_y - dest_h, dest_w, dest_h};
    Rectangle src = {frame * frame_width, 0, frame_width, frame_height};
    DrawTexturePro(knight_attack, src, dest, (Vector2){0, 0}, 0, WHITE);
    Rectangle player_health = {dest.x + (dest.width - 80) / 2.0f,
                               dest.y + dest.height + 8, 80, 6};
    draw_health_bar(player_health, player.health_points,
                    player.max_health_points);
  } break;
  case IDLE: {
    const int frame_width = 32;
    const int frame_height = 37;
    const int frame_count = knight_idle.width / frame_width;
    int current_frame = ((int)(GetTime() * 7.5f)) % frame_count;
    Rectangle src = {current_frame * frame_width, 0, frame_width, frame_height};
    float dest_w = frame_width * scale;
    float dest_h = frame_height * scale;
    float ground_y = board_origin_y - 10;
    float center_x = 100;
    Rectangle dest = {100 - frame_width * scale / 2.0f, ground_y - dest_h,
                      dest_w, dest_h};
    DrawTexturePro(knight_idle, src, dest, (Vector2){0, 0}, 0, WHITE);
    Rectangle player_health = {dest.x + (dest.width - 80) / 2.0f,
                               dest.y + dest.height + 8, 80, 6};
    draw_health_bar(player_health, player.health_points,
                    player.max_health_points);
  } break;
  case HURT: {
    const int frame_width = 32;
    const int frame_height = 36;
    const int frames = 4;
    const float fps = 15.0f;
    float elapsed = GetTime() - player_damage_start_time;
    int frame = (int)(elapsed * fps);
    Rectangle src = {frame * frame_width, 0, frame_width, frame_height};
    float dest_w = frame_width * scale;
    float dest_h = frame_height * scale;
    float ground_y = board_origin_y - 10;
    float center_x = 100;
    Rectangle dest = {100 - frame_width * scale / 2.0f, ground_y - dest_h,
                      dest_w, dest_h};
    DrawTexturePro(knight_hurt, src, dest, (Vector2){0, 0}, 0, WHITE);
    Rectangle player_health = {dest.x + (dest.width - 80) / 2.0f,
                               dest.y + dest.height + 8, 80, 6};
    draw_health_bar(player_health, player.health_points,
                    player.max_health_points);
  } break;
  }

  switch (enemy_action) {
  case ATTACK: {
    const int frame_width = 38;
    const int frame_height = 32;
    const int frames = 9;
    float fps = 10.0f;
    float elapsed = GetTime() - enemy_attack_start_time;
    int frame = (int)(elapsed * fps);
    Rectangle src = {frame * frame_width, 0, frame_width, frame_height};
    float dest_w = frame_width * scale;
    float dest_h = frame_height * scale;
    float ground_y = board_origin_y - 10;
    float center_x = GetScreenWidth() - 100;
    Rectangle dest = {center_x - dest_w / 2.0f, ground_y - dest_h, dest_w,
                      dest_h};
    DrawTexturePro(mushroom_attack, src, dest, (Vector2){0, 0}, 0, WHITE);
    Rectangle enemy_health = {dest.x + (dest.width - 80) / 2.0f,
                              dest.y + dest.height + 8, 80, 6};
    draw_health_bar(enemy_health, enemy.health_points, enemy.max_health_points);
  } break;
  case IDLE: {
    const int frame_width = 80;
    const int frame_height = 64;
    const int frames = 7;
    int frame = ((int)(GetTime() * 8.0f)) % frames;
    Rectangle src = {frame * frame_width, 0, frame_width, frame_height};
    float dest_w = frame_width * scale;
    float dest_h = frame_height * scale;
    float ground_y = board_origin_y - 10;
    float center_x = GetScreenWidth() - 100;
    Rectangle dest = {center_x - dest_w / 2.0f, ground_y - dest_h, dest_w,
                      dest_h};
    DrawTexturePro(mushroom_idle, src, dest, (Vector2){0, 0}, 0, WHITE);
    Rectangle enemy_health = {dest.x + (dest.width - 80) / 2.0f,
                              dest.y + dest.height + 8, 80, 6};
    draw_health_bar(enemy_health, enemy.health_points, enemy.max_health_points);
  } break;
  case HURT: {
    const int frame_width = 32;
    const int frame_height = 32;
    const int frames = 5;
    const float fps = 15.0f;
    float elapsed = GetTime() - enemy_damage_start_time;
    int frame = (int)(elapsed * fps);
    Rectangle src = {frame * frame_width, 0, frame_width, frame_height};
    float dest_w = frame_width * scale;
    float dest_h = frame_height * scale;
    float ground_y = board_origin_y - 10;
    float center_x = GetScreenWidth() - 100;
    Rectangle dest = {center_x - dest_w / 2.0f, ground_y - dest_h, dest_w,
                      dest_h};
    DrawTexturePro(mushroom_hit, src, dest, (Vector2){0, 0}, 0, WHITE);
    Rectangle enemy_health = {dest.x + (dest.width - 80) / 2.0f,
                              dest.y + dest.height + 8, 80, 6};
    draw_health_bar(enemy_health, enemy.health_points, enemy.max_health_points);
  } break;
  }

  // (MAYBE) player text should appear at max text size with low opacity and
  // become more opaque as time goes on
  // (TODO) maybe start with the text transparent then make it more opaque
  if (phase == TURN_TRANSITION_TO_ENEMY || phase == TURN_TRANSITION_TO_PLAYER) {
    const char *text =
        (phase == TURN_TRANSITION_TO_PLAYER) ? "Player Turn" : "Enemy Turn";
    double elapsed = GetTime() - turn_transition_start_time;
    float t = elapsed / TURN_TRANSITION_DURATION;
    if (t >= 1.0f) {
      t = 1.0f;
    }

    float ease_out = 1.0f - (1.0f - t) * (1.0f - t);
    uint8_t start_size = 50;
    uint8_t end_size = 30;
    const int font_size = start_size + ((end_size - start_size) * ease_out);
    const int text_width = MeasureText(text, font_size);
    const int x = (GetScreenWidth() / 2.0f) - (text_width / 2.0f);
    const int y = (GetScreenHeight() / 3.0f);

    DrawText(text, x, y, font_size, WHITE);
  }

  EndDrawing();
}

int main(int argc, char *argv[]) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Wordgame");
  InitAudioDevice();
  SetTargetFPS(60);

  punch_sound = LoadSound("assets/energetic_punch.wav");
  keystroke_sound = LoadSound("assets/Keystroke14.wav");
  backspace_sound = LoadSound("assets/Backspace1.wav");
  upbeat_music = LoadMusicStream("assets/upbeat_bg.wav");

  mushroom_idle = LoadTexture("assets/mushroom/Mushroom-Idle.png");
  mushroom_hit = LoadTexture("assets/mushroom/mushroom-hit.png");
  mushroom_attack = LoadTexture("assets/mushroom/mushroom-attack1.png");

  knight_idle = LoadTexture("assets/knight/IDLE.png");
  knight_attack = LoadTexture("assets/knight/attack-1.png");
  knight_hurt = LoadTexture("assets/knight/hurt1.png");

  background = LoadTexture("assets/background.png");

  load_letter_textures();
  tile_texture = LoadTexture("assets/tile.png");

  load_words();
  word_pointers = malloc(total_words * sizeof(char *));
  current_word = words;
  for (int i = 0; i < total_words; i++) {
    word_pointers[i] = current_word;
    current_word += strlen(current_word) + 1;
  }

  PlayMusicStream(upbeat_music);

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
  UnloadSound(punch_sound);
  UnloadMusicStream(upbeat_music);
  CloseAudioDevice();
  CloseWindow(); // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
