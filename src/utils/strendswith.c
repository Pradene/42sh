#include <string.h>
#include <stdbool.h>

bool strendswith(const char *s, const char *suffix) {
  if (!s || !suffix) {
    return false;
  }

  size_t s_length = strlen(s);
  size_t suffix_length = strlen(suffix);

  if (suffix_length > s_length) {
    return false;
  }

  return strcmp(s + s_length - suffix_length, suffix) == 0;
}