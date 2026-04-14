//Adriana Sánchez-Bravo Cuesta y Alan

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10

int buffer[N];
int num_elementos = 0; 
int in= 0; //indices para fifo
int out= 0;

// contadores de progreso
int producidos = 0;
int consumidos = 0;

// suma
int suma= 0;

// semaforos
sem_t huecos;
sem_t elementos;
sem_t sem_buffer;

FILE *f;

// funcion del productor
void* funcion_productor(void* arg) {
    int id = *(int*)arg; 

    while (1) {
        sem_wait(&huecos);
        sem_wait(&sem_buffer);

        int num;
        if (fscanf(f, "%d", &num) != 1) {
            sem_post(&sem_buffer);
            sem_post(&huecos);
            break;
        }

        buffer[in] = num;
        in= (in+1)%N;
        num_elementos++;
        producidos++; 

        printf("Productor %d mete %d (%d)\n", id, num, producidos);


        sem_post(&sem_buffer);
        sem_post(&elementos);
        
        usleep(100000); // 0.1s
    }

    pthread_exit(NULL);
}

// funcion del consumidor
void* funcion_consumidor(void* arg) {
    int id = *(int*)arg;

    while (1) {
        sem_wait(&elementos);
        sem_wait(&sem_buffer);

        // saca la letra (FIFO)
        int num = buffer[out];
        buffer[out] = 0;
        out= (out+1)%N;
        num_elementos--;
        consumidos++; 

        printf("\tConsumidor %d saca %d (%d)\n", id, num, consumidos);

        // contamos si es vocal
        suma+= num;       

        sem_post(&sem_buffer);
        sem_post(&huecos);
        
        usleep(150000); 
    }

    pthread_exit(NULL);
}

int main() {
    int num_p, num_c;

    printf("Numero de productores: ");
    scanf("%d", &num_p);
    printf("Numero de consumidores: ");
    scanf("%d", &num_c);

    if (num_p <= 0 || num_c <= 0) {
        printf("Tiene que haber al menos 1 de cada\n");
        return 1;
    }

    f = fopen("archivoTexto.txt", "r");
    if (f == NULL) {
        perror("Error abriendo el txt");
        return 1;
    }

    // inicializamos semaforos
    sem_init(&huecos, 0, N);
    sem_init(&elementos, 0, 0);
    sem_init(&sem_buffer, 0, 1);

    // creamos los arrays segun lo que introdujo el usuario
    pthread_t hilos_p[num_p];
    pthread_t hilos_c[num_c];
    int id_p[num_p];
    int id_c[num_c];

    // creacion de hilos
    for (int i = 0; i < num_p; i++) {
        id_p[i] = i + 1;
        pthread_create(&hilos_p[i], NULL, funcion_productor, &id_p[i]);
    }

    for (int i = 0; i < num_c; i++) {
        id_c[i] = i + 1;
        pthread_create(&hilos_c[i], NULL, funcion_consumidor, &id_c[i]);
    }

    // esperamos a que todos terminen
    for (int i = 0; i < num_p; i++) pthread_join(hilos_p[i], NULL);
    for (int i = 0; i < num_c; i++) pthread_join(hilos_c[i], NULL);

    printf("\nTotal producidos: %d\n", producidos);
    printf("Total consumidos: %d\n", consumidos);
    printf("Suma= %d\n", suma);

    // limpiamos todo
    sem_destroy(&huecos);
    sem_destroy(&elementos);
    sem_destroy(&sem_buffer);
    fclose(f);

    return 0;
}