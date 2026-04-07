//Adriana Sánchez-Bravo Cuesta y Yuri Villaverde Sanmartin

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define TOTAL_LETRAS 20

//estructura hilos
typedef struct {
    int id;
    char **buffer;
    int *num_elementos;
    int entrada;
    int salida;
    sem_t *huecos;
    sem_t *elementos;
    sem_t *sem_buffer;
    // para buffer
    int *pos_e; 
    int *pos_s;
} args_hilo;

//contadores de progreso
int producidos = 0, consumidos= 0;

//para contar las vocales
int a = 0, e = 0, i_voc = 0, o = 0, u = 0;

FILE *f;

//funcion del productor
void* funcion_productor(void* arg) {
    //estructura del hilo
    args_hilo *a = (args_hilo*)arg;
    int id = a->id;
    int ids = a->salida;

    //para simplificar el acceso a los semaforos
    sem_t *h = &a->huecos[ids];
    sem_t *el = &a->elementos[ids];
    sem_t *b = &a->sem_buffer[ids];

    for (int n = 0; n < TOTAL_LETRAS; n++) {
        //leer datos
        int c = fgetc(f);
        char letra = (c != EOF) ? (char)c : ' ';

        //espera y bloquea el acceso a buffer
        sem_wait(h);
        sem_wait(b);

        //inserción en el buffer (Circular)
        a->buffer[ids][a->pos_e[ids]] = letra;
        a->pos_e[ids] = (a->pos_e[ids] + 1) % N;
        producidos++;

        printf("Productor %d mete '%c' (%d/%d)\n", id, letra, producidos, TOTAL_LETRAS);

        //liberar mutex
        sem_post(b);
        sem_post(el);

        usleep(100000);
    }

    //para enviar una señal de fin de datos cuando lee todo el archivo para que los demás hilos paren
    sem_wait(h);
    sem_wait(b);
    a->buffer[ids][a->pos_e[ids]] = '\0';
    a->pos_e[ids] = (a->pos_e[ids] + 1) % N;
    sem_post(b);
    sem_post(el);

    pthread_exit(NULL);
}

//funcion de consumidor
void* funcion_consumidor(void* arg) {
    //igual que el anterior pero el índice es de entrada al ser consumidor
    args_hilo *a2 = (args_hilo*)arg;    
    int id = a2->id;
    int ids = a2->entrada;

    sem_t *h = &a2->huecos[ids];
    sem_t *el = &a2->elementos[ids];
    sem_t *b = &a2->sem_buffer[ids];

    while (1) {
        //espera a que hayan elementos en el buffer y cuando los hay empieza a consumir y lo bloquea 
        sem_wait(el);
        sem_wait(b);

        char letra = a2->buffer[ids][a2->pos_s[ids]];
        a2->pos_s[ids] = (a2->pos_s[ids] + 1) % N;

        sem_post(b);
        sem_post(h);

        //si la letra es el fin del archivo, para
        if (letra == '\0') break;

        //aumenta el contador
        consumidos++;
        printf("\tConsumidor %d saca '%c' (%d/%d)\n", id, letra, consumidos, TOTAL_LETRAS);

        //transforma la letra a minúscula y cuenta si es alguna vocal
        char min = tolower(letra);
        if (min == 'a') a++;
        else if (min == 'e') e++;
        else if (min == 'i') i_voc++;
        else if (min == 'o') o++;
        else if (min == 'u') u++;

        usleep(150000);
    }

    pthread_exit(NULL);
}

//funcion de los hilos intermedios
void* funcion_intermedios(void* arg) {
    //estructura del hilo
    args_hilo *a = (args_hilo*)arg;
    int id = a->id;
    int ids_e = a->entrada;
    int ids_s = a->salida;

    //simplificar acceso a los semáforos
    sem_t *h_e = &a->huecos[ids_e];
    sem_t *el_e = &a->elementos[ids_e];
    sem_t *b_e = &a->sem_buffer[ids_e];

    sem_t *h_s = &a->huecos[ids_s];
    sem_t *el_s = &a->elementos[ids_s];
    sem_t *b_s = &a->sem_buffer[ids_s];

    while (1) {
        //consume del buffer de entrada como el consumidor
        sem_wait(el_e);
        sem_wait(b_e);

        char letra = a->buffer[ids_e][a->pos_s[ids_e]];
        a->pos_s[ids_e] = (a->pos_s[ids_e] + 1) % N;

        sem_post(b_e);
        sem_post(h_e);

        //con lo que consumió, produce en el buffer de salida como el productor
        sem_wait(h_s);
        sem_wait(b_s);

        if (letra == '\0') {
            a->buffer[ids_s][a->pos_e[ids_s]] = '\0';
            a->pos_e[ids_s] = (a->pos_e[ids_s] + 1) % N;
            sem_post(b_s);
            sem_post(el_s);
            break;
        }

        //calcula siguiente letra
        char siguiente;
        if (letra == 'z') siguiente = 'a';
        else if (letra == 'Z') siguiente = 'A';
        else if (isalpha(letra)) siguiente = letra + 1;
        else siguiente = letra;

        printf("\tIntermedio %d saca '%c' produce '%c'\n", id, letra, siguiente);

        //coloca la siguiente letra en el buffer de salida
        a->buffer[ids_s][a->pos_e[ids_s]] = siguiente;
        a->pos_e[ids_s] = (a->pos_e[ids_s] + 1) % N;

        sem_post(b_s);
        sem_post(el_s);
    }

    pthread_exit(NULL);
}

int main() {
    int num_p;

    printf("Numero de procesos: ");
    scanf("%d", &num_p);

    //buffers para cada hilo
    char **buffer= malloc((num_p - 1) * sizeof(char*));
    int *num_elementos = malloc((num_p - 1) * sizeof(int));
    int *pos_e = calloc((num_p - 1), sizeof(int));
    int *pos_s = calloc((num_p - 1), sizeof(int));

    //semaforos
    sem_t *huecos = malloc((num_p - 1) * sizeof(sem_t));
    sem_t *elementos = malloc((num_p - 1) * sizeof(sem_t));
    sem_t *sem_buffer = malloc((num_p - 1) * sizeof(sem_t));

    //reserva memoria de forma dinámica para los arrays
    for (int i = 0; i < num_p - 1; i++) {
        buffer[i] = malloc(N * sizeof(char));
        num_elementos[i] = 0;
        sem_init(&huecos[i], 0, N);
        sem_init(&elementos[i], 0, 0);
        sem_init(&sem_buffer[i], 0, 1);
    }

    f = fopen("archivoTexto.txt", "r");
    if (f == NULL) {
        perror("Error abriendo el txt");
        return 1;
    }

    pthread_t hilos[num_p];
    args_hilo args[num_p];

    //se inicializan los hilos
    for (int i = 0; i < num_p; i++) {
        args[i].id = i;
        args[i].buffer = buffer;
        args[i].num_elementos = num_elementos;
        args[i].huecos = huecos;
        args[i].elementos = elementos;
        args[i].sem_buffer = sem_buffer;
        args[i].pos_e = pos_e;
        args[i].pos_s = pos_s;

        if (i == 0) {
            args[i].salida = 0;
            pthread_create(&hilos[i], NULL, funcion_productor, &args[i]);
        } else if (i == num_p - 1) {
            args[i].entrada = i - 1;
            pthread_create(&hilos[i], NULL, funcion_consumidor, &args[i]);
        } else {
            args[i].entrada = i - 1;
            args[i].salida = i;
            pthread_create(&hilos[i], NULL, funcion_intermedios, &args[i]);
        }
    }

    //espera a que todos terminen
    for (int i = 0; i < num_p; i++) pthread_join(hilos[i], NULL);

    printf("\nTotal consumidos: %d\n", consumidos);
    printf("Vocales -> A: %d, E: %d, I: %d, O: %d, U: %d\n", a, e, i_voc, o, u);

    //liberación de memoria
    for (int i = 0; i < num_p - 1; i++) {
        sem_destroy(&huecos[i]);
        sem_destroy(&elementos[i]);
        sem_destroy(&sem_buffer[i]);
        free(buffer[i]);
    }

    free(buffer);
    free(num_elementos);
    free(huecos);
    free(elementos);
    free(sem_buffer);
    free(pos_e);
    free(pos_s);
    fclose(f);

    return 0;
}