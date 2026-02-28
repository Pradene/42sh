#include "ast.h"
#include "env.h"
#include "sb.h"
#include "vec.h"
#include "42sh.h"

void expand_node(Shell *shell, AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case NODE_PIPE:
  case NODE_AND:
  case NODE_OR:
  case NODE_SEMICOLON:
  case NODE_BACKGROUND:
    expand_node(shell, node->operator.left);
    expand_node(shell, node->operator.right);
    return;
  case NODE_BRACE:
  case NODE_PAREN:
    expand_node(shell, node->group.inner);

    expansion(node, shell);
    splitting(node);
    stripping(node);
    return;

  case NODE_COMMAND:
    expansion(node, shell);
    splitting(node);
    stripping(node);
    return;
  }
}

void expand_command(Shell *shell) {
  expand_node(shell, shell->command);
}
