#ifndef __SYMBOLTABLE__
#define __SYMBOLTABLE__

#include <stdbool.h>

#define N 10
#define MAX_ARGS 50

typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
} Primitif;

typedef enum {
    TYPE_PRIMITIF,
    TYPE_FUNCTION
} TypeEnum;

typedef struct {
    Primitif return_type;
    Primitif args_type[MAX_ARGS];
    int args_count;
} Function;

typedef struct {
    TypeEnum type;
    union {
        Primitif primitif;
        Function function;
    };
} Type;

typedef struct {
    Type type;
    char ident[50];
    int address;
} Symbol;

typedef struct SymbolNode {
    Symbol* symbol;
    struct SymbolNode* next;
} SymbolNode;

typedef struct {
    SymbolNode* head;
} LinkedBuckets;

typedef struct {
    LinkedBuckets buckets[N];
    int size;
} SymbolTable;

Symbol* new_symbol(Type type, char* ident);
SymbolNode* new_node(Symbol* symbol);
SymbolTable* new_table();
void free_table(SymbolTable* table);

int hash(char* str);
bool table_contains(SymbolTable* table, char* value);
Type table_get_type(SymbolTable* table, char* value);
int table_get_address(SymbolTable* table, char* value);

bool insert_symbol(SymbolTable* table, Symbol* symbol);
void print_table(SymbolTable* table);

Primitif get_primitif_from_string(char* str);


#endif