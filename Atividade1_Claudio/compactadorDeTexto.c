#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Gero uma struct Palavra que vai ser a base para o HashMAP e comparação entre
   Strings*/
typedef struct {
    char palavra[50];
    int contagem;
} Palavra;

/* A lista terá um array de palavras, quantidade de palavras na lista, e quando
 * começa essa lista*/
typedef struct {
    int quantidade;
    Palavra palavras[50000];
    int inicio;
} Lista;

Lista lista;

// Entrada: Caminho/Rota do Arquivo
FILE* criarArquivo(const char* nomeArquivo) {
    FILE* fp = fopen(nomeArquivo, "w+");
    if (fp != NULL) {
        printf("Arquivo aberto!\n");
        return fp;
    } else {
        printf("Arquivo não conseguiu ser aberto!\n");
    }
}  // Saída de Arquivo com Método de Escrever

// Entrada: NomeArquivo
FILE* lerArquivo(const char* nomeArquivo, const char* tipoAbertura) {
    FILE* fp = fopen(nomeArquivo, tipoAbertura);
    if (fp != NULL) {
        printf("Arquivo '%s' aberto!\n", nomeArquivo);
        return fp;
    } else {
        printf("Arquivo não conseguiu ser aberto!\n");
        return NULL;
    }
}  // Saída de Arquivo com Método deLeitura

// Entrada: String[50] caracteres enviado por leitorDePalavras()
void adicionarLista(char string[50]) {
    int contador = 0;
    while (lista.quantidade > contador) {
        if (strcmp(lista.palavras[contador].palavra, string) == 0) {
            lista.palavras[contador].contagem++;
            return;
        }
        contador++;
    }

    strcpy(lista.palavras[lista.quantidade].palavra, string);
    lista.palavras[lista.quantidade].contagem = 1;
    lista.quantidade++;
}  // Saída: Adicionando registro Palavra na lista

// Entrada: Arquivo
void leitorDePalavras(FILE* fp) {
    char varControle;
    char buffer[50];
    int contador = 0;

    memset(buffer, 0, sizeof(buffer));

    while (fscanf(fp, "%c", &varControle) != EOF) {
        if (varControle == ' ' || varControle == '\n' || varControle == '\t') {
            if (contador > 0) {
                buffer[contador] = '\0';
                adicionarLista(buffer);
                contador = 0;
                memset(buffer, 0, sizeof(buffer));
            }
        } else {
            if (varControle != ';' && varControle != ':' &&
                varControle != ',' && varControle != '.' &&
                varControle != '!' && varControle != '?' &&
                varControle != '"' && varControle != '(' &&
                varControle != ')') {
                if (contador < 49) {
                    buffer[contador++] = varControle;
                }
            }
        }
    }

    if (contador > 0) {
        buffer[contador] = '\0';
        adicionarLista(buffer);
    }
}  // Saída: Palavras separadas e formatadas, enviadas para adicionarLista()

// Entrada: Array Varíavel de Palavras
int comparadorDePalavras(const char* palavra) {
    for (int i = 0; i < lista.quantidade; i++) {
        if (strcmp(palavra, lista.palavras[i].palavra) == 0) {
            return i;
        }
    }

    return -1;
}  // Saída: Valor 1 (V) ou 0(F) para comparação entre Strings

// Entrada: Arquivo
int gerarArquivoCompacto(FILE* fp) {
    FILE* fb = lerArquivo("arquivoCompactado.txt", "w+");

    rewind(fp);

    char varControle;
    char buffer[50];
    int contador = 0;

    memset(buffer, 0, sizeof(buffer));

    while (fscanf(fp, "%c", &varControle) != EOF) {
        if (varControle == '\n' || varControle == ' ' || varControle == '\t') {
            if (contador > 0) {
                buffer[contador] = '\0';

                int indice = comparadorDePalavras(buffer);
                fprintf(fb, "%d", indice);

                contador = 0;
                memset(buffer, 0, sizeof(buffer));
            }

            fprintf(fb, "%c", varControle);
        }

        else {
            if (varControle != ';' && varControle != ':' &&
                varControle != ',' && varControle != '.' &&
                varControle != '!' && varControle != '?' &&
                varControle != '"' && varControle != '(' &&
                varControle != ')') {
                if (contador < 49) buffer[contador++] = varControle;
            }
        }
    }

    return 1;
}  // Retorna um novo arquivo escrito com números ao invés de palavras

// Entrada: Arquivo
void traducaoTexto(FILE* fp) {
    int indice;
    while (fscanf(fp, "%d ", &indice) != EOF) {
        printf("%s ", lista.palavras[indice].palavra);
    }
}  // Saída: Retorna a tradução de cada palavra no terminal


//  Entrada: Arquivo
void gerarListaHash(FILE* fp) {
    for (int i = 0; i < lista.quantidade; i++) {
        fprintf(fp, "\t%d \t| \t%s\n", i, lista.palavras[i].palavra);
    }
    fflush(fp);
} /*Saída:  Retorna uma lista HASH em um arquivo, caso o usuário queira verificar
    integridade*/


// Entrada: Valores já colocados na lista
char* maiorPalavra() {
    int maiorValor = 0;
    int maiorIndice;

    for (int i = 0; i < lista.quantidade; i++) {
        if (lista.palavras[i].contagem > maiorValor) {
            maiorValor = lista.palavras[i].contagem;
            maiorIndice = i;
        }
    }

    return lista.palavras[maiorIndice].palavra;
} // Saída: String mais falada




// Entrada: Argumento que espera o caminho para o arquivo
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Erro: VOCÊ ESQUECEU DE COLOCAR OS ARGUMENTOS KKKK\n");
        return 1;
    }

    lista.quantidade = 0;

    FILE* fp = lerArquivo(argv[1], "r");

    if (fp == NULL) {
        return 1;
    }

    leitorDePalavras(fp);
    rewind(fp);
    gerarArquivoCompacto(fp);

    /*
    FILE* fb = lerArquivo("arquivoCompactado.txt", "r");
    if (fb != NULL) {
        traducaoTexto(fb);
        fclose(fb);
    }

    */

    FILE* hashtable = lerArquivo("hashtable.txt", "w+");
    if (hashtable != NULL) {
        gerarListaHash(hashtable);
        fclose(hashtable);
    }

    fclose(fp);
    return 0;
}