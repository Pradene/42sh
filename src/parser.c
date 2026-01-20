#include "parser.h"
#include "lexer.h"
#include <stdio.h>

static AstNode *parse_command(Tokens *tokens, size_t *i);
static AstNode *parse_group(Tokens *tokens, size_t *i);
static AstNode *parse_pipeline(Tokens *tokens, size_t *i);
static AstNode *parse_logical(Tokens *tokens, size_t *i);
static AstNode *parse_sequence(Tokens *tokens, size_t *i);

static AstNode *parse_command(Tokens *tokens, size_t *i) {
  AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
  if (!node) {
    return NULL;
  }

  node->type = NODE_UNDEFINED;
  node->command.args = (CommandArgs){0};
  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    if (token.type != TOKEN_WORD) {
      break;
    }

    ++(*i);

    node->type = NODE_COMMAND;
    vec_push(&node->command.args, token.s);
  }

  if (node->type == NODE_COMMAND) {
    return node;
  } else {
    free(node);
    return NULL;
  }
}

static AstNode *parse_group(Tokens *tokens, size_t *i) {
  TokenType close;
  AstNodeType type;

  if (vec_at(tokens, *i).type == TOKEN_LPAREN) {
    close = TOKEN_RPAREN;
    type = NODE_PAREN;
  } else if (vec_at(tokens, *i).type == TOKEN_LBRACE) {
    close = TOKEN_RBRACE;
    type = NODE_BRACE;
  } else {
    return parse_command(tokens, i);
  }

  ++(*i);
  AstNode *inner = parse_sequence(tokens, i);
  if (!inner) {
    return NULL;
  }
  if (vec_at(tokens, *i).type != close) {
    ast_free(&inner);
    return NULL;
  }
  ++(*i);
  AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
  node->type = type;
  node->group.inner = inner;
  return node;
}

static AstNode *parse_pipeline(Tokens *tokens, size_t *i) {
  AstNode *left = parse_group(tokens, i);
  if (!left) {
    return NULL;
  }

  while (*i < vec_size(tokens)) {
    if (vec_at(tokens, *i).type != TOKEN_PIPE) {
      break;
    }

    ++(*i);

    AstNode *right = parse_group(tokens, i);
    if (!right) {
      ast_free(&left);
      return NULL;
    }

    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (!node) {
      ast_free(&left);
      ast_free(&right);
      return NULL;
    }

    node->type = NODE_PIPE;
    node->operator.right = right;
    node->operator.left = left;
    left = node;
  }
  return left;
}

static AstNode *parse_logical(Tokens *tokens, size_t *i) {
  AstNode *left = parse_pipeline(tokens, i);
  if (!left) {
    return NULL;
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

    AstNode *right = parse_pipeline(tokens, i);
    if (!right) {
      ast_free(&left);
      return NULL;
    }

    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (!node) {
      ast_free(&left);
      ast_free(&right);
      return NULL;
    }

    node->type = type;
    node->operator.left = left;
    node->operator.right = right;
    left = node;
  }
  return left;
}

static AstNode *parse_sequence(Tokens *tokens, size_t *i) {
  AstNode *left = parse_logical(tokens, i);
  if (!left) {
    return NULL;
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

    AstNode *right;
    if (*i < vec_size(tokens)) {
      right = parse_logical(tokens, i);
    } else {
      right = NULL;
    }

    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (!node) {
      ast_free(&left);
      ast_free(&right);
      return NULL;
    }

    node->type = type;
    node->operator.left = left;
    node->operator.right = right;
    left = node;
  }
  return left;
}

bool parse(Tokens *tokens, AstNode **root) {
  size_t position = 0;

  *root = parse_sequence(tokens, &position);
  if (!(*root) || position != vec_size(tokens)) {
    return false;
  }
  return true;
}
