#include "globals.h"
#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdalign.h>
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
// (TODO) make it so you can upgrade number of racks
#define MAX_RACK_LENGTH 6
#define MAX_ENCOUNTER_ENEMIES 3

#define TURN_TRANSITION_DURATION 1.0f
#define FLAG(N) (1u << N)

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
  POISONS = FLAG(0),
  SELF_DESTRUCT = FLAG(1), // Explodes and dies instead of attacking
  LOCK_TILE = FLAG(2),     // Locks player tile
};
// typedef enum { PLAYER, MUSHROOM, LOCK_AND_KEY, TILE } EntityType;
typedef struct {
  // EntityType type;
  Vector2 position;
  int width;
  int height;
  char tile_value;

  int max_health_points;
  int health_points;
  int block;

  uint8_t min_damage;
  uint8_t max_damage;

  EntityState state;
  uint8_t abilities;
  // tile modifiers
  // uint8_t modifiers;

  bool poisoned;
  Animation animation;
  Color tint;

  // How long has the entity been in this state
  float state_time;
  bool action_applied;
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

typedef struct {
  uint8_t enemy_count;
  Entity enemies[MAX_ENCOUNTER_ENEMIES];
} Encounter;

//----------------------------------------------------------------------------------
// State
//----------------------------------------------------------------------------------
char *words;
int total_words = 0;
LetterBag letter_bag = {0};

// Rack
uint8_t rack[MAX_RACK_LENGTH] = {0}; // index to tile in letter bag
uint8_t rack_length = 0;

char selected_word[MAX_RACK_LENGTH + 1] = {0}; // actual word

uint8_t board[BOARD_COUNT] = {0};              // index to tiles in bag
uint8_t selected_cells[MAX_RACK_LENGTH] = {0}; // Selected board index

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

float phase_time = 0.0f;
Phase phase = TURN_TRANSITION_TO_PLAYER;

int player_pending_damage = 0;
Entity player = {.state = IDLE,
                 .action_applied = false,
                 .health_points = 60,
                 .max_health_points = 60,
                 .tint = WHITE};

// (TODO) Clearer name for this?
int encounter_index = 0;
Encounter encounter_table[] = {
    {1,
     {{
         .health_points = 12,
         .max_health_points = 12,
         .abilities = POISONS,
         .max_damage = 10,
         .min_damage = 7,
         .block = 10,
         .tint = WHITE,
     }}},
    {1,
     {{
         .health_points = 24,
         .max_health_points = 24,
         .abilities = LOCK_TILE,
         .max_damage = 10,
         .min_damage = 7,
         .tint = YELLOW,
     }}},
    {2,
     {
         {
             .health_points = 60,
             .max_health_points = 60,
             .abilities = LOCK_TILE,
             .max_damage = 10,
             .min_damage = 7,
             .tint = YELLOW,
         },
         {
             .health_points = 60,
             .max_health_points = 60,
             .abilities = LOCK_TILE,
             .max_damage = 10,
             .min_damage = 7,
             .tint = GREEN,
         },
     }},
};

// UI
const uint8_t padding = 3;
const uint8_t pitch = TILE_SIZE + padding;
const int board_width = BOARD_ROW * (TILE_SIZE + padding);
const int board_height = BOARD_COL * (TILE_SIZE + padding);
const float board_origin_x = (VIRTUAL_WIDTH / 2.0) - board_width / 2.0;
const float board_origin_y = VIRTUAL_HEIGHT - board_height;
const Rectangle board_rect = {board_origin_x, board_origin_y, board_width,
                              board_height};
const float rack_origin_x = 200;
const float rack_origin_y = board_height;
Vector2 submit_button_pos = {board_origin_x + board_width + 25,
                             VIRTUAL_HEIGHT - 25};
float submit_button_radius = 20;

RenderTexture2D target;
//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------

int calculate_rack_damage(void) {
  int damage = 0;

  for (int i = 0; i < rack_length; i++) {
    char tile_value = letter_bag.tiles[rack[i]].tile_value;
    damage += letter_scores[tile_value - 'a'];
  }

  return damage;
}

void entity_set_state(Entity *entity, EntityState state) {
  if (entity->state == state) {
    return;
  }

  entity->state = state;
  entity->state_time = 0.0f;
  entity->action_applied = false;
}

void entity_take_damage(Entity *entity, int damage) {
  if (entity->health_points <= 0) {
    return;
  }

  int unblocked_damage = 0;
  // If Entity has block deal damage to block
  if (entity->block > 0) {
    int new_block = entity->block - damage;
    entity->block -= damage;
    printf("%i\n", new_block);
    if (new_block < 0) {
      printf("new block%i\n", new_block);
      unblocked_damage = abs(new_block);
    }
  } else {
    unblocked_damage = damage;
  }

  // printf("%i\n", unblocked_damage);
  entity->health_points -= unblocked_damage;
  if (entity->health_points <= 0) {
    entity->health_points = 0;
    entity_set_state(entity, DEATH);
  } else {
    entity_set_state(entity, HURT);
  }
}

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
  int i = 0;
  for (int letter = 0; letter < 26; letter++) {
    for (int j = 0; j < letter_occurrence[letter]; j++) {
      letter_bag.tiles[i++] = (Entity){
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

  // --- Draw score in bottom-left ---
  int score = letter_scores[tile_value - 'a'];

  char score_text[4];
  snprintf(score_text, sizeof(score_text), "%d", score);

  int font_size = 10;

  int text_x = rect.x + 3;
  int text_y = rect.y + rect.height - font_size - 2;

  // Optional outline for readability
  DrawText(score_text, text_x - 1, text_y, font_size, BLACK);
  DrawText(score_text, text_x + 1, text_y, font_size, BLACK);
  DrawText(score_text, text_x, text_y - 1, font_size, BLACK);
  DrawText(score_text, text_x, text_y + 1, font_size, BLACK);

  DrawText(score_text, text_x, text_y, font_size, WHITE);
}

int entity_current_frame(Entity *e) {
  return (int)(e->state_time * e->animation.fps);
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

void draw_health_bar(Rectangle bar, int current_hp, int max_hp, int block) {
  // HP bar background
  DrawRectangleRec(bar, GRAY);
  DrawRectangleRoundedLinesEx(bar, 100, 4, 1, BLACK);

  Rectangle fill = bar;
  fill.width = ((float)current_hp / max_hp) * bar.width;
  DrawRectangleRec(fill, RED);

  // HP text
  char hp_text[32];
  snprintf(hp_text, sizeof(hp_text), "%d/%d", current_hp, max_hp);

  const int font_size = 18;
  const int text_width = MeasureText(hp_text, font_size);

  int text_x = bar.x + (bar.width - text_width) / 2;
  int text_y = bar.y - (float)font_size / 3;

  DrawText(hp_text, text_x - 2, text_y, font_size, BLACK);
  DrawText(hp_text, text_x + 2, text_y, font_size, BLACK);
  DrawText(hp_text, text_x, text_y - 2, font_size, BLACK);
  DrawText(hp_text, text_x, text_y + 2, font_size, BLACK);

  DrawText(hp_text, text_x - 1, text_y - 1, font_size, BLACK);
  DrawText(hp_text, text_x + 1, text_y - 1, font_size, BLACK);
  DrawText(hp_text, text_x - 1, text_y + 1, font_size, BLACK);
  DrawText(hp_text, text_x + 1, text_y + 1, font_size, BLACK);

  DrawText(hp_text, text_x, text_y, font_size, WHITE);

  // Block indicator
  if (block > 0) {
    const int icon_size = 16;
    const int gap = 6;

    Rectangle shield = {bar.x - icon_size - gap,
                        bar.y + (bar.height - icon_size) / 2.0f, icon_size,
                        icon_size};

    // simple square/shield icon
    DrawRectangleRec(shield, SKYBLUE);
    DrawRectangleLinesEx(shield, 2, BLUE);

    // block number
    char block_text[16];
    snprintf(block_text, sizeof(block_text), "%d", block);

    int block_font_size = 12;
    int block_text_width = MeasureText(block_text, block_font_size);
    int block_text_x = shield.x + (shield.width - block_text_width) / 2;
    int block_text_y = shield.y + (shield.height - block_font_size) / 2;

    DrawText(block_text, block_text_x - 1, block_text_y, block_font_size,
             BLACK);
    DrawText(block_text, block_text_x + 1, block_text_y, block_font_size,
             BLACK);
    DrawText(block_text, block_text_x, block_text_y - 1, block_font_size,
             BLACK);
    DrawText(block_text, block_text_x, block_text_y + 1, block_font_size,
             BLACK);

    DrawText(block_text, block_text_x, block_text_y, block_font_size, WHITE);
  }
}

float get_render_scale(void) {
  float scale_x = (float)GetScreenWidth() / VIRTUAL_WIDTH;
  float scale_y = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
  return fminf(scale_x, scale_y);
}

Rectangle get_render_dest_rect(void) {
  float scale = get_render_scale();
  float width = VIRTUAL_WIDTH * scale;
  float height = VIRTUAL_HEIGHT * scale;

  return (Rectangle){
      (GetScreenWidth() - width) * 0.5f,
      (GetScreenHeight() - height) * 0.5f,
      width,
      height,
  };
}

Vector2 get_virtual_mouse(RenderTexture2D target) {
  float scale = fminf((float)GetScreenWidth() / VIRTUAL_WIDTH,
                      (float)GetScreenHeight() / VIRTUAL_HEIGHT);

  float dest_width = VIRTUAL_WIDTH * scale;
  float dest_height = VIRTUAL_HEIGHT * scale;

  float dest_x = (GetScreenWidth() - dest_width) * 0.5f;
  float dest_y = (GetScreenHeight() - dest_height) * 0.5f;

  Vector2 mouse = GetMousePosition();

  Vector2 virtual_mouse = {
      (mouse.x - dest_x) / scale,
      (mouse.y - dest_y) / scale,
  };

  return virtual_mouse;
}

void update_draw() {
  //----------------------------------------------------------------------------------
  // Update
  //----------------------------------------------------------------------------------
  UpdateMusicStream(upbeat_music);
  SetMusicVolume(upbeat_music, 0.05f);
  SetSoundVolume(punch_sound, 0.1f);

  float dt = GetFrameTime();
  player.state_time += dt;
  phase_time += dt;

  Encounter *current_encounter = &encounter_table[encounter_index];

  for (int i = 0; i < current_encounter->enemy_count; i++) {
    current_encounter->enemies[i].state_time += dt;
  }

  switch (phase) {
  case TURN_TRANSITION_TO_ENEMY:
  case TURN_TRANSITION_TO_PLAYER: {
    if (phase_time >= TURN_TRANSITION_DURATION) {
      phase = phase == TURN_TRANSITION_TO_PLAYER ? PLAYER_TURN : ENEMY_TURN;
      phase_time = 0.0f;
    }
  } break;
  case PLAYER_TURN: {
    // submit words
    // (TODO) I'll probably have to change it when action_applied encompassses
    // more than teh player attacking
    if (player.action_applied) {
      bool enemies_alive = true;
      for (int i = 0; i < current_encounter->enemy_count; i++) {
        if (current_encounter->enemies[i].health_points <= 0) {
          enemies_alive = false;
        }
      }

      if (enemies_alive) {
        phase = PLAYER_END_STEP;
        phase_time = 0.0f;
      } else {
        phase = TURN_TRANSITION_TO_PLAYER;
        phase_time = 0.0f;
      }
    }
  } break;
  case PLAYER_END_STEP: {
    phase = TURN_TRANSITION_TO_ENEMY;
    phase_time = 0.0f;
  } break;
  case ENEMY_TURN: {
    bool enemy_attacks_finished = true;
    // TODO enemies should attack from left to right one after another
    for (int i = 0; i < current_encounter->enemy_count; i++) {
      Entity *enemy = &current_encounter->enemies[i];
      if (enemy->state != ATTACK) {
        entity_set_state(enemy, ATTACK);
      }

      if (enemy->health_points > 0 && !enemy->action_applied) {
        enemy_attacks_finished = false;
      }
    }

    if (enemy_attacks_finished) {
      phase = ENEMY_END_STEP;
      phase_time = 0.0f;
    }
  } break;
  case ENEMY_END_STEP: {
    if (phase_time >= TURN_TRANSITION_DURATION) {
      phase = TURN_TRANSITION_TO_PLAYER;
      phase_time = 0.0f;
    }
  } break;
  }

  uint8_t targeted_enemy = 0;
  switch (player.state) {
  case IDLE: {
    player.animation = knight_idle;
  } break;
  case DEATH: {
    player.animation = knight_death;
    // (TODO) GAME OVER
  } break;
  case ATTACK: {
    player.animation = knight_attack;
    int frame = entity_current_frame(&player);

    Entity *enemy = &current_encounter->enemies[targeted_enemy];
    if (!player.action_applied && frame == 3) {
      PlaySound(punch_sound);
      entity_take_damage(enemy, player_pending_damage);
      player.action_applied = true;
    }

    if (frame >= 6) {
      entity_set_state(&player, IDLE);
    }
  } break;
  case HURT: {
    player.animation = knight_hurt;

    PlaySound(punch_sound);
    if (entity_current_frame(&player) >= 4) {
      entity_set_state(&player, IDLE);
    }
  } break;
  }

  for (int i = 0; i < current_encounter->enemy_count; i++) {
    Entity *enemy = &current_encounter->enemies[i];
    switch (enemy->state) {
    case ATTACK: {
      enemy->animation = mushroom_attack;
      uint8_t damage = enemy->min_damage +
                       (rand() % (enemy->max_damage - enemy->min_damage));

      if (!enemy->action_applied && entity_current_frame(enemy) == 6) {
        entity_take_damage(&player, damage);
        enemy->action_applied = true;
      }
      if (entity_current_frame(enemy) >= enemy->animation.frame_count) {
        entity_set_state(enemy, IDLE);
      }
    } break;
    case IDLE: {
      enemy->animation = mushroom_idle;
    } break;
    case HURT: {
      enemy->animation = mushroom_hurt;

      if (entity_current_frame(enemy) >= enemy->animation.frame_count) {
        entity_set_state(enemy, IDLE);
      }
    } break;
    case DEATH: {
      enemy->animation = mushroom_death;

      if (entity_current_frame(enemy) >= 15) {
        encounter_index++;
      }
    } break;
    }
  }

  if (phase != PLAYER_TURN && phase != TURN_TRANSITION_TO_PLAYER) {
    goto draw;
  }

  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    goto draw;
  }

  // Move tiles to rack
  Vector2 mouse_pos = get_virtual_mouse(target);
  if (rack_length < MAX_RACK_LENGTH &&
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
      rack[rack_length] = tile_idx;
      is_selected[tile_idx] = true;

      selected_word[rack_length] = letter_bag.tiles[tile_idx].tile_value;
      selected_word[rack_length + 1] = '\0';
      selected_cells[rack_length] = cell;

      rack_length++;

      valid_word =
          rack_length > 0 &&
          binary_search_word(word_pointers, selected_word, total_words);
      PlaySound(keystroke_sound);
    } else {
      printf("Tile already selected\n");
    }
    player_pending_damage = calculate_rack_damage();

    printf("Current word: %s\n", selected_word);
  }

  // Deselect tiles
  if (CheckCollisionPointRec(mouse_pos,
                             (Rectangle){rack_origin_x, rack_origin_y,
                                         (TILE_SIZE + padding) * rack_length,
                                         TILE_SIZE})) {
    const float relative_x = mouse_pos.x - rack_origin_x;
    const int cell = relative_x / (TILE_SIZE + padding);
    printf("[click] Deselect tile %c\n",
           letter_bag.tiles[rack[cell]].tile_value);

    // Set selected tiles to false
    for (int i = cell; i < rack_length; i++) {
      is_selected[rack[i]] = false;
    }
    rack_length = cell;
    selected_word[rack_length] = '\0';

    valid_word = rack_length > 0 &&
                 binary_search_word(word_pointers, selected_word, total_words);
    printf("Current word: %s\n", selected_word);

    player_pending_damage = calculate_rack_damage();
    PlaySound(backspace_sound);
  }

  // Submit word / Attack
  if (valid_word && CheckCollisionPointCircle(mouse_pos, submit_button_pos,
                                              submit_button_radius)) {
    printf("Submitted word %s!\n", selected_word);
    for (int i = 0; i < rack_length; i++) {
      uint8_t cell = selected_cells[i];
      board[cell] = letter_bag.remaining;
      is_selected[rack[i]] = false;
      letter_bag.remaining--;
      printf("Refilled bag with tile %c\n",
             letter_bag.tiles[letter_bag.remaining].tile_value);
    };

    rack_length = 0;
    selected_word[0] = '\0';
    valid_word = false;

    entity_set_state(&player, ATTACK);
  }

  // (TODO) Right click should clear rack
  // (MAYBE) right click on selected tile to insert before or after

  //----------------------------------------------------------------------------------
  // Draw
  //----------------------------------------------------------------------------------
draw:
  BeginTextureMode(target);
  ClearBackground(BLACK);

  Rectangle src = {0, 0, background.width, background.height};
  Rectangle background_dest = {0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT};
  DrawTexturePro(background, src, background_dest, (Vector2){0, 0}, 0, WHITE);

  // (TODO) draw shadows under sprites

  // Draw Player
  const float scale = 2.5f;
  float ground_y = board_origin_y - 10;
  float dest_w = player.animation.frame_width * scale;
  float dest_h = player.animation.frame_height * scale;
  Rectangle dest = {
      100 - (dest_w / 2.0f),
      ground_y - dest_h,
      dest_w,
      dest_h,
  };
  draw_animation(player.animation, player.state_time, dest, player.tint);

  Rectangle player_health = {
      dest.x + (dest.width - 80) / 2.0f,
      dest.y + dest.height + 6,
      80,
      6,
  };
  draw_health_bar(player_health, player.health_points, player.max_health_points,
                  player.block);

  // Draw Enemies
  for (int i = 0; i < current_encounter->enemy_count; i++) {
    Entity *enemy = &current_encounter->enemies[i];

    float enemy_anchor_x = VIRTUAL_WIDTH - 100;
    float enemy_dest_w = enemy->animation.frame_width * scale;
    float enemy_dest_h = enemy->animation.frame_height * scale;

    Rectangle enemy_dest = {
        enemy_anchor_x - enemy_dest_w / 2.0f - i * 100,
        ground_y - enemy_dest_h,
        enemy_dest_w,
        enemy_dest_h,
    };
    draw_animation(enemy->animation, enemy->state_time, enemy_dest,
                   enemy->tint);

    Rectangle enemy_health = {
        enemy_dest.x,
        enemy_dest.y + enemy_dest.height + 10,
        80,
        6,
    };
    draw_health_bar(enemy_health, enemy->health_points,
                    enemy->max_health_points, enemy->block);
  }

  // draw board
  for (int i = 0; i < BOARD_COUNT; i++) {
    const uint8_t row = i / BOARD_ROW;
    const uint8_t col = i % BOARD_COL;
    const int x = board_origin_x + (col * (TILE_SIZE + padding));
    const int y = board_origin_y + (row * (TILE_SIZE + padding));
    draw_tile(board[i], (Rectangle){x, y, TILE_SIZE, TILE_SIZE},
              is_selected[board[i]]);
  }

  // Draw pending damage
  if (rack_length > 0) {
    char damage_text[32];
    snprintf(damage_text, sizeof(damage_text), "Damage: %d",
             player_pending_damage);

    int font_size = 20;
    int text_x = rack_origin_x;
    int text_y = rack_origin_y - 28;

    DrawText(damage_text, text_x - 1, text_y, font_size, BLACK);
    DrawText(damage_text, text_x + 1, text_y, font_size, BLACK);
    DrawText(damage_text, text_x, text_y - 1, font_size, BLACK);
    DrawText(damage_text, text_x, text_y + 1, font_size, BLACK);

    DrawText(damage_text, text_x, text_y, font_size, valid_word ? RED : GRAY);
  }

  // Draw Rack
  for (int i = 0; i < MAX_RACK_LENGTH; i++) {
    const int x = rack_origin_x + (i * (TILE_SIZE + padding));
    DrawRectangle(x, rack_origin_y, TILE_SIZE, TILE_SIZE, GRAY);
  }

  // Draw Chosen Tiles on Rack
  for (int i = 0; i < rack_length; i++) {
    const int x = rack_origin_x + (i * (TILE_SIZE + padding));
    draw_tile(rack[i], (Rectangle){x, rack_origin_y, TILE_SIZE, TILE_SIZE},
              false);
  }

  // Word Submit Button
  DrawCircleV(submit_button_pos, submit_button_radius,
              valid_word ? GREEN : GRAY);
  DrawText("Submit", submit_button_pos.x - 15, submit_button_pos.y - 5, 10,
           BLACK);
  DrawCircleLinesV(submit_button_pos, submit_button_radius, BLACK);

  // Draw turn transitions
  // (MAYBE) player text should appear at max text size with low opacity and
  // become more opaque as time goes on
  // (TODO) maybe start with the text transparent then make it more opaque
  if (phase == TURN_TRANSITION_TO_ENEMY || phase == TURN_TRANSITION_TO_PLAYER) {
    const char *text =
        (phase == TURN_TRANSITION_TO_PLAYER) ? "Player Turn" : "Enemy Turn";
    float t = phase_time / TURN_TRANSITION_DURATION;
    if (t >= 1.0f) {
      t = 1.0f;
    }

    float ease_out = 1.0f - (1.0f - t) * (1.0f - t);
    uint8_t start_size = 50;
    uint8_t end_size = 30;
    const int font_size = start_size + ((end_size - start_size) * ease_out);
    const int text_width = MeasureText(text, font_size);
    const int x = (VIRTUAL_WIDTH / 2.0f) - (text_width / 2.0f);
    const int y = (VIRTUAL_HEIGHT / 3.0f);

    DrawText(text, x, y, font_size, WHITE);
  }

  EndTextureMode();

  BeginDrawing();
  ClearBackground(BLACK);

  Rectangle target_dest = get_render_dest_rect();
  Rectangle target_src = {0, 0, (float)target.texture.width,
                          -(float)target.texture.height};

  DrawTexturePro(target.texture, target_src, target_dest, (Vector2){0, 0}, 0.0f,
                 WHITE);

  EndDrawing();
}

int main(int argc, char *argv[]) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Wordgame");
  target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
  InitAudioDevice();
  SetTargetFPS(60);

  punch_sound = LoadSound("assets/energetic_punch.wav");
  keystroke_sound = LoadSound("assets/Keystroke14.wav");
  backspace_sound = LoadSound("assets/Backspace1.wav");
  upbeat_music = LoadMusicStream("assets/upbeat_bg.wav");

  mushroom_idle = (Animation){
      LoadTexture("assets/mushroom/mushroom-idle.png"), 30, 33, 7, 8.0f, true};
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
