/*
* Programa que manipula dados de arquivos,
* permitindo leitura, escrita, impressao,
* remocao, adicao e atualizacao de campos
* e registros escritos em disco.
* Autor: 10734345, Vitor Santana Cordeiro
* Turma: BCC B
* Sao Carlos, SP - Brasil
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "escreverTela.h"

#define TAMPAG 32000  //tamanho da pagina de disco (em bytes)

typedef unsigned char byte; //define o tipo de dados "byte"

typedef struct {  //define o tipo de dados "registro de cabecalho"
    byte status;
    long long topoLista;
    char tagCampo[5];
    char desCampo[5][40];
} regCabec;

typedef struct {  //define o tipo de dados "registro de dados"
    char removido;
    int tamanhoRegistro;
    long long encadeamentoLista; //inteiro de 8 bytes
    int idServidor;
    double salarioServidor;
    char telefoneServidor[14];
    int tamCampo4;  //indicador de tamanho
    char tagCampo4;
    char *nomeServidor; //string de tamanho variavel
    int tamCampo5;  //indicador de tamanho
    char tagCampo5;
    char *cargoServidor;  //string de tamanho variavel
} regDados;

//Funcao para auxiliar no debug do codigo (desconsiderar)
void printRegistro(regDados *registro) {
    printf("\nRemovido: %c\n", registro->removido);
    printf("Tamanho: %d\n", registro->tamanhoRegistro);
    printf("encadeamentoLista: %lld\n", registro->encadeamentoLista);
    printf("Id Servidor: %d\n", registro->idServidor);
    printf("Salario Servidor: %.2lf\n", registro->salarioServidor);
    printf("Telefone Servidor: %s\n", registro->telefoneServidor);

    if (registro->nomeServidor != NULL) {
        printf("-- Tamanho nome: %d\n", registro->tamCampo4);
        printf("-- Tag campo: %c\n", registro->tagCampo4);
        printf("Nome Servidor: %s\n", registro->nomeServidor);
    }
    if (registro->cargoServidor != NULL) {
        printf("-- Tamanho cargo: %d\n", registro->tamCampo5);
        printf("-- Tag campo: %c\n", registro->tagCampo5);
        printf("Cargo Servidor: \"%s\"\n", registro->cargoServidor);
    }
    printf("\n");
}

//Funcao para auxiliar no debug do codigo (desconsiderar)
void printLista(FILE *file) {
    long def = ftell(file);
    long long nextBO;   //ira dizer qual o byte offset do proximo registro da lista
    int tam;

    fseek(file, 1, SEEK_SET);   //posiciono o cabecote de leitura no inicio do campo "topoLista" do registro de cabecalho
    fread(&nextBO, 8, 1, file);  //leio e armazeno o seu valor
    printf("CABEC -> ");

    while (nextBO != -1L) {
        fseek(file, nextBO, SEEK_SET);  //vou para o proximo registro no encadeamento
        printf("%ld ", ftell(file));
        fgetc(file);    //"joga fora" o primeiro byte do registro
        fread(&tam, 4, 1, file);   //le o indicador de tamanho do registro
        printf("(%d) -> ", tam);
        fread(&nextBO, 8, 1, file);   //le o byte offset do proximo registro no encadeamento
    }
    printf("-1\n\n");

    fseek(file, def, SEEK_SET);
}

/*
    Funcao que insere um registro de dados
    na posicao atual do ponteiro de escrita
    do arquivo.

    Parametros:
        FILE *file - arquivo binario a ser modificado
        regDados *registro - registro de dados a ser gravado
*/
void insereRegistro(FILE *file, regDados *registro) {

    fwrite(&(registro->removido), 1, 1, file);  //escrevo o campo "removido" no arquivo binario

    if (registro->tamanhoRegistro == -1) {  //se nao for para sobreescrever o indicador de tamanho...
        fseek(file, 4, SEEK_CUR);   //apenas pulo ele
    } else {
        fwrite(&(registro->tamanhoRegistro), 4, 1, file);  //escrevo o indicador de tamanho do registro no arquivo binario
    }

    fwrite(&(registro->encadeamentoLista), 8, 1, file);  //escrevo o campo "encadeamentoLista" no arquivo binario
    fwrite(&(registro->idServidor), 4, 1, file);  //escrevo o campo "idServidor" no arquivo binario
    fwrite(&(registro->salarioServidor), 8, 1, file);  //escrevo o campo "salarioServidor" no arquivo binario

    if (registro->telefoneServidor[0] == '\0') {   //se o campo "telefoneServidor" for nulo, entao...
        fwrite(registro->telefoneServidor, 1, 1, file);  //escrevo o '\0'
        for (int i = 0; i < 13; i++) fputc('@', file);  //completo o campo com lixo
    } else {
        fwrite(registro->telefoneServidor, 14, 1, file);  //escrevo o campo "telefoneServidor" no arquivo binario
    }

    if (registro->nomeServidor != NULL) {
        fwrite(&(registro->tamCampo4), 4, 1, file);  //escrevo seu indicador de tamanho no arquivo binario
        fwrite(&(registro->tagCampo4), 1, 1, file);  //escrevo sua tag no arquivo binario
        fwrite(registro->nomeServidor, strlen(registro->nomeServidor)+1, 1, file);  //escrevo-o no arquivo binario
        free(registro->nomeServidor);    //libero memoria anteriormente alocada
    }

    if (registro->cargoServidor != NULL) {
        fwrite(&(registro->tamCampo5), 4, 1, file);  //escrevo seu indicador de tamanho no arquivo binario
        fwrite(&(registro->tagCampo5), 1, 1, file);  //escrevo sua tag no arquivo binario
        fwrite(registro->cargoServidor, strlen(registro->cargoServidor)+1, 1, file);  //escrevo-o no arquivo binario
        free(registro->cargoServidor);   //libero memoria anteriormente alocada
    }

    return;
}

/*
    Funcao que le o arquivo CSV e organiza seus
    registros em um arquivo binario de saida,
    utilizando organizacao hibrida dos campos.
*/
void leCSV() {
    char fileName[51];   //vai guardar o nome do arquivo a ser aberto
    scanf("%50s", fileName);

    FILE *readFile = fopen(fileName, "r");  //abro o arquivo de nome "fileName" para leitura
    FILE *writeFile = fopen("arquivoTrab1.bin", "wb");  //crio um novo arquivo binario para escrita

    if (readFile == NULL || writeFile == NULL) {   //erro na abertura dos arquivos
        printf("Falha no carregamento do arquivo.");
        return;
    }

    byte lixo[TAMPAG];    //crio uma "pagina de disco" so com lixo
    memset(lixo, '@', TAMPAG);  //preencho com lixo

    fwrite(lixo, TAMPAG, 1, writeFile); //inicializo a primeira pagina de disco do arquivo com lixo
    fseek(writeFile, -TAMPAG, SEEK_CUR);  //retorno o "ponteiro" de escrita para o inicio da pagina de disco

    regCabec cabecalho;   //crio um registro de cabecalho, utilizando valores definidos na especificacao
    cabecalho.status = '0'; //ao se estar escrevendo em um arquivo, seu status deve ser 0
    cabecalho.topoLista = -1L;
    cabecalho.tagCampo[0] = 'i';
    cabecalho.tagCampo[1] = 's';
    cabecalho.tagCampo[2] = 't';
    cabecalho.tagCampo[3] = 'n';
    cabecalho.tagCampo[4] = 'c';

    //agora, so falta armazenar os metadados
    char c, buffer[100];  //o buffer sera utilizado tanto aqui quanto na hora de ler os registros de dados
    int idx = 0, cont = 0;   //indexador e contador

    while ((c = fgetc(readFile)) != EOF) {   //enquanto nao chegar no final do arquivo...
        if (c == ',' || c == '\n') {   //se achei uma virgula ou uma quebra de linha, cheguei ao fim de um campo
            buffer[idx++] = '\0';   //finalizo a string ate entao lida
            strcpy(cabecalho.desCampo[cont++], buffer);   //guardo o metadado lido no campo correspondente
            idx = 0;  //reinicio o indexador do buffer

            if (c == '\n') break;   //se cheguei no '\n', isso significa que a primeira linha (que contem os metadados) ja foi lida
        }
        else buffer[idx++] = c;  //guardo o char lido no buffer
    }

    fwrite(&(cabecalho.status), 1, 1, writeFile);  //escrevo o campo "status" no arquivo binario
    fwrite(&(cabecalho.topoLista), 8, 1, writeFile);  //escrevo o campo "topoLista" no arquivo binario
    for (int i = 0; i < 5; i++) {
        fwrite(&(cabecalho.tagCampo[i]), 1, 1, writeFile);  //escrevo o campo "tagCampoX" no arquivo binario
        fwrite(cabecalho.desCampo[i], strlen(cabecalho.desCampo[i])+1, 1, writeFile);  //escrevo o campo "desCampoX" no arquivo binario
        fseek(writeFile, 40-strlen(cabecalho.desCampo[i])-1, SEEK_CUR); //como "desCampoX" eh de tamanho fixo, pulo para o comeco do proximo campo (se nao a funcao escreveria os dados em cima do campo anterior)
    }

    fseek(writeFile, TAMPAG, SEEK_SET);   //reposiciono o ponteiro de escrita para ficar no comeco da proxima pagina de disco

    //agora leio o resto do arquivo, criando os registros de dados

    regDados registro;   //molde para guardar temporariamente os valores lidos do csv, antes da sua escrita no arquivo binario
    registro.removido = '-';  //os registros vem "nao-removidos" por padrao
    registro.encadeamentoLista = -1L; //por padrao
    registro.tagCampo4 = cabecalho.tagCampo[3]; //char 'n'
    registro.nomeServidor = malloc(100*sizeof(char));
    registro.tagCampo5 = cabecalho.tagCampo[4]; //char 'c'
    registro.cargoServidor = malloc(100*sizeof(char));

    cont = 0, idx = 0;   //reinicio o contador e o indexador do buffer
    memset(buffer, '\0', 100);  //limpo o buffer
    int ehNulo = 0;   //indica se o campo que esta se lendo eh nulo ou nao
    int tamAntigo = 0;  //guarda o tamanho do ultimo registro escrito no arquivo binario
    int haDados = 0;  //indica se o arquivo CSV possui dados (alem do cabecalho)

    while ((c = fgetc(readFile)) != EOF) {  //enquanto nao chegar no final do arquivo...
        haDados = 1;

        if (c == '\n') {    //se cheguei no '\n', significa que este eh o ultimo campo do registro de dados atual

            if (ehNulo) {
                free(registro.cargoServidor);   //libero a memoria alocada anteriormente para o campo
                registro.cargoServidor = NULL;
            } else {
                buffer[idx] = '\0';   //finalizo a string ate entao lida
                if (registro.cargoServidor == NULL) registro.cargoServidor = malloc(100*sizeof(char));  //aloco memoria para o campo, caso ele nao tenha
                strcpy(registro.cargoServidor, buffer);
                registro.tamCampo5 = strlen(registro.cargoServidor) + 2;  // +2 pois conta 1 byte da tag e 1 byte do '\0' (ele nao eh contabilizado na strlen)
            }

            //calculo o tamanho total do registro e o armazeno
            if (registro.nomeServidor == NULL && registro.cargoServidor == NULL) {
                registro.tamanhoRegistro = 34;
                //34 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor e 14 do telefoneServidor
            }
            else if (registro.cargoServidor == NULL) {
                registro.tamanhoRegistro = 38 + registro.tamCampo4;
                //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 4
            }
            else if (registro.nomeServidor == NULL) {
                registro.tamanhoRegistro = 38 + registro.tamCampo5;
                //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 5
            }
            else {
                registro.tamanhoRegistro = 42 + registro.tamCampo4 + registro.tamCampo5;
                //42 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor, 4 do indicador de tamanho do campo 4 e mais 4 do indicador de tamanho do campo 5
            }

            //verifico o espaco disponivel na pagina de disco atual
            if ( (ftell(writeFile)%TAMPAG + registro.tamanhoRegistro + 5) > TAMPAG ) { //se nao ha espaco suficiente... (+5 por conta do campo "removido" e do indicador de tamanho, que nao sao contabilizados no tamanho do registro)
                //insiro o registro em uma outra pagina de disco, mas antes vou precisar completar a pagina de disco atual com lixo e trocar o indicador de tamanho do ultimo registro dela, para que ele contabilize esse lixo tambem
                int diff = TAMPAG - ftell(writeFile)%TAMPAG;  //quantidade necessaria de lixo para completar a pagina de disco

                fseek(writeFile, - (tamAntigo+4), SEEK_CUR); //movo o ponteiro de escrita para o inicio do indicador de tamanho do ultimo registro inserido
                int tamNovo = diff + tamAntigo;  //calculo o valor do novo indicador de tamanho
                fwrite(&tamNovo, 4, 1, writeFile); //sobreescrevo o valor do indicador de tamanho antigo pelo novo

                fseek(writeFile, tamAntigo, SEEK_CUR);  //coloco o ponteiro no final do ultimo registro
                fwrite(lixo, 1, diff, writeFile);  //completo com lixo
            }

            insereRegistro(writeFile, &registro);

            cont = 0; //reinicio o contador
            idx = 0;  //reinicio o indexador do buffer
            memset(buffer, '\0', 100);  //limpo o buffer
            tamAntigo = registro.tamanhoRegistro; //atualizo o tamanho do ultimo registro inserido
        }

        else if (c == ',') {

            switch (cont) {
                case 0: //o campo que li foi o idServidor
                    registro.idServidor = atoi(buffer); //converto o valor lido para inteiro e o armazeno
                    break;
                case 1: //o campo que li foi o salarioServidor
                    registro.salarioServidor = atof(buffer);  //converto o valor lido para double e o armazeno
                    break;
                case 2: //o campo que li foi o telefoneServidor
                    if (ehNulo) {
                        strcpy(registro.telefoneServidor, "\0");
                    } else {
                        for (int i = 0; i < 14; i++) registro.telefoneServidor[i] = buffer[i];
                    }
                    break;
                case 3: //o campo que li foi o nomeServidor
                    if (ehNulo) {
                        free(registro.nomeServidor);  //libero a memoria alocada anteriormente para o campo
                        registro.nomeServidor = NULL;
                    } else {
                        buffer[idx] = '\0';   //finalizo a string ate entao lida
                        if (registro.nomeServidor == NULL) registro.nomeServidor = malloc(100*sizeof(char));  //aloco memoria para o campo, caso ele nao tenha
                        strcpy(registro.nomeServidor, buffer);
                        registro.tamCampo4 = strlen(registro.nomeServidor) + 2;  // +2 pois conta 1 byte da tag e 1 byte do '\0' (ele nao eh contabilizado na strlen)
                    }
            }

            cont++;
            idx = 0;  //reinicio o indexador do buffer
            memset(buffer, '\0', 100);  //limpo o buffer
            ehNulo = 1;   //faz com que, se o proximo char lido for outra virgula, o loop saiba que aquele campo tem valor nulo
        }

        else {
            buffer[idx++] = c;
            ehNulo = 0;   //nao foi outra virgula, entao o valor do campo nao eh nulo
        }

    }

    if (haDados) {
        /* registro os dados da ultima linha do csv (ja que ela nao possui o '\n' no final) */

        //calculo o tamanho total do registro e o armazeno
        if (registro.nomeServidor == NULL && registro.cargoServidor == NULL) {
            registro.tamanhoRegistro = 34;
            //34 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor e 14 do telefoneServidor
        }
        else if (registro.cargoServidor == NULL) {
            registro.tamanhoRegistro = 38 + registro.tamCampo4;
            //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 4
        }
        else if (registro.nomeServidor == NULL) {
            registro.tamanhoRegistro = 38 + registro.tamCampo5;
            //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 5
        }
        else {
            registro.tamanhoRegistro = 42 + registro.tamCampo4 + registro.tamCampo5;
            //42 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor, 4 do indicador de tamanho do campo 4 e mais 4 do indicador de tamanho do campo 5
        }

        //verifico o espaco disponivel na pagina de disco atual
        if ( (ftell(writeFile)%TAMPAG + registro.tamanhoRegistro + 5) > TAMPAG ) { //se nao ha espaco suficiente... (+5 por conta do campo "removido" e do indicador de tamanho, que nao sao contabilizados no tamanho do registro)
            //insiro o registro em uma outra pagina de disco, mas antes vou precisar completar a pagina de disco atual com lixo e trocar o indicador de tamanho do ultimo registro dela, para que ele contabilize esse lixo tambem
            int diff = TAMPAG - ftell(writeFile)%TAMPAG;  //quantidade necessaria de lixo para completar a pagina de disco

            fseek(writeFile, - (tamAntigo+4), SEEK_CUR); //move o ponteiro de escrita para o inicio do indicador de tamanho do ultimo registro inserido
            int tamNovo = diff + tamAntigo;  //calculo o valor do novo indicador de tamanho
            fwrite(&tamNovo, 4, 1, writeFile); //sobreescrevo o valor do indicador de tamanho antigo pelo novo

            fseek(writeFile, tamAntigo, SEEK_CUR);  //coloco o ponteiro no final do ultimo registro
            fwrite(lixo, 1, diff, writeFile);  //completo com lixo
        }

        insereRegistro(writeFile, &registro);
    }

    printf("arquivoTrab1.bin");
    fclose(readFile);

    //antes de fechar o arquivo de escrita, coloco seu status para '1'
    fseek(writeFile, 0, SEEK_SET);  //coloco o ponteiro de escrita no primeiro byte do arquivo
    cabecalho.status = '1';
    fwrite(&(cabecalho.status), 1, 1, writeFile);  //sobrescrevo o campo "status" do arquivo binario
    fclose(writeFile);

    return;
}

/*
    Funcao que imprime na tela, organizadamente,
    um registro de um arquivo binario gerado
    anteriormente por este programa. A funcao
    assume que o usuario ira chama-la quando
    o ponteiro de leitura estiver exatamente no
    comeco do registro.

    Parametro:
        FILE *file - arquivo binario
*/
void mostraRegistro(FILE *file) {
    regDados registro;  //registro que ajudara a guardar os dados lidos
    int tamanho;    //ajudara a contar quantos bytes faltam para terminar o registro

    fread(&tamanho, 4, 1, file);    //leio os 4 bytes do indicador de tamanho do registro e os armazeno em "tamanho"
    fseek(file, 8, SEEK_CUR);   //pulo os 8 bytes do "encadeamentoLista", pois este campo nao sera mostrado
    tamanho -= 8;

    fread(&(registro.idServidor), 4, 1, file);   //leio os 4 bytes do campo "idServidor"
    tamanho -= 4;
    printf("%d", registro.idServidor);   //mostro o valor do campo "idServidor" na tela

    fread(&(registro.salarioServidor), 8, 1, file);   //leio os 8 bytes do campo "salarioServidor"
    tamanho -= 8;
    if (registro.salarioServidor != -1.0) printf(" %.2lf", registro.salarioServidor);   //mostro o valor do campo "salarioServidor" na tela
    else printf("         ");   //se o campo for nulo, mostro 8 espacos em branco

    fread(registro.telefoneServidor, 14, 1, file);   //leio os 14 bytes do campo "telefoneServidor"
    tamanho -= 14;
    if (registro.telefoneServidor[0] != '\0') printf(" %.14s", registro.telefoneServidor);   //mostro o valor do campo "telefoneServidor" na tela
    else printf("               "); //se o campo for nulo, mostro 14 espacos em branco

    if (tamanho == 0) {     //se ja cheguei no fim do registro...
        printf("\n");   //termino de mostrar o registro
        return;
    }

    byte b = fgetc(file);   //leio o byte apontado pelo ponteiro de leitura
    if (b == '@') {    //o registro eh o ultimo de uma pagina de disco
        printf("\n");   //termino de mostrar o registro
        fseek(file, tamanho-1, SEEK_CUR);   //reposiciono o ponteiro de leitura para ficar logo depois do final desse registro
        return;
    }
    ungetc(b, file);    //"devolvo" o byte lido para o arquivo

    //chegando neste ponto, sabemos que pelo menos um dos campos (nomeServidor ou cargoServidor) existe
    //OBS: nao temos certeza de qual, mas, como na hora de mostrar isso eh indiferente, ignoramos a especificidade

    fread(&(registro.tamCampo4), 4, 1, file);   //leio os 4 bytes do indicador de tamanho do campo
    tamanho -= 4;
    printf(" %d", registro.tamCampo4 - 2);  //mostro quantos caracteres o campo possui (-2 por conta do '\0' e da tag)

    fgetc(file);    //pulo a tag do campo
    tamanho -= 1;

    registro.nomeServidor = malloc(100*sizeof(char));
    //como dito, aqui estou guardando como se fosse o campo "nomeServidor", mas na verdade pode ser qualquer um dos dois
    fread(registro.nomeServidor, 1, registro.tamCampo4-1, file);    //leio os bytes do campo
    tamanho -= (registro.tamCampo4-1);  //-1 pois se retirou a tag da contagem
    printf(" %s", registro.nomeServidor);

    //testamos mais uma vez para ver se acabou o registro ou nao

    if (tamanho == 0) {     //se ja cheguei no fim do registro...
        printf("\n");   //termino de mostrar o registro
        free(registro.nomeServidor);    //libero memoria anteriormente alocada
        return;
    }

    b = fgetc(file);   //leio o byte apontado pelo ponteiro de leitura
    if (b == '@') {    //o registro eh o ultimo de uma pagina de disco
        printf("\n");   //termino de mostrar o registro
        fseek(file, tamanho-1, SEEK_CUR);   //reposiciono o ponteiro de leitura para ficar logo depois do final desse registro
        free(registro.nomeServidor);    //libero memoria anteriormente alocada
        return;
    }
    ungetc(b, file);    //"devolvo" o byte lido para o arquivo

    //agora sim, chegando a este ponto, sabemos que o campo previamente lido foi o "nomeServidor"

    fread(&(registro.tamCampo5), 4, 1, file);   //leio os 4 bytes do indicador de tamanho do campo "cargoServidor"
    tamanho -= 4;
    printf(" %d", registro.tamCampo5 - 2);  //mostro quantos caracteres o campo possui (-2 por conta do '\0' e da tag)

    fgetc(file);    //pulo a tag do campo
    tamanho -= 1;

    registro.cargoServidor = malloc(100*sizeof(char));
    fread(registro.cargoServidor, 1, registro.tamCampo5-1, file);    //leio os bytes do campo
    tamanho -= (registro.tamCampo5-1);  //-1 pois se retirou a tag da contagem
    printf(" %s", registro.cargoServidor);

    printf("\n");   //termino de mostrar o registro

    if (tamanho != 0) fseek(file, tamanho, SEEK_CUR);   //se o registro ainda nao terminou (provavelmente porque eh o ultimo registro de uma pagina de disco), reposiciono o ponteiro de leitura para ficar logo depois do final desse registro

    free(registro.nomeServidor);    //libero memoria anteriormente alocada
    free(registro.cargoServidor);   //libero memoria anteriormente alocada

    return;
}

/*
    Le um arquivo binario (anteriormente gerado pelo programa)
    e mostra na tela todos os seus registros, organizadamente.
    Ao final, mostra quantas paginas de disco foram acessadas
    ao todo.
*/
void mostraBin() {
    char fileName[51];   //vai guardar o nome do arquivo a ser aberto
    scanf("%50s", fileName);

    FILE *readFile = fopen(fileName, "rb");  //abre o arquivo "fileName" para leitura binária

    if (readFile == NULL) {   //erro na abertura do arquivo
      printf("Falha no processamento do arquivo.");
      return;
    }

    int acessosPagina = 0; //vai contar a quantidade de acessos a paginas de disco no decorrer da execucao

    if (fgetc(readFile) == '0') {   //se o byte "status" for '0', entao o arquivo esta inconsistente
      printf("Falha no processamento do arquivo.");
      return;
    }
    acessosPagina++;

    fseek(readFile, TAMPAG-1, SEEK_CUR);  //pulo a primeira pagina, que so tem o registro de cabecalho (lembrando que ja tinha lido o campo "status", por isso o -1)

    byte b = fgetc(readFile);

    if (feof(readFile)) {   //se o primeiro byte da primeira pagina de disco contendo os registros de dados for o final do arquivo, entao nao existem registros para serem mostrados
      printf("Registro inexistente.");
      return;
    }

    while (!feof(readFile)) {
        if (ftell(readFile)%TAMPAG == 1) acessosPagina++;   //se o ponteiro de leitura passou pelo primeiro byte de uma pagina de disco, entao conta-se mais um acesso

        if (b == '-') mostraRegistro(readFile);  //mostra o registro se ele nao esta removido
        else if (b == '*') {    //se ele esta removido...
            int pulo;
            fread(&pulo, 4, 1, readFile);   //lera o indicador de tamanho do registro (4 bytes)
            fseek(readFile, pulo, SEEK_CUR);    //pula o registro
        }

        b = fgetc(readFile);
    }

    printf("Número de páginas de disco acessadas: %d", acessosPagina);

    fclose(readFile);

    return;
}

/*
    Funcao que imprime na tela, organizadamente,
    um registro de um arquivo binario gerado
    anteriormente por este programa. A funcao
    assume que o usuario ira chama-la quando
    o ponteiro de leitura estiver exatamente no
    comeco do registro. Alem disso, esta funcao
    imprimira, antes de cada campo, o metadado
    correspondente, que vira a partir de um
    registro de cabecalho passado por parametro.

    Parametros:
        FILE *file - arquivo binario
        regCabec *cabec - cabecalho do arquivo
*/
void mostraRegistroMeta(FILE *file, regCabec *cabec) {
    regDados registro;  //registro que ajudara a guardar os dados lidos
    int tamanho;    //ajudara a contar quantos bytes faltam para terminar o registro
    int tamCampo;   //ajudara na hora de manipular os campos de tamanho variavel

    fread(&tamanho, 4, 1, file);    //leio os 4 bytes do indicador de tamanho do registro e os armazeno em "tamanho"
    fseek(file, 8, SEEK_CUR);   //pulo os 8 bytes do "encadeamentoLista", pois este campo nao sera mostrado
    tamanho -= 8;

    fread(&(registro.idServidor), 4, 1, file);   //leio os 4 bytes do campo "idServidor"
    tamanho -= 4;
    printf("%s: ", cabec->desCampo[0]);   //mostro o metadado referente ao campo
    printf("%d\n", registro.idServidor);   //mostro o valor do campo "idServidor" na tela

    fread(&(registro.salarioServidor), 8, 1, file);   //leio os 8 bytes do campo "salarioServidor"
    tamanho -= 8;
    printf("%s: ", cabec->desCampo[1]);   //mostro o metadado referente ao campo
    if (registro.salarioServidor != -1.0) printf("%.2lf\n", registro.salarioServidor);   //mostro o valor do campo "salarioServidor" na tela
    else printf("valor nao declarado\n");

    fread(registro.telefoneServidor, 14, 1, file);   //leio os 14 bytes do campo "telefoneServidor"
    tamanho -= 14;
    printf("%s: ", cabec->desCampo[2]);   //mostro o metadado referente ao campo
    if (registro.telefoneServidor[0] != '\0') printf("%.14s\n", registro.telefoneServidor);   //mostro o valor do campo "telefoneServidor" na tela
    else printf("valor nao declarado\n");

    if (tamanho == 0) {     //se ja cheguei no fim do registro...
        printf("%s: ", cabec->desCampo[3]);   //mostro o metadado referente ao campo
        printf("valor nao declarado\n");
        printf("%s: ", cabec->desCampo[4]);   //mostro o metadado referente ao campo
        printf("valor nao declarado\n");
        printf("\n");   //termino de mostrar o registro
        return;
    }

    byte b = fgetc(file);   //leio o byte apontado pelo ponteiro de leitura
    if (b == '@') {    //o registro eh o ultimo de uma pagina de disco
        printf("%s: ", cabec->desCampo[3]);   //mostro o metadado referente ao campo
        printf("valor nao declarado\n");
        printf("%s: ", cabec->desCampo[4]);   //mostro o metadado referente ao campo
        printf("valor nao declarado\n");
        printf("\n");   //termino de mostrar o registro
        fseek(file, tamanho-1, SEEK_CUR);   //reposiciono o ponteiro de leitura para ficar logo depois do final desse registro
        return;
    }
    ungetc(b, file);    //"devolvo" o byte lido para o arquivo

    fread(&tamCampo, 4, 1, file);   //leio os 4 bytes do indicador de tamanho do campo
    tamanho -= 4;

    char tag = fgetc(file);    //leio a tag referente ao campo
    tamanho -= 1;

    if (tag == 'n') {   //o campo a ser lido sera "nomeServidor"
        registro.nomeServidor = malloc(100*sizeof(char));   //aloco espaco dinamicamente para guardar o campo
        fread(registro.nomeServidor, 1, tamCampo-1, file);    //leio os bytes do campo
        tamanho -= (tamCampo-1);  //-1 pois se retirou a tag da contagem
        printf("%s: ", cabec->desCampo[3]);   //mostro o metadado referente ao campo
        printf("%s\n", registro.nomeServidor);

        //testamos mais uma vez para ver se acabou o registro ou nao

        if (tamanho == 0) {     //se ja cheguei no fim do registro...
            printf("%s: ", cabec->desCampo[4]);   //mostro o metadado referente ao campo
            printf("valor nao declarado\n");
            printf("\n");   //termino de mostrar o registro
            free(registro.nomeServidor);    //libero memoria anteriormente alocada
            return;
        }

        b = fgetc(file);   //leio o byte apontado pelo ponteiro de leitura
        if (b == '@') {    //o registro eh o ultimo de uma pagina de disco
            printf("%s: ", cabec->desCampo[4]);   //mostro o metadado referente ao campo
            printf("valor nao declarado\n");
            printf("\n");   //termino de mostrar o registro
            fseek(file, tamanho-1, SEEK_CUR);   //reposiciono o ponteiro de leitura para ficar logo depois do final desse registro
            free(registro.nomeServidor);    //libero memoria anteriormente alocada
            return;
        }
        ungetc(b, file);    //"devolvo" o byte lido para o arquivo

        fread(&(registro.tamCampo5), 4, 1, file);   //leio os 4 bytes do indicador de tamanho do campo "cargoServidor"
        tamanho -= 4;

        fgetc(file);    //pulo a tag do campo
        tamanho -= 1;

        registro.cargoServidor = malloc(100*sizeof(char));   //aloco espaco dinamicamente para guardar o campo
        fread(registro.cargoServidor, 1, registro.tamCampo5-1, file);    //leio os bytes do campo
        tamanho -= (registro.tamCampo5-1);  //-1 pois se retirou a tag da contagem
        printf("%s: ", cabec->desCampo[4]);   //mostro o metadado referente ao campo
        printf("%s\n", registro.cargoServidor);

        printf("\n");   //termino de mostrar o registro

        if (tamanho != 0) fseek(file, tamanho, SEEK_CUR);   //se o registro ainda nao terminou (provavelmente porque eh o ultimo registro de uma pagina de disco), reposiciono o ponteiro de leitura para ficar logo depois do final desse registro

        free(registro.nomeServidor);    //libero memoria anteriormente alocada
        free(registro.cargoServidor);   //libero memoria anteriormente alocada
    }
    else {      //o campo a ser lido sera "cargoServidor"
        printf("%s: ", cabec->desCampo[3]);   //mostro o metadado referente ao campo
        printf("valor nao declarado\n");   //o valor de "nomeServidor" nao foi declarado

        registro.cargoServidor = malloc(100*sizeof(char));   //aloco espaco dinamicamente para guardar o campo
        fread(registro.cargoServidor, 1, tamCampo-1, file);    //leio os bytes do campo
        tamanho -= (tamCampo-1);  //-1 pois se retirou a tag da contagem
        printf("%s: ", cabec->desCampo[4]);   //mostro o metadado referente ao campo
        printf("%s\n", registro.cargoServidor);

        printf("\n");   //termino de mostrar o registro
        if (tamanho != 0) fseek(file, tamanho, SEEK_CUR);   //se o registro ainda nao terminou (provavelmente porque eh o ultimo registro de uma pagina de disco), reposiciono o ponteiro de leitura para ficar logo depois do final desse registro
        free(registro.cargoServidor);
    }

    return;
}

/*
    Busca, em todo o arquivo binario, registros que
    satisfacam um criterio de busca determinado pelo
    usuario, mostrando-os na tela assim que sao encontrados.
    Um exemplo de busca seria "cargoServidor ENGENHEIRO",
    no qual a funcao ira mostrar na tela todos os registros
    em que o campo "cargoServidor" possui o valor "ENGENHEIRO".
    Tambem eh mostrado, ao final da execucao, quantas paginas
    de disco foram acessadas ao todo.
*/
void buscaReg() {
    char fileName[51];   //vai guardar o nome do arquivo a ser aberto
    char nomeCampo[51];    //campo a ser considerado na busca
    byte valorCampo[100];    //valor a ser considerado na busca

    scanf("%50s %50s", fileName, nomeCampo);
    scanf(" %[^\r\n]", valorCampo);     //paro de ler antes da quebra de linha

    FILE *readFile = fopen(fileName, "rb");  //abre o arquivo "fileName" para leitura binária

    if (readFile == NULL) {   //erro na abertura do arquivo
      printf("Falha no processamento do arquivo.");
      return;
    }

    int acessosPagina = 0; //vai contar a quantidade de acessos a paginas de disco no decorrer da execucao

    if (fgetc(readFile) == '0') {   //se o byte "status" for '0', entao o arquivo esta inconsistente
      printf("Falha no processamento do arquivo.");
      return;
    }
    acessosPagina++;

    fseek(readFile, 8, SEEK_CUR);   //pulo o campo "topoLista", pois ele nao sera considerado

    //comeco recuperando os metadados do registro de cabecalho
    regCabec cabecalho;
    for (int i = 0; i < 5; i++) {
        fgetc(readFile);    //pulo a tag (nao sera necessaria)
        fread(cabecalho.desCampo[i], 1, 40, readFile);  //recupero a descricao dos campos
    }

    fseek(readFile, TAMPAG-214, SEEK_CUR);  //vou para a segunda pagina de disco (que contem os registros de dados)

    byte b = fgetc(readFile);

    if (feof(readFile)) {   //se o primeiro byte da primeira pagina de disco contendo os registros de dados for o final do arquivo, entao nao existem registros para serem mostrados
      printf("Registro inexistente.");
      return;
    }

    //aqui comeca a comparacao por campos

    int lidos = 0;    //guardara provisoriamente quantos bytes ja se leram de um registro
    int achou = 0;    //indicara se pelo menos um registro foi achado

    if (strcmp(nomeCampo, "idServidor") == 0) {     //se o campo a ser buscado eh "idServidor"...
        while (!feof(readFile)) {
            if (ftell(readFile)%TAMPAG == 1) acessosPagina++;   //se o ponteiro de leitura passou pelo primeiro byte de uma pagina de disco, entao conta-se mais um acesso

            if (b == '-') {     //se ele nao esta removido...
                int indicTam;
                fread(&indicTam, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do registro
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                lidos += 8;

                int valor;
                fread(&valor, 4, 1, readFile);  //leio o valor do campo "idServidor"
                lidos += 4;

                fseek(readFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                if (valor == atoi(valorCampo)) {    //se o valor lido eh igual ao do dado como criterio de busca...
                    mostraRegistroMeta(readFile, &cabecalho);
                    achou = 1;
                    break;  //ja que o numero do idServidor eh unico, se acharmos um igual nao precisaremos mais continuar procurando
                }

                fseek(readFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                lidos = 0;
            }

            else if (b == '*') {    //se ele esta removido...
                int pulo = 0;
                fread(&pulo, 4, 1, readFile);   //lera o indicador de tamanho do registro (4 bytes)
                fseek(readFile, pulo, SEEK_CUR);    //pula o registro
            }

            b = fgetc(readFile);
        }
    }
    else if (strcmp(nomeCampo, "salarioServidor") == 0) {     //se o campo a ser buscado eh "salarioServidor"...
        while (!feof(readFile)) {
            if (ftell(readFile)%TAMPAG == 1) acessosPagina++;   //se o ponteiro de leitura passou pelo primeiro byte de uma pagina de disco, entao conta-se mais um acesso

            if (b == '-') {     //se ele nao esta removido...
                int indicTam;
                fread(&indicTam, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do registro
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                lidos += 8;

                fseek(readFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                lidos += 4;

                double valor;
                fread(&valor, 8, 1, readFile);  //leio o valor do campo "salarioServidor"
                lidos += 8;

                fseek(readFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                if (valor == atof(valorCampo)) {    //se o valor lido eh igual ao do dado como criterio de busca...
                    mostraRegistroMeta(readFile, &cabecalho);
                    achou = 1;
                }
                else {
                    fseek(readFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                }

                lidos = 0;
            }

            else if (b == '*') {    //se ele esta removido...
                int pulo = 0;
                fread(&pulo, 4, 1, readFile);   //lera o indicador de tamanho do registro (4 bytes)
                fseek(readFile, pulo, SEEK_CUR);    //pula o registro
            }

            b = fgetc(readFile);
        }
    }
    else if (strcmp(nomeCampo, "telefoneServidor") == 0) {     //se o campo a ser buscado eh "telefoneServidor"...
        while (!feof(readFile)) {
            if (ftell(readFile)%TAMPAG == 1) acessosPagina++;   //se o ponteiro de leitura passou pelo primeiro byte de uma pagina de disco, entao conta-se mais um acesso

            if (b == '-') {     //se ele nao esta removido...
                int indicTam;
                fread(&indicTam, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do registro
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                lidos += 8;

                fseek(readFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                lidos += 8;

                char valor[15];
                fread(valor, 1, 14, readFile);  //leio o valor do campo "telefoneServidor"
                valor[14] = '\0';   //"encerro" a string
                lidos += 14;

                fseek(readFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                    mostraRegistroMeta(readFile, &cabecalho);
                    achou = 1;
                }
                else {
                    fseek(readFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                }

                lidos = 0;
            }

            else if (b == '*') {    //se ele esta removido...
                int pulo = 0;
                fread(&pulo, 4, 1, readFile);   //lera o indicador de tamanho do registro (4 bytes)
                fseek(readFile, pulo, SEEK_CUR);    //pula o registro
            }

            b = fgetc(readFile);
        }
    }
    else if (strcmp(nomeCampo, "nomeServidor") == 0) {     //se o campo a ser buscado eh "nomeServidor"...
        while (!feof(readFile)) {
            if (ftell(readFile)%TAMPAG == 1) acessosPagina++;   //se o ponteiro de leitura passou pelo primeiro byte de uma pagina de disco, entao conta-se mais um acesso

            if (b == '-') {     //se ele nao esta removido...
                int indicTam;
                fread(&indicTam, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do registro
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                lidos += 8;

                fseek(readFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                lidos += 8;

                fseek(readFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                lidos += 14;

                if (lidos != indicTam+4) {   //se o registro nao terminou...
                    b = fgetc(readFile);    //leio o proximo byte do registro
                    lidos += 1;

                    if (b == '@') {
                        //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                        fseek(readFile, indicTam+4 - lidos, SEEK_CUR);
                    }
                    else {
                        ungetc(b, readFile);    //"devolvo" o byte lido para o arquivo
                        lidos -= 1;

                        int tamCampo;
                        fread(&tamCampo, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do campo
                        lidos += 4;

                        char tag = fgetc(readFile);   //leio a tag do campo
                        lidos += 1;

                        if (tag == 'c') {   //o campo "nomeServidor" nao existe nesse registro de dados
                            fseek(readFile, tamCampo-1, SEEK_CUR);  //vou para o proximo registro
                        }
                        else {  //ele existe
                            char valor[100];
                            fread(valor, 1, tamCampo-1, readFile);  //leio o valor do campo "nomeServidor"
                            lidos += (tamCampo-1);

                            fseek(readFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                            if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                mostraRegistroMeta(readFile, &cabecalho);
                                achou = 1;
                            }
                            else {
                                fseek(readFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                            }
                        }
                    }
                }

                lidos = 0;
            }
            else if (b == '*') {    //se ele esta removido...
                int pulo = 0;
                fread(&pulo, 4, 1, readFile);   //lera o indicador de tamanho do registro (4 bytes)
                fseek(readFile, pulo, SEEK_CUR);    //pula o registro
            }

            b = fgetc(readFile);
        }
    }
    else if (strcmp(nomeCampo, "cargoServidor") == 0) {     //se o campo a ser buscado eh "cargoServidor"...
        while (!feof(readFile)) {
            if (ftell(readFile)%TAMPAG == 1) acessosPagina++;   //se o ponteiro de leitura passou pelo primeiro byte de uma pagina de disco, entao conta-se mais um acesso

            if (b == '-') {     //se ele nao esta removido...
                int indicTam;
                fread(&indicTam, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do registro
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                lidos += 8;

                fseek(readFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                lidos += 4;

                fseek(readFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                lidos += 8;

                fseek(readFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                lidos += 14;

                if (lidos != indicTam+4) {   //se o registro nao terminou...
                    b = fgetc(readFile);    //leio o proximo byte do registro
                    lidos += 1;

                    if (b == '@') {
                        //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                        fseek(readFile, indicTam+4 - lidos, SEEK_CUR);
                    }
                    else {
                        ungetc(b, readFile);    //"devolvo" o byte lido para o arquivo
                        lidos -= 1;

                        int tamCampo;
                        fread(&tamCampo, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do campo
                        lidos += 4;

                        char tag = fgetc(readFile);   //leio a tag do campo
                        lidos += 1;

                        if (tag == 'n') {   //o campo "cargoServidor" pode nao existir
                            fseek(readFile, tamCampo-1, SEEK_CUR);  //pulo o campo "nomeServidor"
                            lidos += (tamCampo-1);

                            if (lidos != indicTam+4) {   //se o registro ainda nao terminou...
                                b = fgetc(readFile);    //leio o proximo byte do registro
                                lidos += 1;

                                if (b == '@') {
                                    //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                    fseek(readFile, indicTam+4 - lidos, SEEK_CUR);
                                }
                                else {
                                    ungetc(b, readFile);    //"devolvo" o byte lido para o arquivo
                                    lidos -= 1;

                                    fread(&tamCampo, 4, 1, readFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                    lidos += 4;

                                    fgetc(readFile);   //pulo a tag do campo
                                    lidos += 1;

                                    char valor[100];
                                    fread(valor, 1, tamCampo-1, readFile);  //leio o valor do campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    fseek(readFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                    if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                        mostraRegistroMeta(readFile, &cabecalho);
                                        achou = 1;
                                    }
                                    else {
                                        fseek(readFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                    }
                                }
                            }
                        }
                        else {  //ele existe
                            char valor[100];
                            fread(valor, 1, tamCampo-1, readFile);  //leio o valor do campo "nomeServidor"
                            lidos += (tamCampo-1);

                            fseek(readFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                            if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                mostraRegistroMeta(readFile, &cabecalho);
                                achou = 1;
                            }
                            else {
                                fseek(readFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                            }
                        }
                    }
                }

                lidos = 0;
            }

            else if (b == '*') {    //se ele esta removido...
                int pulo = 0;
                fread(&pulo, 4, 1, readFile);   //lera o indicador de tamanho do registro (4 bytes)
                fseek(readFile, pulo, SEEK_CUR);    //pula o registro
            }

            b = fgetc(readFile);
        }
    }
    else {  //o usuario digitou errado o nome do campo
        printf("Falha no processamento do arquivo.");
        return;
    }

    if (!achou) {
        printf("Registro inexistente.");
    }
    else {
        printf("Número de páginas de disco acessadas: %d", acessosPagina);
    }

    fclose(readFile);

    return;
}

/*
    Adiciona para a lista ordenada de registros removidos
    um novo elemento, adicionando-o em uma ordem crescente.

    Parametros:
        FILE *file - arquivo binario a ser considerado
        long long newBO - byte offset do registro a ser adicionado
        int tamRegistro - tamanho do registro a ser adicionado
*/
void adicionaLista(FILE *file, long long newBO, int tamRegistro) {
    long long nextBO;   //ira dizer qual o byte offset do proximo registro da lista
    long long changeBO; //ira guardar o byte offset do registro anterior ao atual (util na hora da insercao)
    int tamAtual = 0;   //tamanho do registro atual (considerei que o cabecalho tem tamanho "0")

    fseek(file, 1, SEEK_SET);   //posiciono o cabecote de leitura no inicio do campo "topoLista" do registro de cabecalho
    fread(&nextBO, 8, 1, file);  //leio e armazeno o seu valor

    if (nextBO == -1L) {    //se a lista esta vazia
        fseek(file, -8, SEEK_CUR);
        fwrite(&newBO, 8, 1, file); //sobreescrevo o campo "topoLista", que agora aponta para o novo registro removido
        fseek(file, newBO+1, SEEK_SET); //reposiciono o cabecote para ficar na mesma posicao de quando entrou na funcao
        return;
    }
    else while (nextBO != -1L && tamAtual < tamRegistro) {   //aqui, estou a procurar o local correto para encaixar o novo registro removido
        changeBO = ftell(file) - 13;  //-13 para descontar os bytes ja lidos
        fseek(file, nextBO, SEEK_SET);  //vou para o proximo registro no encadeamento
        fgetc(file);    //"joga fora" o primeiro byte do registro
        fread(&tamAtual, 4, 1, file);   //le o indicador de tamanho do registro
        fread(&nextBO, 8, 1, file);   //le o byte offset do proximo registro no encadeamento
    }

    if (tamAtual >= tamRegistro) {  //inserir no meio
        fseek(file, changeBO+5, SEEK_SET);   //somo 5 aos bytes do byte offset para pular o campo "removido" e o indicador de tamanho do registro de dados
        fread(&nextBO, 8, 1, file);     //le o byte offset do proximo registro no encadeamento
        fseek(file, -8, SEEK_CUR);
        fwrite(&newBO, 8, 1, file);     //sobreescrevo o campo "encadeamentoLista", que agora aponta para o novo registro removido

        fseek(file, newBO+5, SEEK_SET);    //vou para o byte offset do mais novo registro removido e pulo os bytes do campo "removido" e do indicador de tamanho
        fwrite(&nextBO, 8, 1, file);    //termino de atualizar o encadeamento
    }
    else if (nextBO == -1L) {   //inserir no final
        fseek(file, changeBO+5, SEEK_SET);   //somo 5 aos bytes do byte offset para pular o campo "removido" e o indicador de tamanho do registro de dados
        fread(&changeBO, 8, 1, file);     //le o byte offset do proximo registro no encadeamento

        fseek(file, changeBO+5, SEEK_SET);   //somo 5 aos bytes do byte offset para pular o campo "removido" e o indicador de tamanho do registro de dados
        fread(&nextBO, 8, 1, file);     //le o byte offset do proximo registro no encadeamento
        fseek(file, -8, SEEK_CUR);
        fwrite(&newBO, 8, 1, file);     //sobreescrevo o campo "encadeamentoLista", que agora aponta para o novo registro removido

        fseek(file, newBO+5, SEEK_SET);    //vou para o byte offset do mais novo registro removido e pulo os bytes do campo "removido" e do indicador de tamanho
        fwrite(&nextBO, 8, 1, file);    //termino de atualizar o encadeamento
    }

    fseek(file, newBO+1, SEEK_SET);     //reposiciono o cabecote para ficar na mesma posicao de quando entrou na funcao

    return;
}

/*
    Sobreescreve o conteudo de um registro
    logicamente removido com lixo ("@").

    Parametros:
        FILE *file - arquivo binario que
    contem o registro
*/
void completaLixo(FILE *file) {
    long long pos = ftell(file);     //guardo a posicao inicial do registro
    int tamanho;

    fread(&tamanho, 4, 1, file);   //leio o indicador de tamanho e o guardo

    fseek(file, 8, SEEK_CUR);   //pulo os 8 bytes do campo "encadeamentoLista"
    tamanho -= 8;

    for (int i = 0; i < tamanho; i++) fputc('@', file);

    fseek(file, pos, SEEK_SET);  //volto para a posicao inicial do registro

    return;
}

/*
    Busca no arquivo binario registros que satisfacam
    um criterio de busca determinado pelo usuario,
    removendo-os logicamente assim que sao encontrados.
    O usuario deve informar quantas buscas diferentes
    deseja fazer perante uma mesma execucao. Alem disso,
    deve informar, em cada uma delas, o nome e o valor
    do campo a ser buscado.
*/
void removeReg() {
    char fileName[51];   //vai guardar o nome do arquivo a ser aberto
    int n;      //numero de remocoes a serem realizadas
    char nomeCampo[51];    //campo a ser considerado na busca
    byte valorCampo[200];    //valor a ser considerado na busca

    scanf("%50s", fileName);
    scanf("%d", &n);

    FILE *binFile = fopen(fileName, "rb+");  //abre o arquivo "fileName" para leitura e escrita binária

    if (binFile == NULL) {   //erro na abertura do arquivo
      printf("Falha no processamento do arquivo.");
      return;
    }

    if (fgetc(binFile) == '0') {   //se o byte "status" for '0', entao o arquivo esta inconsistente
      printf("Falha no processamento do arquivo.");
      return;
    }
    ungetc('0', binFile);  //como o arquivo foi aberto para escrita, seu status deve ser '0'

    fseek(binFile, TAMPAG, SEEK_CUR);  //pulo o registro de cabecalho

    for (int i = 0; i < n; i++) {
        memset(nomeCampo, 0, 51);   //limpo o vetor (setando tudo para 0)
        memset(valorCampo, 0, 200);  //limpo o vetor (setando tudo para 0)

        scanf("%50s %[^\r\n]", nomeCampo, valorCampo);  //paro de ler antes da quebra de linha

        byte b = fgetc(binFile);

        if (feof(binFile)) {   //se o primeiro byte da primeira pagina de disco contendo os registros de dados for o final do arquivo, entao nao existem registros para serem mostrados
          printf("Registro inexistente.");
          return;
        }

        //aqui comeca a comparacao por campos

        int lidos = 0;    //guardara provisoriamente quantos bytes ja se leram de um registro

        if (strcmp(nomeCampo, "idServidor") == 0) {     //se o campo a ser buscado eh "idServidor"...

            while (!feof(binFile)) {
                if (b == '-') {     //se ele nao esta removido...
                    int indicTam;
                    fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                    lidos += 8;

                    int valor;
                    fread(&valor, 4, 1, binFile);  //leio o valor do campo "idServidor"
                    lidos += 4;

                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                    if (valor == atoi(valorCampo)) {    //se o valor lido eh igual ao do dado como criterio de busca...
                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                        fputc('*', binFile);    //marco o registro como REMOVIDO
                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                        break;  //ja que o numero do idServidor eh unico, se acharmos um igual nao precisaremos mais continuar procurando
                    }

                    fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                    lidos = 0;
                }

                else if (b == '*') {    //se ele esta removido...
                    int pulo = 0;
                    fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                    fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                }

                b = fgetc(binFile);
            }

        }
        else if (strcmp(nomeCampo, "salarioServidor") == 0) {     //se o campo a ser buscado eh "salarioServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...
                valorCampo[0] = '-';
                valorCampo[1] = '1';
                valorCampo[2] = '\0';
            }

            while (!feof(binFile)) {
                if (b == '-') {     //se ele nao esta removido...
                    int indicTam;
                    fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                    lidos += 8;

                    fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                    lidos += 4;

                    double valor;
                    fread(&valor, 8, 1, binFile);  //leio o valor do campo "salarioServidor"
                    lidos += 8;

                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                    if (valor == atof(valorCampo)) {    //se o valor lido eh igual ao do dado como criterio de busca...
                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                        fputc('*', binFile);    //marco o registro como REMOVIDO
                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                    }

                    fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                    lidos = 0;
                }

                else if (b == '*') {    //se ele esta removido...
                    int pulo = 0;
                    fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                    fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                }

                b = fgetc(binFile);
            }

        }
        else if (strcmp(nomeCampo, "telefoneServidor") == 0) {     //se o campo a ser buscado eh "telefoneServidor"...

            char *valorSemAspas = strtok(valorCampo, "\"");     //retiro as aspas da string (para efeitos de comparacao)
            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...
                valorSemAspas[0] = '\0';
            }

            while (!feof(binFile)) {
                if (b == '-') {     //se ele nao esta removido...
                    int indicTam;
                    fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                    lidos += 8;

                    fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                    lidos += 8;

                    char valor[15];
                    fread(valor, 1, 14, binFile);  //leio o valor do campo "telefoneServidor"
                    valor[14] = '\0';   //"encerro" a string
                    lidos += 14;

                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                    if (strcmp(valor, valorSemAspas) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                        fputc('*', binFile);    //marco o registro como REMOVIDO
                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                    }

                    fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                    lidos = 0;
                }

                else if (b == '*') {    //se ele esta removido...
                    int pulo = 0;
                    fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                    fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                }

                b = fgetc(binFile);
            }

        }
        else if (strcmp(nomeCampo, "nomeServidor") == 0) {     //se o campo a ser buscado eh "nomeServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos == indicTam+4) {  //se o registro ja terminou, nao possui o campo "nomeServidor"
                            fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                            fputc('*', binFile);    //marco o registro como REMOVIDO
                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                        }
                        else {   //o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {    //ele eh o ultimo de uma pagina de disco
                                fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                fputc('*', binFile);    //marco o registro como REMOVIDO
                                adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                fseek(binFile, indicTam+4, SEEK_CUR);   //pulo o lixo "acoplado" a ele
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;
                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'c') {   //o campo "nomeServidor" nao existe nesse registro de dados
                                    fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                    long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                    fputc('*', binFile);    //marco o registro como REMOVIDO
                                    adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                    completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                    fseek(binFile, indicTam+4, SEEK_CUR);  //vou para o proximo registro
                                }
                                else {
                                    fseek(binFile, indicTam+4-lidos, SEEK_CUR);  //vou para o proximo registro
                                }
                            }
                        }

                        lidos = 0;
                    }
                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }
            else {
                char *valorSemAspas = strtok(valorCampo, "\"");     //retiro as aspas da string (para efeitos de comparacao)

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos != indicTam+4) {   //se o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {
                                //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                fseek(binFile, indicTam+4 - lidos, SEEK_CUR);
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;

                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'c') {   //o campo "nomeServidor" nao existe nesse registro de dados
                                    fseek(binFile, tamCampo-1, SEEK_CUR);  //vou para o proximo registro
                                }
                                else {  //ele existe
                                    char valor[100];
                                    fread(valor, 1, tamCampo-1, binFile);  //leio o valor do campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                    if (strcmp(valor, valorSemAspas) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                        fputc('*', binFile);    //marco o registro como REMOVIDO
                                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                                    }

                                    fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                }
                            }
                        }

                        lidos = 0;
                    }
                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }

        }
        else if (strcmp(nomeCampo, "cargoServidor") == 0) {     //se o campo a ser buscado eh "cargoServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos == indicTam+4) {   //se o registro ja terminou, nao tem o campo "cargoServidor"
                            fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                            fputc('*', binFile);    //marco o registro como REMOVIDO
                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                        }
                        else {   //o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {    //ele eh o ultimo de uma pagina de disco
                                fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                fputc('*', binFile);    //marco o registro como REMOVIDO
                                adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                fseek(binFile, indicTam+4, SEEK_CUR);   //pulo o lixo "acoplado" a ele
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;

                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'n') {   //o campo "cargoServidor" pode nao existir
                                    fseek(binFile, tamCampo-1, SEEK_CUR);  //pulo o campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    if (lidos == indicTam+4) {   //se o registro ja terminou, nao possui o campo "cargoServidor"
                                        fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                        fputc('*', binFile);    //marco o registro como REMOVIDO
                                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                        fseek(binFile, indicTam+4, SEEK_CUR);   //vou para o proximo registro
                                    }
                                    else {   //se o registro ainda nao terminou, ha chance do campo existir
                                        b = fgetc(binFile);    //leio o proximo byte do registro
                                        lidos += 1;

                                        if (b == '@') {    //ele eh o ultimo de uma pagina de disco
                                            fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                            fputc('*', binFile);    //marco o registro como REMOVIDO
                                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                            fseek(binFile, indicTam+4, SEEK_CUR);   //pulo o lixo "acoplado" a ele
                                        }
                                        else {
                                            fseek(binFile, indicTam+4-lidos, SEEK_CUR);  //vou para o proximo registro
                                        }
                                    }
                                }
                                else {
                                    fseek(binFile, indicTam+4-lidos, SEEK_CUR);  //vou para o proximo registro
                                }
                            }
                        }

                        lidos = 0;
                    }

                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }
            else {
                char *valorSemAspas = strtok(valorCampo, "\"");     //retiro as aspas da string (para efeitos de comparacao)

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos != indicTam+4) {   //se o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {
                                //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                fseek(binFile, indicTam+4 - lidos, SEEK_CUR);
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;

                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'n') {   //o campo "cargoServidor" pode nao existir
                                    fseek(binFile, tamCampo-1, SEEK_CUR);  //pulo o campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    if (lidos != indicTam+4) {   //se o registro ainda nao terminou...
                                        b = fgetc(binFile);    //leio o proximo byte do registro
                                        lidos += 1;

                                        if (b == '@') {
                                            //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                            fseek(binFile, indicTam+4 - lidos, SEEK_CUR);
                                        }
                                        else {
                                            ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                            lidos -= 1;

                                            fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                            lidos += 4;

                                            fgetc(binFile);   //pulo a tag do campo
                                            lidos += 1;

                                            char valor[100];
                                            fread(valor, 1, tamCampo-1, binFile);  //leio o valor do campo "nomeServidor"
                                            lidos += (tamCampo-1);

                                            fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                            if (strcmp(valor, valorSemAspas) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                                fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                                                long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                                fputc('*', binFile);    //marco o registro como REMOVIDO
                                                adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                                completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                                            }

                                            fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                        }
                                    }
                                }
                                else {  //ele existe
                                    char valor[100];
                                    fread(valor, 1, tamCampo-1, binFile);  //leio o valor do campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                    if (strcmp(valor, valorSemAspas) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                        fputc('*', binFile);    //marco o registro como REMOVIDO
                                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo
                                    }

                                    fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                }
                            }
                        }

                        lidos = 0;
                    }

                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }

        }
        else {  //o usuario digitou errado o nome do campo
            printf("Falha no processamento do arquivo.");
            return;
        }

        fseek(binFile, TAMPAG, SEEK_SET);    //volto o ponteiro de leitura para o inicio da segunda pagina de disco (a que inicia os registros de dados)
    }

    binarioNaTela1(binFile);

    //antes de fechar o arquivo, coloco seu status para '1'
    fseek(binFile, 0, SEEK_SET);  //coloco o ponteiro de escrita no primeiro byte do arquivo
    byte status = '1';
    fwrite(&status, 1, 1, binFile);  //sobrescrevo o campo "status" do arquivo binario
    fclose(binFile);

    return;
}

/*
    Procura na lista de removidos um espaco que
    comporte o novo registro a ser inserido. Caso
    nao encontre nenhum, insere-o no final.

    Parametros:
        FILE *file - arquivo binario a ser considerado
        regDados *registro - registro a ser inserido
        long long ultimoBO - byte offset do ultimo registro do arquivo
    Retorno:
        long long - byte offset do ultimo registro do arquivo (apos as modificacoes)
*/
long long achaPosicaoInsere(FILE *file, regDados *registro, long long ultimoBO) {
    long origin = ftell(file);    //guarda a posicao em que o ponteiro de leitura entrou na funcao
    fseek(file, 1, SEEK_SET);  //coloco o ponteiro de leitura no comeco do campo "topoLista"

    long long pos, posAnt, posProx, posUlt;
    int tam = 0;

    fread(&posProx, 8, 1, file);    //leio o campo "topoLista"

    if (posProx == -1L) {
        //insiro o registro no final do arquivo
        fseek(file, 0, SEEK_END);
        posUlt = ftell(file);

        if (ultimoBO != -1) {
            //verifico o espaco disponivel na pagina de disco atual
            if ( (ftell(file)%TAMPAG + registro->tamanhoRegistro + 5) > TAMPAG ) { //se nao ha espaco suficiente... (+5 por conta do campo "removido" e do indicador de tamanho, que nao sao contabilizados no tamanho do registro)
                //insiro o registro em uma outra pagina de disco, mas antes vou precisar completar a pagina de disco atual com lixo e trocar o indicador de tamanho do ultimo registro dela, para que ele contabilize esse lixo tambem
                int diff = TAMPAG - ftell(file)%TAMPAG;  //quantidade necessaria de lixo para completar a pagina de disco

                int tamAntigo;
                fread(&tamAntigo, 4, 1, file);
                fseek(file, -4, SEEK_CUR);

                int tamNovo = diff + tamAntigo;  //calculo o valor do novo indicador de tamanho
                fwrite(&tamNovo, 4, 1, file); //sobreescrevo o valor do indicador de tamanho antigo pelo novo

                fseek(file, tamAntigo, SEEK_CUR);  //coloco o ponteiro no final do ultimo registro
                for (int i = 0; i < diff; i++) fputc('@', file);  //completo com lixo
            }

        }

        insereRegistro(file, registro);
    }
    else {
        while (posProx != -1L && tam < registro->tamanhoRegistro) {
            posAnt = ftell(file) - 13;  //-13 para descontar os bytes ja lidos
            fseek(file, posProx, SEEK_SET);  //vou para o proximo registro no encadeamento

            fgetc(file);    //"joga fora" o primeiro byte do registro
            fread(&tam, 4, 1, file);   //le o indicador de tamanho do registro
            fread(&posProx, 8, 1, file);   //le o byte offset do proximo registro no encadeamento
        }

        if (tam >= registro->tamanhoRegistro) {  //inserir no "meio"
            pos = ftell(file) - 13;    //retiro os 13 bytes que já havia lido
            fseek(file, posAnt+5, SEEK_SET);   //somo 5 aos bytes do byte offset para pular o campo "removido" e o indicador de tamanho do registro de dados
            fwrite(&posProx, 8, 1, file);     //sobreescrevo o campo "encadeamentoLista", que agora aponta para o próximo registro depois do que foi sobreescrito pelo registro adicionado

            fseek(file, pos, SEEK_SET);
            registro->tamanhoRegistro = -1;  //como o registro vai ser inserido em um espaco removido logicamente, seu indicador de tamanho nao deve ser escrito, permanecendo o do registro anterior a ele
            insereRegistro(file, registro);
            posUlt = -1;
        }
        else {   //inserir no final
            fseek(file, 0, SEEK_END);

            if (ultimoBO != -1) {
                //verifico o espaco disponivel na pagina de disco atual
                if ( (ftell(file)%TAMPAG + registro->tamanhoRegistro + 5) > TAMPAG ) { //se nao ha espaco suficiente... (+5 por conta do campo "removido" e do indicador de tamanho, que nao sao contabilizados no tamanho do registro)
                    //insiro o registro em uma outra pagina de disco, mas antes vou precisar completar a pagina de disco atual com lixo e trocar o indicador de tamanho do ultimo registro dela, para que ele contabilize esse lixo tambem
                    int diff = TAMPAG - ftell(file)%TAMPAG;  //quantidade necessaria de lixo para completar a pagina de disco

                    fseek(file, ultimoBO+1, SEEK_SET);    //vou para o comeco do indicador de tamanho do ultimo registro do arquivo
                    int tamAntigo;
                    fread(&tamAntigo, 4, 1, file);
                    fseek(file, -4, SEEK_CUR);

                    int tamNovo = diff + tamAntigo;  //calculo o valor do novo indicador de tamanho
                    fwrite(&tamNovo, 4, 1, file); //sobreescrevo o valor do indicador de tamanho antigo pelo novo

                    fseek(file, tamAntigo, SEEK_CUR);  //coloco o ponteiro no final do ultimo registro
                    for (int i = 0; i < diff; i++) fputc('@', file);  //completo com lixo
                }

            }

            posUlt = ftell(file);
            insereRegistro(file, registro);
        }
    }

    fseek(file, origin, SEEK_SET);     //volto o ponteiro de leitura para o lugar original dele
    return posUlt;
}

/*
    Adiciona novos registros ao arquivo binario,
    reaproveitando os espacos deixados pelos
    registros logicamente removidos. Para tanto,
    percorre-se a lista de removidos ate que se
    encontre um espaco em que caiba o novo registro.
    Caso nao se encontre nenhum espaco grande o
    suficiente, o registro eh escrito no final do
    arquivo. O usuario deve informar quantas insercoes
    diferentes deseja fazer perante uma mesma execucao.
    Alem disso, deve informar, em cada uma delas, o
    valor dos campos do registro.
*/
void adicionaReg() {
    char fileName[51];   //vai guardar o nome do arquivo a ser aberto
    int n;      //numero de insercoes a serem realizadas
    regDados registro;  //ira guardar os dados fornecidos pelo usuario
    char buffer[201];
    long long posUltimoReg = -1;  //ira guardar o byte offset do ultimo registro do arquivo

    scanf("%50s", fileName);
    scanf("%d", &n);

    FILE *binFile = fopen(fileName, "rb+");  //abre o arquivo "fileName" para leitura e escrita binária

    if (binFile == NULL) {   //erro na abertura do arquivo
      printf("Falha no processamento do arquivo.");
      return;
    }

    if (fgetc(binFile) == '0') {   //se o byte "status" for '0', entao o arquivo esta inconsistente
      printf("Falha no processamento do arquivo.");
      return;
    }
    ungetc('0', binFile);  //como o arquivo foi aberto para escrita, seu status deve ser '0'

    registro.removido = '-';  //os registros vem "nao-removidos" por padrao
    registro.encadeamentoLista = -1L; //por padrao
    registro.tagCampo4 = 'n';   //por padrao
    registro.tagCampo5 = 'c';   //por padrao

    for (int i = 0; i < n; i++) {

        scanf("%d ", &(registro.idServidor));   //pego o valor de idServidor (esse campo nao pode ser nulo)

        //pego o valor de salarioServidor
        memset(buffer, 0, 201);   //limpo o buffer
        char c = fgetc(stdin);
        if (c == 'N') {     //o campo eh nulo
            registro.salarioServidor = -1;
            fseek(stdin, 4, SEEK_CUR);  //pulo os caracteres "ULO "
        }
        else {
            ungetc(c, stdin);   //devolvo o char lido para a entrada padrao
            scanf("%s ", buffer);
            registro.salarioServidor = atof(buffer);
        }

        //pego o valor de telefoneServidor
        memset(buffer, 0, 201);   //limpo o buffer
        c = fgetc(stdin);
        if (c == 'N') {     //o campo eh nulo
            registro.telefoneServidor[0] = '\0';
            fseek(stdin, 4, SEEK_CUR);  //pulo os caracteres "ULO "
        }
        else {
            ungetc(c, stdin);   //devolvo o char lido para a entrada padrao
            scanf("%*[\"]%[^\"]", buffer);  //le o campo, desconsiderando as aspas
            fseek(stdin, 2, SEEK_CUR);  //pulo os caracteres "\" "
            strcpy(registro.telefoneServidor, buffer);
        }

        //pego o valor de nomeServidor
        memset(buffer, 0, 201);   //limpo o buffer
        c = fgetc(stdin);
        if (c == 'N') {     //o campo eh nulo
            registro.nomeServidor = NULL;
            fseek(stdin, 4, SEEK_CUR);  //pulo os caracteres "ULO "
        }
        else {
            ungetc(c, stdin);   //devolvo o char lido para a entrada padrao
            scanf("%*[\"]%[^\"]", buffer);  //le o campo, desconsiderando as aspas
            fseek(stdin, 2, SEEK_CUR);  //pulo os caracteres "\" "
            registro.nomeServidor = malloc(100*sizeof(char));
            strcpy(registro.nomeServidor, buffer);
            registro.tamCampo4 = strlen(registro.nomeServidor) + 2;     //conta tambem o '\0' e a tag do campo
        }

        //pego o valor de cargoServidor
        memset(buffer, 0, 201);   //limpo o buffer
        c = fgetc(stdin);
        if (c == 'N') {     //o campo eh nulo
            registro.cargoServidor = NULL;
            fseek(stdin, 4, SEEK_CUR);  //pula os caracteres "ULO "
        }
        else {
            ungetc(c, stdin);   //devolvo o char lido para a entrada padrao
            scanf("%*[\"]%[^\"]", buffer);  //le o campo, desconsiderando as aspas
            fseek(stdin, 2, SEEK_CUR);  //pulo os caracteres "\" "
            registro.cargoServidor = malloc(100*sizeof(char));
            strcpy(registro.cargoServidor, buffer);
            registro.tamCampo5 = strlen(registro.cargoServidor) + 2;     //conta tambem o '\0' e a tag do campo
        }

        //calculo o tamanho total do registro e o armazeno
        if (registro.nomeServidor == NULL && registro.cargoServidor == NULL) {
            registro.tamanhoRegistro = 34;
            //34 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor e 14 do telefoneServidor
        }
        else if (registro.cargoServidor == NULL) {
            registro.tamanhoRegistro = 38 + registro.tamCampo4;
            //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 4
        }
        else if (registro.nomeServidor == NULL) {
            registro.tamanhoRegistro = 38 + registro.tamCampo5;
            //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 5
        }
        else {
            registro.tamanhoRegistro = 42 + registro.tamCampo4 + registro.tamCampo5;
            //42 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor, 4 do indicador de tamanho do campo 4 e mais 4 do indicador de tamanho do campo 5
        }

        long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
        if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
    }

    binarioNaTela1(binFile);

    //antes de fechar o arquivo, coloco seu status para '1'
    fseek(binFile, 0, SEEK_SET);  //coloco o ponteiro de escrita no primeiro byte do arquivo
    byte status = '1';
    fwrite(&status, 1, 1, binFile);  //sobrescrevo o campo "status" do arquivo binario
    fclose(binFile);

    return;
}

/*
    Le os campos de um registro presente em um
    arquivo binario anteriormente gerado por
    este programa e guarda seus valores em uma
    estrutura passada por parametro pelo usuario.
    A funcao assume que o ponteiro de leitura do
    arquivo estara no inicio do registro ao ser
    chamada.

    Parametros:
        FILE *file - arquivo binario contendo o
    registro a ser lido
        regDados *registro - estrutura para onde
    devem ser copiados os dados do registro
*/
void leRegistro(FILE *file, regDados *registro) {
    long ini = ftell(file);    //guardo a posicao de inicio do registro

    registro->removido = fgetc(file);   //leio o campo "removido"
    fread(&(registro->tamanhoRegistro), 4, 1, file);    //leio o indicador de tamanho do registro
    fread(&(registro->encadeamentoLista), 8, 1, file);  //leio o campo "encadeamentoLista"
    fread(&(registro->idServidor), 4, 1, file);     //leio o campo "idServidor"
    fread(&(registro->salarioServidor), 8, 1, file);    //leio o campo "salarioServidor"
    fread(registro->telefoneServidor, 1, 14, file);     //leio o campo "telefoneServidor"

    if ((ftell(file)-ini) == (registro->tamanhoRegistro+5)) {    //se o registro ja acabou, os campos "nomeServidor" e "cargoServidor" sao nulos e, portanto, nao presentes no arquivo
        //if (registro->nomeServidor != NULL) free(registro->nomeServidor);   //libero memoria anteriormente alocada
        //if (registro->cargoServidor != NULL) free(registro->cargoServidor); //libero memoria anteriormente alocada
        registro->nomeServidor = NULL;
        registro->cargoServidor = NULL;
    }
    else {
        char c = fgetc(file);
        if (c == '@') {     //eh o ultimo registro de uma pagina de disco
            //if (registro->nomeServidor != NULL) free(registro->nomeServidor);   //libero memoria anteriormente alocada
            //if (registro->cargoServidor != NULL) free(registro->cargoServidor); //libero memoria anteriormente alocada
            registro->nomeServidor = NULL;
            registro->cargoServidor = NULL;
        }
        else {
            ungetc(c, file);    //devolvo o caracter para a entrada

            int tamCampo;
            fread(&(tamCampo), 4, 1, file);     //leio o indicador de tamanho do campo
            char tag = fgetc(file);     //leio a tag do campo

            if (tag == 'n') {
                registro->tamCampo4 = tamCampo;
                registro->tagCampo4 = tag;
                registro->nomeServidor = malloc(100*sizeof(char));
                fread(registro->nomeServidor, 1, tamCampo-1, file);   //leio o campo "nomeServidor"

                if ((ftell(file)-ini) == (registro->tamanhoRegistro+5)) {    //se o registro ja acabou, o campo "cargoServidor" eh nulo e, portanto, nao presente no arquivo
                    //if (registro->cargoServidor != NULL) free(registro->cargoServidor); //libero memoria anteriormente alocada
                    registro->cargoServidor = NULL;
                }
                else {
                    c = fgetc(file);
                    if (c == '@') {     //eh o ultimo registro de uma pagina de disco
                        ///if (registro->cargoServidor != NULL) free(registro->cargoServidor); //libero memoria anteriormente alocada
                        registro->cargoServidor = NULL;
                    }
                    else {
                        ungetc(c, file);    //"devolvo" o byte lido para o arquivo
                        fread(&(registro->tamCampo5), 4, 1, file);     //leio o indicador de tamanho do campo
                        registro->tagCampo5 = fgetc(file);     //leio a tag do campo
                        registro->cargoServidor = malloc(100*sizeof(char));
                        fread(registro->cargoServidor, 1, registro->tamCampo5-1, file);   //leio o campo "cargoServidor"
                    }
                }
            }
            else {
                //if (registro->nomeServidor != NULL) free(registro->nomeServidor);   //libero memoria anteriormente alocada
                registro->nomeServidor = NULL;

                registro->tamCampo5 = tamCampo;
                registro->tagCampo5 = tag;
                registro->cargoServidor = malloc(100*sizeof(char));
                fread(registro->cargoServidor, 1, tamCampo-1, file);
            }
        }
    }

    fseek(file, ini, SEEK_SET);     //volto ao inicio do registro

    return;
}

/*
    Muda o campo de um registro de dados, de acordo
    com valores passados para a funcao.

    Parametros:
        regDados *registro - registro a ser modificado
        char *nomeCampo - nome do campo a ser modificado
        byte *valorCampo - valor novo do campo
    Retorno:
        int - retorna a diferenca entre o tamanho do
    registro original e o novo registro; sera negativo
    caso o registro novo seja maior que o original.
*/
int mudaRegistro(regDados *registro, char *nomeCampo, byte *valorCampo) {
    int tamOriginal = registro->tamanhoRegistro;
    char *valorSemAspas;    //sera usado para retirar as aspas dos valores string

    if (strcmp(nomeCampo, "idServidor") == 0) {
        registro->idServidor = atoi(valorCampo);
    }
    else if (strcmp(nomeCampo, "salarioServidor") == 0) {
        if (strcmp(valorCampo, "NULO") == 0) {  //se o valor for nulo...
            registro->salarioServidor = -1;
        }
        else {
            registro->salarioServidor = atof(valorCampo);
        }
    }
    else if (strcmp(nomeCampo, "telefoneServidor") == 0) {
        if (strcmp(valorCampo, "NULO") == 0) {  //se o valor for nulo...
            registro->telefoneServidor[0] = '\0';
            for (int i = 1; i < 14; i++) registro->telefoneServidor[i] = '@';   //completo com lixo
        }
        else {
            valorSemAspas = strtok(valorCampo, "\"");
            for (int i = 0; i < 14; i++) registro->telefoneServidor[i] = valorSemAspas[i];
        }
    }
    else if (strcmp(nomeCampo, "nomeServidor") == 0) {
        if (strcmp(valorCampo, "NULO") == 0) {  //se o valor for nulo...
            registro->nomeServidor = NULL;
        }
        else {
            valorSemAspas = strtok(valorCampo, "\"");
            if (registro->nomeServidor == NULL) registro->nomeServidor = malloc(100*sizeof(char));
            strcpy(registro->nomeServidor, valorSemAspas);
            registro->tamCampo4 = strlen(registro->nomeServidor) + 2;
        }
    }
    else if (strcmp(nomeCampo, "cargoServidor") == 0) {
        if (strcmp(valorCampo, "NULO") == 0) {  //se o valor for nulo...
            registro->cargoServidor = NULL;
        }
        else {
            valorSemAspas = strtok(valorCampo, "\"");
            if (registro->cargoServidor == NULL) registro->cargoServidor = malloc(100*sizeof(char));
            strcpy(registro->cargoServidor, valorSemAspas);
            registro->tamCampo5 = strlen(registro->cargoServidor) + 2;
        }
    }

    int tamNovo;

    //calculo o tamanho total do registro modificado
    if (registro->nomeServidor == NULL && registro->cargoServidor == NULL) {
        tamNovo = 34;
        //34 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor e 14 do telefoneServidor
    }
    else if (registro->cargoServidor == NULL) {
        tamNovo = 38 + registro->tamCampo4;
        //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 4
    }
    else if (registro->nomeServidor == NULL) {
        tamNovo = 38 + registro->tamCampo5;
        //38 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor e mais 4 do indicador de tamanho do campo 5
    }
    else {
        tamNovo = 42 + registro->tamCampo4 + registro->tamCampo5;
        //42 -> 8 do encadeamentoLista, 4 do idServidor, 8 do salarioServidor, 14 do telefoneServidor, 4 do indicador de tamanho do campo 4 e mais 4 do indicador de tamanho do campo 5
    }

    if (tamNovo > registro->tamanhoRegistro) registro->tamanhoRegistro = tamNovo;

    return (tamOriginal-tamNovo);
}

/*
    Busca no arquivo binario registros que satisfacam
    um criterio de busca determinado pelo usuario,
    atualizando-os assim que sao encontrados.
    O usuario deve informar quantas buscas diferentes
    deseja fazer perante uma mesma execucao. Alem disso,
    deve informar, em cada uma delas, o nome e o valor
    do campo a ser buscado e, também, o nome e o valor
    do campo a ser atualizado, que pode ou nao ser igual
    ao campo de busca.
*/
void atualizaReg() {
    char fileName[51];   //vai guardar o nome do arquivo a ser aberto
    int n;      //numero de remocoes a serem realizadas
    char nomeCampo[51];    //campo a ser considerado na busca
    byte valorCampo[200];   //valor a ser considerado na busca
    char nomeAtualiza[51];      //nome do campo a ser atualizado
    byte valorAtualiza[200];    //valor do campo a ser atualizado
    long long posUltimoReg = -1;  //ira guardar o byte offset do ultimo registro do arquivo

    scanf("%50s", fileName);
    scanf("%d", &n);

    FILE *binFile = fopen(fileName, "rb+");  //abre o arquivo "fileName" para leitura e escrita binária

    if (binFile == NULL) {   //erro na abertura do arquivo
      printf("Falha no processamento do arquivo.");
      return;
    }

    if (fgetc(binFile) == '0') {   //se o byte "status" for '0', entao o arquivo esta inconsistente
      printf("Falha no processamento do arquivo.");
      return;
    }
    ungetc('0', binFile);  //como o arquivo foi aberto para escrita, seu status deve ser '0'

    fseek(binFile, TAMPAG, SEEK_CUR);  //pulo o registro de cabecalho

    for (int i = 0; i < n; i++) {
        memset(nomeCampo, 0, 51);   //limpo o vetor (setando tudo para 0)
        memset(valorCampo, 0, 200);  //limpo o vetor (setando tudo para 0)
        memset(nomeAtualiza, 0, 51);   //limpo o vetor (setando tudo para 0)
        memset(valorAtualiza, 0, 200);  //limpo o vetor (setando tudo para 0)

        scanf("%50s ", nomeCampo);

        char c = fgetc(stdin);
        if (c == '\"') {    //se o valor comecar com uma aspas, eh uma string com espacos (ou seja, nao pode ser lida do jeito convencional)
            scanf("%[^\"]", valorCampo);    //paro de ler a string na ultima aspas
            fgetc(stdin);   //pulo um char
            fgetc(stdin);   //pulo mais um char
        } else {    //o valor nao tem espacos (pode ser lido normalmente)
            ungetc(c, stdin);   //devolvo o char lido para a entrada
            scanf("%s ", valorCampo);
        }

        scanf("%50s %[^\r\n]", nomeAtualiza, valorAtualiza);  //paro de ler antes da quebra de linha

        byte b = fgetc(binFile);

        if (feof(binFile)) {   //se o primeiro byte da primeira pagina de disco contendo os registros de dados for o final do arquivo, entao nao existem registros para serem mostrados
          printf("Registro inexistente.");
          return;
        }

        //aqui comeca a comparacao por campos

        int lidos = 0;    //guardara provisoriamente quantos bytes ja se leram de um registro
        regDados registro;  //estrutura de dados que guardara os dados lidos de um registro

        if (strcmp(nomeCampo, "idServidor") == 0) {     //se o campo a ser buscado eh "idServidor"...

            while (!feof(binFile)) {
                if (b == '-') {     //se ele nao esta removido...
                    int indicTam;
                    fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                    lidos += 8;

                    int valor;
                    fread(&valor, 4, 1, binFile);  //leio o valor do campo "idServidor"
                    lidos += 4;

                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                    if (valor == atoi(valorCampo)) {    //se o valor lido eh igual ao do dado como criterio de busca...
                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                        leRegistro(binFile, &registro); //importo o registro do arquivo binario
                        int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                        if (diff < 0) {     //o registro novo eh maior do que o original
                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                            fputc('*', binFile);    //marco o registro como REMOVIDO
                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                            long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                            if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                        }
                        else {
                            insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                            for (int i = 0; i < diff; i++) {
                                fputc('@', binFile);    //completo o registro com lixo
                            }
                        }

                        break;  //ja que o numero do idServidor eh unico, se acharmos um igual nao precisaremos mais continuar procurando
                    }

                    fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                    lidos = 0;
                }

                else if (b == '*') {    //se ele esta removido...
                    int pulo = 0;
                    fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                    fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                }

                b = fgetc(binFile);
            }

        }
        else if (strcmp(nomeCampo, "salarioServidor") == 0) {     //se o campo a ser buscado eh "salarioServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...
                valorCampo[0] = '-';
                valorCampo[1] = '1';
                valorCampo[2] = '\0';
            }

            while (!feof(binFile)) {
                if (b == '-') {     //se ele nao esta removido...
                    int indicTam;
                    fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                    lidos += 8;

                    fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                    lidos += 4;

                    double valor;
                    fread(&valor, 8, 1, binFile);  //leio o valor do campo "salarioServidor"
                    lidos += 8;

                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                    if (valor == atof(valorCampo)) {    //se o valor lido eh igual ao do dado como criterio de busca...
                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                        leRegistro(binFile, &registro); //importo o registro do arquivo binario
                        int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                        if (diff < 0) {     //o registro novo eh maior do que o original
                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                            fputc('*', binFile);    //marco o registro como REMOVIDO
                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                            long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                            if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                            fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                        }
                        else {
                            insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                            for (int i = 0; i < diff; i++) {
                                fputc('@', binFile);    //completo o registro com lixo
                            }
                        }
                    }
                    else {
                        fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                    }

                    lidos = 0;
                }

                else if (b == '*') {    //se ele esta removido...
                    int pulo = 0;
                    fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                    fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                }

                b = fgetc(binFile);
            }

        }
        else if (strcmp(nomeCampo, "telefoneServidor") == 0) {     //se o campo a ser buscado eh "telefoneServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...
                valorCampo[0] = '\0';
            }

            while (!feof(binFile)) {
                if (b == '-') {     //se ele nao esta removido...
                    int indicTam;
                    fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                    lidos += 8;

                    fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                    lidos += 4;

                    fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                    lidos += 8;

                    char valor[15];
                    fread(valor, 1, 14, binFile);  //leio o valor do campo "telefoneServidor"
                    valor[14] = '\0';   //"encerro" a string
                    lidos += 14;

                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                    if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                        leRegistro(binFile, &registro); //importo o registro do arquivo binario
                        int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                        if (diff < 0) {     //o registro novo eh maior do que o original
                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                            fputc('*', binFile);    //marco o registro como REMOVIDO
                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                            long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                            if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                            fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                        }
                        else {
                            insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                            for (int i = 0; i < diff; i++) {
                                fputc('@', binFile);    //completo o registro com lixo
                            }
                        }

                    }
                    else {
                        fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                    }

                    lidos = 0;
                }

                else if (b == '*') {    //se ele esta removido...
                    int pulo = 0;
                    fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                    fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                }

                b = fgetc(binFile);
            }

        }
        else if (strcmp(nomeCampo, "nomeServidor") == 0) {     //se o campo a ser buscado eh "nomeServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos == indicTam+4) {  //se o registro ja terminou, nao possui o campo "nomeServidor"
                            fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                            leRegistro(binFile, &registro); //importo o registro do arquivo binario
                            int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                            if (diff < 0) {     //o registro novo eh maior do que o original
                                long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                fputc('*', binFile);    //marco o registro como REMOVIDO
                                adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                            }
                            else {
                                insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                for (int i = 0; i < diff; i++) {
                                    fputc('@', binFile);    //completo o registro com lixo
                                }
                            }

                        }
                        else {   //o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {    //ele eh o ultimo de uma pagina de disco
                                fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                if (diff < 0) {     //o registro novo eh maior do que o original
                                    long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                    fputc('*', binFile);    //marco o registro como REMOVIDO
                                    adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                    completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                    long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                    if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                }
                                else {
                                    insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                    for (int i = 0; i < diff; i++) {
                                        fputc('@', binFile);    //completo o registro com lixo
                                    }
                                }

                                fseek(binFile, indicTam+4, SEEK_CUR);   //pulo o lixo "acoplado" a ele
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;
                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'c') {   //o campo "nomeServidor" nao existe nesse registro de dados
                                    fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                    leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                    int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                    if (diff < 0) {     //o registro novo eh maior do que o original
                                        long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                        fputc('*', binFile);    //marco o registro como REMOVIDO
                                        adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                        completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                        long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                        if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                    }
                                    else {
                                        insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                        for (int i = 0; i < diff; i++) {
                                            fputc('@', binFile);    //completo o registro com lixo
                                        }
                                    }

                                    fseek(binFile, indicTam+4, SEEK_CUR);  //vou para o proximo registro
                                }
                                else {
                                    fseek(binFile, indicTam+4-lidos, SEEK_CUR);  //vou para o proximo registro
                                }
                            }
                        }

                        lidos = 0;
                    }
                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }
            else {

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos != indicTam+4) {   //se o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {
                                //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                fseek(binFile, indicTam+4 - lidos, SEEK_CUR);
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;

                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'c') {   //o campo "nomeServidor" nao existe nesse registro de dados
                                    fseek(binFile, indicTam+4 - lidos, SEEK_CUR);  //vou para o proximo registro
                                }
                                else {  //ele existe
                                    char valor[200];
                                    fread(valor, 1, tamCampo-1, binFile);  //leio o valor do campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                    if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                                        leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                        int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                        if (diff < 0) {     //o registro novo eh maior do que o original
                                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                            fputc('*', binFile);    //marco o registro como REMOVIDO
                                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                            long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                            if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                            fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                        }
                                        else {
                                            insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                            for (int i = 0; i < diff; i++) {
                                                fputc('@', binFile);    //completo o registro com lixo
                                            }
                                        }
                                    }
                                    else {
                                        fseek(binFile, indicTam+4, SEEK_CUR);   //pulo o lixo "acoplado" a ele
                                    }

                                }
                            }
                        }

                        lidos = 0;
                    }
                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }

        }
        else if (strcmp(nomeCampo, "cargoServidor") == 0) {     //se o campo a ser buscado eh "cargoServidor"...

            if (strcmp(valorCampo, "NULO") == 0) {  //se o valor do campo for nulo...

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos == indicTam+4) {   //se o registro ja terminou, nao tem o campo "cargoServidor"
                            fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                            leRegistro(binFile, &registro); //importo o registro do arquivo binario
                            int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                            if (diff < 0) {     //o registro novo eh maior do que o original
                                long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                fputc('*', binFile);    //marco o registro como REMOVIDO
                                adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                fseek(binFile, indicTam+4, SEEK_CUR);   //vou para o proximo registro
                            }
                            else {
                                insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                for (int i = 0; i < diff; i++) {
                                    fputc('@', binFile);    //completo o registro com lixo
                                }
                            }
                        }
                        else {   //o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {    //ele eh o ultimo de uma pagina de disco
                                fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                if (diff < 0) {     //o registro novo eh maior do que o original
                                    long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                    fputc('*', binFile);    //marco o registro como REMOVIDO
                                    adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                    completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                    long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                    if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                    fseek(binFile, indicTam+4, SEEK_CUR);   //vou para o proximo registro
                                }
                                else {
                                    insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                    for (int i = 0; i < diff; i++) {
                                        fputc('@', binFile);    //completo o registro com lixo
                                    }
                                }
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;

                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'n') {   //o campo "cargoServidor" pode nao existir
                                    fseek(binFile, tamCampo-1, SEEK_CUR);  //pulo o campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    if (lidos == indicTam+4) {   //se o registro ja terminou, nao possui o campo "cargoServidor"
                                        fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                        leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                        int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                        if (diff < 0) {     //o registro novo eh maior do que o original
                                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                            fputc('*', binFile);    //marco o registro como REMOVIDO
                                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                            long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                            if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                            fseek(binFile, indicTam+4, SEEK_CUR);   //vou para o proximo registro
                                        }
                                        else {
                                            insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                            for (int i = 0; i < diff; i++) {
                                                fputc('@', binFile);    //completo o registro com lixo
                                            }
                                        }
                                    }
                                    else {   //se o registro ainda nao terminou, ha chance do campo existir
                                        b = fgetc(binFile);    //leio o proximo byte do registro
                                        lidos += 1;

                                        if (b == '@') {    //ele eh o ultimo de uma pagina de disco
                                            fseek(binFile, -(lidos+1), SEEK_CUR);    //volto ao inicio do registro
                                            leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                            int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                            if (diff < 0) {     //o registro novo eh maior do que o original
                                                long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                                fputc('*', binFile);    //marco o registro como REMOVIDO
                                                adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                                completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                                long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                                if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                                fseek(binFile, indicTam+4, SEEK_CUR);   //vou para o proximo registro
                                            }
                                            else {
                                                insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                                for (int i = 0; i < diff; i++) {
                                                    fputc('@', binFile);    //completo o registro com lixo
                                                }
                                            }
                                        }
                                        else {
                                            fseek(binFile, indicTam+4-lidos, SEEK_CUR);  //vou para o proximo registro
                                        }
                                    }
                                }
                                else {
                                    fseek(binFile, indicTam+4-lidos, SEEK_CUR);  //vou para o proximo registro
                                }
                            }
                        }

                        lidos = 0;
                    }

                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }
            }
            else {

                while (!feof(binFile)) {
                    if (b == '-') {     //se ele nao esta removido...
                        /* if (strcmp(valorCampo, "ENGENHEIRO") == 0 && ( strcmp(valorAtualiza, "\"ENGENHEIRO ELETRICISTA\"") == 0 || strcmp(valorAtualiza, "\"ENGENHEIRO ELETRICISTA") == 0)) {
                            printf("\n\nByte Offset atual: %ld\n", ftell(binFile)-1);
                            regDados reg;
                            fseek(binFile, -1, SEEK_CUR);
                            leRegistro(binFile, &reg);
                            fseek(binFile, 1, SEEK_CUR);
                            printRegistro(&reg);
                        } */
                        int indicTam;
                        fread(&indicTam, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do registro
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "encadeamentoLista" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 4, SEEK_CUR);  //pulo o campo "idServidor" (4 bytes)
                        lidos += 4;

                        fseek(binFile, 8, SEEK_CUR);  //pulo o campo "salarioServidor" (8 bytes)
                        lidos += 8;

                        fseek(binFile, 14, SEEK_CUR);  //pulo o campo "telefoneServidor" (14 bytes)
                        lidos += 14;

                        if (lidos != indicTam+4) {   //se o registro nao terminou...
                            b = fgetc(binFile);    //leio o proximo byte do registro
                            lidos += 1;

                            if (b == '@') {
                                //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                fseek(binFile, indicTam+4 - lidos, SEEK_CUR);
                            }
                            else {
                                ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                lidos -= 1;

                                int tamCampo;
                                fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                lidos += 4;

                                char tag = fgetc(binFile);   //leio a tag do campo
                                lidos += 1;

                                if (tag == 'n') {   //o campo "cargoServidor" pode nao existir
                                    fseek(binFile, tamCampo-1, SEEK_CUR);  //pulo o campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    if (lidos != indicTam+4) {   //se o registro ainda nao terminou...
                                        b = fgetc(binFile);    //leio o proximo byte do registro
                                        lidos += 1;

                                        if (b == '@') {
                                            //ele eh o ultimo de uma pagina de disco, e o lixo "acoplado" a ele deve ser pulado
                                            fseek(binFile, indicTam+4 - lidos, SEEK_CUR);
                                        }
                                        else {
                                            ungetc(b, binFile);    //"devolvo" o byte lido para o arquivo
                                            lidos -= 1;

                                            fread(&tamCampo, 4, 1, binFile);   //leio e armazeno o valor do indicador de tamanho do campo
                                            lidos += 4;

                                            fgetc(binFile);   //pulo a tag do campo
                                            lidos += 1;

                                            char valor[200];
                                            fread(valor, 1, tamCampo-1, binFile);  //leio o valor do campo "nomeServidor"
                                            lidos += (tamCampo-1);

                                            fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                            if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                                fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                                                leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                                    if (registro.idServidor == 2416299) printRegistro(&registro);
                                                int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)
                                                    if (registro.idServidor == 2416299) printRegistro(&registro);
                                                    if (registro.idServidor == 2416299) printf("Diff: %d\n", diff);
                                                if (diff < 0) {     //o registro novo eh maior do que o original
                                                    long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                                    fputc('*', binFile);    //marco o registro como REMOVIDO
                                                    adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                                    completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                                    long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                                    if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                                    fseek(binFile, indicTam+4 - lidos, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                                }
                                                else {
                                                    insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                                    for (int i = 0; i < diff; i++) {
                                                        fputc('@', binFile);    //completo o registro com lixo
                                                    }
                                                }
                                            }
                                            else {
                                                fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                            }

                                        }
                                    }
                                }
                                else {  //ele existe
                                    char valor[200];
                                    fread(valor, 1, tamCampo-1, binFile);  //leio o valor do campo "nomeServidor"
                                    lidos += (tamCampo-1);

                                    fseek(binFile, -lidos, SEEK_CUR);    //volto ao inicio do registro

                                    if (strcmp(valor, valorCampo) == 0) {    //se o valor lido eh igual ao do dado como criterio de busca...
                                        fseek(binFile, -1, SEEK_CUR);   //vou para o comeco do campo "removido"
                                        leRegistro(binFile, &registro); //importo o registro do arquivo binario
                                        int diff = mudaRegistro(&registro, nomeAtualiza, valorAtualiza);    //atualizo o registro (fora do arquivo)

                                        if (diff < 0) {     //o registro novo eh maior do que o original
                                            long long byteOffset = ftell(binFile);  //guardo o byte offset do registro a ser logicamente removido
                                            fputc('*', binFile);    //marco o registro como REMOVIDO
                                            adicionaLista(binFile, byteOffset, indicTam);    //adiciono o registro a lista de removidos
                                            completaLixo(binFile);  //sobreescrevo todos os campos do registro (menos encadeamentoLista) com lixo

                                            long long temp = achaPosicaoInsere(binFile, &registro, posUltimoReg);  //adiciona o registro novo ao arquivo
                                            if (temp != -1) posUltimoReg = temp;    //se o valor retornado pela funcao atualiza o BO do ultimo registro da lista, entao este valor deve ser guardado
                                            fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                        }
                                        else {
                                            insereRegistro(binFile, &registro); //gravo o registro atualizado no arquivo
                                            for (int i = 0; i < diff; i++) {
                                                fputc('@', binFile);    //completo o registro com lixo
                                            }
                                        }
                                    }
                                    else {
                                        fseek(binFile, indicTam+4, SEEK_CUR);    //vou para o proximo registro (+4 por conta dos bytes do indicador de tamanho, que ele mesmo nao contabiliza)
                                    }

                                }
                            }
                        }

                        lidos = 0;
                    }

                    else if (b == '*') {    //se ele esta removido...
                        int pulo = 0;
                        fread(&pulo, 4, 1, binFile);   //lera o indicador de tamanho do registro (4 bytes)
                        fseek(binFile, pulo, SEEK_CUR);    //pula o registro
                    }

                    b = fgetc(binFile);
                }

            }

        }
        else {  //o usuario digitou errado o nome do campo
            printf("Falha no processamento do arquivo.");
            return;
        }

        fseek(binFile, TAMPAG, SEEK_SET);    //volto o ponteiro de leitura para o inicio da segunda pagina de disco (a que inicia os registros de dados)
    }

    binarioNaTela1(binFile);

    //antes de fechar o arquivo, coloco seu status para '1'
    fseek(binFile, 0, SEEK_SET);  //coloco o ponteiro de escrita no primeiro byte do arquivo
    byte status = '1';
    fwrite(&status, 1, 1, binFile);  //sobrescrevo o campo "status" do arquivo binario
    fclose(binFile);

    return;
}

/*
  Cuida da execucao do programa.
*/
int main() {
    int opt; //vai guardar a opcao digitada pelo usuario
    scanf("%d ", &opt);

    switch (opt) {
        case 1:
            leCSV();
            break;
        case 2:
            mostraBin();
            break;
        case 3:
            buscaReg();
            break;
        case 4:
            removeReg();
            break;
        case 5:
            adicionaReg();
            break;
        case 6:
            atualizaReg();
            break;
        default:
            printf("Opção inválida!\n");
    }

    return 0;
}
