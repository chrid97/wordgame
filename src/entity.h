#define FLAG(N) (1u << N)
typedef enum { IDLE, ATTACK, HURT, DEATH, BLOCK } EntityState;
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

  // int intent;
  // tile modifiers
  // uint8_t modifiers;

  bool poisoned;
  Animation animation;
  Color tint;

  // How long has the entity been in this state
  float state_time;
  bool action_applied;
} Entity;
