#ifndef __TREE_H__
#define __TREE_H__

#include "SymbolTable.h"

typedef enum {
  PROG,
  variables,
  functions,
  function,
  header,
  body,
  parameters,
  declarations,
  instructions,
  function_call,
  function_call_params,
  switch_,
  case_,
  default_,
  break_,
  addsub,
  divstar,
  eq,
  order,
  and,
  or,
  type,
  void_,
  if_,
  else_,
  while_,
  return_,
  character,
  num,
  ident,
  assignment,
  not,

  /* list all other node labels, if any */
  /* The list must coincide with the string array in tree.c */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
} label_t;



typedef struct Node {
  label_t label;
  struct Node *firstChild, *nextSibling;
  int lineno;

  union {
    char byte;
    int num;
    char ident[64];
    char comp[3];
  };
  SymbolTable* sym_table;
} Node;

Node *makeNode(label_t label);
void addSibling(Node *node, Node *sibling);
void addChild(Node *parent, Node *child);
void deleteTree(Node*node);
void printTree(Node *node, bool printTables);

#define FIRSTCHILD(node) node->firstChild
#define SECONDCHILD(node) node->firstChild->nextSibling
#define THIRDCHILD(node) node->firstChild->nextSibling->nextSibling

#endif