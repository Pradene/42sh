#pragma once

#include "ast.h"
#include "env.h"

bool is_builtin(const char *cmd);
void exec_builtin(AstNode *node, Variables *env);

void builtin_exit(AstNode *node, Variables *env);
void builtin_export(AstNode *node, Variables *env);
void builtin_type(AstNode *node, Variables *env);
void builtin_echo(AstNode *node, Variables *env);
void builtin_cd(AstNode *node, Variables *env);
void builtin_unset(AstNode *node, Variables *env);
