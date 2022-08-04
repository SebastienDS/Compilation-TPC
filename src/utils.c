#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <tree.h>

#include "utils.h"

extern char* StringFromLabel[];

static int stack_alignment = 0;

static void insert_stack(FILE* file, int bytes) {
    stack_alignment += bytes;
    fprintf(file, "\t; stack: %d\n", stack_alignment);
}

static void push_stack(FILE* file) {
    insert_stack(file, 8);
}

static void pop_stack(FILE* file) {
    insert_stack(file, -8);
}

static int get_required_alignment() {
    int tmp = stack_alignment % 16;
    return tmp == 0 ? 0 : 16 - tmp;
}

void get_new_label(char buffer[25]) {
    static int count = 0;

    sprintf(buffer, "__label_%d", count);
    count++;
}

void fillSymbolTable(SymbolTable* table, Node* declarations) {
    for (Node *child = declarations->firstChild; child != NULL; child = child->nextSibling) {
        Type var_type;
        var_type.type = TYPE_PRIMITIF;
        var_type.primitif = get_primitif_from_string(child->ident);
        insertDeclType(table, var_type, child);
    }
}

void insertDeclType(SymbolTable* table, Type var_type, Node* node) {
    for (Node *child = node->firstChild; child != NULL; child = child->nextSibling) {
        table->size += get_type_size(var_type);
        Symbol* symbol = new_symbol(var_type, child->ident);
        symbol->address = table->size;
        bool inserted = insert_symbol(table, symbol);
        if (!inserted) {
            fprintf(stderr, "Line %d: Variable %s already declared\n", child->lineno, child->ident);
            exit(2);
        }
    }
}

Type get_type(Tables* tables, char* value) {
    if (table_contains(tables->local, value)) {
        return table_get_type(tables->local, value);
    }
    if (table_contains(tables->global, value)) {
        return table_get_type(tables->global, value);
    }

    fprintf(stderr, "Tables don't contains %s\n", value);
    exit(2);
}

int get_type_size(Type type) {
    if (type.type != TYPE_PRIMITIF) return -1;

    switch (type.primitif) {
    case TYPE_CHAR:
    case TYPE_INT:
        return 4;
    default:
        return -1;
    }
}

void get_string_address(Tables* tables, char* value, char buffer[25]) {
    if (table_contains(tables->local, value)) {
        int address = table_get_address(tables->local, value);
        sprintf(buffer, "rbp - %d", address);
        return;
    }
    if (table_contains(tables->global, value)) {
        strcpy(buffer, value);
        return;
    }

    fprintf(stderr, "Tables don't contains %s\n", value);
    exit(2);
}


void compile_global_declarations(Node* declarations, FILE* file, SymbolTable* table) {
    fprintf(file, "section .data\n");
    
    for (Node *child = declarations->firstChild; child != NULL; child = child->nextSibling) {
        Type var_type;
        var_type.type = TYPE_PRIMITIF;
        var_type.primitif = get_primitif_from_string(child->ident);
        compile_global_declaration(child, file, var_type);
        insertDeclType(table, var_type, child);
    }

    fprintf(file, "\n");
}

void compile_global_declaration(Node* declaration, FILE* file, Type var_type) {
    for (Node *child = declaration->firstChild; child != NULL; child = child->nextSibling) {
        switch (var_type.primitif) {
        case TYPE_CHAR:
        case TYPE_INT:
            fprintf(file,
                "\t%s dd 0\n", child->ident
            );
            break;
        default:
            break; 
        }
    }
}

void compile_declarations(Node* declarations, FILE* file, Tables* tables) {
    Type funct;
    funct.type = TYPE_FUNCTION;

    // set builtin functions
    funct.function.args_count = 0;
    funct.function.return_type = TYPE_CHAR;
    insert_symbol(tables->global, new_symbol(funct, "getchar"));

    funct.function.args_count = 1;
    funct.function.args_type[0] = TYPE_CHAR;
    funct.function.return_type = TYPE_VOID;
    insert_symbol(tables->global, new_symbol(funct, "putchar"));

    funct.function.args_count = 0;
    funct.function.return_type = TYPE_INT;
    insert_symbol(tables->global, new_symbol(funct, "getint"));

    funct.function.args_count = 1;
    funct.function.args_type[0] = TYPE_INT;
    funct.function.return_type = TYPE_VOID;
    insert_symbol(tables->global, new_symbol(funct, "putint"));

    compile_global_declarations(declarations, file, tables->global);
}

void compile_prog(Node* tree, FILE* file) {
    Tables tables;

    // define globals 
    tree->sym_table = new_table();
    tables.global = tree->sym_table;

    compile_declarations(FIRSTCHILD(tree), file, &tables);

    fprintf(file, 
        "section .text\n"
        "\textern getchar\n"
        "\textern putchar\n"
        "\textern getint\n"
        "\textern putint\n"
        "\tglobal _start\n"
    );

    Node* functions = SECONDCHILD(tree);
    declare_functions(functions, &tables);

    if (table_contains(tables.global, "main")) {
        Type t = table_get_type(tables.global, "main");
        if (t.type == TYPE_PRIMITIF) {
            fprintf(stderr, "main should be a function\n");
            exit(2);
        }
        if (t.function.args_count != 0) {
            fprintf(stderr, "Warning: main function must have no parameters, %d given\n", t.function.args_count);
        }
        if (t.function.return_type != TYPE_INT) {
            fprintf(stderr, "main function must return int\n");
            exit(2);
        }
    } else {
        fprintf(stderr, "Program should contains a main function\n");
        exit(2);
    }

    compile_functions(functions, file, &tables);
}

void declare_functions(Node* functions, Tables* tables) {
    for (Node *func = functions->firstChild; func != NULL; func = func->nextSibling) {
        // define function
        Node* header = FIRSTCHILD(func);
        Node* function_name = SECONDCHILD(header);
        Node* parameters = THIRDCHILD(header);

        Type funct;
        funct.type = TYPE_FUNCTION;

        Node* return_type = FIRSTCHILD(header);
        if (return_type->label == void_) {
            funct.function.return_type = TYPE_VOID;
        }
        else {
            funct.function.return_type = get_primitif_from_string(return_type->ident);
        }

        int count = 0;
        for (Node *child = parameters->firstChild; child != NULL; child = child->nextSibling) {
            funct.function.args_type[count] = get_primitif_from_string(child->ident);
            count++;
        }
        funct.function.args_count = count;

        bool inserted = insert_symbol(tables->global, new_symbol(funct, function_name->ident));
        if (!inserted) {
            fprintf(stderr, "Line %d: Function %s already declared\n", func->lineno, function_name->ident);
            exit(2);
        }
    }
}

void compile_functions(Node* functions, FILE* file, Tables* tables) {
    for (Node *child = functions->firstChild; child != NULL; child = child->nextSibling) {
        stack_alignment = 0; // reset stack
        compile_function(child, file, tables);
    }
}

void compile_function(Node* func, FILE* file, Tables* tables) {
    // define function
    Node* header = FIRSTCHILD(func);
    Node* function_name = SECONDCHILD(header);
    Node* parameters = THIRDCHILD(header);

    func->sym_table = new_table();
    tables->local = func->sym_table;
    tables->function_name = function_name->ident;

    Type funct = get_type(tables, function_name->ident);

    // define params
    fillSymbolTable(tables->local, parameters);

    Node* body = SECONDCHILD(func);

    // define locals
    Node* declarations = FIRSTCHILD(body);
    fillSymbolTable(tables->local, declarations);

    Node* instructions = SECONDCHILD(body);

    if (strcmp(function_name->ident, "main") == 0) {
        fprintf(file, 
            "\n_start:\n"
            "\tcall main\n"
            "\tpush rax\n\n"
            "\tmov rax, 60\n"
            // "\tmov rdi, 0\n"
            "\tpop rdi\n"
            "\tsyscall\n"
        );
    }

    fprintf(file, 
        "\n%s:\n"
        "\tpush rbp\n"
        "\tmov rbp, rsp\n\n",
        function_name->ident
    );

    if (tables->local->size != 0) {
        fprintf(file, "\tsub rsp, %d\n\n", tables->local->size);
        insert_stack(file, tables->local->size);
    }

    int j = 0;
    for (Node *child = parameters->firstChild; child != NULL; child = child->nextSibling) {
        char* ident = FIRSTCHILD(child)->ident;
        char buffer[25];
        get_string_address(tables, ident, buffer);

        switch (j) {
        case 0:
            fprintf(file, "\tmov dword [%s], edi\n", buffer);
            break;
        case 1:
            fprintf(file, "\tmov dword [%s], esi\n", buffer);
            break;
        case 2:
            fprintf(file, "\tmov dword [%s], edx\n", buffer);
            break;
        case 3:
            fprintf(file, "\tmov dword [%s], ecx\n", buffer);
            break;
        case 4:
            fprintf(file, "\tmov dword [%s], e8\n", buffer);
            break;
        case 5:
            fprintf(file, "\tmov dword [%s], e9\n", buffer);
            break;
        default:
            fprintf(stderr, "Warning line %d: Arguments count > 6 not already working\n", child->lineno);
            break;
        }

        j++;
    }

    bool have_returned = compile_instructions(instructions, file, tables);
    if (!have_returned && funct.function.return_type != TYPE_VOID) {
        fprintf(stderr, "Warning Line %d: The function %s must return a value\n", func->lineno, function_name->ident);
    }
}

bool compile_instructions(Node* instructions, FILE* file, Tables* tables) {
    bool have_returned = false;

    for (Node *child = instructions->firstChild; child != NULL; child = child->nextSibling) {
        bool returned = compile_instruction(child, file, tables);
        if (returned && !have_returned) {
            if (child->nextSibling != NULL) {
                fprintf(stderr, "Line %d: unreachable instructions\n", child->lineno);
            }
            have_returned = true;
        }
    }

    return have_returned;
}

bool compile_instruction(Node* instr, FILE* file, Tables* tables) {
    bool have_returned = false;

    switch (instr->label) {
    case assignment:
        compile_assignment(instr, file, tables);
        break;

    case if_:
        have_returned = compile_if(instr, file, tables);
        break;

    case while_:
        have_returned = compile_while(instr, file, tables);
        break;

    case switch_:
        have_returned = compile_switch(instr, file, tables);
        break;

    case function_call:
        compile_expression(instr, file, tables);
        fprintf(file, "\tpop rax\n");
        pop_stack(file);
        break;

    case return_:
        have_returned = compile_return(instr, file, tables);
        break;

    case body:
        have_returned = compile_instructions(instr, file, tables);
        break;

    default:
        fprintf(stderr, "Line %d: instruction not compiled %s\n", instr->lineno, StringFromLabel[instr->label]);
        break;
    }

    fprintf(file, "\n");

    return have_returned;
}

void compile_assignment(Node* instr, FILE* file, Tables* tables) {
    Node* var = FIRSTCHILD(instr);

    Type type1 = get_type(tables, var->ident);
    Type type2 = compile_expression(SECONDCHILD(instr), file, tables);

    if (type1.type != TYPE_PRIMITIF || type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", var->lineno);
        exit(2);
    }

    if (type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", var->lineno);
        exit(2);
    }

    if (type1.primitif == TYPE_CHAR && type2.primitif == TYPE_INT) {
        fprintf(stderr, "Warning line %d: Implicit convertion int -> char\n", var->lineno);
    }

    char buffer[25];
    get_string_address(tables, var->ident, buffer);

    fprintf(file, 
        "\tpop rax\n"
        "\tmov dword [%s], eax\n", buffer
    );
    pop_stack(file);
}

bool compile_if(Node* instr, FILE* file, Tables* tables) {
    bool have_returned_if = false;
    bool have_returned_else = false; 

    char label_after_if[25];
    get_new_label(label_after_if);

    Type type = compile_expression(FIRSTCHILD(instr), file, tables);
    if (type.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", instr->lineno);
        exit(2);
    }
    if (type.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", instr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rax\n"
        "\tcmp rax, 0\n"
        "\tje %s\n\n",
        label_after_if
    );
    pop_stack(file);

    Node* if_body = SECONDCHILD(instr);
    if (if_body != NULL) {
        if (if_body->label == body) {
            have_returned_if = compile_instructions(if_body, file, tables);
        }
        else {
            have_returned_if = compile_instruction(if_body, file, tables);
        }
    }

    if (if_body != NULL) {
        Node* else_block = THIRDCHILD(instr);
        if (else_block != NULL) {
            char label_if_jump_after_else[25];
            get_new_label(label_if_jump_after_else);

            fprintf(file, "\tjmp %s\n\n", label_if_jump_after_else);
            fprintf(file, "\t%s:\n", label_after_if);

            Node* block = FIRSTCHILD(else_block);
            if (block->label == body) {
                have_returned_else = compile_instructions(block, file, tables);
            }
            else {
                have_returned_else = compile_instruction(block, file, tables);
            }

            fprintf(file, "\t%s:\n", label_if_jump_after_else);
        }
        else {
            fprintf(file, "\t%s:\n", label_after_if);
        }
    }

    return have_returned_if && have_returned_else;
}

bool compile_while(Node* instr, FILE* file, Tables* tables) {
    bool have_returned = false;

    char label_while[25];
    char label_after_while[25];
    get_new_label(label_while);
    get_new_label(label_after_while);

    fprintf(file, "\t%s:\n", label_while);

    Type type = compile_expression(FIRSTCHILD(instr), file, tables);
    if (type.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", instr->lineno);
        exit(2);
    }
    if (type.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", instr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rax\n"
        "\tcmp rax, 0\n"
        "\tje %s\n\n",
        label_after_while
    );
    pop_stack(file);

    Node* body = SECONDCHILD(instr);
    if (body != NULL) {
        have_returned = compile_instructions(body, file, tables);
   
        fprintf(file, "\tjmp %s\n", label_while);
    }

    fprintf(file, "\t%s:\n", label_after_while);

    return have_returned;
}

static void verify_constant_expression(Node* expr) {
    switch (expr->label) {
    case ident:
    case function_call:
        fprintf(stderr, "Line %d: switch expression must be constant\n", expr->lineno);
        exit(2);
        break;
    default:
        break;
    }

    for (Node *node = expr->firstChild; node != NULL; node = node->nextSibling) {
        verify_constant_expression(node);
    }
}

int eval_constant_expression(Node* expr) {
    switch (expr->label) {
    case not:
        return !eval_constant_expression(FIRSTCHILD(expr));

    case or:
        return eval_constant_expression(FIRSTCHILD(expr)) || eval_constant_expression(SECONDCHILD(expr));

    case and:
        return eval_constant_expression(FIRSTCHILD(expr)) && eval_constant_expression(SECONDCHILD(expr));

    case eq:
        if (strcmp(expr->comp, "==") == 0) {
            return eval_constant_expression(FIRSTCHILD(expr)) == eval_constant_expression(SECONDCHILD(expr));
        }
        else if (strcmp(expr->comp, "!=") == 0) {
            return eval_constant_expression(FIRSTCHILD(expr)) != eval_constant_expression(SECONDCHILD(expr));
        }
        return 0;

    case order:
        if (strcmp(expr->comp, "<") == 0) {
            return eval_constant_expression(FIRSTCHILD(expr)) < eval_constant_expression(SECONDCHILD(expr));
        }
        else if (strcmp(expr->comp, ">") == 0) {
            return eval_constant_expression(FIRSTCHILD(expr)) > eval_constant_expression(SECONDCHILD(expr));
        }
        else if (strcmp(expr->comp, "<=") == 0) {
            return eval_constant_expression(FIRSTCHILD(expr)) <= eval_constant_expression(SECONDCHILD(expr));
        }
        else if (strcmp(expr->comp, ">=") == 0) {
            return eval_constant_expression(FIRSTCHILD(expr)) >= eval_constant_expression(SECONDCHILD(expr));
        }
        return 0;

    case addsub:
        if (SECONDCHILD(expr) == NULL) {
            switch (expr->byte) {
            case '+':
                return eval_constant_expression(FIRSTCHILD(expr));
            case '-':
                return -eval_constant_expression(FIRSTCHILD(expr));
                break;
            default:
                return 0;
            }
        }
        else {
            switch (expr->byte) {
            case '+':
                return eval_constant_expression(FIRSTCHILD(expr)) + eval_constant_expression(SECONDCHILD(expr));
            case '-':
                return eval_constant_expression(FIRSTCHILD(expr)) - eval_constant_expression(SECONDCHILD(expr));
            default:
                return 0;
            }
        }
        return 0;

    case divstar:
        switch (expr->byte) {
        case '*':
            return eval_constant_expression(FIRSTCHILD(expr)) * eval_constant_expression(SECONDCHILD(expr));
        case '/':
            return eval_constant_expression(FIRSTCHILD(expr)) / eval_constant_expression(SECONDCHILD(expr));
        case '%':
            return eval_constant_expression(FIRSTCHILD(expr)) % eval_constant_expression(SECONDCHILD(expr));
        default:
            return 0;
        }

    case num:
        return expr->num;

    case character:;
        return (int)expr->ident[1];

    default:
        return 0;
    }
}

bool compile_switch(Node* instr, FILE* file, Tables* tables) {
    Type type = compile_expression(FIRSTCHILD(instr), file, tables);
    if (type.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", instr->lineno);
        exit(2);
    }
    if (type.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", instr->lineno);
        exit(2);
    }

    Node* body = SECONDCHILD(instr);

    char label_break[25];
    get_new_label(label_break);
    
    int default_count = 0;

    int count = 0;
    for (Node *node = body->firstChild; node != NULL; node = node->nextSibling) {
        if (node->label == case_) {
            count++;
        }
    }

    char* case_values = malloc(sizeof(int) * count);
    if (case_values == NULL) {
        perror("malloc");
        exit(3);
    }

    int i = 0;
    for (Node *node = body->firstChild; node != NULL; node = node->nextSibling) {
        char label_next[25];
        get_new_label(label_next);

        switch (node->label) {
        case case_:;
            verify_constant_expression(FIRSTCHILD(node));
            int n = eval_constant_expression(FIRSTCHILD(node));
            case_values[i] = n;
            i++;

            Type t = compile_expression(FIRSTCHILD(node), file, tables);
            if (t.type != TYPE_PRIMITIF) {
                fprintf(stderr, "Line %d: A primitif type is required here\n", node->lineno);
                exit(2);
            }
            if (t.primitif == TYPE_VOID) {
                fprintf(stderr, "Line %d: this expression can't have void type\n", node->lineno);
                exit(2);
            }

            fprintf(file, 
                "\tpop rcx\n"
                "\tpop rax\n"
                "\tpush rax\n"  // remet dans la pile pour le prochain case
                "\tcmp rax, rcx\n"
                "\tjne %s\n\n",
                label_next
            );
            pop_stack(file);

            compile_switch_instructions(SECONDCHILD(node), file, tables, label_break);

            if (node->nextSibling == NULL) {
                Node* body = SECONDCHILD(node);
                Node* child = FIRSTCHILD(body);

                if (child == NULL) {
                    fprintf(stderr, "Line %d: Last case can't be empty\n", body->lineno);
                    exit(2);
                }
            }

            break;

        case default_:;
            default_count++;
            compile_switch_instructions(FIRSTCHILD(node), file, tables, label_break);

            if (node->nextSibling == NULL) {
                Node* body = FIRSTCHILD(node);
                Node* child = FIRSTCHILD(body);
                if (child == NULL) {
                    fprintf(stderr, "Line %d: Last default can't be empty\n", body->lineno);
                    exit(2);
                }
            }

            break;

        default:
            break;
        }

        fprintf(file, "\t%s:\n", label_next);
    }

    fprintf(file, 
        "\t%s:\n"
        "\tpop rax\n", // enleve de la pile lexpression du switch
        label_break
    );
    pop_stack(file);

    if (default_count > 1) {
        fprintf(stderr, "Line %d: switch must have max 1 default, %d counted\n", instr->lineno, default_count);
        exit(2);
    }

    for (i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (case_values[i] == case_values[j]) {
                fprintf(stderr, "switch expressions must be 2 by 2 distinct, case %d duplicated\n", case_values[i]);
                exit(2);
            }
        }
    }

    free(case_values);

    return false;
}

bool compile_switch_instructions(Node* instr, FILE* file, Tables* tables, char label_break[25]) {
    bool have_returned = false;

    for (Node *child = instr->firstChild; child != NULL; child = child->nextSibling) {
        if (child->label == break_) {
            fprintf(file, " \tjmp %s\n", label_break);
            return false;
        }

        bool returned = compile_instruction(child, file, tables);
        if (returned && !have_returned) {
            if (child->nextSibling != NULL) {
                fprintf(stderr, "Line %d: unreachable instructions\n", child->lineno);
            }
            have_returned = true;
        }
    }

    return have_returned;
}

bool compile_return(Node* instr, FILE* file, Tables* tables) {
    Type function_type = get_type(tables, tables->function_name);

    Node* child = FIRSTCHILD(instr);
    if (child != NULL) {
        Type type = compile_expression(child, file, tables);
        
        if (type.type != TYPE_PRIMITIF) {
            fprintf(stderr, "Line %d: A primitif type is required here\n", instr->lineno);
            exit(2);
        } 
        if (type.primitif == TYPE_VOID) {
            fprintf(stderr, "Line %d: this expression can't have void type\n", instr->lineno);
            exit(2);
        }
        if (function_type.function.return_type == TYPE_VOID) {
            fprintf(stderr, "Warning Line %d: Function %s must return void and something returned\n", instr->lineno, tables->function_name);
        }
        if (type.primitif == TYPE_INT && function_type.function.return_type == TYPE_CHAR) {
            fprintf(stderr, "Warning line %d: Implicit convertion int -> char\n", instr->lineno);
        }

        fprintf(file, "\tpop rax\n\n");
        pop_stack(file);
    }
    else {
        if (function_type.function.return_type != TYPE_VOID) {
            fprintf(stderr, "Warning Line %d: Function %s must return something and nothing returned\n", instr->lineno, tables->function_name);
        } 
    }

    fprintf(file, 
        "\tmov rsp, rbp\n"
        "\tpop rbp\n"
        "\tret\n"
    );
    return true;
}

Type compile_expression(Node* expr, FILE* file, Tables* tables) {
    switch (expr->label) {
    case not:
        return compile_not(expr, file, tables);

    case or:
        return compile_or(expr, file, tables);

    case and:
        return compile_and(expr, file, tables);

    case eq:
        return compile_eq(expr, file, tables);

    case order:
        return compile_order(expr, file, tables);

    case addsub:
        if (SECONDCHILD(expr) == NULL) {
            return compile_unary_addsub(expr, file, tables);
        }
        else {
            return compile_addsub(expr, file, tables);
        }

    case divstar:
        return compile_divstar(expr, file, tables);

    case num:
        return compile_num(expr, file, tables);

    case character:
        return compile_character(expr, file, tables);

    case ident:
        return compile_ident(expr, file, tables);

    case function_call:
        return compile_function_call(expr, file, tables);

    default:
        fprintf(stderr, "Line %d: expression not compiled %s\n", expr->lineno, StringFromLabel[expr->label]);
        break;
    }

    Type expression_type;
    expression_type.type = TYPE_PRIMITIF;   
    expression_type.primitif = TYPE_INT;

    return expression_type;
}

Type compile_not(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    Type t = compile_expression(FIRSTCHILD(expr), file, tables);
    if (t.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (t.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rdi\n"
        "\tmov eax, 0\n"
        "\ttest edi, edi\n"
        "\tsete al\n"
        "\tpush rax\n"
    );
    
    type.primitif = TYPE_INT;
    return type;
}

Type compile_or(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    char label_true[25];
    char label_b[25];
    get_new_label(label_true);
    get_new_label(label_b);

    Type type1 = compile_expression(FIRSTCHILD(expr), file, tables);
    if (type1.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type1.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rax\n"
        "\tcmp rax, 0\n"
        "\tjne %s\n",
        label_true
    );
    pop_stack(file);

    Type type2 = compile_expression(SECONDCHILD(expr), file, tables);
    if (type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tjmp %s\n"
        "\t%s:\n"
        "\tpush 1\n"
        "\t%s:\n",
        label_b,
        label_true,
        label_b
    );
    // push already notified :(
    
    type.primitif = TYPE_INT;
    return type;
}

Type compile_and(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    char label_false[25];
    char label_b[25];
    get_new_label(label_false);
    get_new_label(label_b);

    Type type1 = compile_expression(FIRSTCHILD(expr), file, tables);
    if (type1.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type1.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rax\n"
        "\tcmp rax, 0\n"
        "\tje %s\n",
        label_false
    );
    pop_stack(file);

    Type type2 = compile_expression(SECONDCHILD(expr), file, tables);
    if (type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tjmp %s\n"
        "\t%s:\n"
        "\tpush 0\n"
        "\t%s:\n",
        label_b,
        label_false,
        label_b
    );
    // push already notified :(

    type.primitif = TYPE_INT;
    return type;
}

Type compile_eq(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    char label_true[25];
    char label_false[25];
    get_new_label(label_true);
    get_new_label(label_false);

    Type type1 = compile_expression(FIRSTCHILD(expr), file, tables);
    Type type2 = compile_expression(SECONDCHILD(expr), file, tables);

    if (type1.type != TYPE_PRIMITIF || type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type1.primitif == TYPE_VOID || type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rcx\n"
        "\tpop rax\n"
        "\tcmp rax, rcx\n"
    );
    pop_stack(file);
    pop_stack(file);

    if (strcmp(expr->comp, "==") == 0) {
        fprintf(file, "\tje %s\n", label_true);

    }
    else if (strcmp(expr->comp, "!=") == 0) {
        fprintf(file, "\tjne %s\n", label_true);
    }
    else {
        fprintf(stderr, "unknown eq\n");
        exit(2);
    }

    fprintf(file,
        "\tpush 0\n"
        "\tjmp %s\n"
        "\t%s:\n"
        "\tpush 1\n"
        "\t%s:\n",
        label_false,
        label_true,
        label_false
    );
    push_stack(file);
    
    type.primitif = TYPE_INT;
    return type;
}

Type compile_order(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    char label_true[25];
    char label_false[25];
    get_new_label(label_true);
    get_new_label(label_false);

    Type type1 = compile_expression(FIRSTCHILD(expr), file, tables);
    Type type2 = compile_expression(SECONDCHILD(expr), file, tables);

    if (type1.type != TYPE_PRIMITIF || type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type1.primitif == TYPE_VOID || type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rcx\n"
        "\tpop rax\n"
        "\tcmp rax, rcx\n"
    );
    pop_stack(file);
    pop_stack(file);

    if (strcmp(expr->comp, "<") == 0) {
        fprintf(file, "\tjl %s\n", label_true);
    }
    else if (strcmp(expr->comp, ">") == 0) {
        fprintf(file, "\tjg %s\n", label_true);
    }
    else if (strcmp(expr->comp, "<=") == 0) {
        fprintf(file, "\tjle %s\n", label_true);
    }
    else if (strcmp(expr->comp, ">=") == 0) {
        fprintf(file, "\tjge %s\n", label_true);
    }
    else {
        fprintf(stderr, "Jsp c quoi\n");
        exit(2);
    }

    fprintf(file,
        "\tpush 0\n"
        "\tjmp %s\n"
        "\t%s:\n"
        "\tpush 1\n"
        "\t%s:\n",
        label_false,
        label_true,
        label_false
    );
    push_stack(file);
    
    type.primitif = TYPE_INT;
    return type;
}

Type compile_unary_addsub(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    Type t = compile_expression(FIRSTCHILD(expr), file, tables);
    if (t.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (t.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file, "\tpop rax\n");

    if (expr->byte == '-') {
        fprintf(file, "\tneg rax\n"); // valide ?
    }

    fprintf(file, "\tpush rax\n");


    type.primitif = TYPE_INT;
    return type;
}

Type compile_addsub(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    Node* a = FIRSTCHILD(expr);
    Node* b = SECONDCHILD(expr);

    Type type1 = compile_expression(a, file, tables);
    Type type2 = compile_expression(b, file, tables);

    if (type1.type != TYPE_PRIMITIF || type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type1.primitif == TYPE_VOID || type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rcx\n"
        "\tpop rax\n"
    );
    pop_stack(file);
    pop_stack(file);

    switch (expr->byte) {
    case '+':
        fprintf(file, "\tadd rax, rcx\n");
        break;
    case '-':
        fprintf(file, "\tsub rax, rcx\n");
        break;
    default:
        break;
    }

    fprintf(file, "\tpush rax\n");
    push_stack(file);

    type.primitif = TYPE_INT;
    return type;
}

Type compile_divstar(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    Node* a = FIRSTCHILD(expr);
    Node* b = SECONDCHILD(expr);

    Type type1 = compile_expression(a, file, tables);
    Type type2 = compile_expression(b, file, tables);

    if (type1.type != TYPE_PRIMITIF || type2.type != TYPE_PRIMITIF) {
        fprintf(stderr, "Line %d: A primitif type is required here\n", expr->lineno);
        exit(2);
    }
    if (type1.primitif == TYPE_VOID || type2.primitif == TYPE_VOID) {
        fprintf(stderr, "Line %d: this expression can't have void type\n", expr->lineno);
        exit(2);
    }

    fprintf(file,
        "\tpop rcx\n"
        "\tpop rax\n"
    );
    pop_stack(file);
    pop_stack(file);

    switch (expr->byte) {
    case '*':
        fprintf(file, "\timul rax, rcx\n");
        break;
    case '/':
        fprintf(file, 
            "\tmov rdx, 0\n" // divise rdx:rax par rcx
            "\tidiv rcx\n"
        );
        break;
    case '%':
        fprintf(file, 
            "\tmov rdx, 0\n" // divise rdx:rax par rcx
            "\tidiv rcx\n"
            "\tmov rax, rdx\n"
        );
        break;
    default:
        break;
    }

    fprintf(file, "\tpush rax\n");
    push_stack(file);

    type.primitif = TYPE_INT;
    return type;
}

Type compile_num(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    fprintf(file, "\tpush %d\n", expr->num);
    push_stack(file);
    
    type.primitif = TYPE_INT;
    return type;
}

Type compile_character(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;
    
    fprintf(file, "\tpush %s\n", expr->ident);
    push_stack(file);

    type.primitif = TYPE_CHAR;
    return type;
}

Type compile_ident(Node* expr, FILE* file, Tables* tables) {
    char buffer[25];
    get_string_address(tables, expr->ident, buffer);

    fprintf(file, 
        "\tmov eax, dword [%s]\n"
        "\tpush rax\n", 
        buffer
    );
    push_stack(file);

    return get_type(tables, expr->ident);
}

Type compile_function_call(Node* expr, FILE* file, Tables* tables) {
    Type type;
    type.type = TYPE_PRIMITIF;

    Node* function_name = FIRSTCHILD(expr);
    Type func_type = get_type(tables, function_name->ident);
    if (func_type.type != TYPE_FUNCTION) {
        fprintf(stderr, "Line %d: Variable %s is not a callable function\n", function_name->lineno, function_name->ident);
        exit(2);
    }
    
    Node* params = SECONDCHILD(expr);
    if (params == NULL) {
        if (func_type.function.args_count != 0) {
            fprintf(stderr, "Line %d: Function %s requires %d parameters, 0 given\n", 
                function_name->lineno, function_name->ident, func_type.function.args_count);
            exit(2);
        }
    }
    else {
        int args_count = 0;
        for (Node *child = params->firstChild; child != NULL; child = child->nextSibling) {
            Type t = compile_expression(child, file, tables);
            if (t.type != TYPE_PRIMITIF) {
                fprintf(stderr, "Line %d: A primitif type is required here\n", child->lineno);
                exit(2);
            }
            if (t.primitif == TYPE_VOID) {
                fprintf(stderr, "Line %d: this expression can't have void type\n", child->lineno);
                exit(2);
            }
            if (func_type.function.args_type[args_count] == TYPE_CHAR && t.primitif == TYPE_INT) {
                fprintf(stderr, "Warning line %d: Implicit convertion int -> char\n", child->lineno);
            }

            args_count++;
        }

        if (args_count != func_type.function.args_count) {
            fprintf(stderr, "Line %d: Function %s requires %d parameters, %d given\n", 
                function_name->lineno, function_name->ident, func_type.function.args_count, args_count);
            exit(2);
        }
        
        for (int j = func_type.function.args_count - 1; j > -1; j--) {
            switch (j) {
            case 0:
                fprintf(file, "\tpop rdi\n");
                pop_stack(file);
                break;
            case 1:
                fprintf(file, "\tpop rsi\n");
                pop_stack(file);
                break;
            case 2:
                fprintf(file, "\tpop rdx\n");
                pop_stack(file);
                break;
            case 3:
                fprintf(file, "\tpop rcx\n");
                pop_stack(file);
                break;
            case 4:
                fprintf(file, "\tpop r8\n");
                pop_stack(file);
                break;
            case 5:
                fprintf(file, "\tpop r9\n");
                pop_stack(file);
                break;
            default:
                // already on the stack
                break;
            }
        }
    }

    int required_alignment = get_required_alignment();
    if (required_alignment) {
        fprintf(file, "\tsub rsp, %d\n", required_alignment);
    }

    fprintf(file,  
        "\tcall %s\n",
        function_name->ident
    );

    if (required_alignment) {
        fprintf(file, "\tadd rsp, %d\n", required_alignment);
    }
    push_stack(file);

    fprintf(file, "\tpush rax\n");

    type.primitif = func_type.function.return_type;
    return type;
}