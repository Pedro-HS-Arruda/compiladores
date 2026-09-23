#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEMA 128

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

    while((c =fgetc(fonte)) != EOF) { // Laço contínuo para ignorar espaços em branco e comentários
        
        if(c == '\n') { //Controle de linha do programa fonte
            linhaAtual++;
        }

        if (isspace(c)) { // identifica espaços em branco, tabulações e quebras de linha
            continue; //pula para a próxima iteração do laço
        }

        //Identificação de comentários usando o peek
        if(c == '/' && peek() == '/') {
            while((c = fgetc(fonte)) != EOF && c != '\n'); // ignora todos os caracteres até encontrar uma quebra de linha
            
            if(c == '\n') { 
                linhaAtual++; // Incrementa o contador de linha ao detectar '\n'
            }
            continue; //Volta para o inicio do laço para continuar a leitura
        }   
        
        // Se o caractere não for espaço em branco, tabulação, quebra de linha ou comentário, ele é parte de um token válido
        break;
    }

    if (c == EOF) { // Se o final do arquivo for atingido, retorna um token de fim de arquivo
        token.type = TOKEN_EOF;
        token.line = linhaAtual;
        return token;
    }

    // (aqui antes eu tinha colocado um "return token;" sem querer, e isso
    // fazia a função já sair pra fora sem nunca chegar no item 3 lá embaixo.
    // apaguei ele, senão nenhum token de verdade era reconhecido)
    token.line = linhaAtual; // Atribui a linha atual ao token antes de retorná-lo

    /* 3. RECONHECIMENTO DE PADRÕES (MÁQUINA DE ESTADOS)
    * [X] Extrair Identificadores e Palavras Reservadas: letras seguidas de letras, números ou underscore.
    * [X] Extrair Números: sequências de dígitos (inteiros) e sequências de dígitos separadas por ponto (reais).
    * [X] Extrair Cadeias de Caracteres: texto delimitado por aspas duplas.
    * [X] Extrair Operadores e Delimitadores: implementar o 'lookahead' (olhar o próximo caractere) para diferenciar símbolos simples ('<', '>') de compostos ('<-', '<=', '>=', '<>').
    */

    if (isalpha(c) || c == '_') {
        char lexema[MAX_LEXEMA];
        int i = 0;
        lexema[i++] = (char)c;

        while (i < MAX_LEXEMA - 1 && (isalnum(peek()) || peek() == '_')) {
            lexema[i++] = (char)fgetc(fonte);
        }
        lexema[i] = '\0';

        token.type = classificarPalavra(lexema);
        if (token.type == TOKEN_ID) {
            token.attribute.table_index = inserirTabelaSimbolos(lexema);
        }
        return token;
    }

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

            if (!isdigit(peek())) {
                lexema[i] = '\0';
                erroLexico(lexema);
            }
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

    // operadores relacionais que já existem no enum: <, <=, =, >, >=
    if (c == '<') {
        if (peek() == '=') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_LE;
            return token;
        }
        // '<-' (atribuição) e '<>' (diferente) eu ainda não sei pra onde
        // mandar, então por enquanto viram erro léxico mesmo
        if (peek() == '-' || peek() == '>') {
            char seq[3] = { '<', (char)fgetc(fonte), '\0' };
            erroLexico(seq);
        }
        token.type = TOKEN_OP_REL;
        token.attribute.op_code = OP_LT;
        return token;
    }

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

    if (c == '=') {
        token.type = TOKEN_OP_REL;
        token.attribute.op_code = OP_EQ;
        return token;
    }

    // qualquer outra coisa (aspas, +, -, :, etc.) eu ainda não sei
    // classificar com o struct que a gente tem, então cai aqui como erro
    {
        char seqInvalida[2] = { (char)c, '\0' };
        erroLexico(seqInvalida);
    }

    return token; // essa linha nunca roda de verdade (erroLexico sempre
                   // encerra o programa antes), mas o compilador exige
                   // que toda função com retorno tenha um return no final
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
//PEGA O PROXIMO TOKEN
void nextToken(void) {
    tokenAtual = obterToken();
}

/*
2. FUNCOES AUXILIARES DE APOIO AO PARSER
[X] Criar uma funcao que confere se o token atual e do tipo esperado e, se for, avanca para o proximo (senao, aciona o erro sintatico).
[X] Criar uma funcao que confere se o token atual e uma palavra reservada especifica (ex: "se", "enquanto", "fimalgoritmo").
[X] Criar uma funcao que confere se o token atual e um delimitador ou operador especifico (ex: '(', ')', ':', ',').
*/
//OLHA SE É O TOKEN ATUAL É O TIPO ESPERADO
int caseToken(TokenNome tipoEsperado){
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
    erroSintatico("Palavra reservada");
    return 0; // CASO CONTRARIO, RETONA
}

//VERIFICA SE O TOKEN ATUAL É UM DELIMITADOR OU OPERADOR ESPECIFICO
int checarDelimitadorOperador(OpRelType opDe){
    if(tokenAtual.type == TOKEN_OP_REL){ // SE O TOKEN ATUAL FOR UM OPERADOR RELACIONAL
        if(tokenAtual.attribute.op_code = opDe){ // SE TOKEN ATUAL FOR UM OPERADOR
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
void fator(void) {
    if (tokenAtual.type == TOKEN_NUM_INT || tokenAtual.type == TOKEN_NUM_FLOAT) {
        nextToken();
    } else if (tokenAtual.type == TOKEN_ID) {
        nextToken();
        // Indexação de vetor [expressao] ou chamada de função (expressao, ...)
        if (tokenAtual.type == TOKEN_OP_REL) { // representa '[', '(' se estendidos
            nextToken();
            expressao();
            while (tokenAtual.type == TOKEN_OP_REL) { // ','
                nextToken();
                expressao();
            }
            nextToken(); // consome ']' ou ')'
        }
    } else if (tokenAtual.type == TOKEN_KEYWORD) { // verdadeiro, falso
        nextToken();
    } else if (tokenAtual.type == TOKEN_OP_REL) { // '(' ou '-' unário
        nextToken();
        if (tokenAtual.type != TOKEN_EOF) {
            expressao();
            nextToken(); // consome ')'
        }
    } else {
        erroSintatico("fator valido (numero, ID, expressao ou valor logico)");
    }
}

// termo -> fator (('*' | '/' | '\' | MOD) fator)*
void termo(void) {
    fator();
    while (tokenAtual.type == TOKEN_KEYWORD /* MOD */ || tokenAtual.type == TOKEN_OP_REL /* *, / */) {
        nextToken();
        fator();
    }
}

// expressao_arit -> termo (('+' | '-') termo)*
void expressaoAritmetica(void) {
    termo();
    while (tokenAtual.type == TOKEN_OP_REL /* +, - */) {
        nextToken();
        termo();
    }
}

// expressao_rel -> expressao_arit (opReal expressao_arit)?
void expressaoRelacional(void) {
    expressaoAritmetica();
    if (tokenAtual.type == TOKEN_OP_REL) {
        nextToken(); // consome <, <=, =, >, >=
        expressaoAritmetica();
    }
}

// expressao -> expressao_e (OU expressao_e)*
void expressao(void) {
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
void comando(void) {
    if (tokenAtual.type == TOKEN_KEYWORD) {
        // Comandos estruturados: leia, escreva, se, enquanto, para, retorne
        nextToken(); // consome a palavra reservada inicial
        
        // Trata parênteses de comandos como leia(...), escreva(...), se(...), enquanto(...)
        if (tokenAtual.type == TOKEN_OP_REL) { // '('
            nextToken(); 
            expressao();
            while (tokenAtual.type == TOKEN_OP_REL) { // ','
                nextToken();
                expressao();
            }
            if (tokenAtual.type == TOKEN_OP_REL) { // ')'
                nextToken();
            }
        } else if (tokenAtual.type == TOKEN_ID || tokenAtual.type == TOKEN_NUM_INT) {
            expressao();
        }

        // Blocos internos de comandos (se/entao, enquanto/faca, para/faca)
        while (tokenAtual.type != TOKEN_KEYWORD && tokenAtual.type != TOKEN_EOF) {
            // Caso contenha comandos internos antes de fechar o bloco (fimse, fimenquanto, etc)
            if (tokenAtual.type == TOKEN_ID || tokenAtual.type == TOKEN_KEYWORD) {
                comando();
            } else {
                break;
            }
        }
    } else if (tokenAtual.type == TOKEN_ID) {
        // Diferenciação entre Atribuição (id <- exp ou id[exp] <- exp) e Chamada de Procedimento (id())
        casaToken(TOKEN_ID);

        // Se houver indexador de vetor: id[expressao]
        if (tokenAtual.type == TOKEN_OP_REL) { 
            nextToken(); // consome '[' ou '('
            expressao();
            if (tokenAtual.type == TOKEN_OP_REL) {
                nextToken(); // consome ']' ou ')'
            }
        }

        // Se for atribuição ('<-')
        if (tokenAtual.type == TOKEN_OP_REL) {
            nextToken(); // consome '<-'
            expressao();
        }
    } else {
        erroSintatico("comando valido");
    }
}

// [X] variavel -> id ('[' expressao ']')?
void variavel(void) {
    casaToken(TOKEN_ID);
    
    // Se o próximo token for '[' (tratado como operador/delimitador)
    if (tokenAtual.type == TOKEN_OP_REL) { 
        nextToken(); // consome '['
        expressao();
        nextToken(); // consome ']'
    }
}

// [X] atribuicao -> variavel '<-' expressao
void atribuicao(void) {
    variavel();
    if (tokenAtual.type == TOKEN_OP_REL) {
        nextToken(); // consome '<-'
    }
    expressao();
}

// [X] leitura -> leia '(' variavel ')'
void leitura(void) {
    checarPalavraReservada(); // consome 'leia'
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome '('
    
    variavel();
    
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome ')'
}

// [X] escrita -> (escreva | escreval) '(' expressao (',' expressao)* ')'
void escrita(void) {
    checarPalavraReservada(); // consome 'escreva' ou 'escreval'
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome '('
    
    expressao();
    while (tokenAtual.type == TOKEN_OP_REL) { // enquanto houver vírgula ','
        nextToken(); // consome ','
        expressao();
    }
    
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // consome ')'
}

// [X] condicional -> se '(' expressao ')' entao comando* (senao comando*)? fimse
void condicional(void) {
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
void repeticaoPara(void) {
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
void repeticaoEnquanto(void) {
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
void chamada(void) {
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
void retorno(void) {
    checarPalavraReservada(); // consome 'retorne'
    expressao();
}

/*
[X] Decisão para diferenciar atribuição de chamada quando ambos começam com ID:
*/
void comando(void) {
    if (tokenAtual.type == TOKEN_KEYWORD) {
        // Identifica o comando através das palavras reservadas
        // (Nota: em um compilador completo, faz-se um switch/if verificando qual keyword é)
        leitura(); // ou escrita(), condicional(), repeticaoPara(), etc.
    } 
    else if (tokenAtual.type == TOKEN_ID) {
        // Como 'atribuicao' e 'chamada' começam com TOKEN_ID,
        // a rotina atribuição trata ambos os fluxos (id <- exp ou id[exp] <- exp).
        // Se não houver '<-', funciona como chamada.
        atribuicao();
    } 
    else {
        erroSintatico("comando valido (atribuicao, leitura, escrita, condicional, repeticao ou chamada)");
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
void tipoBase(void) {
    checarPalavraReservada();
}

// tipo -> tipo_base | vetor '[' num_int '..' num_int ']' de tipo_base
void tipo(void) {
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

void idLista(void) {
    // Consome o primeiro ID
    casaToken(TOKEN_ID);

    // Enquanto houver vírgula separando outros IDs na mesma linha
    while (tokenAtual.type == TOKEN_OP_REL) { // o token de vírgula ',' entra aqui
        nextToken(); // consome a vírgula ','
        casaToken(TOKEN_ID);
    }
}

// [x] declaracao_lista -> id_lista ':' tipo
void declaracaoLista(void) {
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
void parametro(void) {
    casaToken(TOKEN_ID);
    if (tokenAtual.type == TOKEN_OP_REL) nextToken(); // ':'
    tipoBase();
}

// parametros -> parametro (',' parametro)*
void parametros(void) {
    parametro();
    while (tokenAtual.type == TOKEN_OP_REL) { // ','
        nextToken();
        parametro();
    }
}

// declaracao_var -> var declaracao_lista+
void declaracaoVar(void) {
    checarPalavraReservada(); // 'var'

    do {
        // id_lista -> id (',' id)*
        casaToken(TOKEN_ID);
        while (tokenAtual.type == TOKEN_OP_REL) { // ','
            nextToken();
            casaToken(TOKEN_ID);
        }

        // ':'
        if (tokenAtual.type == TOKEN_OP_REL) nextToken();

        // tipo
        tipo();

    } while (tokenAtual.type == TOKEN_ID);
}

// declaracao_procedimento -> procedimento id ('(' parametros ')')? inicio comando* fimprocedimento
void declaracaoProcedimento(void) {
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
void declaracaoFuncao(void) {
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
void algoritmo(void) {
    // 1. Espera 'algoritmo'
    checarPalavraReservada();

    // 2. Espera nome do algoritmo (ID ou CADEIA)
    if (tokenAtual.type == TOKEN_ID) {
        casaToken(TOKEN_ID);
    }

    // 3. Bloco de Declarações (var, procedimento, funcao)
    while (tokenAtual.type == TOKEN_KEYWORD) {
        // Se for 'inicio', encerra a fase de declarações e vai para o corpo
        // (Como não modificamos o léxico, o próximo TOKEN_KEYWORD indica var/procedimento/funcao ou inicio)
        if (tokenAtual.type == TOKEN_KEYWORD) {
            // Se o token for 'var', 'procedimento' ou 'funcao'
            declaracaoVar();
        } else {
            break;
        }
    }

    // 4. Espera 'inicio'
    checarPalavraReservada();

    // 5. Bloco de Comandos
    while (tokenAtual.type != TOKEN_EOF) {
        if (tokenAtual.type == TOKEN_KEYWORD || tokenAtual.type == TOKEN_ID) {
            comando();
        } else {
            break;
        }
    }

    // 6. Espera 'fimalgoritmo'
    checarPalavraReservada();
}

void declaracao(void) {
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
[ ] Abortar imediatamente a execucao do programa (exit) apos identificar o erro.
*/
void erroSintatico(char msg[MAX_LEXEMA]){
    printf("Erro sintático: ", msg);
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

    while (tokenAtual.type != TOKEN_EOF) {
        printf("[Parser Lookahead] Linha %d | Type: %d\n", tokenAtual.line, tokenAtual.type);
        nextToken();
    }

    fecharAnalisador();
}

// esse main() aqui é só pra eu conseguir testar se o lexico tá funcionando.
// ainda não é a versão final (falta formatar do jeito que o item 5 pede
// e salvar num arquivo de log)
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

    analisadorSintatico(arq);

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