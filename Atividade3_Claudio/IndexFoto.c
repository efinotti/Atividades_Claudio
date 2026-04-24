#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Foto {
    char nome[256];

    int ano;
    int mes;
    int dia;

    int hora;
    int minuto;
    int segundo;

    long double tempoTotal;  // Em segundos

    double latitude;
    double longitude;

    struct Foto* proximaFoto;

} Foto;

typedef struct {
    Foto* dados;
    int tamanho;
} ListaFotos;

typedef struct {
    int tagID;
    int tipo;
    int quantidade;
    int offset;
} Metadados;

typedef struct {
    Metadados metadados[50];
    int quantidadeAdicionada;
} ListaMetadados;

typedef struct {
    // 256 pra ter CERTEZA que não vai dar estouro de pilha
    char nomeArquivo[256];

    // Posição inicial (MARCO) do cabeçalho, usado com offset
    long int marcoTiff;

    int tamanhoCabecalho;  // Tamanho total do Cabeçalho
    int littleEndian;      // 1 (Little Endian) : 2 (Big Endian)
    int offsetMetaDados;   // OFFSET Inicial dos Metadados Gerais

    ListaMetadados ifd0;  // Só de TAG de IFD0
    ListaMetadados gps;   // Só de TAG de GPS
    ListaMetadados exif;  // TAG GERALZONA

    int gpsExiste;
    int dataExiste;

} Arquivo;

// Entrada: Nome do Arquivo, Tipo de Leitura
FILE* aberturaArquivo(const char* nomeArquivo, const char* tentativaLeitura) {
    return fopen(nomeArquivo, tentativaLeitura);
}

// Entrada: lista de metadados
void iniciarListaMetadados(ListaMetadados* lista) {
    lista->quantidadeAdicionada = 0;  // A quantidade inicial é 0
}

// Entrada: Lista Específica e dados específicos a serem adicionados
void inserirListaMetadados(ListaMetadados* lista, int tagId, int tipo,
                           int quantidade, int offset) {
    // Sem ordenar, meu objetivo é apenas inserir na lista
    int i = lista->quantidadeAdicionada;

    lista->metadados[i].tagID = tagId;  // IMPORTANTE
    lista->metadados[i].tipo = tipo;
    lista->metadados[i].quantidade = quantidade;
    lista->metadados[i].offset = offset;  // IMPORTANTE

    lista->quantidadeAdicionada++;
}

// Entrada: Arquivo Físico
int encontrarEXIF(FILE* fp, Arquivo* arquivo) {
    unsigned char buffer[2];
    while (fread(buffer, 1, 2, fp) == 2) {
        if (buffer[0] == 0xFF && buffer[1] == 0xE1) {
            // Depois de encontrar APP01, pular dois BYTES
            fseek(fp, 2, SEEK_CUR);

            char id[6];  // Buffer para o ID (EXIF)
            fread(id, 1, 6, fp);
            if (memcmp(id, "Exif\0\0", 6) == 0) {
                /*
                Caso eu encontre, pego um ponteiro específico para o espaço de
                memória após os seis caracteres, que vou chamar de MARCO, visto
                que ele é o começo do cabeçalho
                */
                arquivo->marcoTiff = ftell(fp);
                return 1;
            }
        }
    }
    return 0;
}

// Entrada: Arquivo Digital e Arquivo Físico
void ordemArmazenamento(FILE* fp, Arquivo* arquivo) {
    // Crio um buffer que pega todos os valores do Endian
    unsigned char ordem[2];  // Ajustado para 2 bytes (II ou MM)

    fread(ordem, 1, 2, fp);

    // Caso o primeiro byte seja 0x49 (49), ele é um little endian
    if (ordem[0] == 0x49)
        arquivo->littleEndian = 1;
    else
        arquivo->littleEndian = 0;

    // IMPORTANTE: Pula os 2 bytes "mágicos" (0x002A) que existem no cabeçalho
    // TIFF Sem isso, o próximo fread de offset leria os bytes errados.
    fseek(fp, 2, SEEK_CUR);
}

// Entrada: Arquivo Físico, Arquivo Digital
int lerOffset(FILE* fp, Arquivo* arquivo) {
    // Crio um buffer com os quatro bytes respectivos do OFFSET
    unsigned char offset[4];
    fread(offset, 1, 4, fp);

    // Faço os cálculos pra ler o offset
    if (arquivo->littleEndian)
        return offset[0] | (offset[1] << 8) | (offset[2] << 16) |
               (offset[3] << 24);
    else
        return (offset[0] << 24) | (offset[1] << 16) | (offset[2] << 8) |
               offset[3];
}

// Entrada: Arquivo Físico, e Booleana de Little Endian
int lerQuantidadeTags(FILE* fp, int little) {
    // Crio um buffer para colocar os dois bytes que indicam quantidade tags
    unsigned char n[2];
    fread(n, 1, 2, fp);

    // Realizo o cálculo dependendo do endian
    if (little)
        return n[0] | (n[1] << 8);
    else
        return (n[0] << 8) | n[1];
}

// Entrada: Arquivo, Offset da primeira tag, arquivo digital, lista específica
void lerIFD(FILE* fp, int offset, Arquivo* arquivo, ListaMetadados* lista) {
    // Vou no OFFSET específico (relativo ao Marco TIFF)
    fseek(fp, arquivo->marcoTiff + offset, SEEK_SET);

    // Inicio a lista de metadados especificada
    iniciarListaMetadados(lista);

    // Leio a quantidade de TAGS
    int qtd = lerQuantidadeTags(fp, arquivo->littleEndian);

    // Vejo todas as tags, em uma iteração
    for (int i = 0; i < qtd; i++) {
        unsigned char buffer[12];  // Cada tag tem 12 bytes de tamanho
        fread(buffer, 1, 12, fp);  // Envio toda a tag para o buffer

        int tag, tipo, quantidade, off;

        if (arquivo->littleEndian) {
            tag = buffer[0] | (buffer[1] << 8);
            tipo = buffer[2] | (buffer[3] << 8);

            quantidade = buffer[4] | (buffer[5] << 8) | (buffer[6] << 16) |
                         (buffer[7] << 24);

            off = buffer[8] | (buffer[9] << 8) | (buffer[10] << 16) |
                  (buffer[11] << 24);
        } else {
            tag = (buffer[0] << 8) | buffer[1];
            tipo = (buffer[2] << 8) | buffer[3];

            quantidade = (buffer[4] << 24) | (buffer[5] << 16) |
                         (buffer[6] << 8) | buffer[7];

            off = (buffer[8] << 24) | (buffer[9] << 16) | (buffer[10] << 8) |
                  buffer[11];
        }

        // Insiro na lista específica
        inserirListaMetadados(lista, tag, tipo, quantidade, off);
    }
}

// Entrada: Lista de Metadados e TAG
int procurarTag(ListaMetadados* lista, int tag) {
    for (int i = 0; i < lista->quantidadeAdicionada; i++)
        /*
        Se a tag for igual a pedido, retorna o offset específico para os dados
        */
        if (lista->metadados[i].tagID == tag) return lista->metadados[i].offset;

    return -1;
}

/* ================= DATA ================= */

// Entrada: Buffer da data e Foto
void converterDataEXIF(char* data, Foto* foto) {
    /*
    Uso o sscanf ("feito para mexer com strings") para retirar dados
    dentro da própria formatação da string
    */
    if (sscanf(data, "%d:%d:%d %d:%d:%d", &foto->ano, &foto->mes, &foto->dia,
               &foto->hora, &foto->minuto, &foto->segundo) == 6) {
        foto->tempoTotal = (long double)foto->segundo + (foto->minuto * 60.0) +
                           (foto->hora * 3600.0) + (foto->dia * 86400.0) +
                           (foto->mes * 2628000.0) + (foto->ano * 31536000.0);
    }
}

// Entrada: Arquivo Físico, Arquivo Digital, Foto
void lerDataHora(FILE* fp, Arquivo* arquivo, Foto* foto) {
    // Procuro o offset específico da TAG 0x8769, na lista principal
    int offsetEXIF = procurarTag(&arquivo->ifd0, 0x8769);

    // Se offset de EXIF for -1, indica que data não existe
    if (offsetEXIF == -1) {
        arquivo->dataExiste = 0;
        return;
    }

    // Aqui meu objetivo é colocar dentro da lista exif, todos os metadados
    // (exif)
    lerIFD(fp, offsetEXIF, arquivo, &arquivo->exif);

    /*
    A partir disso, procuro dentro da lista de metadados do EXIF,
    qual é o offset pra eu achar o metadado de Data (cujo valor é 0x9003)
    */
    int offsetData = procurarTag(&arquivo->exif, 0x9003);

    if (offsetData == -1) {
        arquivo->dataExiste = 0;
        return;
    }

    arquivo->dataExiste = 1;

    char data[32];  // Crio um buffer pra colocar a data inteira

    // Vou até o offset de onde está a data e coloco no buffer o valor
    fseek(fp, arquivo->marcoTiff + offsetData, SEEK_SET);
    fread(data, 1, 20, fp);

    // Coloco \0 no final pra eu conseguir tratar como uma string
    data[19] = '\0';

    converterDataEXIF(data, foto);
}

/* ================= GPS ================= */

// Entrada: Arquivo físico, boolean do arquivo ser little endian
double lerRational(FILE* fp, int little) {
    // Crio um buffer pra colocar os valores (oito bytes (feito pra transformar
    // em double))
    unsigned char valorRation[8];

    fread(valorRation, 1, 8, fp);

    // Os valores não podem ser negativos, então eu coloco ele como unsigned
    unsigned int num, den;

    if (little) {  // Verifico se é littleEndian
        num = valorRation[0] | (valorRation[1] << 8) | (valorRation[2] << 16) |
              (valorRation[3] << 24);
        den = valorRation[4] | (valorRation[5] << 8) | (valorRation[6] << 16) |
              (valorRation[7] << 24);
    } else {
        num = (valorRation[0] << 24) | (valorRation[1] << 16) |
              (valorRation[2] << 8) | valorRation[3];
        den = (valorRation[4] << 24) | (valorRation[5] << 16) |
              (valorRation[6] << 8) | valorRation[7];
    }

    if (den == 0) return 0;  // Se for 0, retorna 0

    return (double)num / den;  // Retorno o valor como double
}

// Entrada: Arquivo Físico, Arquivo Digital, Ponteiro para Foto
void lerCoordenadasGPS(FILE* fp, Arquivo* arquivo, Foto* foto) {
    // Procuro o dado da TAG 0x001 dentro da lista gps (referencial "N/S")
    int refLat = procurarTag(&arquivo->gps, 0x0001);

    // Procuro o dado da TAG 0x0002 dentro da lista gps (graus, minutos
    // segundos)
    int lat = procurarTag(&arquivo->gps, 0x0002);

    // Procuro o dado da TAG 0x003 dentro da lista gps (referencial "E/W")
    int refLon = procurarTag(&arquivo->gps, 0x0003);

    // Procuro o dado da TAG 0x0004 dentro da lista gps (graus, minutos
    // segundos)
    int lon = procurarTag(&arquivo->gps, 0x0004);

    // Se for negativo os graus, minutos e segundos, ele retorna
    if (lat == -1 || lon == -1) return;

    // Guardo um caracter referencia (N/S/L/W)
    char refLatChar, refLonChar;

    fseek(fp, arquivo->marcoTiff + refLat, SEEK_SET);
    fread(&refLatChar, 1, 1, fp);

    fseek(fp, arquivo->marcoTiff + refLon, SEEK_SET);
    fread(&refLonChar, 1, 1, fp);

    // Crio double pra guardar as referencias de localização
    double graus, minutos, segundos;

    // Vou até o offset da latitude, primeiramente
    fseek(fp, arquivo->marcoTiff + lat, SEEK_SET);
    graus = lerRational(fp, arquivo->littleEndian);
    minutos = lerRational(fp, arquivo->littleEndian);
    segundos = lerRational(fp, arquivo->littleEndian);

    // Realizo os cálculos pra saber o valor específico da latitude
    double latitude = graus + minutos / 60.0 + segundos / 3600.0;
    if (refLatChar == 'S') latitude *= -1;

    // Vou até o offset específico da longitude
    fseek(fp, arquivo->marcoTiff + lon, SEEK_SET);
    graus = lerRational(fp, arquivo->littleEndian);
    minutos = lerRational(fp, arquivo->littleEndian);
    segundos = lerRational(fp, arquivo->littleEndian);

    // Realizo os cálculos pra saber o valor específico da longitude
    double longitude = graus + minutos / 60.0 + segundos / 3600.0;
    if (refLonChar == 'W') longitude *= -1;

    // Coloco na foto os valores da latitude e longitude
    foto->latitude = latitude;
    foto->longitude = longitude;
}

// Entrada: Arquivo Digital, Arquivo Físico, Foto
void lerGPS(FILE* fp, Arquivo* arquivo, Foto* foto) {
    // Como o o offset de GPS está dentro de ifd0, eu busco
    int offsetGPS = procurarTag(&arquivo->ifd0, 0x8825);

    if (offsetGPS == -1) {
        arquivo->gpsExiste = 0;
        return;
    }

    // Indico que gps existe
    arquivo->gpsExiste = 1;

    // Coloco na lista "gps" no arquivo, todas as tags sobre gps
    lerIFD(fp, offsetGPS, arquivo, &arquivo->gps);

    /*
    Peço para que entre na função, para enviar para "Foto" os dados
    interpretados do GPS (latitude/longitude)
    */
    lerCoordenadasGPS(fp, arquivo, foto);
}

/* ================= LISTA ================= */

// Entrada: Lista de Fotos
void iniciarListaFotos(ListaFotos* listaFotos) {
    // O ponteiro para o primeiro valor é nulo, pois ele não existe
    listaFotos->dados = NULL;
    // Coloca que o tamanho também é 0, por não ter nada
    listaFotos->tamanho = 0;
}

void inserirListaFotos(ListaFotos* listaFotos, Foto* foto) {
    /*
    Verifico se existe um ponteiro para uma foto, se não, ele insere como
    primeiro da lista
    */
    if (listaFotos->dados == NULL) {
        listaFotos->dados = foto;
        foto->proximaFoto = NULL;
    } else {  // Caso exista, ele continua a regra de negócios
        Foto* atual = listaFotos->dados;

        /*
        Se a foto a inserir for mais antiga que a primeira, ele entra na
        primeira posição
        */
        if (foto->tempoTotal < atual->tempoTotal) {
            foto->proximaFoto = atual;
            listaFotos->dados = foto;
        } else {
            /*
            Caminho pela lista até encontrar a posição correta baseada no tempo
            */
            while (atual->proximaFoto != NULL &&
                   atual->proximaFoto->tempoTotal < foto->tempoTotal) {
                atual = atual->proximaFoto;
            }
            foto->proximaFoto = atual->proximaFoto;
            atual->proximaFoto = foto;
        }
    }
    // Adiciona um no tamanho, pois adicionei uma foto.
    listaFotos->tamanho++;
}

// Entrada: Arquivo
Foto* criarFoto(Arquivo* arquivo) {
    // Crio um ponteiro para varíavel foto, e aloco um espaço pra ela
    Foto* foto = malloc(sizeof(Foto));

    // Pego o nome do arquivo e coloco em foto
    strcpy(foto->nome, arquivo->nomeArquivo);

    // Seto tudo como 0/NULL para declarar posteriormente
    foto->ano = foto->mes = foto->dia = 0;
    foto->hora = foto->minuto = foto->segundo = 0;
    foto->latitude = foto->longitude = 0;
    foto->proximaFoto = NULL;
    foto->tempoTotal = 0;

    return foto;
}

/* ================= LEITURA ================= */

// Entrada: Arquivo Digital, Nome do Arquivo, ListaFotos
int leituraArquivo(FILE* fp, const char* nomeArquivo, ListaFotos* listaFotos) {
    Arquivo arquivo;  // Crio o arquivo

    // Pego o nome do arquivo e coloco no arquivo
    strcpy(arquivo.nomeArquivo, nomeArquivo);

    // Tento encontrar o EXIF, se não retorno -1
    if (!encontrarEXIF(fp, &arquivo)) return -1;

    // Verifico o endian e posiciono o ponteiro para o primeiro offset
    ordemArmazenamento(fp, &arquivo);

    // Pego o offset específico da lista inicial de metadados
    arquivo.offsetMetaDados = lerOffset(fp, &arquivo);

    // Envio para receber uma lista com todos os metadados
    lerIFD(fp, arquivo.offsetMetaDados, &arquivo, &arquivo.ifd0);

    // Crio uma foto
    Foto* foto = criarFoto(&arquivo);

    // Leio o GPS e coloco dentro da foto
    lerGPS(fp, &arquivo, foto);
    // Leio a Data e Hora e coloco dentro da foto
    lerDataHora(fp, &arquivo, foto);

    /*
    Se existe data e gps, eu irei inserir na lista. Caso contrário, eu libero a
    foto
    */
    printf("Arquivo: %s | GPS: %s | Data: %s\n", nomeArquivo,
           arquivo.gpsExiste ? "Sim" : "Nao",
           arquivo.dataExiste ? "Sim" : "Nao");

    if (arquivo.dataExiste && arquivo.gpsExiste)
        inserirListaFotos(listaFotos, foto);  // Se existir, insere
    else
        free(foto);  // Se não existir, ele só apaga a foto
    return 0;
}

/* ============= PEGAR OS ARQUIVOS DENTRO DA PASTA ============= */

// Entrada: String do caminho da Pasta, Lista de Fotos
void lerPorPasta(const char* caminhoPasta, ListaFotos* listaFotos) {
    DIR* d;                     // TIPO DIR
    struct dirent* dir;         // Struct dirent
    char caminhoCompleto[512];  // O caminho completo

    d = opendir(caminhoPasta);  // Aqui eu abro o diretório e coloco no DIR (d)

    if (d) {                                  // Se existir um diretório
        while ((dir = readdir(d)) != NULL) {  // Enquanto não for vazio
            // Ignora os diretórios "." e ".."
            if (strcmp(dir->d_name, ".") == 0 ||
                strcmp(dir->d_name, "..") == 0) {
                continue;
            }  // Aqui ele ignora os diretorios ('/..' e '/.')

            // Monta o caminho completo formatando uma string
            snprintf(caminhoCompleto, sizeof(caminhoCompleto), "%s/%s",
                     caminhoPasta, dir->d_name);

            // Abre o arquivo usando o caminho
            FILE* fp = aberturaArquivo(caminhoCompleto, "rb");
            if (!fp) continue;

            printf("\nProcessando: %s\n", dir->d_name);
            leituraArquivo(fp, caminhoCompleto, listaFotos);

            fclose(fp);
        }
        closedir(d);
    } else {
        printf("Erro ao abrir a pasta ou pasta vazia!\n");
    }
}

// Entrada: Lista
void liberarLista(ListaFotos* lista) {
    Foto* atual = lista->dados;
    while (atual != NULL) {  // Vai liberando cada foto de cada vez
        Foto* proximo = atual->proximaFoto;
        free(atual);
        atual = proximo;
    }
}

/* ================= IMPRIMIR LISTA FILTRANDO ================= */

// Entrada: Latitude Inicial/Final, Longitude Inicial/Final e ListaFotos
void buscarFiltrando(int latitudeInicial, int latitudeFinal,
                     int longitudeInicial, int longitudeFinal,
                     ListaFotos* listaFotos) {
    Foto* f = listaFotos->dados;
    while (f != NULL) {  // Enquanto não existir arquivo, ele faz.
        if ((f->latitude > latitudeInicial && f->latitude < latitudeFinal) &&
            (f->longitude > longitudeInicial) &&
            (f->longitude < longitudeFinal)) {
            printf("[%02d/%02d/%d][%02d:%02d:%02d] %s (Lat: %.4f, Lon: %.4f)\n",
                   f->dia, f->mes, f->ano, f->hora, f->minuto, f->segundo,
                   f->nome, f->latitude, f->longitude);
        }
        f = f->proximaFoto;  // Vai pegando as proximas fotos
    }
}

/* ================= MENU INICIAL ================= */

void menuInicial() {
    int longitudeInicial, longitudeFinal;
    int latitudeInicial, latitudeFinal;

    printf("Digite o valor da longitude inicial: ");
    scanf("%lf", &longitudeInicial);
    printf("Digite o valor da longitude final: ");
    scanf("%lf", &longitudeFinal);

    printf("Digite o valor da latitude inicial: ");
    scanf("%lf", &latitudeInicial);
    printf("Digite o valor da latitude final: ");
    scanf("%lf", &latitudeFinal);

    ListaFotos minhaLista;
    iniciarListaFotos(&minhaLista);

    // Lendo a pasta de arquivos
    lerPorPasta("./Arquivos", &minhaLista);

    printf("\nTotal de fotos com metadados validos: %d\n\n",
           minhaLista.tamanho);

    buscarFiltrando(latitudeInicial, latitudeFinal, longitudeInicial,
                    longitudeFinal, &minhaLista);

    liberarLista(&minhaLista);
}

/* ================= MAIN ================= */

int main() {
    menuInicial();
    return 0;
}