#include "parser.h"
#include "error.h"
#include "lexer.h"
#include "vec.h"
#include <stdio.h>

// static ParseResult parse_redirect(Tokens *tokens, size_t *i);
static ParseResult parse_command(Tokens *tokens, size_t *i);
static ParseResult parse_group(Tokens *tokens, size_t *i);
static ParseResult parse_pipeline(Tokens *tokens, size_t *i);
static ParseResult parse_logical(Tokens *tokens, size_t *i);
static ParseResult parse_sequence(Tokens *tokens, size_t *i);

// static ParseResult parse_redirect(Tokens *tokens, size_t *i) {
//   if (*i >= vec_size(tokens)) {
//     return (ParseResult){.is_ok = false, .err = UNEXPECTED_TOKEN};
//   }
//
//   AstNode *node = (AstNode *)malloc(sizeof(AstNode));
//   if (!node) {
//     return (ParseResult){.is_ok = false, .err = MEM_ALLOCATION_FAILED};
//   }
//
//   Token token = vec_at(tokens, *i);
//   switch (token.type) {
//   case TOKEN_REDIRECT_IN:
//   case TOKEN_HEREDOC:
//   case TOKEN_REDIRECT_FD_IN:
//   case TOKEN_REDIRECT_OUT:
//   case TOKEN_REDIRECT_APPEND:
//   case TOKEN_REDIRECT_FD_OUT:
//     node->type = NODE_REDIRECT;
//     // node->redirection.type = ;
//     // node->redirection.fd = ;
//     // node->redirection.target = ;
//     return (ParseResult){.is_ok = true, .ok = node};
//   default:
//     free(node);
//     return (ParseResult){.is_ok = false, .err = UNEXPECTED_TOKEN};
//   }
// }

static ParseResult parse_command(Tokens *tokens, size_t *i) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    return (ParseResult){.is_ok = false, .err = MEM_ALLOCATION_FAILED};
  }

  node->type = NODE_COMMAND;
  node->command = (Command){0};
  while (*i < vec_size(tokens)) {
    Token token = vec_at(tokens, *i);
    if (token.type != TOKEN_WORD) {
      break;
    }

    ++(*i);

    vec_push(&node->command, token.s);
  }

  if (vec_size(&node->command) != 0) {
    return (ParseResult){.is_ok = true, .ok = node};
  } else {
    free(node);
    return (ParseResult){.is_ok = false, .err = UNEXPECTED_TOKEN};
  }
}

static ParseResult parse_group(Tokens *tokens, size_t *i) {
  TokenType close;
  AstNodeType type;

  if (*i >= vec_size(tokens)) {
    return (ParseResult){.is_ok = false, .err = UNEXPECTED_TOKEN};
  }

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

  if (*i >= vec_size(tokens)) {
    return (ParseResult){.is_ok = false, .err = INCOMPLETE_INPUT};
  }

  ParseResult inner_result = parse_sequence(tokens, i);
  if (!inner_result.is_ok) {
    return inner_result;
  }

  AstNode *inner = inner_result.ok;
  if (*i >= vec_size(tokens)) {
    ast_free(inner);
    return (ParseResult){.is_ok = false, .err = INCOMPLETE_INPUT};
  }

  if (type == NODE_BRACE) {
    if (inner->type != NODE_SEMICOLON) {
      ast_free(inner);
      return (ParseResult){.is_ok = false, .err = INCOMPLETE_INPUT};
    }
  }
  if (*i >= vec_size(tokens) && vec_at(tokens, *i).type != close) {
    ast_free(inner);
    return (ParseResult){.is_ok = false, .err = INCOMPLETE_INPUT};
  }
  ++(*i);

  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    ast_free(inner);
    return (ParseResult){.is_ok = false, .err = MEM_ALLOCATION_FAILED};
  }
  node->type = type;
  node->group.inner = inner;
  return (ParseResult){.is_ok = true, .ok = node};
}

static ParseResult parse_pipeline(Tokens *tokens, size_t *i) {
  ParseResult left_result = parse_group(tokens, i);
  if (!left_result.is_ok) {
    return left_result;
  }

  while (*i < vec_size(tokens) && vec_at(tokens, *i).type == TOKEN_PIPE) {
    ++(*i);

    ParseResult right_result = parse_group(tokens, i);
    if (!right_result.is_ok) {
      ast_free(left_result.ok);
      return (ParseResult){.is_ok = false, .err = INCOMPLETE_INPUT};
    }

    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      ast_free(left_result.ok);
      ast_free(right_result.ok);
      return (ParseResult){.is_ok = false, .err = MEM_ALLOCATION_FAILED};
    }

    node->type = NODE_PIPE;
    node->operator.right = right_result.ok;
    node->operator.left = left_result.ok;
    left_result.ok = node;
  }
  return left_result;
}

static ParseResult parse_logical(Tokens *tokens, size_t *i) {
  ParseResult left_result = parse_pipeline(tokens, i);
  if (!left_result.is_ok) {
    return left_result;
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

    ParseResult right_result = parse_pipeline(tokens, i);
    if (!right_result.is_ok) {
      ast_free(left_result.ok);
      return (ParseResult){.is_ok = false, .err = INCOMPLETE_INPUT};
    }

    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      ast_free(left_result.ok);
      ast_free(right_result.ok);
      return (ParseResult){.is_ok = false, .err = MEM_ALLOCATION_FAILED};
    }

    node->type = type;
    node->operator.left = left_result.ok;
    node->operator.right = right_result.ok;
    left_result.ok = node;
  }
  return left_result;
}

static ParseResult parse_sequence(Tokens *tokens, size_t *i) {
  ParseResult left_result = parse_logical(tokens, i);
  if (!left_result.is_ok) {
    return left_result;
  }

  AstNode *left = left_result.ok;
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

    ParseResult right_result;
    if (*i < vec_size(tokens)) {
      right_result = parse_logical(tokens, i);
    } else {
      right_result = (ParseResult){.is_ok = true, .ok = NULL};
    }

    AstNode *right = right_result.is_ok ? right_result.ok : NULL;
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (!node) {
      ast_free(left);
      ast_free(right);
      return (ParseResult){.is_ok = false, .err = MEM_ALLOCATION_FAILED};
    }

    node->type = type;
    node->operator.left = left;
    node->operator.right = right;
    left_result.ok = node;
  }

  return left_result;
}

ParseResult parse(Tokens *tokens) {
  size_t i = 0;
  ParseResult result = parse_sequence(tokens, &i);
  if (result.is_ok && i < vec_size(tokens)) {
    ast_free(result.ok);
    return (ParseResult){.is_ok = false, .err = UNEXPECTED_TOKEN};
  }
  return result;
}
