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

typedef struct {
  Texture2D texture;
  int frame_width;
  int frame_height;
  int frame_count;
  float fps;
  bool looping;
} Animation;

typedef enum { IDLE, ATTACK, HURT, DEATH } EntityState;
enum MonsterAbilityFlags {
  POISONS,
  SELF_DESTRUCT, // Explodes instead of attacking
  LOCK_TILE,     // Locks player tile
};
// typedef enum { PLAYER, MUSHROOM, LOCK_AND_KEY, TILE } EntityType;
typedef struct {
  // EntityType type;
  Vector2 position;
  int width;
  int height;
  char tile_value;

  uint8_t max_health_points;
  uint8_t health_points;

  EntityState state;
  // tile modifiers
  // uint8_t modifiers;

  bool poisoned;
  Animation animation;
  Color tint;
} Entity;

typedef struct {
  Entity tiles[LETTER_BAG_SIZE];
  uint8_t remaining;
  uint8_t size; // total number of tiles
} LetterBag;

typedef enum {
  TURN_TRANSITION_TO_PLAYER,
  PLAYER_TURN,
  PLAYER_END_STEP,
  TURN_TRANSITION_TO_ENEMY,
  ENEMY_TURN,
  ENEMY_END_STEP,
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

Animation mushroom_idle;
Animation mushroom_hurt;
Animation mushroom_attack;
Animation mushroom_death;

Animation knight_idle;
Animation knight_attack;
Animation knight_hurt;
Animation knight_death;

Sound punch_sound;
Sound keystroke_sound;
Sound backspace_sound;

Music upbeat_music;

EntityState enemy_action = IDLE;
EntityState player_action = IDLE;

Phase phase = TURN_TRANSITION_TO_PLAYER;

int player_pending_damage = 0;
float player_attack_start_time = 0.0f;
float player_damage_start_time = -1.0f;
float player_death_start_time = -1.0f;
bool player_hit_applied = false;

float enemy_attack_start_time = 0.0f;
float enemy_damage_start_time = 0.0f;
float enemy_death_start_time = 0.0f;
bool enemy_hit_applied = false;

float turn_transition_start_time = 1.0f;
Entity player = {.health_points = 60, .max_health_points = 60, .tint = WHITE};
Entity enemy = {.health_points = 12, .max_health_points = 12};
int current_enemy = 0;

Entity enemies[30] = {};

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
          // .type = TILE,
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
  if (letter_bag.tiles[tile_idx].poisoned) {
    tint = GREEN;
  }
  DrawTexture(tile_texture, rect.x, rect.y, tint);

  char tile_value = letter_bag.tiles[tile_idx].tile_value;
  Texture2D letter_tex = letter_textures[tile_value - 'a'];

  float x = rect.x + (rect.width - letter_tex.width) / 2.0f;
  float y = rect.y + (rect.height - letter_tex.height) / 2.0f;
  DrawTexture(letter_tex, (int)x, (int)y, tint);
}

void draw_animation(Animation anim, float t, Rectangle dest, Color tint) {
  int frame = (int)(t * anim.fps);

  if (anim.looping) {
    frame %= anim.frame_count;
  } else if (frame >= anim.frame_count) {
    frame = anim.frame_count - 1;
  }

  Rectangle src = {frame * anim.frame_width, 0, anim.frame_width,
                   anim.frame_height};

  DrawTexturePro(anim.texture, src, dest, (Vector2){0, 0}, 0, tint);
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

void update_draw(void) {
  //----------------------------------------------------------------------------------
  // Update
  //----------------------------------------------------------------------------------
  UpdateMusicStream(upbeat_music);
  SetMusicVolume(upbeat_music, 0.05f);
  SetSoundVolume(punch_sound, 0.1f);

  player.tint = WHITE;

  if (enemy.health_points == 0 && enemy_action != DEATH) {
    enemy_action = DEATH;
    enemy_death_start_time = GetTime();
  }

  // if (player.health_points == 0 && player_action != DEATH) {
  //   player_action = DEATH;
  //   player_death_start_time = GetTime();
  // }

  switch (phase) {
  case TURN_TRANSITION_TO_ENEMY:
  case TURN_TRANSITION_TO_PLAYER: {
    if (turn_transition_start_time > 0) {
      double elapsed = GetTime() - turn_transition_start_time;
      if (elapsed >= TURN_TRANSITION_DURATION) {
        phase = phase == TURN_TRANSITION_TO_PLAYER ? PLAYER_TURN : ENEMY_TURN;
        turn_transition_start_time = 0.0f;
      }
    }
  } break;
  case PLAYER_TURN: {
    // submit words
  } break;
  case PLAYER_END_STEP: {
    // apply poison damage
    uint8_t poison_count = 0;
    for (int i = 0; i < BOARD_COUNT; i++) {
      if (letter_bag.tiles[board[i]].poisoned) {
        poison_count++;
      }
    }

    if (poison_count == 0) {
      phase = TURN_TRANSITION_TO_ENEMY;
      break;
    }

    player_action = HURT;
    player_damage_start_time = GetTime();
    // (TODO) tint not working
    player.tint = GREEN;
    player.health_points -= poison_count * 2;
    phase = TURN_TRANSITION_TO_ENEMY;
  } break;
  case ENEMY_TURN: {
    if (enemy_action != ATTACK) {
      enemy_action = ATTACK;
      enemy_attack_start_time = GetTime();
      enemy_hit_applied = false;
    }
  } break;
  case ENEMY_END_STEP: {
  } break;
  }

  switch (player_action) {
  case IDLE: {
  } break;
  case DEATH: {
    // (TODO) GAME OVER
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
      if (phase == PLAYER_END_STEP) {
        phase = TURN_TRANSITION_TO_ENEMY;
      }
      player_action = IDLE;
      // (TODO) Should maybe be player turn end
      // phase = TURN_TRANSITION_TO_ENEMY;
    }
  } break;
  case HURT: {
    PlaySound(punch_sound);
    double elapsed = GetTime() - player_damage_start_time;
    int frame = (int)(elapsed * 15);
    if (frame >= 4) {
      player_action = IDLE;
      player_damage_start_time = -1.0f;
    }
    // todo rpobably a better way to do this
    if (player.health_points == 0 && player_action != DEATH) {
      player_action = DEATH;
      player_death_start_time = GetTime();
    }
  } break;
  }

  switch (enemy_action) {
    // enemy attack
  case ATTACK: {
    const int frames = 9;
    float fps = 10.0f;
    float elapsed = GetTime() - enemy_attack_start_time;
    int frame = (int)(elapsed * fps);
    if (!enemy_hit_applied && frame == 6) {
      // (TODO) I should probably play the damage sound when the target is
      // hit? I could even pass a sound effect to it if i wanted it to be
      // different based on the players attack
      // PlaySound(punch_sound);
      player_action = HURT;
      enemy_hit_applied = true;
      player_damage_start_time = GetTime();
      int damage = (rand() % 5) + 4;
      if (damage >= player.health_points) {
        player.health_points = 0;
      } else {
        player.health_points -= damage;
      }

      if (rand() % 2 % 2 == 0) {
        const int board_pos = rand() % 16;
        // (THINKING) maybe this should be a property on the player?
        for (int i = 0; i < BOARD_COUNT; i++) {
          letter_bag.tiles[board[board_pos]].poisoned = true;
        }
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
      phase = PLAYER_END_STEP;
      turn_transition_start_time = GetTime();
    }
  } break;
  case DEATH: {
    current_enemy++;
    enemy = enemies[current_enemy];

  } break;
  }

  if (phase != PLAYER_TURN && phase != TURN_TRANSITION_TO_PLAYER) {
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

  // draw board
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

  // (TODO) draw shadows under sprites
  Animation player_anim = {0};
  float player_time = 0;
  switch (player_action) {
  case ATTACK: {
    player_anim = knight_attack;
    player_time = GetTime() - player_attack_start_time;
  } break;
  case IDLE: {
    player_anim = knight_idle;
    player_time = GetTime();
  } break;
  case HURT: {
    player_anim = knight_hurt;
    player_time = GetTime() - player_damage_start_time;
  } break;
  case DEATH: {
    player_anim = knight_death;
    player_time = GetTime() - player_death_start_time;
  } break;
  }

  const float scale = 2.5f;
  float ground_y = board_origin_y - 10;
  float dest_w = player_anim.frame_width * scale;
  float dest_h = player_anim.frame_height * scale;
  Rectangle dest = {100 - (dest_w / 2.0f), ground_y - dest_h, dest_w, dest_h};
  draw_animation(player_anim, player_time, dest, player.tint);

  Rectangle player_health = {dest.x + (dest.width - 80) / 2.0f,
                             dest.y + dest.height + 6, 80, 6};
  draw_health_bar(player_health, player.health_points,
                  player.max_health_points);

  Animation enemy_anim = {0};
  float enemy_time = 0;
  switch (enemy_action) {
  case ATTACK: {
    enemy_anim = mushroom_attack;
    enemy_time = GetTime() - enemy_attack_start_time;
  } break;
  case IDLE: {
    enemy_anim = mushroom_idle;
    enemy_time = GetTime();
  } break;
  case HURT: {
    enemy_anim = mushroom_hurt;
    enemy_time = GetTime() - enemy_damage_start_time;
  } break;
  case DEATH: {
    enemy_anim = mushroom_death;
    enemy_time = GetTime() - enemy_death_start_time;
  } break;
  }

  float enemy_anchor_x = GetScreenWidth() - 100;
  float enemy_dest_w = enemy_anim.frame_width * scale;
  float enemy_dest_h = enemy_anim.frame_height * scale;

  Rectangle enemy_dest = {enemy_anchor_x - enemy_dest_w / 2.0f,
                          ground_y - enemy_dest_h, enemy_dest_w, enemy_dest_h};

  draw_animation(enemy_anim, enemy_time, enemy_dest, WHITE);

  Rectangle enemy_health = {enemy_anchor_x - 40.0f,
                            enemy_dest.y + enemy_dest.height + 8, 80, 6};

  draw_health_bar(enemy_health, enemy.health_points, enemy.max_health_points);

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

  mushroom_idle = (Animation){
      LoadTexture("assets/mushroom/Mushroom-Idle.png"), 80, 64, 7, 8.0f, true};
  mushroom_attack =
      (Animation){LoadTexture("assets/mushroom/mushroom-attack1.png"),
                  38,
                  32,
                  9,
                  10.0f,
                  false};
  mushroom_hurt = (Animation){
      LoadTexture("assets/mushroom/mushroom-hit.png"), 32, 32, 5, 15.0f, false};
  mushroom_death =
      (Animation){LoadTexture("assets/mushroom/mushroom-death.png"),
                  53,
                  37,
                  15,
                  15.0f,
                  false};

  knight_idle =
      (Animation){LoadTexture("assets/knight/IDLE.png"), 32, 37, 7, 7.5, true};
  knight_attack = (Animation){
      LoadTexture("assets/knight/attack-1.png"), 60, 36, 6, 15.0f, false};
  knight_hurt = (Animation){
      LoadTexture("assets/knight/hurt1.png"), 32, 36, 4, 15.0f, false};
  knight_death = (Animation){
      LoadTexture("assets/knight/knight-death.png"), 46, 35, 12, 10.0f, false};

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

  enemies[0] = enemy;
  enemies[1] = (Entity){
      .health_points = 30,
      .max_health_points = 30,
  };
  enemies[2] = (Entity){.health_points = 60, .max_health_points = 60};
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
