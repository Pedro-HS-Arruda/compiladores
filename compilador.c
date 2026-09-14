#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

/*
 *
 * 2. LIMPEZA DE ENTRADA (ESPAÇOS E COMENTÁRIOS)
 * [ ] Implementar a lógica de leitura avançando caractere por caractere.
 * [ ] Descartar espaços em branco, tabulações e quebras de linha (incrementando o contador de linha ao detectar '\n').
 * [ ] Descartar comentários: ao identificar '//', ignorar todos os caracteres seguintes até encontrar uma quebra de linha.
 */

/* 3. RECONHECIMENTO DE PADRÕES (MÁQUINA DE ESTADOS)
 * [ ] Extrair Identificadores e Palavras Reservadas: letras seguidas de letras, números ou underscore.
 * [ ] Extrair Números: sequências de dígitos (inteiros) e sequências de dígitos separadas por ponto (reais).
 * [ ] Extrair Cadeias de Caracteres: texto delimitado por aspas duplas.
 * [ ] Extrair Operadores e Delimitadores: implementar o 'lookahead' (olhar o próximo caractere) para diferenciar símbolos simples ('<', '>') de compostos ('<-', '<=', '>=', '<>').
 */

/* 4. CLASSIFICAÇÃO E RETORNO DE TOKENS
 * [ ] Criar a função que avalia o lexema recém-extraído e define seu tipo.
 * [ ] Garantir que Palavras Reservadas da linguagem (algoritmo, var, inicio, se, enquanto, etc.) tenham prioridade de classificação sobre Identificadores comuns.
 * [ ] Preencher e retornar a struct Token com o tipo, linha e atributo correspondente.
 */

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