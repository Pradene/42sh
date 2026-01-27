#include "42sh.h"
#include "ast.h"
#include "sb.h"
#include "status.h"
#include "token.h"
#include "vec.h"

#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

char *read_line(const char *prompt) {
  static char *buffer = NULL;

  if (buffer) {
    char *newline = strchr(buffer, '\n');
    if (newline) {
      size_t line_len = newline - buffer;
      char *result = malloc(line_len + 1);
      if (!result) {
        return NULL;
      }

      strncpy(result, buffer, line_len);
      result[line_len] = '\0';

      char *next = newline + 1;
      if (*next) {
        char *new_buffer = malloc(strlen(next) + 1);
        if (new_buffer) {
          strcpy(new_buffer, next);
          free(buffer);
          buffer = new_buffer;
        }
      } else {
        free(buffer);
        buffer = NULL;
      }

      return result;
    } else {
      char *result = buffer;
      buffer = NULL;
      return result;
    }
  } else {

    char *input = readline(prompt);
    if (!input) {
      return NULL;
    }

    char *newline = strchr(input, '\n');
    if (newline) {
      size_t first_line_len = newline - input;

      if (*(newline + 1)) {
        buffer = malloc(strlen(newline + 1) + 1);
        if (buffer) {
          strcpy(buffer, newline + 1);
        }
      }

      char *result = malloc(first_line_len + 1);
      if (!result) {
        free(input);
        return NULL;
      }

      strncpy(result, input, first_line_len);
      result[first_line_len] = '\0';
      free(input);
      return result;
    }
    return input;
  }
}

void read_heredoc(StringBuffer *sb, Redir *redir) {
  char template[] = "/tmp/heredoc_XXXXXX";
  int fd = mkstemp(template);
  if (fd < 0) {
    return;
  }

  struct sigaction sa, old_sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigint_heredoc_handler;
  sigaction(SIGINT, &sa, &old_sa);

  unlink(template);

  char *line;
  while ((line = read_line("> ")) != NULL) {
    sb_append_char(sb, '\n');
    sb_append(sb, line);

    if (strcmp(line, redir->target_path) == 0) {
      break;
    }

    write(fd, line, strlen(line));
    write(fd, "\n", 1);

    free(line);
  }

  lseek(fd, 0, SEEK_SET);
  redir->target_fd = fd;
  sigaction(SIGINT, &old_sa, NULL);
}

void read_heredocs(StringBuffer *sb, AstNode *root) {
  if (!root) {
    return;
  }

  switch (root->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    return;
  case NODE_BRACE:
  case NODE_PAREN:
    vec_foreach(Redir, redir, &root->group.redirs) {
      if (redir->type == REDIRECT_HEREDOC) {
        read_heredoc(sb, redir);
      }
    }
    return;
  case NODE_COMMAND:
    vec_foreach(Redir, redir, &root->command.redirs) {
      if (redir->type == REDIRECT_HEREDOC) {
        read_heredoc(sb, redir);
      }
    }
    return;
  }
}

AstNode *get_command() {
  StringBuffer sb = {0};
  char *line = read_line("$ ");
  if (!line) {
    exit(EXIT_SUCCESS);
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
        sb_append_char(&sb, '\n');
        line = read_line("> ");
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
        line = read_line("> ");
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

    read_heredocs(&sb, root);

    add_history(sb_as_cstr(&sb));

    sb_free(&sb);
    return root;
  }

  sb_free(&sb);
  return NULL;
}

int main(int argc, char **argv, char **envp) {
  (void)argc;
  (void)argv;

  Environment env = {0};
  env_copy(&env, (const char **)envp);

  rl_clear_signals();
  rl_getc_function = fgetc;

  using_history();

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  sigaction(SIGQUIT, &sa, NULL);

  while (true) {
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    AstNode *root = get_command();
    if (!root) {
      continue;
    }

    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);

    execute_command(root, &env);

    ast_free(root);
  }

  env_free(&env);

  clear_history();

  return 0;
}
