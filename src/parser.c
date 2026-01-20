#include "parser.h"

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

static AstNode *parse_pipeline(Tokens *tokens, size_t *i) {
  AstNode *left = parse_command(tokens, i);
  if (!left) {
    return NULL;
  }

  while (*i < vec_size(tokens) && vec_at(tokens, *i).type == TOKEN_PIPE) {
    ++(*i);
    AstNode *right = parse_command(tokens, i);
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
    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (!node) {
      ast_free(&left);
      return NULL;
    }

    if (token.type == TOKEN_AND) {
      node->type = NODE_AND;
    } else if (token.type == TOKEN_OR) {
      node->type = NODE_OR;
    } else {
      free(node);
      break;
    }

    ++(*i);

    AstNode *right = parse_pipeline(tokens, i);
    if (!right) {
      ast_free(&left);
      ast_free(&node);
      return NULL;
    }

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
    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (!node) {
      ast_free(&left);
      return NULL;
    }

    if (token.type == TOKEN_OPERAND) {
      node->type = NODE_BACKGROUND;
    } else if (token.type == TOKEN_SEMICOLON) {
      node->type = NODE_SEMICOLON;
    } else {
      free(node);
      break;
    }

    ++(*i);

    AstNode *right;
    if (*i < vec_size(tokens)) {
      right = parse_logical(tokens, i);
      if (!right) {
        ast_free(&left);
        ast_free(&node);
        return NULL;
      }
    } else {
      right = NULL;
    }

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
