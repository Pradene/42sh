#include "lexer.h"
#include "parser.h"
#include "strbuf.h"

#include <readline/history.h>
#include <readline/readline.h>

AstNode *get_command() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    return NULL;
  }

  sb_append(&sb, line);

  AstNode *root = NULL;
  while (true) {
    Tokens tokens = (Tokens){0};
    if (!tokenize(sb_as_cstr(&sb), &tokens) || !parse(&tokens, &root)) {
      free(line);
      line = readline("> ");
      sb_append(&sb, line);
      ast_free(&root);
    } else {
      break;
    }
  }

  add_history(sb_as_cstr(&sb));

  return root;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  using_history();

  while (true) {
    AstNode *root = get_command();
    if (!root) {
      continue;
    }

    ast_print(root);
  }

  clear_history();
  return 0;
}
