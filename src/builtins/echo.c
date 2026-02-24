#include "ast.h"
#include "builtin.h"
#include "env.h"
#include "vec.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static char *interpret_escapes(const char *str) {
  char *result = malloc(strlen(str) + 1);
  size_t j = 0;
  
  for (size_t i = 0; str[i]; i++) {
    if (str[i] == '\\' && str[i + 1]) {
      i++;
      switch (str[i]) {
        case 'n':
          result[j++] = '\n';
          break;
        case 't':
          result[j++] = '\t';
          break;
        case 'r':
          result[j++] = '\r';
          break;
        case 'b':
          result[j++] = '\b';
          break;
        case 'a':
          result[j++] = '\a';
          break;
        case 'v':
          result[j++] = '\v';
          break;
        case 'f':
          result[j++] = '\f';
          break;
        case '\\':
          result[j++] = '\\';
          break;
        default:
          result[j++] = str[i];
          break;
      }
    } else {
      result[j++] = str[i];
    }
  }
  result[j] = '\0';
  return result;
}

void builtin_echo(AstNode *node, HashTable *env) {
  (void)env;
  char **args = node->command.args.data;
  size_t argc = vec_size(&node->command.args);

  bool newline = true;
  bool interpret = false;
  size_t index = 1;

  while (argc > index) {
    if (strcmp(args[index], "-n") == 0) {
        newline = false;
    } else if (strcmp(args[index], "-e") == 0) {
        interpret = true;
    } else if (strcmp(args[index], "-E") == 0) {
        interpret = false;
    } else {
        break;
    }
    index++;
  }

  for (size_t i = index; i < argc; ++i) {
    char *output = NULL;

    if (interpret) {
      output = interpret_escapes(output);
    } else {
      output = strdup(args[i]);
    }
    
    write(STDOUT_FILENO, output, strlen(output));
    if (i < argc - 1) {
      write(STDOUT_FILENO, " ", 1);
    }
    
    if (output) {
      free(output);
    }
  }

  if (newline) {
    write(STDOUT_FILENO, "\n", 1);
  }
}