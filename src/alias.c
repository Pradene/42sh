#include "42sh.h"

#include <ctype.h>

void expand_alias(Shell *shell) {
  bool command_pos = true;
  char *input = sb_as_cstr(&shell->input);

  StringBuffer sb = {0};
  size_t i = 0;
  while (input[i]) {
    if (isspace(input[i])) {
      sb_append_char(&sb, input[i++]);
      continue;
    }

    if (input[i] == '|' || input[i] == ';' || input[i] == '(' || input[i] == '{' || input[i] == '\n') {
      sb_append_char(&sb, input[i++]);
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

    if (command_pos && (isalnum((unsigned char)input[i]) || input[i] == '_')) {
      size_t word_start = i;
      while (input[i] && (isalnum((unsigned char)input[i]) || input[i] == '_' || input[i] == '-' || input[i] == '.')) {
        ++i;
      }
      size_t word_len = i - word_start;
      char *word = strndup(input + word_start, word_len);
      if (!word) {
        sb_free(&sb);
        return;
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

  sb_free(&shell->input);
  shell->input = sb;
}
