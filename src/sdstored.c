#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>

/*

gcc -o sdstored.c sdstored

./sdstored conf Ficheiros/


1) Suportar vários clientes e pedidos

2) Enquanto pedido estiver a ser usado, todas as transformacoes pedidas devem ser identificadas como em utilizacao

3) O numero de instancias de uma transformacao estiver no max, o pedido fica em pausa

        comandos[0] //pipe privado
        comandos[1] //nome do comando (status/proc-file)
        -- caso seja proc --
        comandos[2] //nome do ficheiro original
        comandos[3] //nome do ficheiro final
        comandos[4,5,6,7,8] //filtros aplicados
*/

int *nope_usados;
int *bcompress_usados;
int *bdecompress_usados;
int *gcompress_usados;
int *gdecompress_usados;
int *encrypt_usados;
int *decrypt_usados;
int *num_tarefa;

int status;

int private_pipe;

//[nome][num_max]
char nop[2][30];
char bcompress[2][30];
char bdecompress[2][30];
char gcompress[2][30];
char gdecompress[2][30];
char encrypt[2][30];
char decrypt[2][30];

char info_nop[50];
char info_bcompress[50];
char info_bdecompress[50];
char info_gcompress[50];
char info_gdecompress[50];
char info_encrypt[50];
char info_decrypt[50];

char *tasks[30];

int num_transformacoes_usadas = 0;
char *transformacoes_usadas[10];
char *transformacoes[7] = {"nop", "bcompress", "bdecompress", "gcompress", "gdecompress", "encrypt", "decrypt"};

char *comandos[16];
char executar[30];

void aplica_transf()
{

    int fdin;
    int fdout;

    if (fork() == 0)
    {

        fdin = open(comandos[2], O_RDONLY, 0644);
        dup2(fdin, 0);
        close(fdin);

        fdout = creat(comandos[3], 0644);
        dup2(fdout, 1);
        close(fdout);

        int p[2];
        int i;

        for (i = 4; i < 4 + num_transformacoes_usadas - 1; i++)
        {

            // Sempre que vai aplicar nova transformação, enviar aviso que está a processar
            char *a_processar = "A processar...\n\0";
            write(private_pipe, a_processar, strlen(a_processar) + 1);
            sleep(1);

            // Path para o executavel da trans
            // printf("exec: %s\n", pathT);
            char pathT[30];

            // ler no p[0]
            // escrever no p[1]
            pipe(p);

            if (fork() == 0)
            {

                dup2(p[1], 1);
                close(p[0]);
                close(p[1]);

                strcpy(pathT, executar);
                strcat(pathT, "/");
                strcat(pathT, comandos[i]);
                execl(pathT, pathT, NULL);
            }
            else
            {
                dup2(p[0], 0);
                close(p[0]);
                close(p[1]);
            }
        }

        char *a_processar = "A processar...\n\0";
        write(private_pipe, a_processar, strlen(a_processar) + 1);

        // ultimo
        //  Path para o executavel da trans
        char pathT[30];
        strcpy(pathT, executar);
        strcat(pathT, "/");
        strcat(pathT, comandos[i]);
        execl(pathT, pathT, NULL);
    }
    wait(&status);
    printf("DONE!");
}

// Ler Fichero de Configuração
void ler_config(FILE *f_config)
{

    char buf[30];
    int fil = 0;
    int pos = 0;

    while ((fscanf(f_config, "%s", buf) != EOF))
    {

        if (pos % 2 == 0)
            fil = 0;

        if ((pos >= 0 && pos < 2))
        {
            strcpy(nop[fil], buf);
        }
        else if (pos >= 2 && pos < 4)
        {
            strcpy(bcompress[fil], buf);
        }
        else if (pos >= 4 && pos < 6)
        {
            strcpy(bdecompress[fil], buf);
        }
        else if (pos >= 6 && pos < 8)
        {
            strcpy(gcompress[fil], buf);
        }
        else if (pos >= 8 && pos < 10)
        {
            strcpy(gdecompress[fil], buf);
        }
        else if (pos >= 10 && pos < 12)
        {
            strcpy(encrypt[fil], buf);
        }
        else if (pos >= 12 && pos < 14)
        {
            strcpy(decrypt[fil], buf);
        }
        pos++;
        fil++;
    }
}

int main(int argc, char *argv[])
{
    int *nop_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *bcompress_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *bdecompress_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *gcompress_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *gdecompress_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *encrypt_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *decrypt_usados = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *num_tarefa = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    *nop_usados = 0;
    *bcompress_usados = 0;
    *bdecompress_usados = 0;
    *gcompress_usados = 0;
    *gdecompress_usados = 0;
    *encrypt_usados = 0;
    *decrypt_usados = 0;
    *num_tarefa = 0;

    if (argc != 3)
        return -1;

    char comandos_juntos[300] = "";
    strcpy(executar, argv[2]);

    char path_trans[100];
    strcpy(path_trans, argv[2]);

    FILE *f_config = fopen(argv[1], "r");
    if (f_config == NULL)
    {
        perror("open");
        return -1;
    }

    // Ler Fichero de Configuração
    ler_config(f_config);

    // criar pipe publico
    char path[30] = "tmp/fifoPublic";
    if (mkfifo(path, 0666) < 0)
    {
        perror("mkfifo");
        return 1;
    }

    int public_fifo = open(path, O_RDONLY);
    int decoy_fifo;
    decoy_fifo = open(path, O_WRONLY);

    // Erro a abrir pipe
    if (public_fifo < 0)
    {
        perror("open public fifo");
    }

    printf("FifoPublic is open\n");

    while (1)
    {

        strcpy(comandos_juntos, "\0");
        read(public_fifo, comandos_juntos, 1024);
        write(1, comandos_juntos, strlen(comandos_juntos));
        write(1, "\n", 2);

        strcpy(executar, argv[2]);
        num_transformacoes_usadas = 0;

        if (fork() == 0)
        {

            int nop_usados_tarefa = 0;
            int bcompress_usados_tarefa = 0;
            int bdecompress_usados_tarefa = 0;
            int gcompress_usados_tarefa = 0;
            int gdecompress_usados_tarefa = 0;
            int encrypt_usados_tarefa = 0;
            int decrypt_usados_tarefa = 0;

            int pode_correrNop = 1;
            int pode_correrBcompress = 1;
            int pode_correrBdecompress = 1;
            int pode_correrGcompress = 1;
            int pode_correrGdecompress = 1;
            int pode_correrEncrypt = 1;
            int pode_correrDecrypt = 1;

            int pos = 0;
            char *palavra;
            palavra = strtok(comandos_juntos, " ");
            // guarda trans usadas e quantos foram usados da cada
            while (palavra != NULL)
            {
                comandos[pos] = palavra;
                transformacoes_usadas[num_transformacoes_usadas] = palavra;
                num_transformacoes_usadas++;

                if (strcmp(palavra, "nop") == 0)
                    nop_usados_tarefa++;
                else if (strcmp(palavra, "bcompress") == 0)
                    bcompress_usados_tarefa++;
                else if (strcmp(palavra, "bdecompress") == 0)
                    bdecompress_usados_tarefa++;
                else if (strcmp(palavra, "gcompress") == 0)
                    gcompress_usados_tarefa++;
                else if (strcmp(palavra, "gdecompress") == 0)
                    gdecompress_usados_tarefa++;
                else if (strcmp(palavra, "encrypt") == 0)
                    encrypt_usados_tarefa++;
                else if (strcmp(palavra, "decrypt") == 0)
                    decrypt_usados_tarefa++;

                palavra = strtok(NULL, " ");

                pos++;
            }

            char private_path[40] = "tmp/";
            strcat(private_path, comandos[0]);
            private_pipe = open(private_path, O_WRONLY);
            if (private_pipe < 0)
            {
                perror("open private fifo");
            }
            if (strcmp(comandos[1], "proc-file") == 0)
            {
                char *em_espera = "Em Espera...\n\0";
                write(private_pipe, em_espera, strlen(em_espera) + 1);
            }

            if (strcmp(comandos[1], "status") == 0)
            {

                // for (int i = 0; i < 30; i++)
                // {
                //     write(private_pipe, tasks[i], strlen(tasks[i]));
                // }

                sprintf(info_nop, "filter nop: %d/%d (running/max)\n%c", *nop_usados, atoi(nop[1]), '\0');
                sprintf(info_bcompress, "filter bcompress: %d/%d (running/max)\n%c", *bcompress_usados, atoi(bcompress[1]), '\0');
                sprintf(info_bdecompress, "filter bdecompress: %d/%d (running/max)\n%c", *bdecompress_usados, atoi(bdecompress[1]), '\0');
                sprintf(info_gcompress, "filter gcompress: %d/%d (running/max)\n%c", *gcompress_usados, atoi(gcompress[1]), '\0');
                sprintf(info_gdecompress, "filter gdecompress: %d/%d (running/max)\n%c", *gdecompress_usados, atoi(gdecompress[1]), '\0');
                sprintf(info_encrypt, "filter encrypt: %d/%d (running/max)\n%c", *encrypt_usados, atoi(encrypt[1]), '\0');
                sprintf(info_decrypt, "filter decrypt: %d/%d (running/max)\n%c", *decrypt_usados, atoi(decrypt[1]), '\0');

                write(private_pipe, info_nop, strlen(info_nop) + 1);
                write(private_pipe, info_bcompress, strlen(info_bcompress) + 1);
                write(private_pipe, info_bdecompress, strlen(info_bdecompress) + 1);
                write(private_pipe, info_gcompress, strlen(info_gcompress) + 1);
                write(private_pipe, info_gdecompress, strlen(info_gdecompress) + 1);
                write(private_pipe, info_encrypt, strlen(info_encrypt) + 1);
                write(private_pipe, info_decrypt, strlen(info_decrypt) + 1);

                strcpy(info_nop, "");
                strcpy(info_bcompress, "");
                strcpy(info_bdecompress, "");
                strcpy(info_gcompress, "");
                strcpy(info_gdecompress, "");
                strcpy(info_encrypt, "");
                strcpy(info_decrypt, "");
            }
            if (strcmp(comandos[1], "proc-file") == 0)
            {
                num_transformacoes_usadas -= 4;
                // verifica se as trans que a task quer usar excedem o máximo
                if (nop_usados_tarefa > atoi(nop[1]) || bcompress_usados_tarefa > atoi(bcompress[1]) || bdecompress_usados_tarefa > atoi(bdecompress[1]) || gcompress_usados_tarefa > atoi(gcompress[1]) || gdecompress_usados_tarefa > atoi(gdecompress[1]) || encrypt_usados_tarefa > atoi(encrypt[1]) || decrypt_usados_tarefa > atoi(decrypt[1]))
                {
                    char *mens = "Transformações Pedidas Ultrapassam os limites!\n";
                    write(1, mens, strlen(mens));
                    break;
                }

                // Adiciona Task
                // sprintf(tasks[*num_tarefa], "task #%d: %s\n", *num_tarefa, comandos_juntos);
                for (int i = 0; i < *num_tarefa; i++)
                    strcat(tasks[i], comandos[i + 1]);
                // printf("%s\n", tasks[*num_tarefa]);
                // write(private_pipe, tasks[*num_tarefa], strlen(tasks[*num_tarefa]));
                *num_tarefa = *num_tarefa + 1;

                // ver se os filtros que quer usar estão no máximo
                do
                {
                    for (int i = 0; i < num_transformacoes_usadas; i++)
                    {
                        if (strcmp(transformacoes[i], "nop") == 0)
                            if (*nop_usados + nop_usados_tarefa > atoi(nop[1]))
                                pode_correrNop = 0;
                            else
                                pode_correrNop = 1;
                        else if (strcmp(transformacoes[i], "bcompress") == 0)
                            if (*bcompress_usados + bcompress_usados_tarefa > atoi(bcompress[1]))
                                pode_correrBcompress = 0;
                            else
                                pode_correrBcompress = 1;
                        else if (strcmp(transformacoes[i], "bdecompress") == 0)
                            if (*bdecompress_usados + bdecompress_usados_tarefa > atoi(bdecompress[1]))
                                pode_correrBdecompress = 0;
                            else
                                pode_correrBdecompress = 1;
                        else if (strcmp(transformacoes[i], "gcompress") == 0)
                            if (*gcompress_usados + gcompress_usados_tarefa > atoi(gcompress[1]))
                                pode_correrGcompress = 0;
                            else
                                pode_correrGcompress = 1;
                        else if (strcmp(transformacoes[i], "gdecompress") == 0)
                            if (*gdecompress_usados + gdecompress_usados_tarefa > atoi(gdecompress[1]))
                                pode_correrGdecompress = 0;
                            else
                                pode_correrGdecompress = 1;
                        else if (strcmp(transformacoes[i], "encrypt") == 0)
                            if (*encrypt_usados + encrypt_usados_tarefa > atoi(encrypt[1]))
                                pode_correrEncrypt = 0;
                            else
                                pode_correrEncrypt = 1;
                        else if (strcmp(transformacoes[i], "decrypt") == 0)
                        {
                            if (*decrypt_usados + decrypt_usados_tarefa > atoi(decrypt[1]))
                                pode_correrDecrypt = 0;
                            else
                                pode_correrDecrypt = 1;
                        }
                    }
                    usleep((rand() % 250) * 1000);

                } while (!pode_correrNop || !pode_correrBcompress || !pode_correrBdecompress || !pode_correrGcompress || !pode_correrGdecompress || !pode_correrEncrypt || !pode_correrDecrypt);
                // Depois de saber que pode executar, adiciona os filtros que vai usar
                for (int i = 0; i < num_transformacoes_usadas; i++)
                {

                    if (strcmp(transformacoes_usadas[i + 4], "nop") == 0)
                        *nop_usados = *nop_usados + 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "bcompress") == 0)
                        *bcompress_usados = *bcompress_usados + 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "bdecompress") == 0)
                        *bdecompress_usados = *bdecompress_usados + 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "gcompress") == 0)
                        *gcompress_usados = *gcompress_usados + 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "gdecompress") == 0)
                        *gdecompress_usados = *gdecompress_usados + 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "encrypt") == 0)
                        *encrypt_usados = *encrypt_usados + 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "decrypt") == 0)
                        *decrypt_usados = *decrypt_usados + 1;
                }

                aplica_transf();

                // Subtrai os filtros que usou
                for (int i = 0; i < num_transformacoes_usadas; i++)
                {
                    if (strcmp(transformacoes_usadas[i + 4], "nop") == 0)
                        *nop_usados = *nop_usados - 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "bcompress") == 0)
                        *bcompress_usados = *bcompress_usados - 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "bdecompress") == 0)
                        *bdecompress_usados = *bdecompress_usados - 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "gcompress") == 0)
                        *gcompress_usados = *gcompress_usados - 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "gdecompress") == 0)
                        *gdecompress_usados = *gdecompress_usados - 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "encrypt") == 0)
                        *encrypt_usados = *encrypt_usados - 1;
                    else if (strcmp(transformacoes_usadas[i + 4], "decrypt") == 0)
                        *decrypt_usados = *decrypt_usados - 1;
                }

                char *done = "Done!\n\0";
                write(private_pipe, done, strlen(done) + strlen("\n"));

                num_transformacoes_usadas = 0;
            }
            write(private_pipe, "\0", 1);
            close(private_pipe);
        }
    }
}