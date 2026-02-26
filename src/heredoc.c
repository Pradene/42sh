#include "ast.h"
#include "sb.h"
#include "vec.h"
#include "42sh.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void read_heredoc(StringBuffer *sb, Redir *redir) {
  char template[] = "/tmp/heredoc_XXXXXX";
  int fd = mkstemp(template);
  if (fd < 0) {
    return;
  }

  unlink(template);

  char *line;
  while ((line = readline("> ")) != NULL) {
    sb_append(sb, line);

    if (strcmp(line, redir->target_path) == 0) {
      free(line);
      break;
    }

    write(fd, line, strlen(line));
    write(fd, "\n", 1);

    free(line);
  }

  lseek(fd, 0, SEEK_SET);
  redir->target_fd = fd;
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
    read_heredocs(sb, root->operator.left);
    read_heredocs(sb, root->operator.right);
    return;
  case NODE_BRACE:
  case NODE_PAREN:
    read_heredocs(sb, root->group.inner);
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

bool has_heredocs(AstNode *ast) {
  if (!ast) {
    return false;
  }

  switch (ast->type) {
  case NODE_PAREN:
  case NODE_BRACE:
    for (size_t i = 0; i < vec_size(&ast->group.redirs); i++) {
      Redir *redir = &((Redir *)ast->group.redirs.data)[i];
      if (redir->type == REDIRECT_HEREDOC) {
        return true;
      }
    }
    return has_heredocs(ast->group.inner);
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
    return has_heredocs(ast->operator.left) || has_heredocs(ast->operator.right);
  case NODE_COMMAND:
    for (size_t i = 0; i < vec_size(&ast->command.redirs); i++) {
      Redir *redir = &((Redir *)ast->command.redirs.data)[i];
      if (redir->type == REDIRECT_HEREDOC) {
        return true;
      }
    }
    break;
  default:
    break;
  }

  return false;
}