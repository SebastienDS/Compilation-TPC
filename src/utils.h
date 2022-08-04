#ifndef __UTILS__
#define __UTILS__

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "tree.h"
#include "SymbolTable.h"

typedef struct {
    char* function_name;
    SymbolTable* global;
    SymbolTable* local;
} Tables;

void get_new_label(char buffer[25]);

void fillSymbolTable(SymbolTable* table, Node* declarations);
void insertDeclType(SymbolTable* table, Type var_type, Node* node);

Type get_type(Tables* tables, char* value);
int get_type_size(Type type);
void get_string_address(Tables* tables, char* value, char buffer[25]);

void compile_global_declarations(Node* declarations, FILE* file, SymbolTable* table);
void compile_global_declaration(Node* declaration, FILE* file, Type var_type);

void compile_prog(Node* tree, FILE* file);
void declare_functions(Node* functions, Tables* tables);
void compile_functions(Node* functions, FILE* file, Tables* tables);
void compile_function(Node* func, FILE* file, Tables* tables);


bool compile_instructions(Node* instr, FILE* file, Tables* tables);
bool compile_instruction(Node* instr, FILE* file, Tables* tables);

void compile_assignment(Node* instr, FILE* file, Tables* tables);
bool compile_if(Node* instr, FILE* file, Tables* tables);
bool compile_while(Node* instr, FILE* file, Tables* tables);
bool compile_switch(Node* instr, FILE* file, Tables* tables);
bool compile_switch_instructions(Node* instr, FILE* file, Tables* tables, char label_break[25]);
bool compile_return(Node* instr, FILE* file, Tables* tables);


Type compile_expression(Node* expr, FILE* file, Tables* tables);

Type compile_not(Node* expr, FILE* file, Tables* tables);
Type compile_or(Node* expr, FILE* file, Tables* tables);
Type compile_and(Node* expr, FILE* file, Tables* tables);
Type compile_eq(Node* expr, FILE* file, Tables* tables);
Type compile_order(Node* expr, FILE* file, Tables* tables);
Type compile_unary_addsub(Node* expr, FILE* file, Tables* tables);
Type compile_addsub(Node* expr, FILE* file, Tables* tables);
Type compile_divstar(Node* expr, FILE* file, Tables* tables);
Type compile_num(Node* expr, FILE* file, Tables* tables);
Type compile_character(Node* expr, FILE* file, Tables* tables);
Type compile_ident(Node* expr, FILE* file, Tables* tables);
Type compile_function_call(Node* expr, FILE* file, Tables* tables);


#endif