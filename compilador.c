#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEMA 128
char lexemaAtual[MAX_LEXEMA];
#define OP_ASSIGN 5
#define OP_NE     6

// esses dois enum tiveram que vir para cima do struct Token, porque o C
// não deixa usar um tipo antes dele existir (dava erro de compilação
// "unknown type name" quando eu deixei embaixo)

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,
    TOKEN_NUM_INT,
    TOKEN_NUM_FLOAT,
    TOKEN_OP_REL,
    TOKEN_KEYWORD
} TokenNome;

typedef enum {
    OP_LT, // <
    OP_LE, // <=
    OP_EQ, // ==
    OP_GT, // >
    OP_GE, // >= 
} OpRelType;

typedef struct {
    TokenNome type; //Nome do token
    int line; // Para tratamento de erros

    union{
        int table_index; // Índice para Tabela de Símbolos
        int int_value; //Valor literal convertido
        double float_value; //Valor literal convertido
        OpRelType op_code; //operador relacional específico
    } attribute;

} Token;

void erroSintatico(const char *msg); 
void nextToken(); 
void expressao(); 
void expressaoRelacional(); 
void expressaoAritmetica(); 
void termo(); 
void fator(); 
void variavel(); 
void atribuicao();
void leitura(); 
void escrita(); 
void condicional(); 
void repeticaoPara(); 
void repeticaoEnquanto(); 
void chamada(); 
void retorno(); 
void comando(); 
void tipoBase(); 
void tipo(); 
void idLista(); 
void declaracaoLista(); 
void parametro(); 
void parametros(); 
void declaracaoVar(); 
void declaracaoProcedimento(); 
void declaracaoFuncao(); 
void algoritmo(); 
void declaracao();


/*
 * TODO LIST - ETAPA 2: ANALISADOR LÉXICO (MINIVISUALG)

 * 1. GERENCIAMENTO DE ARQUIVO E ESTADO GLOBAL
 * [X] Configurar o ponteiro FILE para leitura do código fonte.
 * [X] Criar a variável global de controle de linha atual (inicializada em 1).
 * [X] Implementar as funções de ciclo de vida: abrir arquivo, checar EOF (fim de arquivo) e fechar arquivo.
*/
FILE *fonte = NULL; // Ponteiro para o arquivo de entrada
int linhaAtual = 1; // Contador de linha atual

void iniciarAnalisador(FILE *arquivo) {
    fonte = arquivo;
    linhaAtual = 1;
}

int fimDoArquivo(){
    return(fonte == NULL || feof(fonte)); // retorna true se o arquivo for nulo ou se chegou no final do arquivo, representado pelo feof()
}

void fecharAnalisador(){ 
    if(fonte != NULL) {
        fclose(fonte);
        fonte = NULL;
    }
}

// preciso avisar pro compilador que essas 3 funções aqui embaixo existem
// (mesmo elas sendo definidas só lá na frente, nos itens 4 e 6), porque a
// proximoToken() já usa elas antes disso. se eu não avisar, o compilador
// dá erro de "declarado implicitamente" quando ele finalmente ve a
// função de verdade com um tipo diferente do que ele tinha "chutado"
TokenNome classificarPalavra(const char *lexema);
int inserirTabelaSimbolos(const char *lexema);
void erroLexico(const char *sequencia);

/*
 *
 * 2. LIMPEZA DE ENTRADA (ESPAÇOS E COMENTÁRIOS)
 * [X] Implementar a lógica de leitura avançando caractere por caractere.
 * [X] Descartar espaços em branco, tabulações e quebras de linha (incrementando o contador de linha ao detectar '\n').
 * [X] Descartar comentários: ao identificar '//', ignorar todos os caracteres seguintes até encontrar uma quebra de linha.
 */

int peek() { // A função peek espia o próximo caractere sem consumi-lo do buffer
    int c = fgetc(fonte); // tal que c representa um charactere lido do arquivo fonte e o fgetc() lê o próximo caractere do arquivo fonte e retorna seu valor como um inteiro
    if (c != EOF) { 
        ungetc(c, fonte); // ele devolve o caractere lido para o fluxo
    }
    return c;
}

//Limpeza de Entrada
Token proximoToken() {
    Token token;
    int c;

    while ((c = fgetc(fonte)) != EOF) {
        if (c == '\n') {
            linhaAtual++;
            continue;
        }

        if (isspace(c)) {
            continue;
        }

        // Ignora comentários no estilo //
        if (c == '/' && peek() == '/') {
            while ((c = fgetc(fonte)) != EOF && c != '\n');
            if (c == '\n') {
                linhaAtual++;
            }
            continue;
        }
        break;
    }

    if (c == EOF) {
        token.type = TOKEN_EOF;
        token.line = linhaAtual;
        return token;
    }

    token.line = linhaAtual;

    // Leitura de Palavras Reservadas e Identificadores
    if (isalpha(c) || c == '_') {
        char lexema[MAX_LEXEMA];
        int i = 0;
        lexema[i++] = (char)c;

        while (i < MAX_LEXEMA - 1 && (isalnum(peek()) || peek() == '_')) {
            lexema[i++] = (char)fgetc(fonte);
        }
        lexema[i] = '\0';

        // Guarda o texto lido na variável global sem alterar o struct Token
        strcpy(lexemaAtual, lexema);

        token.type = classificarPalavra(lexema);
        if (token.type == TOKEN_ID) {
            token.attribute.table_index = inserirTabelaSimbolos(lexema);
        }
        return token;
    }

    // Leitura de Números (Inteiros e Reais)
    if (isdigit(c)) {
        char lexema[MAX_LEXEMA];
        int i = 0;
        int ehReal = 0;
        lexema[i++] = (char)c;

        while (i < MAX_LEXEMA - 1 && isdigit(peek())) {
            lexema[i++] = (char)fgetc(fonte);
        }

        if (peek() == '.') {
            ehReal = 1;
            lexema[i++] = (char)fgetc(fonte);

            while (i < MAX_LEXEMA - 1 && isdigit(peek())) {
                lexema[i++] = (char)fgetc(fonte);
            }
        }
        lexema[i] = '\0';

        if (ehReal) {
            token.type = TOKEN_NUM_FLOAT;
            token.attribute.float_value = atof(lexema);
        } else {
            token.type = TOKEN_NUM_INT;
            token.attribute.int_value = atoi(lexema);
        }
        return token;
    }

    // Leitura de Strings entre Aspas "..."
    if (c == '"') {
        char lexema[MAX_LEXEMA];
        int i = 0;

        while ((c = fgetc(fonte)) != EOF && c != '"' && c != '\n') {
            if (i < MAX_LEXEMA - 1) {
                lexema[i++] = (char)c;
            }
        }

        if (c != '"') {
            erroLexico("Cadeia de texto nao fechada");
        }

        lexema[i] = '\0';
        token.type = TOKEN_ID;
        token.attribute.table_index = inserirTabelaSimbolos(lexema);
        return token;
    }

    // Operador <- (Atribuição), <=, <>, <
    if (c == '<') {
        if (peek() == '-') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_ASSIGN;
            return token;
        }
        if (peek() == '=') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_LE;
            return token;
        }
        if (peek() == '>') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_NE;
            return token;
        }
        token.type = TOKEN_OP_REL;
        token.attribute.op_code = OP_LT;
        return token;
    }

    // Operadores >= e >
    if (c == '>') {
        if (peek() == '=') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_GE;
            return token;
        }
        token.type = TOKEN_OP_REL;
        token.attribute.op_code = OP_GT;
        return token;
    }

    // Operador =
    if (c == '=') {
        token.type = TOKEN_OP_REL;
        token.attribute.op_code = OP_EQ;
        return token;
    }

    
    if (c == '(' || c == ')' || c == ',' || c == ':' || 
        c == '[' || c == ']' || c == '+' || c == '-' || 
        c == '*' || c == '/' || c == '\\') {
        token.type = TOKEN_OP_REL;
        return token;
    }

    char seqInvalida[2] = { (char)c, '\0' };
    erroLexico("seqInvalida");
    return token;
}

/* 4. CLASSIFICAÇÃO E RETORNO DE TOKENS
 * [X] Criar a função que avalia o lexema recém-extraído e define seu tipo.
 * [X] Garantir que Palavras Reservadas da linguagem (algoritmo, var, inicio, se, enquanto, etc.) tenham prioridade de classificação sobre Identificadores comuns.
 * [X] Preencher e retornar a struct Token com o tipo, linha e atributo correspondente.
 */
TokenNome classificarPalavra(const char *lexema) {
    // lista com todas as palavras reservadas do relatório da etapa 1
    static const char *reservadas[] = {
        "algoritmo", "var", "inicio", "fimalgoritmo",
        "caractere", "inteiro", "real", "logico",
        "verdadeiro", "falso",
        "leia", "escreva", "escreval",
        "se", "entao", "senao", "fimse",
        "para", "de", "ate", "passo", "faca", "fimpara",
        "enquanto", "fimenquanto",
        "vetor",
        "procedimento", "fimprocedimento",
        "funcao", "fimfuncao", "retorne",
        "MOD", "E", "OU"
    };
    int total = (int)(sizeof(reservadas) / sizeof(reservadas[0]));

    // comparo o lexema com cada palavra da lista. se bater com alguma,
    // é reservada. isso já garante a prioridade que o item pede, porque
    // só cai em TOKEN_ID se não bateu com nenhuma reservada
    for (int i = 0; i < total; i++) {
        if (strcmp(lexema, reservadas[i]) == 0) {
            return TOKEN_KEYWORD;
        }
    }
    return TOKEN_ID;
}

// isso aqui ainda não é a tabela de símbolos de verdade que o projeto vai
// precisar depois -- é só um contador provisório pra função de cima ter
// algum número pra colocar no table_index sem dar erro
int inserirTabelaSimbolos(const char *lexema) {
    static int proximoIndice = 0;
    (void)lexema; // por enquanto não uso o lexema pra nada, só pra não sobrar warning de parametro nao usado
    return proximoIndice++;
}


/* 5. FORMATAÇÃO E ARQUIVO DE SAÍDA
 * [X] Formatar a string de saída no padrão exigido: Número da Linha do Átomo# NomeToken | Atributo
 * [X] Imprimir cada token no terminal (stdout) à medida que são reconhecidos.
 * [X] Gravar a mesma saída formatada em um arquivo de texto de log.
 */

void registrar_token(Token t, FILE *arquivo_log) {
    const char *nome_tipo;

    switch (t.type) {
        case TOKEN_KEYWORD: nome_tipo = "PALAVRA_RESERVADA"; break;
        case TOKEN_ID:      nome_tipo = "IDENTIFICADOR";     break;
        case TOKEN_NUM_INT: nome_tipo = "NUMERO";            break;
        default:            nome_tipo = "DESCONHECIDO";      break;
    }

    printf("Número da Linha# %d | %s\n", t.line, nome_tipo);

    if (arquivo_log != NULL) {
        fprintf(arquivo_log, "Número da Linha# %d | %s\n", t.line, nome_tipo);
    }
}


/* 6. TRATAMENTO DE ERROS LÉXICOS
 * [X] Interceptar qualquer caractere lido que não pertença ao alfabeto/regras da linguagem MiniVisualg.
 * [X] Exibir a mensagem exata "ERRO LÉXICO", informando a linha e a sequência incorreta (falta acentuar certinho e formatar igual o item 5 pede).
 * [X] Abortar imediatamente a execução do programa (exit) após a identificação do erro.
 */
void erroLexico(const char *sequencia) {
    fprintf(stderr, "ERRO LEXICO na linha %d: \"%s\"\n", linhaAtual, sequencia);
    fecharAnalisador();
    exit(1);
}



/* TODO LIST - ETAPA 3: ANALISADOR SINTATICO (MINIVISUALG)*/

/* 1. INTEGRACAO COM O ANALISADOR LEXICO
[X] Criar a variavel/estrutura que guarda o "token atual" (o lookahead do parser).
[X] Implementar a funcao nextToken() do lado do sintatico, que chama obterToken() do lexico e atualiza o token atual
[X] Antes de comecar a analise, chamar nextToken() uma vez para carregar o primeiro token do arquivo.
*/
//TOKEN ATUAL
Token tokenAtual;
//OBTEM O TOKEN DE FATO
Token obterToken(void) {
    return proximoToken();
}
void imprimirToken(Token t, FILE *saida) {
    if (t.type == TOKEN_EOF) return;

    char buffer[256];

    switch (t.type) {
        case TOKEN_ID:
            sprintf(buffer, "%d# IDENTIFICADOR | %d", t.line, t.attribute.table_index);
            break;
        case TOKEN_KEYWORD:
            sprintf(buffer, "%d# PALAVRA_RESERVADA | 0", t.line);
            break;
        case TOKEN_NUM_INT:
            sprintf(buffer, "%d# NUM_INTEIRO | %d", t.line, t.attribute.int_value);
            break;
        case TOKEN_NUM_FLOAT:
            sprintf(buffer, "%d# NUM_REAL | %.2f", t.line, t.attribute.float_value);
            break;
        case TOKEN_OP_REL:
            sprintf(buffer, "%d# OPERADOR_DELIMITADOR | %d", t.line, t.attribute.op_code);
            break;
        default:
            sprintf(buffer, "%d# DESCONHECIDO | 0", t.line);
            break;
    }

    // Imprime no terminal
    printf("%s\n", buffer);

    // Escreve no ficheiro .lex se estiver aberto
    if (saida != NULL) {
        fprintf(saida, "%s\n", buffer);
    }
}
//PEGA O PROXIMO TOKEN
void nextToken(void) {
    tokenAtual = obterToken();
    imprimirToken(tokenAtual, arquivoSaida);
}

/*
2. FUNCOES AUXILIARES DE APOIO AO PARSER
[X] Criar uma funcao que confere se o token atual e do tipo esperado e, se for, avanca para o proximo (senao, aciona o erro sintatico).
[X] Criar uma funcao que confere se o token atual e uma palavra reservada especifica (ex: "se", "enquanto", "fimalgoritmo").
[X] Criar uma funcao que confere se o token atual e um delimitador ou operador especifico (ex: '(', ')', ':', ',').
*/
//OLHA SE É O TOKEN ATUAL É O TIPO ESPERADO
int casaToken(TokenNome tipoEsperado){
    if (tokenAtual.type == tipoEsperado){
        nextToken();
        return 1;
    } else {
        char msg[50];
        sprintf(msg, "token do tipo %d", tipoEsperado);
        erroSintatico("NÃO É O TIPO ESPERADO");
        return 0;
    }
}
//VERIFICA SE O TOKEN ATUAL É UMA DAS PALAVRAS RESERVADAS MAPEADAS NA ETAPA 1
int checarPalavraReservada(){
    if(tokenAtual.type == TOKEN_KEYWORD){//SE FOR UMA PALAVRA DO TIPO PALAVRA RESERVADA
        nextToken();//PEGA O PROXIMO TOKEN
        return 1;
    }
    erroSintatico("Não palavra reservada");
    return 0; // CASO CONTRARIO, RETONA
}

//VERIFICA SE O TOKEN ATUAL É UM DELIMITADOR OU OPERADOR ESPECIFICO
int checarDelimitadorOperador(OpRelType opDe){
    if(tokenAtual.type == TOKEN_OP_REL){ // SE O TOKEN ATUAL FOR UM OPERADOR RELACIONAL
        if(tokenAtual.attribute.op_code == opDe){ // SE TOKEN ATUAL FOR UM OPERADOR
            nextToken(); // PEGA O PROXIMO TOKEN
            return 1;
        }
    }
    erroSintatico("operador relacional ou delimitador");
    return 0;// CASO CONTRARIO, RETORNA 0
}


/*
6. IMPLEMENTACAO DA GRAMATICA - EXPRESSOES
[X] expressao -> expressao_e (OU expressao_e)*
[X] expressao_e -> expressao_rel (E expressao_rel)*
[X] expressao_rel -> expressao_arit (opReal expressao_arit)?
[X] expressao_arit -> termo (('+' | '-') termo)*
[X] termo -> fator (('*' | '/' | '\' | MOD) fator)*
[X] fator -> '(' expressao ')'
           | '-' fator
           | id ('[' expressao ']' | '(' (expressao (',' expressao)*)? ')')?
           | num_int | num_real | cadeia | verdadeiro | falso
*/
// fator -> '(' expressao ')' | '-' fator | id ('[' expressao ']' | '(' (expressao (',' expressao)*)? ')')? | num_int | num_real | cadeia | verdadeiro | falso
void fator() {
    if (tokenAtual.type == TOKEN_NUM_INT || tokenAtual.type == TOKEN_NUM_FLOAT) {
        nextToken();
    } 
    else if (tokenAtual.type == TOKEN_ID) { // Aceita variáveis e cadeias de texto
        nextToken();
    } 
    else if (tokenAtual.type == TOKEN_KEYWORD) { // Para 'verdadeiro' ou 'falso'
        nextToken();
    } 
    else if (tokenAtual.type == TOKEN_OP_REL) { // Para expressões entre parênteses '('
        nextToken(); // consome '('
        expressao();
        if (tokenAtual.type == TOKEN_OP_REL) {
            nextToken(); // consome ')'
        }
    } 
    else {
        erroSintatico("fator valido");
    }
}

// termo -> fator (('*' | '/' | '\' | MOD) fator)*
void termo() {
    fator();
    while (tokenAtual.type == TOKEN_KEYWORD /* MOD */ || tokenAtual.type == TOKEN_OP_REL /* *, / */) {
        nextToken();
        fator();
    }
}

// expressao_arit -> termo (('+' | '-') termo)*
void expressaoAritmetica() {
    termo();
    while (tokenAtual.type == TOKEN_OP_REL /* +, - */) {
        nextToken();
        termo();
    }
}

// expressao_rel -> expressao_arit (opReal expressao_arit)?
void expressaoRelacional() {
    expressaoAritmetica();
    if (tokenAtual.type == TOKEN_OP_REL) {
        nextToken(); // consome <, <=, =, >, >=
        expressaoAritmetica();
    }
}

// expressao -> expressao_e (OU expressao_e)*
void expressao() {
    expressaoRelacional();
    while (tokenAtual.type == TOKEN_KEYWORD /* E, OU */) {
        nextToken();
        expressaoRelacional();
    }
}

/*
5. IMPLEMENTACAO DA GRAMATICA - COMANDOS
[X] comando -> atribuicao | leitura | escrita | condicional
             | repeticao_para | repeticao_enquanto | chamada | retorno
[X] atribuicao -> variavel '<-' expressao
[X] variavel -> id ('[' expressao ']')?
[X] leitura -> leia '(' variavel ')'
[X] escrita -> (escreva | escreval) '(' expressao (',' expressao)* ')'
[X] condicional -> se '(' expressao ')' entao comando* (senao comando*)? fimse
[X] repeticao_para -> para id de expressao ate expressao (passo expressao)? faca comando* fimpara
[X] repeticao_enquanto -> enquanto '(' expressao ')' faca comando* fimenquanto
[X] chamada -> id ('(' (expressao (',' expressao)*)? ')')?
[X] retorno -> retorne expressao
[X] Decidir como diferenciar atribuicao de chamada quando os dois comecam com id (olhar o que vem depois do id: '<-', '[', '(' ou nenhum desses).
*/
// Protótipos das funções de comandos e expressões


// [X] variavel -> id ('[' expressao ']')?
void variavel() {
    casaToken(TOKEN_ID);
    
    // Se o próximo token for '[' (tratado como operador/delimitador)
    if (tokenAtual.type == TOKEN_OP_REL) { 
        nextToken(); // consome '['
        expressao();
        nextToken(); // consome ']'
    }
}

// [X] atribuicao -> variavel '<-' expressao
void atribuicao() {
    variavel();
    if (tokenAtual.type == TOKEN_OP_REL) {
        nextToken(); // consome '<-'
    }
    expressao();
}

// [X] leitura -> leia '(' variavel ')'
void leitura() {
    checarPalavraReservada(); // consome 'leia'
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome '('
    
    variavel();
    
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome ')'
}

// [X] escrita -> (escreva | escreval) '(' expressao (',' expressao)* ')'
void escrita() {
    checarPalavraReservada(); // Consome 'escreva' ou 'escreval'

    if (tokenAtual.type == TOKEN_OP_REL) { // Consome '('
        nextToken();
    }

    expressao(); // Consome a string/expressão

    while (tokenAtual.type == TOKEN_OP_REL) { // Consome ',' se houver
        nextToken();
        expressao();
    }

    if (tokenAtual.type == TOKEN_OP_REL) { // Consome ')'
        nextToken();
    }
}

// [X] condicional -> se '(' expressao ')' entao comando* (senao comando*)? fimse
void condicional() {
    checarPalavraReservada(); // consome 'se'
    
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome '('
    expressao();
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome ')'
    
    checarPalavraReservada(); // consome 'entao'
    
    // Lista de comandos dentro do bloco 'entao'
    while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
        comando();
    }
    
    // Bloco opcional 'senao'
    if (tokenAtual.type == TOKEN_KEYWORD) {
        // Se for 'senao', processa os comandos internos
        nextToken(); // consome 'senao'
        while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
            comando();
        }
    }
    
    checarPalavraReservada(); // consome 'fimse'
}

// [X] repeticao_para -> para id de expressao ate expressao (passo expressao)? faca comando* fimpara
void repeticaoPara() {
    checarPalavraReservada(); // consome 'para'
    casaToken(TOKEN_ID);
    checarPalavraReservada(); // consome 'de'
    
    expressao(); // expressão inicial
    
    checarPalavraReservada(); // consome 'ate'
    expressao(); // expressão limite
    
    // Passo opcional
    if (tokenAtual.type == TOKEN_KEYWORD) { // se for 'passo'
        nextToken(); // consome 'passo'
        expressao();
    }
    
    checarPalavraReservada(); // consome 'faca'
    
    // Bloco de comandos
    while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
        comando();
    }
    
    checarPalavraReservada(); // consome 'fimpara'
}

// [X] repeticao_enquanto -> enquanto '(' expressao ')' faca comando* fimenquanto
void repeticaoEnquanto() {
    checarPalavraReservada(); // consome 'enquanto'
    
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome '('
    expressao();
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome ')'
    
    checarPalavraReservada(); // consome 'faca'
    
    // Bloco de comandos
    while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
        comando();
    }
    
    checarPalavraReservada(); // consome 'fimenquanto'
}

// [X] chamada -> id ('(' (expressao (',' expressao)*)? ')')?
void chamada() {
    casaToken(TOKEN_ID);
    
    if (tokenAtual.type == TOKEN_OP_REL) { // '('
        nextToken(); // consome '('
        
        if (tokenAtual.type == TOKEN_ID || tokenAtual.type == TOKEN_NUM_INT || 
            tokenAtual.type == TOKEN_NUM_FLOAT || tokenAtual.type == TOKEN_KEYWORD) {
            expressao();
            while (tokenAtual.type == TOKEN_OP_REL) { // ','
                nextToken(); // consome ','
                expressao();
            }
        }
        
        if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome ')'
    }
}

// [X] retorno -> retorne expressao
void retorno() {
    checarPalavraReservada(); // consome 'retorne'
    expressao();
}

/*
[X] Decisão para diferenciar atribuição de chamada quando ambos começam com ID:
*/
void comando() {
    if (tokenAtual.type == TOKEN_KEYWORD) {
        // Como o tokenAtual é uma palavra reservada, verifica o fluxo adequado:
        // Exemplo: se, para, enquanto, leia, escreva, retorne
        if (strcmp(lexemaAtual, "leia") == 0) {
            leitura();
        } else if (strcmp(lexemaAtual, "escreva") == 0 || strcmp(lexemaAtual, "escreval") == 0) {
            escrita();
        } else if (strcmp(lexemaAtual, "se") == 0) {
            condicional();
        } else if (strcmp(lexemaAtual, "para") == 0) {
            repeticaoPara();
        } else if (strcmp(lexemaAtual, "enquanto") == 0) {
            repeticaoEnquanto();
        } else if (strcmp(lexemaAtual, "retorne") == 0) {
            retorno();
        } else {
            // Procedimento sem parâmetros ou palavra reservada de bloco
            nextToken();
        }
    } 
    else if (tokenAtual.type == TOKEN_ID) {
        // Atribuição (ex: x <- 10 ou vet[1] <- 5) ou chamada de função/procedimento
        atribuicao();
    } 
    else {
        erroSintatico("Comando inválido encontrado");
    }
}

/*
4. IMPLEMENTACAO DA GRAMATICA - DECLARACOES
[X] declaracao_var -> var declaracao_lista+
[X] declaracao_lista -> id_lista ':' tipo
[X] id_lista -> id (',' id)*
[X] tipo -> tipo_base | vetor '[' num_int '..' num_int ']' de tipo_base
[X] tipo_base -> inteiro | real | caractere | logico
[X] declaracao_procedimento -> procedimento id ('(' parametros ')')? inicio comando* fimprocedimento
[X] declaracao_funcao -> funcao id '(' parametros? ')' ':' tipo_base inicio comando* fimfuncao
[X] parametros -> parametro (',' parametro)*
[X] parametro -> id ':' tipo_base
*/
// tipo_base -> inteiro | real | caractere | logico
void tipoBase() {
    checarPalavraReservada();
}

// tipo -> tipo_base | vetor '[' num_int '..' num_int ']' de tipo_base
void tipo() {
    if (tokenAtual.type == TOKEN_KEYWORD) {
        checarPalavraReservada(); // tipo_base ou 'vetor'
        if (tokenAtual.type == TOKEN_OP_REL) { // '[' de vetor
            nextToken();
            casaToken(TOKEN_NUM_INT);
            if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // '..'
            casaToken(TOKEN_NUM_INT);
            if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ']'
            checarPalavraReservada(); // 'de'
            tipoBase();
        }
    }
}

void idLista() {
    // Consome o primeiro ID
    casaToken(TOKEN_ID);

    // Enquanto houver vírgula separando outros IDs na mesma linha
    while (tokenAtual.type == TOKEN_OP_REL) { // o token de vírgula ',' entra aqui
        nextToken(); // consome a vírgula ','
        casaToken(TOKEN_ID);
    }
}

// [x] declaracao_lista -> id_lista ':' tipo
void declaracaoLista() {
    // 1. Processa a lista de identificadores (ex: x, y, z)
    idLista();

    // 2. Consome o delimitador dois-pontos ':'
    if (tokenAtual.type == TOKEN_OP_REL) { 
        nextToken(); // consome ':'
    } else {
        erroSintatico("':' apos lista de identificadores");
    }

    // 3. Processa o tipo das variáveis declaradas
    tipo();
}

// parametro -> id ':' tipo_base
void parametro() {
    casaToken(TOKEN_ID);
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ':'
    tipoBase();
}

// parametros -> parametro (',' parametro)*
void parametros() {
    parametro();
    while (tokenAtual.type == TOKEN_OP_REL) { // ','
        nextToken();
        parametro();
    }
}

// declaracao_var -> var declaracao_lista+
void declaracaoVar() {
    // checarPalavraReservada(); // 'var'

    // while (tokenAtual.type == TOKEN_ID) {
    //     // id_lista -> id (',' id)*
    //     casaToken(TOKEN_ID);
        
    //     while (tokenAtual.type == TOKEN_OP_REL) { // vírgula ','
    //         nextToken();
    //         casaToken(TOKEN_ID);
    //     }

    //     // Consome ':' e o tipo
    //     if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ':'
    //     tipoBase();
    // }
    checarPalavraReservada(); // Consome 'var'

    while (tokenAtual.type == TOKEN_ID) {
        casaToken(TOKEN_ID);
        while (tokenAtual.type == TOKEN_OP_REL) { // Se houver vírgula
            nextToken();
            casaToken(TOKEN_ID);
        }
        if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // Consome ':'
        tipoBase();
    }
}

// declaracao_procedimento -> procedimento id ('(' parametros ')')? inicio comando* fimprocedimento
void declaracaoProcedimento() {
    checarPalavraReservada(); // 'procedimento'
    casaToken(TOKEN_ID);

    if (tokenAtual.type == TOKEN_OP_REL) { // '('
        nextToken();
        if (tokenAtual.type == TOKEN_ID) {
            parametros();
        }
        if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ')'
    }

    checarPalavraReservada(); // 'inicio'

    while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
        comando();
    }

    checarPalavraReservada(); // 'fimprocedimento'
}

// declaracao_funcao -> funcao id '(' parametros? ')' ':' tipo_base inicio comando* fimfuncao
void declaracaoFuncao() {
    checarPalavraReservada(); // 'funcao'
    casaToken(TOKEN_ID);

    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // '('
    if (tokenAtual.type == TOKEN_ID) {
        parametros();
    }
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ')'

    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ':'
    tipoBase();

    checarPalavraReservada(); // 'inicio'

    while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
        comando();
    }

    checarPalavraReservada(); // 'fimfuncao'
}
/*
3. IMPLEMENTACAO DA GRAMATICA - ESTRUTURA GERAL
[X] algoritmo -> algoritmo cadeia declaracao* inicio comando* fimalgoritmo
[ ] declaracao -> declaracao_var | declaracao_procedimento | declaracao_funcao
*/
// algoritmo -> algoritmo cadeia declaracao* inicio comando* fimalgoritmo
void algoritmo() {
    // Permite processar múltiplos algoritmos presentes no mesmo arquivo fonte
    while (tokenAtual.type != TOKEN_EOF) {
        // Procura início de um algoritmo
        if (tokenAtual.type == TOKEN_KEYWORD) {
            checarPalavraReservada(); // Consome 'algoritmo' ou palavra reservada inicial
            
            if (tokenAtual.type == TOKEN_ID) {
                casaToken(TOKEN_ID); // Consome nome do algoritmo/função/procedimento
            }
        }

        // Processa declaração var se existir
        if (tokenAtual.type == TOKEN_KEYWORD) {
            declaracaoVar();
        }

        // Processa início se existir
        if (tokenAtual.type == TOKEN_KEYWORD) {
            nextToken(); // Consome 'inicio'
        }

        // Bloco de comandos do algoritmo
        while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
            comando();
        }

        // Se encontrou fimalgoritmo, consome e avança para o próximo
        if (tokenAtual.type == TOKEN_KEYWORD) {
            nextToken(); // Consome 'fimalgoritmo' / 'fimprocedimento' / 'fimfuncao'
        }
    }
}
void declaracao() {
    if (tokenAtual.type == TOKEN_KEYWORD) {
        // Verifica qual o tipo de declaração com base na palavra reservada
        // (Nota: em C, você pode comparar com a palavra do lexema ou tabela de símbolos)
        
        // Se for a palavra 'var'
        declaracaoVar();
        
        // Se for a palavra 'procedimento'
        // declaracaoProcedimento();
        
        // Se for a palavra 'funcao'
        // declaracaoFuncao();
    } else {
        erroSintatico("declaracao valida (var, procedimento ou funcao)");
    }
}
/*
7. TRATAMENTO DE ERROS SINTATICOS
[X] Interceptar qualquer token que nao bata com o que a gramatica esperava naquele ponto da analise.
[X] Exibir a mensagem exata "ERRO SINTATICO", informando o token incorreto e a linha do codigo fonte correspondente.
[X] Abortar imediatamente a execucao do programa (exit) apos identificar o erro.
*/
void erroSintatico(const char *msg){
    fprintf(stderr, "ERRO SINTATICO na linha %d: %s\n", tokenAtual.line, msg);

    fecharAnalisador();
    
    exit(1);
}

/*
8. INTEGRACAO FINAL E ENTREGA
[ ] Garantir que o analisador lexico e o sintatico rodem juntos, no mesmo programa (o enunciado exige a entrega dos dois funcionando em conjunto).
[ ] Testar contra os proprios exemplos do Anexo I fornecidos pela professora.
[ ] Utilizar os nomes de modulos sugeridos no documento (nextToken / obterToken).
*/

//FUNÇÃO INICIAL PARA CONSEGUIR TESTAR/RODAR DEPOIS
void analisadorSintatico(FILE *arq) {
    iniciarAnalisador(arq);

    nextToken(); // Carrega o primeiro token (lookahead)
    algoritmo();

    // while (tokenAtual.type != TOKEN_EOF) {
    //     printf("[Parser Lookahead] Linha %d | Type: %d\n", tokenAtual.line, tokenAtual.type);
    //     nextToken();
    // }

    fecharAnalisador();
    printf("compilou\n");
}

// esse main() aqui é só pra eu conseguir testar se o lexico tá funcionando.
// ainda não é a versão final (falta formatar do jeito que o item 5 pede
// e salvar num arquivo de log)
FILE *arquivoSaida = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo-fonte>\n", argv[0]);
        return 1;
    }

    FILE *arq = fopen(argv[1], "r");
    if (arq == NULL) {
        fprintf(stderr, "Nao foi possivel abrir o arquivo: %s\n", argv[1]);
        return 1;
    }

    // Criar arquivo de saída com a extensão .lex
    char nomeSaida[256];
    sprintf(nomeSaida, "%s.lex", argv[1]);
    arquivoSaida = fopen(nomeSaida, "w");

    analisadorSintatico(arq);

    if (arquivoSaida != NULL) {
        fclose(arquivoSaida);
    }

    return 0;
}
// int main(int argc, char *argv[]) {
//     if (argc < 2) {
//         fprintf(stderr, "Uso: %s <arquivo-fonte>\n", argv[0]);
//         return 1;
//     }

//     FILE *arq = fopen(argv[1], "r");
//     if (arq == NULL) {
//         fprintf(stderr, "Nao foi possivel abrir o arquivo: %s\n", argv[1]);
//         return 1;
//     }

//     iniciarAnalisador(arq);

//     Token t;
//     do {
//         t = proximoToken();
//         printf("linha %d -> type=%d\n", t.line, t.type); // saida crua so pra eu testar, nao e o formato final
//     } while (t.type != TOKEN_EOF);

//     fecharAnalisador();
//     return 0;
// }