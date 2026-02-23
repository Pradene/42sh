#include <stdlib.h>
#include <string.h>

static size_t count_words(const char *s, char c) {
  size_t count = 0;
  size_t i = 1;

  while (s[i - 1]) {
    if ((s[i] == c || s[i] == 0) && s[i - 1] != c) {
      count++;
    }
    i++;
  }
  return (count);
}

char **split_at(const char *s, char c) {
  char **strings;
  size_t length = 0;
  size_t i = 0;
  size_t j = 0;

  strings = malloc((count_words(s, c) + 1) * sizeof(char *));
  if (!strings) {
    return NULL;
  }
  while (j < strlen(s) + 1) {
    if (s[j] != c && s[j] != 0) {
      length++;
    }
    if (length && (s[j] == c || s[j] == 0)) {
      strings[i] = strndup(&s[j] - length, length);
      if (!strings[i]) {
        for (size_t k = 0; k < i; ++k) {
          free(strings[k]);
        }
        free(strings);
        return NULL;
      }
      length = 0;
      i++;
    }
    j++;
  }
  strings[i] = 0;
  return (strings);
}
