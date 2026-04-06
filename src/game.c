#include "raylib.h"
#define MAX_ENTITIES 1024
#define SENTINEL_VALUE 0

typedef struct Entity {
  Vector2 pos;
} Entity;

// skip 0 and use it as an invalid index
struct Entities {
  Entity entities[MAX_ENTITIES];
  bool used[MAX_ENTITIES];
  int next_empty_slot; // Should be set to 1
};

void update() {}
void draw() {}
