#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEMA 128

/* ===================== ENUMS =====================
 * Precisam vir ANTES do struct Token (bug de compilacao da versao anterior:
 * o struct usava TokenNome e OpRelType antes deles existirem).
 */

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,
    TOKEN_NUM_INT,
    TOKEN_NUM_FLOAT,
    TOKEN_OP_REL,
    TOKEN_KEYWORD,
    /* Extensoes necessarias: a gramatica do relatorio (Etapa 1) exige cadeias,
       atribuicao, operadores aritmeticos e delimitadores, que nao apareciam
       no modelo de token da Figura 2. */
    TOKEN_CADEIA,
    TOKEN_ATRIB,
    TOKEN_OP_ARIT,
    TOKEN_DELIM
} TokenNome;

typedef enum {
    OP_LT, // <
    OP_LE, // <=
    OP_EQ, // =   (a gramatica usa '=' como igualdade, nao '==')
    OP_GT, // >
    OP_GE, // >=
    OP_NE  // <>  -- extensao: a gramatica usa '<>' para "diferente" e o modelo original nao previa
} OpRelType;

typedef struct {
    TokenNome type;
    int line;

    union {
        int table_index;
        int int_value;
        double float_value;
        OpRelType op_code;
        char delim_char;   // extensao: operador aritmetico / delimitador
        char *str_value;   // extensao: conteudo de uma cadeia
    } attribute;
} Token;

/* ===================== ITEM 1 (ja pronto) ===================== */

FILE *fonte = NULL;
int linhaAtual = 1;

void iniciarAnalisador(FILE *arquivo) {
    fonte = arquivo;
    linhaAtual = 1;
}

int fimDoArquivo(void) {
    return (fonte == NULL || feof(fonte));
}

void fecharAnalisador(void) {
    if (fonte != NULL) {
        fclose(fonte);
        fonte = NULL;
    }
}

int peek(void) {
    int c = fgetc(fonte);
    if (c != EOF) {
        ungetc(c, fonte);
    }
    return c;
}

/* ===================== Suporte minimo ao item 6 =====================
 * A formatacao/arquivo de log do item 6 ainda NAO esta implementada.
 * Isso aqui e so o essencial para o item 3 conseguir terminar quando
 * encontra algo fora das regras: mensagem, linha, sequencia e exit().
 */
void erroLexico(const char *sequencia) {
    fprintf(stderr, "ERRO LEXICO na linha %d: \"%s\"\n", linhaAtual, sequencia);
    fecharAnalisador();
    exit(1);
}

/* ===================== Suporte minimo ao item 4 (tabela de simbolos) =====================
 * Ainda NAO e a tabela de simbolos de verdade -- e so um contador para o
 * item 3 poder preencher table_index sem quebrar. Precisa ser substituida
 * antes de considerar o item 4 concluido (hoje nao guarda o lexema, nao
 * evita duplicatas, etc.).
 */
int inserirTabelaSimbolos(const char *lexema) {
    static int proximoIndice = 0;
    (void)lexema;
    return proximoIndice++;
}

/* ===================== ITEM 4 (seu codigo, com a lista completa) ===================== */

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

/* ===================== ITEM 2 + ITEM 3 ===================== */

Token obterToken(void) {
    Token token;
    int c;

    /* ---- Item 2 (ja pronto): limpeza de espacos e comentarios ---- */
    while ((c = fgetc(fonte)) != EOF) {
        if (c == '\n') {
            linhaAtual++;
        }
        if (isspace(c)) {
            continue;
        }
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

    /* ---- Item 3: identificadores e palavras reservadas ---- */
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

    /* ---- Item 3: numeros inteiros e reais ---- */
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
            lexema[i++] = (char)fgetc(fonte); // consome o '.'

            if (!isdigit(peek())) {
                lexema[i] = '\0';
                erroLexico(lexema); // ex: "3." sem digito depois nao e valido
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

    /* ---- Item 3: cadeias de caracteres ---- */
    if (c == '"') {
        char lexema[MAX_LEXEMA];
        int i = 0;
        int ch;

        while ((ch = fgetc(fonte)) != EOF && ch != '"' && ch != '\n') {
            if (i < MAX_LEXEMA - 1) {
                lexema[i++] = (char)ch;
            }
        }
        lexema[i] = '\0';

        if (ch != '"') {
            erroLexico(lexema); // cadeia nao fechada antes do fim de linha/arquivo
        }

        token.type = TOKEN_CADEIA;
        token.attribute.str_value = malloc(strlen(lexema) + 1);
        if (token.attribute.str_value != NULL) {
            strcpy(token.attribute.str_value, lexema);
        }
        return token;
    }

    /* ---- Item 3: '<' abre tres possibilidades: <-, <=, <>, < ---- */
    if (c == '<') {
        int prox = peek();
        if (prox == '-') {
            fgetc(fonte);
            token.type = TOKEN_ATRIB;
            return token;
        }
        if (prox == '=') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_LE;
            return token;
        }
        if (prox == '>') {
            fgetc(fonte);
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_NE;
            return token;
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

    /* ---- Item 3: operadores aritmeticos ---- */
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '\\') {
        token.type = TOKEN_OP_ARIT;
        token.attribute.delim_char = (char)c;
        return token;
    }

    /* ---- Item 3: delimitadores, incluindo '..' do vetor ---- */
    if (c == '.') {
        if (peek() == '.') {
            fgetc(fonte);
            token.type = TOKEN_DELIM;
            token.attribute.delim_char = '.';
            return token;
        }
        char seq[2] = { (char)c, '\0' };
        erroLexico(seq); // um '.' sozinho nao existe na gramatica
    }

    if (c == '(' || c == ')' || c == '[' || c == ']' || c == ':' || c == ',') {
        token.type = TOKEN_DELIM;
        token.attribute.delim_char = (char)c;
        return token;
    }

    /* ---- Item 6 (minimo): qualquer coisa que sobrar nao pertence a linguagem ---- */
    {
        char seqInvalida[2] = { (char)c, '\0' };
        erroLexico(seqInvalida);
    }

    return token; // inalcancavel (erroLexico sempre sai), mas exigido pelo compilador
}

/* ===================== main() de teste =====================
 * Temporario, so para voce conseguir compilar e ver os tokens saindo.
 * NAO e o item 5 (formatacao "linha# TOKEN | atributo" + arquivo de log)
 * nem o item 6 completo -- so o suficiente para validar o item 3 agora.
 */
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

    iniciarAnalisador(arq);

    Token t;
    do {
        t = obterToken();
        printf("linha %d -> type=%d\n", t.line, t.type); // saida crua, so para testar
    } while (t.type != TOKEN_EOF);

    fecharAnalisador();
    return 0;
}