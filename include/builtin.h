#pragma once

#include "ast.h"
#include "env.h"
#include "ht.h"
#include "42sh.h"

typedef struct {
  const char *name;
  void (*fn)(AstNode *, Shell *);
} Builtin;

bool is_builtin(const char *cmd);
void exec_builtin(AstNode *node, Shell *shell);

void builtin_exit(AstNode *node, Shell *shell);
void builtin_export(AstNode *node, Shell *shell);
void builtin_type(AstNode *node, Shell *shell);
void builtin_echo(AstNode *node, Shell *shell);
void builtin_cd(AstNode *node, Shell *shell);
void builtin_unset(AstNode *node, Shell *shell);
void builtin_alias(AstNode *node, Shell *shell);
void builtin_unalias(AstNode *node, Shell *shell);
