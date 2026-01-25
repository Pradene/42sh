#pragma once

#include "ast.h"
#include "status.h"
#include "token.h"
#include "env.h"

void sigint_handler(int code);

void expansion(AstNode *root, Environment *env);
void stripping(AstNode *root);

StatusCode lex(char *input, Tokens *tokens);
StatusCode parse(Tokens *tokens, AstNode **root);
