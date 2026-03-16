#include "42sh.h"
#include "utils.h"

#include <ctype.h>
#include <stdio.h>

char *expand_alias(const char *input) {
  StringBuffer sb = {0};
  bool command_pos = true;
  size_t i = 0;

  while (input[i]) {
    if (isspace((unsigned char)input[i])) {
      sb_append_char(&sb, input[i++]);
      continue;
    }

    if (input[i] == '|' || input[i] == ';' || input[i] == '(' || input[i] == '{' || input[i] == '\n') {
      sb_append_char(&sb, input[i++]);
      if (input[i - 1] == '|' && input[i] == '|') {
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

      command_pos = false;

      char *word = strndup(input + word_start, i - word_start);
      if (!word) {
        sb_free(&sb);
        return NULL;
      }

      HtEntry *entry = ht_get(aliases, word);
      free(word);
      if (!entry) {
        for (size_t j = word_start; j < i; ++j) {
          sb_append_char(&sb, input[j]);
        }

        continue;
      }
      
      char *replacement = entry->value;

      if (replacement) {
        sb_append(&sb, replacement);
        
        if (strendswith(replacement, " ")) {
          command_pos = true;
        }
      }

      continue;
    }

    sb_append_char(&sb, input[i++]);
    command_pos = false;
  }

  return sb_as_cstr(&sb);
}
