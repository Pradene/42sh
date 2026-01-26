#pragma once

#include "ast.h"
#include "status.h"
#include "token.h"
#include "env.h"

void sigint_handler(int code);
void sigint_heredoc_handler(int code);

void expansion(AstNode *root, const Environment *env);
void stripping(AstNode *root);

StatusCode lex(const char *input, Tokens *tokens);
StatusCode parse(const Tokens *tokens, AstNode **root);

void execute_command(AstNode *root, Environment *env);