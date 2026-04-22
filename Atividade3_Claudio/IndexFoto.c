#include <stdio.h>
#include <stdlib.h>

#define QTDMAX_FOTO 1000000

#define ANTES_EXIF 12
#define TAMANHO_ESPACO 2

typedef struct {
    int dia;
    int mes;
    int ano;

    int hora;
    int minuto;
} Data;

typedef struct {
    int latitude;
    int longitude;
} LatLong;

typedef struct {
    Data date;
    LatLong local;
} Fotos;

typedef struct {
    Fotos fotos[QTDMAX_FOTO];
} ListaFotos;

FILE* aberturaArquivo(const char* nomeArquivo, const char* tentativaLeitura) {
    FILE* fp = fopen("./JPEG-Baixa-Qualidade.jpg", "rb");
    if (fp == NULL) {
        return NULL;
    } else {
        return fp;
    }
}

FILE* encontrarEXIF(FILE* fp) {
    unsigned char atual;
    unsigned char anterior = 0;
    int encontrado = 0;

    int ch;

    while ((ch = fgetc(fp)) != EOF) {
        atual = (unsigned char)ch;

        if (anterior == 0xFF && atual == 0xE1) {
            encontrado = 1;
            break;
        }
        anterior = atual;
    }

    if (encontrado) {
        printf("Marcador EXIF encontrado!!\n");
    } else {
        printf("Marcador EXIF não encontrado!.\n");
    }

    return fp;
}

void verificarEXID(FILE* fp){
    char id[6];
    fread(id, 1, 6, fp);
    printf("Encontramos o %s!\n",  id);
}

void lerCabecalho(FILE* fp) {
    char ch_cabecalhoTamanho[2];
    fread(ch_cabecalhoTamanho, 2, 1, fp);
    int TAMANHO_CABECALHO = (ch_cabecalhoTamanho[0] << 8) | (ch_cabecalhoTamanho[1] & 0xFF);
    /* Vi no Stack Overflow
    / Precisamos juntar esses dois caracteres alfanumericos e interpretar como um número.
    / Conseguimos fazer isso colocando o valor mais a esquerda (8 bits), por se tratar do valor mais importante
    / O outro, colocamos a direita deste, com esta mascara 0xFF para falar que é um número positivo.
    / (BITWISE)
    */

    printf("TAMANHO CABEÇALHO: %d\n", TAMANHO_CABECALHO);

    verificarEXID(fp);
}

void leituraArquivo(FILE* fp) {
    fp = encontrarEXIF(fp); 
    lerCabecalho(fp);
    
}

int main() {
    FILE* fp = aberturaArquivo("./JPEG-Baixa-Qualidade.jpg", "rb");

    leituraArquivo(fp);

    fclose(fp);
    return 0;
}