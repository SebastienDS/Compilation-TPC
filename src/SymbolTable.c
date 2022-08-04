#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "SymbolTable.h"

Symbol* new_symbol(Type type, char* ident) {
    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    if (symbol == NULL) {
        perror("Symbol");
        exit(3);
    }

    symbol->type = type;
    strcpy(symbol->ident, ident);
    symbol->address = -1;

    return symbol;
}

SymbolNode* new_node(Symbol* symbol) {
    SymbolNode* node = (SymbolNode*)malloc(sizeof(SymbolNode));
    if (node == NULL) {
        perror("Node");
        exit(3);
    }

    node->symbol = symbol;
    node->next = NULL;

    return node;
}

SymbolTable* new_table() {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (table == NULL) {
        perror("SymbolTable");
        exit(3);
    }

    for (int i = 0; i < N; i++) {
        table->buckets[i].head = NULL;
    }

    table->size = 0;

    return table;
}

void free_table(SymbolTable* table) {
    for (int i = 0; i < N; i++) {
        LinkedBuckets* linked = &table->buckets[i];
        
        while (linked->head != NULL) {
            SymbolNode* next = linked->head->next;
            free(linked->head->symbol);
            free(linked->head);
            linked->head = next;
        }
    }
}

int hash(char* str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash % N;
}

bool table_contains(SymbolTable* table, char* value) {
    int h = hash(value);
    LinkedBuckets* linked = &table->buckets[h];

    for (SymbolNode* node = linked->head; node != NULL; node = node->next) {
        Symbol* s = node->symbol;
        
        if (strcmp(value, s->ident) == 0) {
            return true;
        }
    }

    return false;
}

Type table_get_type(SymbolTable* table, char* value) {
    int h = hash(value);
    LinkedBuckets* linked = &table->buckets[h];

    for (SymbolNode* node = linked->head; node != NULL; node = node->next) {
        Symbol* s = node->symbol;
        
        if (strcmp(value, s->ident) == 0) {
            return s->type;
        }
    }
    
    fprintf(stderr, "Symtable don't contains %s\n", value);
    exit(2);
}

int table_get_address(SymbolTable* table, char* value) {
    int h = hash(value);
    LinkedBuckets* linked = &table->buckets[h];

    for (SymbolNode* node = linked->head; node != NULL; node = node->next) {
        Symbol* s = node->symbol;
        
        if (strcmp(value, s->ident) == 0) {
            return s->address;
        }
    }
    
    fprintf(stderr, "Symtable don't contains %s\n", value);
    exit(2);
}

bool insert_symbol(SymbolTable* table, Symbol* symbol) {
    if (table_contains(table, symbol->ident)) return false;

    int h = hash(symbol->ident);
    LinkedBuckets* linked = &table->buckets[h];

    SymbolNode* node = new_node(symbol);
    node->next = linked->head;
    linked->head = node;

    return true;
}

static void print_symbol(Symbol* symbol) {
    if(symbol->type.type == TYPE_PRIMITIF) {
        printf("(%d - %d - %s) -> ", symbol->address, symbol->type.primitif, symbol->ident);
    }
    else {
        printf("(%d - %s ", 
            symbol->type.function.return_type, 
            symbol->ident
        );
        printf("[");
        for (int i = 0; i < symbol->type.function.args_count; i++) {
            printf("%d, ", symbol->type.function.args_type[i]);
        }
        printf("]) -> ");
    }
}

static void print_bucket(LinkedBuckets* bucket) {
    for (SymbolNode* node = bucket->head; node != NULL; node = node->next) {
        Symbol* symbol = node->symbol;
        print_symbol(symbol);
    }
    printf("NULL\n");
}

void print_table(SymbolTable* table) {
    printf("[ size=%d\n", table->size);
    for (int i = 0; i < N; i++) {
        LinkedBuckets* linked = &table->buckets[i];
        printf("\t%d : ", i);
        print_bucket(linked);        
    }
    printf("]\n");
}

Primitif get_primitif_from_string(char* str) {
    if (strcmp(str, "void") == 0) return TYPE_VOID;
    else if (strcmp(str, "int") == 0) return TYPE_INT;
    else if (strcmp(str, "char") == 0) return TYPE_CHAR;
    else {
        fprintf(stderr, "Unknown type\n");
        exit(2);
    }
}