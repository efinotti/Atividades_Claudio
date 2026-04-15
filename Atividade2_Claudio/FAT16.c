#include <stdio.h>
#include <string.h>

// -1 (EOF), -2 (LIVRE), qualquerNumero (apontando para outro)

typedef struct {
    char nomeArquivo[20];
    int tamanho;
    int primeiro_bloco;
} Arquivo;

typedef struct{
    Arquivo arquivos[1000];
    int quantidadeInserida;
} Diretorio;

#define TAMANHO_BLOCOS 512
#define QUANT_BLOCOS 2880

short FAT[QUANT_BLOCOS];
unsigned char disco[QUANT_BLOCOS][TAMANHO_BLOCOS];

Diretorio diretorio;

void verificarFAT(FILE* fp, Arquivo* arquivo, int tamanho, int primeiro, int ultimoIndice);
void inserirLista(FILE* fp, Arquivo* arquivo, int tamanhoArquivo, int indice);

void adicionarDiretorio(Arquivo arquivo) {
    diretorio.arquivos[diretorio.quantidadeInserida] = arquivo;
    diretorio.quantidadeInserida++;
}

void iniciarFAT() {
    for (int i = 0; i < QUANT_BLOCOS; i++) {
        FAT[i] = -2;    
    }
}

FILE* abrirArquivo(const char* caminho, const char* abertura) {
    FILE* fp = fopen(caminho, abertura);

    if (fp != NULL) {
        return fp;
    } else {
        return NULL;
    }
}

int verificarTamanhoArquivo(FILE* fp) {
    rewind(fp);

    char varTemp;
    int contador = 0;
    while (fscanf(fp, "%c", &varTemp) != EOF) {
        contador++;
    }

    return contador;
}

void inserirLista(FILE* fp, Arquivo* arquivo, int tamanhoArquivo, int indice) {
    int contador = 0;

    while (contador < TAMANHO_BLOCOS && fscanf(fp, "%c", &disco[indice][contador]) != EOF) {
        contador++;
    }

    if (tamanhoArquivo > TAMANHO_BLOCOS) {
        verificarFAT(fp, arquivo, tamanhoArquivo - TAMANHO_BLOCOS, 0, indice);
    }
}

void verificarFAT(FILE* fp, Arquivo* arquivo, 
    int tamanho, int primeiro, int ultimoIndice) {
    if (primeiro) {
        rewind(fp);
    }

    int indice;
    int encontrado = 0;
    
    for (int i = 0; i < QUANT_BLOCOS; i++) {
        if (FAT[i] == -2) {
            indice = i;
            encontrado = 1;
            break;
        }
    }

    if (encontrado) {
        if (primeiro) {
            arquivo->primeiro_bloco = indice;
        }

        if (ultimoIndice != -1) {
            FAT[ultimoIndice] = indice;
        }

        FAT[indice] = -1;

        inserirLista(fp, arquivo, tamanho, indice);
    } else {
        printf("DISCO CHEIO!!!");
    }
}

Arquivo criarArquivoDigital(const char* caminho, const char* nome) {
    FILE* fp = abrirArquivo(caminho, "r");

    Arquivo arquivo;
    strcpy(arquivo.nomeArquivo, nome);
    arquivo.tamanho = verificarTamanhoArquivo(fp);
    verificarFAT(fp, &arquivo, arquivo.tamanho, 1, -1);
    adicionarDiretorio(arquivo);
    
    return arquivo;
}

int main() {
    iniciarFAT();

    diretorio.quantidadeInserida = 0;

    Arquivo cap1 =
        criarArquivoDigital("arquivos/CAP1_OnePiece.txt", "CapituloUm");
    return 1;
}