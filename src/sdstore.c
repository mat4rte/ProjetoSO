#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
/*

gcc -o sdstore.c sdstore

./sdstore camFicheiro camFicheirpTransf t1 t2 t3 t4

1) Pedido Pendente
2) Pedido Em Processamento
3) Pedido Concluído

-- comando termina --

*status*
 ./sdstore status
consultar estado de funcionamento do servidor (pedidos de processamento em execução, estado da utilizacao das transformações)




*/

int main(int argc, char *argv[])
{

    char comandos[300] = "";

    // Cria Pipe Privado
    char fifo_privado[10];
    char path_public[30];
    char path_privado[30];
    sprintf(fifo_privado, "fifo%d", getpid());
    sprintf(path_privado, "tmp/%s", fifo_privado);
    sprintf(path_public, "tmp/fifoPublic");

    // comandos[0] = fifo_privado; //guarda pid na primeira posicao do array
    // write(1, path_privado, strlen(path_privado) + 1);
    if (mkfifo(path_privado, 0666) < 0)
    {
        perror("mkfifo");
        return 1;
    }
    int fifo_Public = open(path_public, O_WRONLY);

    if (fifo_Public < 0)
    {
        perror("open Public");
    }

    if (argc >= 2)
    {

        // escreve comandos numa string e envia para o servidor para pipe publico
        strcat(comandos, fifo_privado);
        for (int i = 1; i < argc; i++)
        {
            strcat(comandos, " ");
            strcat(comandos, argv[i]);
        }
        strcat(comandos, "\0");
        write(fifo_Public, comandos, strlen(comandos) + 1);
        close(fifo_Public);

        close(fifo_Public);
    }

    int fifo_Private = open(path_privado, O_RDONLY);
    if (fifo_Private < 0)
    {
        perror("open Private");
    }

    // escreve mensagens do pipe privado
    char mens[100];
    int bytes_read = 0;
    while ((bytes_read = read(fifo_Private, mens, 1024)) > 0)
    {
        write(1, mens, bytes_read);
    }

    return 1;
}