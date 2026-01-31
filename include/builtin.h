#pragma once

#include "ast.h"
#include "env.h"

bool is_builtin(const char *cmd);
void exec_builtin(AstNode *node, Environment *env);

void builtin_exit(AstNode *node, Environment *env);
void builtin_export(AstNode *node, Environment *env);
void builtin_type(AstNode *node, Environment *env);
void builtin_echo(AstNode *node, Environment *env);
void builtin_cd(AstNode *node, Environment *env);