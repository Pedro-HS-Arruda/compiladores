#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEMA 128

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
    /* Prototipos das funcoes dos itens 4 e 6, que o proximoToken() ja usa aqui embaixo antes delas serem definidas de fato mais pra frente no
    * arquivo. Sem isso o compilador "chuta" o tipo (declaracao implicita) e da erro de incompatibilidade quando acha a definicao real depois. */
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
    int i = 0;

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

 return token; // Retorna o token válido encontrado
 
 
 token.line = linhaAtual; // Atribui a linha atual ao token antes de retorná-lo

/* 3. RECONHECIMENTO DE PADRÕES (MÁQUINA DE ESTADOS)
 * [X] Extrair Identificadores e Palavras Reservadas: letras seguidas de letras, números ou underscore.
 * [X] Extrair Números: sequências de dígitos (inteiros) e sequências de dígitos separadas por ponto (reais).
 * [ ] Extrair Cadeias de Caracteres: texto delimitado por aspas duplas.
 * [] Extrair Operadores e Delimitadores: implementar o 'lookahead' (olhar o próximo caractere) para diferenciar símbolos simples ('<', '>') de compostos ('<-', '<=', '>=', '<>').
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
 
    /* Operadores relacionais que EXISTEM no enum: <, <=, =, >, >= */
    if (c == '<') {
        if (peek() == '=') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_LE;
            return token;
        }
        /* '<-' (atribuicao) e '<>' (diferente) -- por ora, erro lexico. */
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
 
    {
        char seqInvalida[2] = { (char)c, '\0' };
        erroLexico(seqInvalida);
    }
 
    return token; // inalcancavel (erroLexico sempre sai), exigido pelo compilador
    
}

/* 4. CLASSIFICAÇÃO E RETORNO DE TOKENS
 * [ ] Criar a função que avalia o lexema recém-extraído e define seu tipo.
 * [ ] Garantir que Palavras Reservadas da linguagem (algoritmo, var, inicio, se, enquanto, etc.) tenham prioridade de classificação sobre Identificadores comuns.
 * [ ] Preencher e retornar a struct Token com o tipo, linha e atributo correspondente.
 */
TokenNome classificarPalavra(const char *lexema) {
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
 
    for (int i = 0; i < total; i++) {
        if (strcmp(lexema, reservadas[i]) == 0) {
            return TOKEN_KEYWORD;
        }
    }
    return TOKEN_ID;
}
 
/* Ainda NAO e a tabela de simbolos de verdade -- so um contador provisorio. */
int inserirTabelaSimbolos(const char *lexema) {
    static int proximoIndice = 0;
    (void)lexema;
    return proximoIndice++;
}


/* 5. FORMATAÇÃO E ARQUIVO DE SAÍDA
 * [ ] Formatar a string de saída no padrão exigido: "Linha# NOME_TOKEN | Atributo".
 * [ ] Imprimir cada token no terminal (stdout) à medida que são reconhecidos.
 * [ ] Gravar a mesma saída formatada em um arquivo de texto de log.
 */

/* 6. TRATAMENTO DE ERROS LÉXICOS
 * [ ] Interceptar qualquer caractere lido que não pertença ao alfabeto/regras da linguagem MiniVisualg.
 * [ ] Exibir a mensagem exata "ERRO LÉXICO", informando a linha e a sequência incorreta.
 * [ ] Abortar imediatamente a execução do programa (exit) após a identificação do erro.
 */
void erroLexico(const char *sequencia) {
    fprintf(stderr, "ERRO LEXICO na linha %d: \"%s\"\n", linhaAtual, sequencia);
    fecharAnalisador();
    exit(1);

}

