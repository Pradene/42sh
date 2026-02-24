#pragma once

#include "ast.h"
#include "env.h"
#include "ht.h"

typedef struct {
  const char *name;
  void (*fn)(AstNode *, HashTable *);
} Builtin;

bool is_builtin(const char *cmd);
void exec_builtin(AstNode *node, HashTable *env);

void builtin_exit(AstNode *node, HashTable *env);
void builtin_export(AstNode *node, HashTable *env);
void builtin_type(AstNode *node, HashTable *env);
void builtin_echo(AstNode *node, HashTable *env);
void builtin_cd(AstNode *node, HashTable *env);
void builtin_unset(AstNode *node, HashTable *env);
void builtin_alias(AstNode *node, HashTable *env);