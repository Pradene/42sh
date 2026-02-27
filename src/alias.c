#include "42sh.h"

#include <ctype.h>

char *expand_alias(const char *input, const Shell *shell) {
  StringBuffer sb = {0};
  size_t i = 0;
  bool command_pos = true; // start of input is a command position

  while (input[i]) {
    // Skip whitespace, preserving it
    if (input[i] == ' ' || input[i] == '\t') {
      sb_append_char(&sb, input[i++]);
      continue;
    }

    // These tokens put the next word back into command position
    if (input[i] == '|' || input[i] == ';' || input[i] == '(' || input[i] == '{' || input[i] == '\n') {
      sb_append_char(&sb, input[i++]);
      // handle ||
      if (input[i-1] == '|' && input[i] == '|') {
        sb_append_char(&sb, input[i++]);
      }
      command_pos = true;
      continue;
    }

    if (input[i] == '&') {
      sb_append_char(&sb, input[i++]);
      if (input[i] == '&') {
        sb_append_char(&sb, input[i++]);
      }
      command_pos = true;
      continue;
    }

    // If we're in a command position, try to match an alias
    if (command_pos && (isalnum((unsigned char)input[i]) || input[i] == '_')) {
      size_t word_start = i;
      while (input[i] && (isalnum((unsigned char)input[i]) || input[i] == '_' || input[i] == '-' || input[i] == '.')) {
        ++i;
      }
      size_t word_len = i - word_start;
      char *word = strndup(input + word_start, word_len);
      if (!word) {
        sb_free(&sb);
        return strdup(input);
      }

      HashEntry *entry = ht_get(&shell->aliases, word);
      free(word);

      if (entry) {
        sb_append(&sb, entry->value);
      } else {
        for (size_t j = word_start; j < i; ++j) {
          sb_append_char(&sb, input[j]);
        }
      }
      command_pos = false;
      continue;
    }

    sb_append_char(&sb, input[i++]);
    command_pos = false;
  }

  char *result = sb_as_cstr(&sb);
  return result ? result : strdup(input);
}
