#pragma once

#include "ast.h"
#include "env.h"
#include "ht.h"
#include "42sh.h"

typedef struct {
  const char *name;
  void (*fn)(AstNode *);
} Builtin;

bool is_builtin(const char *cmd);
void exec_builtin(AstNode *node);

void builtin_exit(AstNode *node);
void builtin_export(AstNode *node);
void builtin_type(AstNode *node);
void builtin_echo(AstNode *node);
void builtin_cd(AstNode *node);
void builtin_set(AstNode *node);
void builtin_unset(AstNode *node);
void builtin_alias(AstNode *node);
void builtin_unalias(AstNode *node);
void builtin_hash(AstNode *node);
