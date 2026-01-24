#include "parser.h"
#include "error.h"
#include "lexer.h"
#include "vec.h"
#include <stdio.h>

static StatusCode parse_redirect(Tokens *tokens, size_t *i,
                                 Redirection *redirect);
static StatusCode parse_command(Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_group(Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_pipeline(Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_logical(Tokens *tokens, size_t *i, AstNode **root);
static StatusCode parse_sequence(Tokens *tokens, size_t *i, AstNode **root);

static StatusCode parse_redirect(Tokens *tokens, size_t *i,
                                 Redirection *redirect) {
  if (*i >= vec_size(tokens)) {
    return UNEXPECTED_TOKEN;
  }

  Token token = vec_at(tokens, *i);
  switch (token.type) {
  case TOKEN_REDIRECT_IN:
  case TOKEN_HEREDOC:
  case TOKEN_REDIRECT_OUT:
  case TOKEN_REDIRECT_APPEND:
    ++(*i);
    if (*i >= vec_size(tokens) || vec_at(tokens, *i).type != TOKEN_WORD) {
      return UNEXPECTED_TOKEN;
    }
    redirect->type = token.type;
    redirect->target = strdup(vec_at(tokens, *i).s);
    if (!redirect->target) {
      return MEM_ALLOCATION_FAILED;
    }
    ++(*i);
    return OK;
  default:
    return UNEXPECTED_TOKEN;
  }
}

static StatusCode parse_command(Tokens *tokens, size_t *i, AstNode **root) {
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
      Redirection redirect = {0};
      StatusCode status = parse_redirect(tokens, i, &redirect);
      if (status != OK) {
        free(node);
        return status;
      }
      vec_push(&node->command.redirects, redirect);
    } else {
      break;
    }
  }

  if (vec_size(&node->command.args) != 0 ||
      vec_size(&node->command.redirects) != 0) {
    *root = node;
    return OK;
  } else {
    free(node);
    return UNEXPECTED_TOKEN;
  }
}

static StatusCode parse_group(Tokens *tokens, size_t *i, AstNode **root) {
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
    return parse_command(tokens, i, root);
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
  node->group.redirects = (Redirections){0};

  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    if (token.type == TOKEN_REDIRECT_IN || token.type == TOKEN_REDIRECT_OUT ||
        token.type == TOKEN_REDIRECT_APPEND || token.type == TOKEN_HEREDOC) {
      Redirection redirect = {0};
      StatusCode status = parse_redirect(tokens, i, &redirect);
      if (status != OK) {
        ast_free(node);
        return UNEXPECTED_TOKEN;
      }

      vec_push(&node->group.redirects, redirect);
    } else {
      break;
    }
  }

  *root = node;
  return OK;
}

static StatusCode parse_pipeline(Tokens *tokens, size_t *i, AstNode **root) {
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

static StatusCode parse_logical(Tokens *tokens, size_t *i, AstNode **root) {
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

static StatusCode parse_sequence(Tokens *tokens, size_t *i, AstNode **root) {
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

StatusCode parse(Tokens *tokens, AstNode **root) {
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
