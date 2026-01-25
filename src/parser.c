#include "ast.h"
#include "status.h"
#include "token.h"
#include "vec.h"

#include <stdio.h>

static StatusCode parse_redir(const Tokens *tokens, size_t *i, Redir *redir);
static StatusCode parse_simple_command(const Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_group(const Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_pipeline(const Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_logical(const Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_sequence(const Tokens *tokens, size_t *i, AstNode **root);

static StatusCode parse_redir(const Tokens *tokens, size_t *i, Redir *redir) {
  if (*i >= vec_size(tokens)) {
    return UNEXPECTED_TOKEN;
  }

  Token token = vec_at(tokens, *i);
  RedirType type;
  switch (token.type) {
  case TOKEN_REDIRECT_IN:
    type = REDIRECT_IN;
    break;
  case TOKEN_HEREDOC:
    type = REDIRECT_HEREDOC;
    break;
  case TOKEN_REDIRECT_OUT:
    type = REDIRECT_OUT;
    break;
  case TOKEN_REDIRECT_APPEND:
    type = REDIRECT_APPEND;
    break;
  default:
    return UNEXPECTED_TOKEN;
  }

  ++(*i);

  if (*i >= vec_size(tokens) || vec_at(tokens, *i).type != TOKEN_WORD) {
    return UNEXPECTED_TOKEN;
  }

  redir->type = type;
  redir->target = strdup(vec_at(tokens, *i).s);
  if (!redir->target) {
    return MEM_ALLOCATION_FAILED;
  }

  ++(*i);

  return OK;
}

static StatusCode parse_simple_command(const Tokens *tokens, size_t *i, AstNode **root) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    return MEM_ALLOCATION_FAILED;
  }

  node->type = NODE_COMMAND;
  node->command = (Command){0};
  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);

    if (token.type == TOKEN_WORD) {
      ++(*i);
      char *s = strdup(token.s);
      if (!s) {
        free(node);
        return MEM_ALLOCATION_FAILED;
      }
      vec_push(&node->command.args, s);
    } else if (token.type == TOKEN_REDIRECT_OUT ||
               token.type == TOKEN_REDIRECT_APPEND ||
               token.type == TOKEN_REDIRECT_IN || token.type == TOKEN_HEREDOC) {
      Redir redir = {0};
      StatusCode status = parse_redir(tokens, i, &redir);
      if (status != OK) {
        free(node);
        return status;
      }
      vec_push(&node->command.redirs, redir);
    } else {
      break;
    }
  }

  if (vec_size(&node->command.args) != 0 ||
      vec_size(&node->command.redirs) != 0) {
    *root = node;
    return OK;
  } else {
    free(node);
    return UNEXPECTED_TOKEN;
  }
}

static StatusCode parse_group(const Tokens *tokens, size_t *i, AstNode **root) {
  TokenType close;
  AstNodeType type;

  if (*i >= vec_size(tokens)) {
    return UNEXPECTED_TOKEN;
  }

  if (vec_at(tokens, *i).type == TOKEN_LPAREN) {
    close = TOKEN_RPAREN;
    type = NODE_PAREN;
  } else if (vec_at(tokens, *i).type == TOKEN_LBRACE) {
    close = TOKEN_RBRACE;
    type = NODE_BRACE;
  } else {
    return parse_simple_command(tokens, i, root);
  }

  ++(*i);

  if (*i >= vec_size(tokens)) {
    return INCOMPLETE_INPUT;
  }

  AstNode *inner = NULL;
  StatusCode status = parse_sequence(tokens, i, &inner);
  if (status != OK) {
    return status;
  }

  if (*i >= vec_size(tokens)) {
    ast_free(inner);
    return INCOMPLETE_INPUT;
  }

  if (vec_at(tokens, *i).type != close) {
    ast_free(inner);
    return UNEXPECTED_TOKEN;
  }
  ++(*i);

  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    ast_free(inner);
    return MEM_ALLOCATION_FAILED;
  }
  node->type = type;
  node->group.inner = inner;
  node->group.redirs = (Redirs){0};

  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    if (token.type == TOKEN_REDIRECT_IN || token.type == TOKEN_REDIRECT_OUT ||
        token.type == TOKEN_REDIRECT_APPEND || token.type == TOKEN_HEREDOC) {
      Redir redir = {0};
      StatusCode status = parse_redir(tokens, i, &redir);
      if (status != OK) {
        ast_free(node);
        return UNEXPECTED_TOKEN;
      }

      vec_push(&node->group.redirs, redir);
    } else {
      break;
    }
  }

  *root = node;
  return OK;
}


static StatusCode parse_pipeline(const Tokens *tokens, size_t *i, AstNode **root) {
  AstNode *left = NULL;
  StatusCode status = parse_group(tokens, i, &left);
  if (status != OK) {
    return status;
  }

  while (*i < vec_size(tokens) && vec_at(tokens, *i).type == TOKEN_PIPE) {
    ++(*i);

    AstNode *right = NULL;
    status = parse_group(tokens, i, &right);
    if (status != OK) {
      ast_free(left);
      if (*i >= vec_size(tokens)) {
        return INCOMPLETE_INPUT;
      } else {
        return UNEXPECTED_TOKEN;
      }
    }

    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      ast_free(left);
      ast_free(right);
      return MEM_ALLOCATION_FAILED;
    }

    node->type = NODE_PIPE;
    node->operator.right = right;
    node->operator.left = left;
    left = node;
  }

  *root = left;
  return OK;
}

static StatusCode parse_logical(const Tokens *tokens, size_t *i, AstNode **root) {
  AstNode *left = NULL;
  StatusCode status = parse_pipeline(tokens, i, &left);
  if (status != OK) {
    return status;
  }

  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);

    AstNodeType type;
    if (token.type == TOKEN_AND) {
      type = NODE_AND;
    } else if (token.type == TOKEN_OR) {
      type = NODE_OR;
    } else {
      break;
    }

    ++(*i);

    AstNode *right = NULL;
    status = parse_pipeline(tokens, i, &right);
    if (status != OK) {
      ast_free(left);
      if (*i >= vec_size(tokens)) {
        return INCOMPLETE_INPUT;
      } else {
        return UNEXPECTED_TOKEN;
      }
    }

    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      ast_free(left);
      ast_free(right);
      return MEM_ALLOCATION_FAILED;
    }

    node->type = type;
    node->operator.left = left;
    node->operator.right = right;
    left = node;
  }

  *root = left;
  return OK;
}

static StatusCode parse_sequence(const Tokens *tokens, size_t *i, AstNode **root) {
  AstNode *left = NULL;
  StatusCode status = parse_logical(tokens, i, &left);
  if (status != OK) {
    return status;
  }

  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);

    AstNodeType type;
    if (token.type == TOKEN_OPERAND) {
      type = NODE_BACKGROUND;
    } else if (token.type == TOKEN_SEMICOLON) {
      type = NODE_SEMICOLON;
    } else {
      break;
    }

    ++(*i);

    AstNode *right = NULL;
    if (*i < vec_size(tokens)) {
      Token next = vec_at(tokens, *i);
      if (next.type != TOKEN_RPAREN && next.type != TOKEN_RBRACE) {
        status = parse_logical(tokens, i, &right);
        if (status != OK) {
          ast_free(left);
          return status;
        }
      }
    }

    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      ast_free(left);
      ast_free(right);
      return MEM_ALLOCATION_FAILED;
    }

    node->type = type;
    node->operator.left = left;
    node->operator.right = right;
    left = node;
  }

  *root = left;
  return OK;
}

StatusCode parse(const Tokens *tokens, AstNode **root) {
  size_t i = 0;
  AstNode *node = NULL;
  StatusCode status = parse_sequence(tokens, &i, &node);
  if (status != OK) {
    return status;
  }

  if (i < vec_size(tokens)) {
    ast_free(node);
    return UNEXPECTED_TOKEN;
  }

  *root = node;
  return OK;
}
