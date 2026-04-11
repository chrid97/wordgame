#include <stdio.h>
#include <stdlib.h>

typedef enum {
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  WORD_TYPE_COUNT
} WordType;

int main(void) {
  FILE *file = fopen("./all_words.txt", "r");
  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *words;
  words = malloc(file_size + 1);
  int elements_read = fread(words, 1, file_size, file);

  int letter_frequency[26] = {0};
  for (int i = 0; i < elements_read; i++) {
    char current_character = words[i];
    if (current_character == '\n') {
      continue;
    }

    letter_frequency[current_character - 97]++;
  }

  for (int i = 0; i < 26; i++) {
    printf("%c: %i\n", 97 + i, letter_frequency[i]);
  }

  fclose(file);
  return 0;
}
