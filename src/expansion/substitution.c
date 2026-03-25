#include "42sh.h"
#include "parser.h"
#include "ast.h"
#include "vec.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static char *run_substitution(char *cmd) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    return NULL;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return strdup("");
  } else if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    AstNode *node = NULL;
    char *expanded = expand_alias(cmd);
    StatusCode status = parse(expanded, &node);
    free(expanded);
    free(cmd);

    if (status == OK) {
      execute_command(node);
      ast_free(node);
    }

    cleanup();
    exit(exit_status);
  } else {
    close(pipefd[1]);
    
    StringBuffer sb = {0};
    char buf[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
      for (ssize_t i = 0; i < n; ++i) {
        sb_append_char(&sb, buf[i]);
      }
    }
    close(pipefd[0]);
    
    int status;
    setpgid(pid, pid);
    waitpid(pid, &status, 0);
    
    char *result = sb_as_cstr(&sb);
    if (!result) {
      return strdup("");
    }
    
    size_t len = strlen(result);
    while (len > 0 && result[len - 1] == '\n') {
      result[--len] = '\0';
    }
    
    return result;
  }
}

static char *expand_substitution(const char *s) {
  StringBuffer sb = {0};
  size_t i = 0;

  while (s[i]) {
    if (s[i] == '$' && s[i + 1] == '(') {
      i += 2;
      size_t start = i;
      int depth = 1;
      while (s[i] && depth > 0) {
        if (s[i] == '(') {
          depth++;
        } else if (s[i] == ')') {
          depth--;
        }

        ++i;
      }

      char *cmd = strndup(s + start, i - start - 1);

      char *expanded_cmd = expand_substitution(cmd);
      free(cmd);

      char *output = run_substitution(expanded_cmd);
      free(expanded_cmd);

      if (output) {
        sb_append(&sb, output);
        free(output);
      }
    } else {
      sb_append_char(&sb, s[i++]);
    }
  }

  return sb_as_cstr(&sb);
}

void command_substitution(AstNode *node) {
  if (!node) return;

  switch (node->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_BACKGROUND:
  case NODE_SEMICOLON:
    return;
  case NODE_BRACE:
  case NODE_PAREN:
    vec_foreach(Redir, redir, &node->group.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD || redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *result = expand_substitution(redir->path);
      if (result) {
        free(redir->path);
        redir->path = result;
      }
    }
    return;
  case NODE_COMMAND:
    vec_foreach(char *, arg, &node->command.args) {
      char *result = expand_substitution(*arg);
      if (result) {
        free(*arg);
        *arg = result;
      }
    }
    vec_foreach(Redir, redir, &node->command.redirs) {
      if (redir->type == REDIRECT_HEREDOC || redir->type == REDIRECT_OUT_FD || redir->type == REDIRECT_IN_FD) {
        continue;
      }
      char *result = expand_substitution(redir->path);
      if (result) {
        free(redir->path);
        redir->path = result;
      }
    }
    return;
  }
}
