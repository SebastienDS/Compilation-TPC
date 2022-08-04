%{

#include "tree.h"
#include "tpcas.tab.h"
#include <stdio.h>
#include <string.h>

%}

%option nounput
%option noinput
%option noyywrap
%option yylineno

%x COMM

%%

"//".* ;
"/*" BEGIN COMM;

<COMM>"*/" BEGIN INITIAL;
<COMM>. ;
<COMM>\n ;


"=" return yytext[0];
"+" {
    yylval.byte = '+';
    return ADDSUB;
}
"-" {
    yylval.byte = '-';
    return ADDSUB;
}
"*" {
    yylval.byte = '*';
    return DIVSTAR;
}
"/" {
    yylval.byte = '/';
    return DIVSTAR;
}
"%" {
    yylval.byte = '%';
    return DIVSTAR;
}
"!" return NOT;
"==" {
    strcpy(yylval.comp, yytext);
    return EQ;
}
"!=" {
    strcpy(yylval.comp, yytext);
    return EQ;
}
"<" {
    strcpy(yylval.comp, yytext);
    return ORDER;
}
">" {
    strcpy(yylval.comp, yytext);
    return ORDER;
}
"<=" {
    strcpy(yylval.comp, yytext);
    return ORDER;
}
">=" {
    strcpy(yylval.comp, yytext);
    return ORDER;
}
"&&" return AND;
"||" return OR;
";" return yytext[0];
"," return yytext[0];
"(" return yytext[0];
")" return yytext[0];
"{" return yytext[0];
"}" return yytext[0];
":" return yytext[0];

int|char {
    strcpy(yylval.ident, yytext);
    return TYPE;
}
void return VOID;

if return IF;
else return ELSE;
while return WHILE;
return return RETURN;

switch return SWITCH;
case return CASE;
break return BREAK;
default return DEFAULT;


"\'\\"['nt]"\'" {
    strcpy(yylval.ident, yytext);
    return CHARACTER;
}
"\'"."\'" {
    strcpy(yylval.ident, yytext);
    return CHARACTER;
}
[0-9]+ {
    yylval.num = atoi(yytext);
    return NUM;
}
[a-zA-Z_][a-zA-Z_0-9]* {
    strcpy(yylval.ident, yytext);
    return IDENT;
}

[\n\t\r ] ;
. { fprintf(stderr, "line %d : lexical error, unexpected symbol '%c'\n", yylineno, yytext[0]); return INVALID_SYMBOL; }


%%