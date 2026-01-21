#include "lexer.h"
#include "parser.h"
#include "sb.h"
#include "vec.h"

#include <readline/history.h>
#include <readline/readline.h>

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
    if (!lex_result.is_ok && lex_result.err == 1) {
      for (size_t i = 0; i < vec_size(&lex_result.ok); ++i) {
        Token *token = &vec_at(&lex_result.ok, i);
        if (token->s) {
          free(token->s);
        }
      }
      vec_free(&lex_result.ok);

      line = readline("> ");
      if (!line) {
        break;
      }
      sb_append(&sb, line);
      free(line);
      continue;
    }
    lex_result.ok = lex_result.ok;

    ParseResult parse_result = parse(&lex_result.ok);
    if (!parse_result.is_ok && parse_result.err == 1) {
      vec_free(&lex_result.ok);
      line = readline("> ");
      if (!line) {
        break;
      }
      sb_append(&sb, line);
      free(line);
      continue;
    } else if (!parse_result.is_ok) {
      vec_free(&lex_result.ok);
      break;
    }

    add_history(sb_as_cstr(&sb));
    vec_free(&lex_result.ok);
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
