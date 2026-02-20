// #include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
  A_WORDS,
  B_WORDS,
  C_WORDS,
  D_WORDS,
  E_WORDS,
  F_WORDS,
  G_WORDS,
  H_WORDS,
  I_WORDS,
  J_WORDS,
  K_WORDS,
  L_WORDS,
  M_WORDS,
  N_WORDS,
  O_WORDS,
  P_WORDS,
  Q_WORDS,
  R_WORDS,
  S_WORDS,
  T_WORDS,
  U_WORDS,
  V_WORDS,
  W_WORDS,
  X_WORDS,
  Y_WORDS,
  Z_WORDS,
  WORD_TYPE_COUNT
} WordType;

char *words;
int file_size;
int total_words = 0;
int word_score = 0;

char letter_bag[100] = {0};
uint8_t remaining_letters = 0;
char hand[15] = {0};

void init_letter_bag() {
  int i = 0;
  // a: 8
  for (int j = 0; j < 8; j++)
    letter_bag[i++] = 'a';

  // b: 2
  for (int j = 0; j < 2; j++)
    letter_bag[i++] = 'b';

  // c: 4
  for (int j = 0; j < 4; j++)
    letter_bag[i++] = 'c';

  // d: 4
  for (int j = 0; j < 4; j++)
    letter_bag[i++] = 'd';

  // e: 11
  for (int j = 0; j < 11; j++)
    letter_bag[i++] = 'e';

  // f: 1
  letter_bag[i++] = 'f';

  // g: 3
  for (int j = 0; j < 3; j++)
    letter_bag[i++] = 'g';

  // h: 2
  for (int j = 0; j < 2; j++)
    letter_bag[i++] = 'h';

  // i: 8
  for (int j = 0; j < 8; j++)
    letter_bag[i++] = 'i';

  // j: 1
  letter_bag[i++] = 'j';

  // k: 1
  letter_bag[i++] = 'k';

  // l: 5
  for (int j = 0; j < 5; j++)
    letter_bag[i++] = 'l';

  // m: 3
  for (int j = 0; j < 3; j++)
    letter_bag[i++] = 'm';

  // n: 7
  for (int j = 0; j < 7; j++)
    letter_bag[i++] = 'n';

  // o: 6
  for (int j = 0; j < 6; j++)
    letter_bag[i++] = 'o';

  // p: 3
  for (int j = 0; j < 3; j++)
    letter_bag[i++] = 'p';

  // q: 1
  letter_bag[i++] = 'q';

  // r: 7
  for (int j = 0; j < 7; j++)
    letter_bag[i++] = 'r';

  // s: 8
  for (int j = 0; j < 8; j++)
    letter_bag[i++] = 's';

  // t: 5
  for (int j = 0; j < 5; j++)
    letter_bag[i++] = 't';

  // u: 4
  for (int j = 0; j < 4; j++)
    letter_bag[i++] = 'u';

  // v: 1
  letter_bag[i++] = 'v';

  // w: 1
  letter_bag[i++] = 'w';

  // x: 1
  letter_bag[i++] = 'x';

  // y: 2
  for (int j = 0; j < 2; j++)
    letter_bag[i++] = 'y';

  // z: 1
  letter_bag[i++] = 'z';
}

void shuffle_bag() {
  for (int i = 99; i > 0; i--) {
    int j = rand() % (i + 1);
    char temp = letter_bag[i];
    letter_bag[i] = letter_bag[j];
    letter_bag[j] = temp;
  }
}

void draw_hand() {
  for (int i = 0; i < 16; i++) {
    if (letter_bag[i] == -1) {
      continue;
    }
    hand[i] = letter_bag[i];
    letter_bag[i] = -1;
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
  file_size = ftell(file);
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
  load_words();

  srand(time(NULL));
  init_letter_bag();
  shuffle_bag();
  draw_hand();

  for (int i = 0; i < 16; i++) {
    if (i % 4 == 0) {
      printf("\n");
    }
    printf("%c ", hand[i]);
  }
  printf("\n");

  printf("Type word: \n");
  char user_input[12];
  scanf("%11s", user_input);

  char *p = words;

  bool match_found = false;

  for (int i = 0; i < total_words; i++) {
    if (strcmp(p, user_input) == 0) {
      int damage = strlen(p);
      printf("You deal %i damage\n", damage);
      match_found = true;
      break;
    }
    p += strlen(p) + 1;
  }

  if (match_found) {
    printf("Match found\n");
  } else {
    printf("Match not found\n");
  }

  return 0;
}
