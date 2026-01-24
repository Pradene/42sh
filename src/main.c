#include "42sh.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "sb.h"
#include "vec.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

char *strip_quotes(const char *s) {
  if (!s || s[0] == '\0') {
    return strdup(s);
  }

  size_t length = strlen(s);
  char *result = (char *)malloc(length + 1);
  if (!result) {
    return NULL;
  }

  size_t out_idx = 0;
  int in_single_quote = 0;
  int in_double_quote = 0;

  for (size_t i = 0; i < length; ++i) {
    char c = s[i];

    if (c == '\'' && !in_double_quote) {
      in_single_quote = !in_single_quote;
      continue;
    }

    if (c == '"' && !in_single_quote) {
      in_double_quote = !in_double_quote;
      continue;
    }

    if (c == '\\' && in_double_quote && i + 1 < length) {
      char next = s[i + 1];
      if (next == '"' || next == '\\') {
        result[out_idx++] = next;
        ++i;
        continue;
      }
    }

    result[out_idx++] = c;
  }

  result[out_idx] = '\0';
  return result;
}

void quote_stripping(AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    quote_stripping(root->operator.left);
    quote_stripping(root->operator.right);
    return;

  case NODE_BRACE:
  case NODE_PAREN:
    quote_stripping(root->group.inner);
    return;

  case NODE_COMMAND:

    for (size_t i = 0; i < vec_size(&root->command.args); ++i) {
      char *original = vec_at(&root->command.args, i);
      char *stripped = strip_quotes(original);

      if (stripped) {
        free(original);
        vec_remove(&root->command.args, i);
        vec_insert(&root->command.args, i, stripped);
      }
    }
    return;
  }
}

AstNode *get_statement() {
  StringBuffer sb = {0};
  char *line = readline("$ ");
  if (!line) {
    exit(EXIT_SUCCESS);
    return NULL;
  }

  sb_append(&sb, line);
  free(line);

  while (true) {
    StatusCode status;
    Tokens tokens = {0};
    AstNode *root = NULL;

    status = lex(sb_as_cstr(&sb), &tokens);
    if (status != OK) {
      if (status == INCOMPLETE_INPUT) {
        line = readline("> ");
        if (!line) {
          sb_free(&sb);
          exit(EXIT_SUCCESS);
        }
        sb_append(&sb, line);
        free(line);
        continue;
      }
      break;
    }

    status = parse(&tokens, &root);
    tokens_free(&tokens);
    if (status != OK) {
      if (status == INCOMPLETE_INPUT) {
        line = readline("> ");
        if (!line) {
          sb_free(&sb);
          exit(EXIT_SUCCESS);
        }
        sb_append(&sb, line);
        free(line);
        continue;
      }
      break;
    }

    add_history(sb_as_cstr(&sb));

    quote_stripping(root);

    sb_free(&sb);
    return root;
  }

  sb_free(&sb);
  return NULL;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;
  (void)envp;

  using_history();

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigint_handler;
  sigaction(SIGINT, &sa, NULL);

  sa.sa_handler = SIG_IGN;
  sigaction(SIGQUIT, &sa, NULL);

  while (true) {
    AstNode *root = get_statement();
    if (!root) {
      continue;
    }

    ast_print(root);
    ast_free(root);
  }

  clear_history();
  return 0;
}
