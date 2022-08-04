/* tree.c */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "tree.h"
#include "SymbolTable.h"
extern int yylineno;       /* from lexer */

const char *StringFromLabel[] = {
  "PROG",
  "VARIABLES",
  "FUNCTIONS",
  "FUNCTION",
  "HEADER",
  "BODY",
  "PARAMETERS",
  "DECLARATIONS",
  "INSTRUCTIONS",
  "FUNCTION_CALL",
  "FUNCTION_CALL_PARAMS",
  "SWITCH",
  "CASE",
  "DEFAULT",
  "BREAK",
  "ADDSUB",
  "DIVSTAR",
  "EQ",
  "ORDER",
  "AND",
  "OR",
  "TYPE",
  "VOID",
  "IF",
  "ELSE",
  "WHILE",
  "RETURN",
  "CHARACTER",
  "NUM",
  "IDENT",
  "ASSIGNMENT",
  "NOT",
  /* list all other node labels, if any */
  /* The list must coincide with the label_t enum in tree.h */
  /* To avoid listing them twice, see https://stackoverflow.com/a/10966395 */
};

Node *makeNode(label_t label) {
  Node *node = malloc(sizeof(Node));
  if (!node) {
    printf("Run out of memory\n");
    exit(3);
  }
  node->label = label;
  node-> firstChild = node->nextSibling = NULL;
  node->lineno=yylineno;
  node->sym_table = NULL;
  return node;
}

void addSibling(Node *node, Node *sibling) {
  Node *curr = node;
  while (curr->nextSibling != NULL) {
    curr = curr->nextSibling;
  }
  curr->nextSibling = sibling;
}

void addChild(Node *parent, Node *child) {
  if (parent->firstChild == NULL) {
    parent->firstChild = child;
  }
  else {
    addSibling(parent->firstChild, child);
  }
}

void deleteTree(Node *node) {
  if (node->firstChild) {
    deleteTree(node->firstChild);
  }
  if (node->nextSibling) {
    deleteTree(node->nextSibling);
  }
  if (node && node->sym_table != NULL) free_table(node->sym_table);
  free(node);
}

static void print_node(Node* node, bool printTables) {
  switch (node->label) {
  case addsub:
  case divstar:
    printf("%s[%c]\n", StringFromLabel[node->label], node->byte);
    break;
    
  case num:
    printf("%s[%d]\n", StringFromLabel[node->label], node->num);
    break;

  case character:
  case ident:
  case type:
    printf("%s[%s]\n", StringFromLabel[node->label], node->ident);
    break;

  case order:
  case eq:
    printf("%s[%s]\n", StringFromLabel[node->label], node->comp);
    break;
  
  case PROG:
    printf("%s\n", StringFromLabel[node->label]);
    if (printTables) {
      printf("Globals : ");
      print_table(node->sym_table);
    }
    break;
    
  case function:
    printf("%s\n", StringFromLabel[node->label]);
    if (printTables) {
      printf("Locals : ");
      print_table(node->sym_table);
    }
    break;

  default:
    printf("%s\n", StringFromLabel[node->label]);
    break;
  }
}

void printTree(Node *node, bool printTables) {
  static bool rightmost[128]; // tells if node is rightmost sibling
  static int depth = 0;       // depth of current node
  for (int i = 1; i < depth; i++) { // 2502 = vertical line
    printf(rightmost[i] ? "    " : "\u2502   ");
  }
  if (depth > 0) { // 2514 = L form; 2500 = horizontal line; 251c = vertical line and right horiz 
    printf(rightmost[depth] ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ");
  }
  
  print_node(node, printTables);

  depth++;
  for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) {
    rightmost[depth] = (child->nextSibling) ? false : true;
    printTree(child, printTables);
  }
  depth--;
}
