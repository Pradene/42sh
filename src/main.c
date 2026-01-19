#include "lexer.h"
#include "vec.h"
#include <ctype.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <readline/rlstdc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

const char *token_type_str(TokenType type) {
  switch (type) {
  case TOKEN_EOF:
    return "EOF";
  case TOKEN_WORD:
    return "WORD";
  case TOKEN_AND:
    return "AND";
  case TOKEN_OR:
    return "OR";
  case TOKEN_PIPE:
    return "PIPE";
  case TOKEN_SEMICOLON:
    return "SEMICOLON";
  case TOKEN_OPERAND:
    return "OPERAND";
  case TOKEN_DOLLAR:
    return "DOLLAR";
  case TOKEN_LPAREN:
    return "LPAREN";
  case TOKEN_RPAREN:
    return "RPAREN";
  case TOKEN_LBRACE:
    return "LBRACE";
  case TOKEN_RBRACE:
    return "RBRACE";
  case TOKEN_REDIRECT_OUT:
    return "REDIRECT_OUT";
  case TOKEN_REDIRECT_APPEND:
    return "REDIRECT_APPEND";
  case TOKEN_HEREDOC:
    return "HEREDOC";
  case TOKEN_REDIRECT_IN:
    return "REDIRECT_IN";
  case TOKEN_UNDEFINED:
    return "UNDEFINED";
  }

  __builtin_unreachable();
}

static bool has_unfinished_quote(const char *s) {
  bool in_single = false;
  bool in_double = false;

  for (size_t i = 0; s[i]; ++i) {
    if (s[i] == '\\' && s[i + 1]) {
      ++i;
      continue;
    }
    if (s[i] == '\'' && !in_double) {
      in_single = !in_single;
    } else if (s[i] == '\"' && !in_single) {
      in_double = !in_double;
    }
  }

  return in_single || in_double;
}

typedef VEC(char) StringBuilder;
char *read_input() {
  StringBuilder sb = {0};
  char *line = readline("$ ");
  if (!line)
    return NULL;

  while (line) {
    size_t len = strlen(line);

    if (len > 0 && line[len - 1] == '\\') {
      for (size_t i = 0; i < len - 1; ++i) {
        vec_push(&sb, line[i]);
      }
      free(line);
      line = readline("> ");
      continue;
    }

    for (size_t i = 0; i < len; ++i) {
      vec_push(&sb, line[i]);
    }
    free(line);

    if (has_unfinished_quote(sb.data)) {
      vec_push(&sb, '\n');
      line = readline("> ");
      continue;
    }

    break;
  }

  vec_push(&sb, '\0');
  return sb.data;
}

bool charset_contains(const char *set, char c) {
  return strchr(set, c) != NULL;
}

bool match(const char *s1, const char *s2) {
  return !strncmp(s1, s2, strlen(s1));
}

Token next_token(char *s, size_t *i) {
  while (s[*i]) {
    if (isspace(s[*i])) {
      ++(*i);
      continue;
    }
    size_t start = *i;

    if (match(">>", s + *i)) {
      *i += 2;
      return (Token){
          .type = TOKEN_REDIRECT_APPEND, .s = NULL, .position = start};
    }
    if (match("<<", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_HEREDOC, .s = NULL, .position = start};
    }
    if (match("&&", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_AND, .s = NULL, .position = start};
    }
    if (match("||", s + *i)) {
      *i += 2;
      return (Token){.type = TOKEN_OR, .s = NULL, .position = start};
    }
    if (match("|", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_PIPE, .s = NULL, .position = start};
    }
    if (match("<", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_REDIRECT_IN, .s = NULL, .position = start};
    }
    if (match(">", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_REDIRECT_OUT, .s = NULL, .position = start};
    }
    if (match(";", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_SEMICOLON, .s = NULL, .position = start};
    }
    if (match("$", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_DOLLAR, .s = NULL, .position = start};
    }
    if (match("(", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_LPAREN, .s = NULL, .position = start};
    }
    if (match(")", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_RPAREN, .s = NULL, .position = start};
    }
    if (match("{", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_LBRACE, .s = NULL, .position = start};
    }
    if (match("}", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_RBRACE, .s = NULL, .position = start};
    }
    if (match("&", s + *i)) {
      ++(*i);
      return (Token){.type = TOKEN_OPERAND, .s = NULL, .position = start};
    }

    while (s[*i]) {
      if (isspace(s[*i]) || charset_contains("|&;[](){}<>", s[*i])) {
        break;
      }

      if (s[*i] == '\\') {
        ++(*i);
        if (s[*i]) {
          ++(*i);
        }
      } else if (s[*i] == '\"' || s[*i] == '\'') {
        char quote = s[(*i)++];
        while (s[*i] && s[*i] != quote) {
          if (quote == '\"' && s[*i] == '\\') {
            ++(*i);
            if (s[*i]) {
              ++(*i);
            }
          } else {
            ++(*i);
          }
        }
        if (s[*i] == quote) {
          ++(*i);
        }
      } else {
        ++(*i);
      }
    }

    char *word = strndup(s + start, *i - start);
    return (Token){.type = TOKEN_WORD, .s = word, .position = start};
  }
  return (Token){.type = TOKEN_EOF, .s = NULL, .position = *i};
}

typedef VEC(Token) Tokens;
bool tokenize(char *s, Tokens *tokens) {
  Token token;
  size_t position = 0;

  while (true) {
    token = next_token(s, &position);
    if (token.type == TOKEN_UNDEFINED)
      return false;
    if (token.type == TOKEN_EOF)
      return true;
    vec_push(tokens, token);
  }
  return true;
}

int main() {
  while (true) {
    Tokens tokens = {0};
    char *input = read_input();
    if (!input || !tokenize(input, &tokens)) {
      return 1;
    }
    for (size_t i = 0; i < tokens.size; ++i) {
      printf("%s", token_type_str(tokens.data[i].type));
      if (tokens.data[i].type == TOKEN_WORD) {
        printf(": %s", tokens.data[i].s);
      }
      printf("\n");
    }
  }
  return 0;
}
