#include <stdio.h>
#include <string.h>

// -1 (EOF), -2 (LIVRE), qualquerNumero (apontando para outro)

/*Gero uma estrutura chamada Arquivo, utilizada para guardar os blocos iniciais
  Vou citá-la como arquivo digital (a cópia do arquivo) */
typedef struct {
    char nomeArquivo[20];
    int tamanho;
    int primeiro_bloco;
} Arquivo;

// O diretório é uma lista de Arquivo (no momento, quase uma pilhaKKKKKKKKKK)
typedef struct {
    Arquivo arquivos[1000];
    int quantidadeInserida;
} Diretorio;

// TAMANHO_BLOCOS é o tamanho de cada espaço de armazenamento
// QUANT_BLOCOS é a "quantidade" de indices possíveis para guardar o arquivo
#define TAMANHO_BLOCOS 512
#define QUANT_BLOCOS 2880

// TABELA FAT referencia os indices de cada chunk do arquivo, e seu proximo
// chunk.
short FAT[QUANT_BLOCOS];
unsigned char disco[QUANT_BLOCOS][TAMANHO_BLOCOS];

Diretorio diretorio;

void reescreverTexto(Arquivo arquivo) {
    int indiceInicial = arquivo.primeiro_bloco;
    int indiceAtual = indiceInicial;

    while (indiceAtual != -1) {
        for (int i = 0; i < TAMANHO_BLOCOS; i++) {
            printf("%c", disco[indiceAtual][i]);
        }

        indiceAtual = FAT[indiceAtual];
    }
}

/* Faço a declaração dessas duas funções, por que são recursivas e estavam dando
 problema, por não existirem uma pra outra.*/
void verificarFAT(FILE* fp, Arquivo* arquivo, int tamanho, int primeiro,
                  int ultimoIndice);
void inserirLista(FILE* fp, Arquivo* arquivo, int tamanhoArquivo, int indice);

// Entrada: Arquivo
void adicionarDiretorio(Arquivo arquivo) {
    diretorio.arquivos[diretorio.quantidadeInserida] = arquivo;
    diretorio.quantidadeInserida++;
}  // Saída: Adição de um arquivo no diretório

// Entrada: Varíavel da Tabela FAT
void iniciarFAT() {
    for (int i = 0; i < QUANT_BLOCOS; i++) {
        FAT[i] = -2;
    }
}  // Saída: Declaração de todos os espaços como livre

// Entrada: caminho do arquivo físico, tipo de abertura
FILE* abrirArquivo(const char* caminho, const char* abertura) {
    FILE* fp = fopen(caminho, abertura);

    if (fp != NULL) {
        return fp;
    } else {
        return NULL;
    }
}  // Saída: Retorno do Arquivo Aberto

// Entrada: Arquivo Físico
int verificarTamanhoArquivo(FILE* fp) {
    rewind(fp);

    char varTemp;
    int contador = 0;
    while (fscanf(fp, "%c", &varTemp) != EOF) {
        contador++;
    }

    return contador;
}  // Saída: Tamanho total do arquivo

/* Entrada: ArquivoFísico, Arquivo Digital, Tamanho Restante do Arquivo e o
   índice para o arquivo ser inserido*/
void inserirLista(FILE* fp, Arquivo* arquivo, int tamanhoArquivo, int indice) {
    int contador = 0;

    while (contador < TAMANHO_BLOCOS &&
           fscanf(fp, "%c", &disco[indice][contador]) != EOF) {
        contador++;
    }

    if (tamanhoArquivo > TAMANHO_BLOCOS) {
        verificarFAT(fp, arquivo, tamanhoArquivo - TAMANHO_BLOCOS, 0, indice);
    }
}  // Saída: Arquivo armazenado no FAT / Chama verificarFAT se tamanho for maior


/* Entrada: Arquivo Físico, Arquivo Digital, Tamanho Restante, um valor booleano
    que indica se esse é a primeira iteração do arquivo, e o índice do último
   índice */
void verificarFAT(FILE* fp, Arquivo* arquivo, int tamanho, int primeiro,
                  int ultimoIndice) {
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
}  // Saída:  Retorno do índice do FAT, se não encontrar o Disco está cheio!


// Entrada: Caminho e o Nome para colocar no Arquivo Digital
Arquivo criarArquivoDigital(const char* caminho, const char* nome) {
    FILE* fp = abrirArquivo(caminho, "r");

    Arquivo arquivo;
    strcpy(arquivo.nomeArquivo, nome);
    arquivo.tamanho = verificarTamanhoArquivo(fp);
    verificarFAT(fp, &arquivo, arquivo.tamanho, 1, -1);
    adicionarDiretorio(arquivo);

    return arquivo;
} // Saída: Arquivo Digital Organizado e Armazenado no Disco

int main() {
    iniciarFAT();

    diretorio.quantidadeInserida = 0;

    Arquivo cap1 =
        criarArquivoDigital("arquivos/CAP1_OnePiece.txt", "CapituloUm");

    reescreverTexto(cap1);
    return 1;
}