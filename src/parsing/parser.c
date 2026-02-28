#include "parser.h"
#include "ast.h"
#include "lexer.h"
#include "status.h"
#include "vec.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static StatusCode parse_redir(ParserState *state, Redir *redir);
static StatusCode parse_simple_command(ParserState *state, AstNode **root);
static StatusCode parse_group(ParserState *state, AstNode **root);
static StatusCode parse_pipeline(ParserState *state, AstNode **root);
static StatusCode parse_logical(ParserState *state, AstNode **root);
static StatusCode parse_sequence(ParserState *state, AstNode **root);

static StatusCode parser_peek(ParserState *state, Token *token) {
  if (!state->token_ready) {
    state->current_token = next_token(state->input, &state->position);
    if (state->current_token.type == TOKEN_ERROR) {
      return INCOMPLETE_INPUT;
    }
    state->token_ready = true;
  }
  *token = state->current_token;
  return OK;
}

static StatusCode parser_advance(ParserState *state) {
  if (!state->token_ready) {
    state->current_token = next_token(state->input, &state->position);
    if (state->current_token.type == TOKEN_ERROR) {
      return INCOMPLETE_INPUT;
    }
    state->token_ready = true;
    return OK;
  }

  state->token_ready = false;
  return OK;
}

static bool parser_match(ParserState *state, TokenType type) {
  Token token;
  StatusCode status = parser_peek(state, &token);
  if (status != OK) {
    return false;
  }
  return token.type == type;
}

static bool is_redirect_token(ParserState *state) {
  return (
    parser_match(state, TOKEN_REDIRECT_IN) ||
    parser_match(state, TOKEN_REDIRECT_OUT) ||
    parser_match(state, TOKEN_REDIRECT_APPEND) ||
    parser_match(state, TOKEN_HEREDOC) ||
    parser_match(state, TOKEN_REDIRECT_IN_FD) ||
    parser_match(state, TOKEN_REDIRECT_OUT_FD)
  );
}

static bool is_valid_fd(const char *s, int *fd) {
  char *end;
  long value = strtol(s, &end, 10);

  if (*end != '\0') {
    return false;
  }

  *fd = (int)value;
  return true;
}

static StatusCode read_heredoc(ParserState *state, Redir *redir) {
  char template[] = "/tmp/heredoc_XXXXXX";
  int fd = mkstemp(template);
  if (fd < 0) {
    return INCOMPLETE_INPUT;
  }

  unlink(template);

  const char *delim = redir->path;
  size_t delim_length = strlen(delim);

  while (true) {
    const char *line_start = state->input + state->position;
    const char *newline = strchr(line_start, '\n');

    if (!newline) {
      close(fd);
      return INCOMPLETE_INPUT;
    }

    size_t line_length = newline - line_start;
    state->position += line_length + 1;

    if (line_length == delim_length && strncmp(line_start, delim, delim_length) == 0) {
      break;
    }

    write(fd, line_start, line_length);
    write(fd, "\n", 1);
  }

  lseek(fd, 0, SEEK_SET);
  free(redir->delimiter);
  redir->delimiter = NULL;
  redir->path = NULL;
  redir->fd = fd;
  return OK;
}

static StatusCode read_heredocs(ParserState *state) {
  for (size_t i = 0; i < state->heredocs.size; i++) {
    StatusCode status = read_heredoc(state, state->heredocs.data[i]);
    if (status != OK) {
      return status;
    }
  }
  free(state->heredocs.data);
  state->heredocs.size = 0;
  return OK;
}

static StatusCode parse_redir(ParserState *state, Redir *redir) {
  if (!is_redirect_token(state)) {
    return UNEXPECTED_TOKEN;
  }

  Token op_token;
  StatusCode status = parser_peek(state, &op_token);
  if (status != OK) {
    return status;
  }

  RedirType type;
  switch (op_token.type) {
  case TOKEN_REDIRECT_IN:     type = REDIRECT_IN;      break;
  case TOKEN_HEREDOC:         type = REDIRECT_HEREDOC; break;
  case TOKEN_REDIRECT_OUT:    type = REDIRECT_OUT;     break;
  case TOKEN_REDIRECT_APPEND: type = REDIRECT_APPEND;  break;
  case TOKEN_REDIRECT_IN_FD:  type = REDIRECT_IN_FD;   break;
  case TOKEN_REDIRECT_OUT_FD: type = REDIRECT_OUT_FD;  break;
  default:                    return UNEXPECTED_TOKEN;
  }

  parser_advance(state);

  if (!parser_match(state, TOKEN_WORD)) {
    return UNEXPECTED_TOKEN;
  }

  Token word_token;
  status = parser_peek(state, &word_token);
  if (status != OK) {
    return status;
  }

  redir->type = type;

  if (type == REDIRECT_IN_FD || type == REDIRECT_OUT_FD) {
    if (!is_valid_fd(word_token.s, &redir->fd)) {
      return UNEXPECTED_TOKEN;
    }
  } else if (type == REDIRECT_HEREDOC) {
    redir->delimiter = word_token.s;
    if (!redir->delimiter) {
      return MEM_ALLOCATION_FAILED;
    }
  } else {
    redir->path = word_token.s;
    if (!redir->path) {
      return MEM_ALLOCATION_FAILED;
    }
  }

  parser_advance(state);
  return OK;
}

static StatusCode parse_simple_command(ParserState *state, AstNode **root) {
  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    return MEM_ALLOCATION_FAILED;
  }

  node->type = NODE_COMMAND;
  node->command = (Command){0};

  while (true) {
    Token token;
    StatusCode status = parser_peek(state, &token);
    if (status != OK) {
      ast_free(node);
      return status;
    }

    if (parser_match(state, TOKEN_WORD)) {
      Token token;
      StatusCode status = parser_peek(state, &token);
      if (status != OK) {
        ast_free(node);
        return status;
      }
      vec_push(&node->command.args, token.s);
      parser_advance(state);
    } else if (is_redirect_token(state)) {
      Redir redir = {0};
      StatusCode status = parse_redir(state, &redir);
      if (status != OK) {
        ast_free(node);
        return status;
      }
      vec_push(&node->command.redirs, redir);
      if (redir.type == REDIRECT_HEREDOC) {
        vec_push(&state->heredocs, &vec_last(&node->command.redirs));
      }
    } else {
      break;
    }
  }

  if (vec_size(&node->command.args) || vec_size(&node->command.redirs)) {
    *root = node;
    return OK;
  } else {
    free(node);
    return UNEXPECTED_TOKEN;
  }
}

static StatusCode parse_group(ParserState *state, AstNode **root) {
  TokenType close;
  AstNodeType type;

  if (parser_match(state, TOKEN_LPAREN)) {
    close = TOKEN_RPAREN;
    type = NODE_PAREN;
  } else if (parser_match(state, TOKEN_LBRACE)) {
    close = TOKEN_RBRACE;
    type = NODE_BRACE;
  } else {
    return parse_simple_command(state, root);
  }

  parser_advance(state);

  if (parser_match(state, TOKEN_EOF)) {
    return INCOMPLETE_INPUT;
  }

  AstNode *inner = NULL;
  StatusCode status = parse_sequence(state, &inner);
  if (status != OK) {
    return status;
  }

  if (!parser_match(state, close)) {
    ast_free(inner);
    return parser_match(state, TOKEN_EOF) ? INCOMPLETE_INPUT : UNEXPECTED_TOKEN;
  }
  parser_advance(state);

  AstNode *node = (AstNode *)malloc(sizeof(AstNode));
  if (!node) {
    ast_free(inner);
    return MEM_ALLOCATION_FAILED;
  }
  node->type = type;
  node->group.inner = inner;
  node->group.redirs = (Redirs){0};

  while (is_redirect_token(state)) {
    Redir redir = {0};
    StatusCode status = parse_redir(state, &redir);
    if (status != OK) {
      ast_free(node);
      return status;
    }
    vec_push(&node->group.redirs, redir);
    if (redir.type == REDIRECT_HEREDOC) {
      vec_push(&state->heredocs, &vec_last(&node->group.redirs));
    }
  }

  *root = node;
  return OK;
}

static StatusCode parse_pipeline(ParserState *state, AstNode **root) {
  AstNode *left = NULL;
  StatusCode status = parse_group(state, &left);
  if (status != OK) {
    return status;
  }

  while (parser_match(state, TOKEN_PIPE)) {
    parser_advance(state);

    AstNode *right = NULL;
    status = parse_group(state, &right);
    if (status != OK) {
      ast_free(left);
      return parser_match(state, TOKEN_EOF) ? INCOMPLETE_INPUT : UNEXPECTED_TOKEN;
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

static StatusCode parse_logical(ParserState *state, AstNode **root) {
  AstNode *left = NULL;
  StatusCode status = parse_pipeline(state, &left);
  if (status != OK) {
    return status;
  }

  while (parser_match(state, TOKEN_AND) || parser_match(state, TOKEN_OR)) {
    Token op_token;
    StatusCode status = parser_peek(state, &op_token);
    if (status != OK) {
      ast_free(left);
      return status;
    }

    AstNodeType type = (op_token.type == TOKEN_AND) ? NODE_AND : NODE_OR;

    parser_advance(state);

    AstNode *right = NULL;
    status = parse_pipeline(state, &right);
    if (status != OK) {
      ast_free(left);
      return parser_match(state, TOKEN_EOF) ? INCOMPLETE_INPUT : UNEXPECTED_TOKEN;
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

static StatusCode parse_sequence(ParserState *state, AstNode **root) {
  AstNode *left = NULL;
  StatusCode status = parse_logical(state, &left);
  if (status != OK) {
    return status;
  }

  while (parser_match(state, TOKEN_OPERAND) || parser_match(state, TOKEN_SEMICOLON) || parser_match(state, TOKEN_NEWLINE)) {
    Token sep_token;
    StatusCode status = parser_peek(state, &sep_token);
    if (status != OK) {
      ast_free(left);
      return status;
    }

    AstNodeType type;
    if (sep_token.type == TOKEN_OPERAND) {
      type = NODE_BACKGROUND;
    } else if (sep_token.type == TOKEN_SEMICOLON) {
      type = NODE_SEMICOLON;
    } else {
      *root = left;
      return OK;
    }

    parser_advance(state);

    AstNode *right = NULL;
    if (
      !parser_match(state, TOKEN_RPAREN) &&
      !parser_match(state, TOKEN_NEWLINE) &&
      !parser_match(state, TOKEN_RBRACE) &&
      !parser_match(state, TOKEN_EOF)
    ) {
      status = parse_logical(state, &right);
      if (status != OK) {
        ast_free(left);
        return status;
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

StatusCode parse(Shell *shell) {
  char *input = sb_as_cstr(&shell->input);
  if (!input) {
    return UNEXPECTED_TOKEN;
  }

  ParserState state = {
    .input = (char *)input,
    .position = 0,
    .current_token = {0},
    .token_ready = false,
    .heredocs = {0}
  };

  StatusCode status = parse_sequence(&state, &shell->command);
  if (status != OK) {
    vec_foreach(Redir *, heredoc, &state.heredocs) {
      free((*heredoc)->delimiter);
    }
    vec_free(&state.heredocs);
    return status;
  }

  if (parser_match(&state, TOKEN_NEWLINE)) {
    parser_advance(&state);
    StatusCode ds = read_heredocs(&state);
    if (ds != OK) {
      vec_foreach(Redir *, heredoc, &state.heredocs) {
        free((*heredoc)->delimiter);
      }
      vec_free(&state.heredocs);
      ast_free(shell->command);
      return ds;
    }
    return OK;
  }

  if (!parser_match(&state, TOKEN_EOF)) {
    vec_foreach(Redir *, heredoc, &state.heredocs) {
      free((*heredoc)->delimiter);
    }
    vec_free(&state.heredocs);
    ast_free(shell->command);
    return UNEXPECTED_TOKEN;
  }

  vec_foreach(Redir *, heredoc, &state.heredocs) {
    free((*heredoc)->delimiter);
  }
  vec_free(&state.heredocs);

  return OK;
}
