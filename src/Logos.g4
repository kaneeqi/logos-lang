grammar Logos;

/* 
 _     ________  _ _____ ____ 
/ \   /  __/\  \///  __//  __\
| |   |  \   \  / |  \  |  \/|
| |_/\|  /_  /  \ |  /_ |    /
\____/\____\/__/\\\____\\_/\_\
                               
    Logos - Lenguaje de Logica Proposicional
    Teoria de Compiladores 2025-2
*/

// ============================================
// LEXER RULES
// ============================================

// Comentarios
COMMENT      : '//' ~[\r\n]* -> skip;
BLOCKCOMMENT : '/*' .*? '*/' -> skip;

// Palabras clave
TRUE    : 'true';
FALSE   : 'false';
VAR     : 'var';
CONST   : 'const';
PRINT   : 'print';
IF      : 'if';
ELSE    : 'else';
WHILE   : 'while';
FUNC    : 'func';
RETURN  : 'return';

// Operadores logicos
AND     : '&&';
OR      : '||';
XOR     : '^^';
NOT     : '!';
IMPLIES : '=>';
IFF     : '<=>';  // Si y solo si (bicondicional)

// Delimitadores
LPAREN  : '(';
RPAREN  : ')';
LBRACE  : '{';
RBRACE  : '}';
COMMA   : ',';
ASSIGN  : '=';
SEMI    : ';';

// Identificadores
ID      : [a-zA-Z_][a-zA-Z_0-9]*;

// Espacios en blanco
WS      : [ \t\r\n]+ -> skip;

// ============================================
// PARSER RULES
// ============================================

program
    : statement* EOF
    ;

statement
    : varDecl                           # stmtVarDecl
    | constDecl                         # stmtConstDecl
    | assignStmt                        # stmtAssign
    | printStmt                         # stmtPrint
    | ifStmt                            # stmtIf
    | whileStmt                         # stmtWhile
    | exprStmt                          # stmtExpr
    ;

varDecl
    : VAR ID ASSIGN expr SEMI
    ;

constDecl
    : CONST ID ASSIGN expr SEMI
    ;

assignStmt
    : ID ASSIGN expr SEMI
    ;

printStmt
    : PRINT LPAREN expr RPAREN SEMI
    ;

ifStmt
    : IF LPAREN expr RPAREN block (ELSE block)?
    ;

whileStmt
    : WHILE LPAREN expr RPAREN block
    ;

block
    : LBRACE statement* RBRACE
    | statement
    ;

exprStmt
    : expr SEMI
    ;

// Expresiones con precedencia (de menor a mayor):
// 1. IFF (bicondicional)
// 2. IMPLIES (implicacion)
// 3. OR
// 4. XOR
// 5. AND
// 6. NOT (unario)
// 7. Atomos (literales, ids, parentesis)

expr
    : expr IFF expr                     # exprIff
    | expr IMPLIES expr                 # exprImplies
    | expr OR expr                      # exprOr
    | expr XOR expr                     # exprXor
    | expr AND expr                     # exprAnd
    | NOT expr                          # exprNot
    | LPAREN expr RPAREN                # exprParen
    | TRUE                              # exprTrue
    | FALSE                             # exprFalse
    | ID                                # exprId
    ;
