// #include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
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

char *words;
int file_size;
int total_words = 0;

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
  printf("Type word: \n");

  char user_input[12];
  scanf("%11s", user_input);

  char *p = words;

  bool match_found = false;

  for (int i = 0; i < total_words; i++) {
    if (strcmp(p, user_input) == 0) {
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
