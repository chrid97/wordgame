#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char words[200000][12];

void load_words() {
  char filename[32] = "all_words.txt";
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "ERROR: Failed to load %s\n", filename);
    exit(1);
  }

  for (int i = 0; i < sizeof(file); i++) {
  }
}

int main(int argc, char *argv[]) {
  // InitWindow(960, 540, "Wordgame");
  // while (!WindowShouldClose()) {
  // }

  char input[12];
  printf("Type word: \n");
  scanf("%s", input);

  char word_match[12];
  bool word_found = false;
  // while (fgets(word_match, sizeof(word_match), file) != NULL) {
  //   word_match[strcspn(word_match, "\r\n")] = 0;
  //   if (strcmp(input, word_match) == 0) {
  //     word_found = true;
  //     break;
  //   }
  // }

  if (word_found) {
    printf("found match!\n");
  } else {
    printf("no match found :(\n");
  }

  return 0;
}
