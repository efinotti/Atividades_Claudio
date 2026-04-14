#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char palavra[50];
    int contagem;
} Palavra;

typedef struct {
    int quantidade;
    Palavra palavras[1000000];
    int inicio;
} Lista;

Lista lista;

FILE* criarArquivo(const char* nomeArquivo) {
    FILE* fp = fopen(nomeArquivo, "w+");
    if (fp != NULL) {
        printf("Arquivo aberto!\n");
        return fp;
    } else {
        printf("Arquivo não conseguiu ser aberto!\n");
    }
}

FILE* lerArquivo(const char* nomeArquivo, const char* tipoAbertura) {
    FILE* fp = fopen(nomeArquivo, tipoAbertura);
    if (fp != NULL) {
        printf("Arquivo '%s' aberto!\n", nomeArquivo);
        return fp;
    } else {
        printf("Arquivo não conseguiu ser aberto!\n");
        return NULL;
    }
}

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
}

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
}

int comparadorDePalavras(const char* palavra) {
    for (int i = 0; i < lista.quantidade; i++) {
        if (strcmp(palavra, lista.palavras[i].palavra) == 0) {
            return i;
        }
    }

    return -1;
}
int gerarArquivoCompacto(FILE* fp) {
    FILE* fb = lerArquivo("bibliaCompactada.txt", "w+");

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
}

void traducaoTexto(FILE* fp) {
    int indice;
    while (fscanf(fp, "%d ", &indice) != EOF) {
        printf("%s ", lista.palavras[indice].palavra);
    }
}

void main() {
    char nomeArquivo[50];

    lista.quantidade = 0;
    FILE* fp = lerArquivo("biblia.txt", "r");
    leitorDePalavras(fp);
    rewind(fp);
    gerarArquivoCompacto(fp);

    rewind(fp);
    FILE* fb = lerArquivo("bibliaCompactada.txt", "r");

    traducaoTexto(fb);

    fclose(fp);
    fclose(fb);
}