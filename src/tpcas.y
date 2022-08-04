%{

#define YYERROR_VERBOSE

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include "tree.h"
#include "SymbolTable.h"
#include "utils.h"

void yyerror(const char *);
int yylex();
int yyparse();

extern int yylineno;

bool print_tree = false;
bool print_tables = false;
Node* tree = NULL;

%}

%token INVALID_SYMBOL
%token ADDSUB DIVSTAR EQ ORDER AND OR NOT
%token TYPE VOID IF ELSE WHILE RETURN CHARACTER NUM IDENT
%token SWITCH CASE BREAK DEFAULT

%union {
    Node* node;
    char byte;
    int num;
    char ident[64];
    char comp[3];
}

%type <node> S Prog DeclVars Declarateurs DeclFoncts DeclFonct EnTeteFonct Parametres ListTypVar Corps SuiteInstr Instr InstrAux
%type <node> Exp TB FB M E T F LValue Arguments ListExp SwitchCorps SwitchCorpsAux CaseCorps


%type <byte> ADDSUB DIVSTAR
%type <num> NUM
%type <ident> CHARACTER IDENT TYPE
%type <comp> ORDER EQ


%expect 1


%%
S: Prog { 
        tree = $$;
    }
    ;
Prog: 
	DeclVars DeclFoncts { 
        $$ = makeNode(PROG);
        Node* decl = makeNode(declarations);
        Node* funcs = makeNode(functions);

        addChild($$, decl);
        addChild($$, funcs);
        
        addChild(decl, $1);
        addChild(funcs, $2);
    }
    ;
DeclVars:
	DeclVars TYPE Declarateurs ';' {
        $$ = $1;
        Node* t = makeNode(type);
        strcpy(t->ident, $2);
        addChild(t, $3);

        if ($$ == NULL) {
            $$ = t;
        } else {
           addSibling($$, t);
        }
    }
    | %empty {
        $$ = NULL;
    }
    ;
Declarateurs:
	Declarateurs ',' IDENT {
        $$ = $1;
        Node* t = makeNode(ident);
        strcpy(t->ident, $3);
        addSibling($$, t);
    }
    |  IDENT {
        $$ = makeNode(ident);
        strcpy($$->ident, $1);
    }
    ;
DeclFoncts:
    DeclFoncts DeclFonct {
        $$ = $1;
        
        Node* func = makeNode(function);    
        addChild(func, $2);

        addSibling($$, func);
    }
    | DeclFonct {
        $$ = makeNode(function);    
        addChild($$, $1);
    }
    ;
DeclFonct:
	EnTeteFonct Corps {
        $$ = makeNode(header);
        addChild($$, $1);
        addSibling($$, $2);
    } 
    ;
EnTeteFonct:
    TYPE IDENT '(' Parametres ')' {
        $$ = makeNode(type);
        strcpy($$->ident, $1);

        Node* var = makeNode(ident);
        strcpy(var->ident, $2);

        addSibling($$, var);
        Node* params = makeNode(parameters);
        addChild(params, $4);
        addSibling($$, params);
    } 
    | VOID IDENT '(' Parametres ')' {
        $$ = makeNode(void_);
        Node* var = makeNode(ident);
        strcpy(var->ident, $2);

        addSibling($$, var);
        Node* params = makeNode(parameters);
        addChild(params, $4);
        addSibling($$, params);
    }
    ;
Parametres:
	VOID {
        $$ = NULL;
    }
    | ListTypVar {
        $$ = $1;
    }
    ;
ListTypVar:
    ListTypVar ',' TYPE IDENT {
        $$ = $1;

        Node* t = makeNode(type);
        strcpy(t->ident, $3);

        Node* var = makeNode(ident);
        strcpy(var->ident, $4);

        addChild(t, var);
        addSibling($$, t);
    } 
    | TYPE IDENT {
        $$ = makeNode(type);
        strcpy($$->ident, $1);

        Node* var = makeNode(ident);
        strcpy(var->ident, $2);

        addChild($$, var);
    }
    ;
Corps:
	'{' DeclVars SuiteInstr '}' {
        $$ = makeNode(body);
        Node* decl = makeNode(declarations);
        Node* instr = makeNode(instructions);

        addChild($$, decl);
        addChild($$, instr);

        addChild(decl, $2); 
        addChild(instr, $3); 
    }
    ;
SuiteInstr:
    SuiteInstr Instr {
        $$ = $1;

        if ($$ == NULL) {
            $$ = $2;
        } else {
            addSibling($$, $2);
        }
    }
    | %empty {
        $$ = NULL;
    }
    ;
Instr:
    InstrAux {
        $$ = $1;
    }
    | '{' SuiteInstr '}' {
        $$ = makeNode(body);
        addChild($$, $2);
    }
    ;
InstrAux:
     LValue '=' Exp ';' {
        $$ = makeNode(assignment);
        addChild($$, $1);
        addChild($$, $3);
    }
    | IF '(' Exp ')' Instr {
        $$ = makeNode(if_);
        addChild($$, $3);
        addChild($$, $5);
    }
    | IF '(' Exp ')' Instr ELSE Instr { 
        $$ = makeNode(if_);
        addChild($$, $3);
        addChild($$, $5);
        Node* e = makeNode(else_);
        addChild($$, e);
        addChild(e, $7);
    }
    | WHILE '(' Exp ')' Instr {
        $$ = makeNode(while_);
        addChild($$, $3);
        addChild($$, $5);
    }
    | IDENT '(' Arguments ')' ';' {
        $$ = makeNode(function_call);
        Node* var = makeNode(ident);
        strcpy(var->ident, $1);
        
        addChild($$, var);
        addChild($$, $3);
    }
    | RETURN Exp ';' {
        $$ = makeNode(return_);
        addChild($$, $2);
    }
    | RETURN ';' {
        $$ = makeNode(return_);
    }
    | ';' {
        $$ = NULL;
    }
    | SWITCH '(' Exp ')' '{' SwitchCorps '}' {
        $$ = makeNode(switch_);
        addChild($$, $3);

        Node* b = makeNode(body);
        addChild($$, b);
        addChild(b, $6);
    }
    ;
SwitchCorps:
    SwitchCorpsAux SwitchCorps {
        $$ = $1;
        addSibling($$, $2);
    }
    | %empty {
        $$ = NULL;
    }
    ;
SwitchCorpsAux:
    CASE Exp ':' CaseCorps {
        $$ = makeNode(case_);
        addChild($$, $2);
        
        Node* b = makeNode(body);
        addChild($$, b);
        addChild(b, $4);
    }
    | DEFAULT ':' CaseCorps {
        $$ = makeNode(default_);

        Node* b = makeNode(body);
        addChild($$, b);
        addChild(b, $3);
    }
    ;
CaseCorps:
    InstrAux CaseCorps {
        $$ = $1;

        if ($$ == NULL) {
            $$ = $2;
        } else {
            addSibling($$, $2);
        }
    }
    | BREAK ';' CaseCorps {
        $$ = makeNode(break_);
        addSibling($$, $3);
    }
    | %empty {
        $$ = NULL;
    }
    ;
Exp:  
	Exp OR TB {
        $$ = makeNode(or);
        addChild($$, $1);
        addChild($$, $3);
    }
    | TB {
        $$ = $1;
    }
    ;
TB: 
	TB AND FB {
        $$ = makeNode(and);
        addChild($$, $1);
        addChild($$, $3);
    }
    | FB {
        $$ = $1;
    }
    ;
FB: 
	FB EQ M {
        $$ = makeNode(eq);
        strcpy($$->comp, $2);
        addChild($$, $1);
        addChild($$, $3);
    }
    | M {
        $$ = $1;
    }
    ;
M:
	M ORDER E {
        $$ = makeNode(order);
        strcpy($$->comp, $2);
        addChild($$, $1);
        addChild($$, $3);
    }
    | E {
        $$ = $1;
    } 
    ;
E:  
	E ADDSUB T {
        $$ = makeNode(addsub);
        $$->byte = $2;
        addChild($$, $1);
        addChild($$, $3);
    }
    | T {
        $$ = $1;
    }
    ;    
T:  
	T DIVSTAR F {
        $$ = makeNode(divstar);
        $$->byte = $2;
        addChild($$, $1);
        addChild($$, $3);
    }
    | F {
        $$ = $1;
    }
    ;
F:  
	ADDSUB F {
        $$ = makeNode(addsub);
        $$->byte = $1;
        addChild($$, $2);
    }
    | NOT F {
        $$ = makeNode(not);
        addChild($$, $2);
    }
    | '(' Exp ')' {
        $$ = $2;
    }
    | NUM {
        $$ = makeNode(num);
        $$->num = $1;
    }
    | CHARACTER {
        $$ = makeNode(character);
        strcpy($$->ident, $1);
    }
    | LValue {
        $$ = $1;
    }
    | IDENT '(' Arguments ')' {
        $$ = makeNode(function_call);

        Node* var = makeNode(ident);
        strcpy(var->ident, $1);

        addChild($$, var);
        addChild($$, $3);
    }
    ;
LValue:
    IDENT {
        $$ = makeNode(ident);
        strcpy($$->ident, $1);
    }
    ;
Arguments:
    ListExp {
        $$ = makeNode(function_call_params);
        addChild($$, $1);
    }
    | %empty {
        $$ = NULL;
    }
    ;
ListExp:
    Exp ',' ListExp {
        $$ = $1;
        addSibling($$, $3);
    }
    | Exp {
        $$ = $1;
    }
    ;

%%

void print_usage() {
    printf("Usage: ./tpcas [OPTION] [FILE.tpc]\n\
    -t, --tree affiche l’arbre abstrait sur la sortie standard\n\
    -h, --help affiche une description de l’interface utilisateur et termine l’exécution\n");
}

void write_file_to_stdin(char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        perror("Could not open file");
        exit(3);
    }

    int fp = fileno(file);

    if (dup2(fp, STDIN_FILENO) == -1) {
        perror("File Descriptor error");
        exit(3);
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"tree", optional_argument, NULL, 't'},
        {"symtabs", optional_argument, NULL, 's'},
        {"help", optional_argument, NULL, 'h'},
        {0, 0, 0, 0},
    };

    int opt;

    while ((opt = getopt_long(argc, argv, "tsh", long_options, NULL )) != -1) {
        switch (opt) {
            case 't': 
                print_tree = true;
                break;
            case 's':
                print_tables = true;
                break;
            case 'h': 
                print_usage();
                return 0;
            default:
                return 2;
        }
    }

    for (; optind < argc; optind++) {     
        char* path = argv[optind];
        write_file_to_stdin(path);
    }      

	int value = yyparse();
    if (tree == NULL || value != 0) return value;

    FILE* file = fopen("bin/_anonymous.asm", "w");
    if (file == NULL) {
        perror("Cannot open file");
        exit(3);
    }

    compile_prog(tree, file);
    fclose(file);

    if (print_tree) {
        printTree(tree, print_tables);
    }

    deleteTree(tree);
    return value;
}

void yyerror(const char* s) {
	fprintf(stderr, "line %d : %s\n", yylineno, s);
}