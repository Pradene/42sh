#include <string.h>

bool strendswith(const char *s, const char *suffix) {
  if (!s || !suffix)
    return false;

  size_t s_len = strlen(s);
  size_t suffix_len = strlen(suffix);

  if (suffix_len > s_len)
    return false;

  return strcmp(s + s_len - suffix_len, suffix) == 0;
}