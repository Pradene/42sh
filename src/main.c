#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "sb.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <stddef.h>
#include <stdio.h>

AstNode *get_statement() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    return NULL;
  }

  sb_append(&sb, line);
  free(line);

  while (true) {
    LexResult lex_result = lex(sb_as_cstr(&sb));
    if (!lex_result.is_ok) {
      if (lex_result.err == INCOMPLETE_INPUT) {
        line = readline("> ");
        if (!line) {
          break;
        }
        sb_append(&sb, line);
        free(line);
        continue;
      }
      break;
    }

    Tokens tokens = lex_result.ok;
    ParseResult parse_result = parse(&tokens);
    tokens_free(&tokens);
    if (!parse_result.is_ok) {
      if (parse_result.err == INCOMPLETE_INPUT) {
        line = readline("> ");
        if (!line) {
          break;
        }
        sb_append(&sb, line);
        free(line);
        continue;
      }
      break;
    }

    add_history(sb_as_cstr(&sb));
    sb_free(&sb);
    return parse_result.ok;
  }

  sb_free(&sb);
  return NULL;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  using_history();

  while (true) {
    AstNode *root = get_statement();
    if (!root) {
      continue;
    }

    ast_print(root);
    ast_free(root);
    root = NULL;
  }

  clear_history();
  return 0;
}
