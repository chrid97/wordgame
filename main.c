// #include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

void load_words() {
  char filename[32] = "words.txt";
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "ERROR: Failed to load %s\n", filename);
    exit(1);
  }

  // handle fseek error
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // do i have to allocate 1 more byte for null terminator?
  words = malloc(file_size);
  int elements_read = fread(words, 1, file_size, file);
  for (int i = 0; i < elements_read; i++) {
    if (words[i] == '\n') {
      words[i] = '\0';
    }
  }

  fclose(file);
}

int main(int argc, char *argv[]) {
  load_words();
  printf("Type word: \n");

  char user_input[12];
  scanf("%s", user_input);

  char *p = words;

  uint8_t user_input_pointer = 0;
  uint8_t current_word_length = 0;
  for (int i = 0; i < file_size; i++) {
    if (words[i] != '\0') {
      current_word_length++;
    }
  }

  return 0;
}
