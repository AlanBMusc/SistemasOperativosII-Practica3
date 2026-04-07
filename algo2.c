//vpluntario 1 de la practica 2
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define N 10
#define ITERACIONES 80

// variables globales compartidas por los hilos
int buffer[N];
int count = 0;

// semaforos
sem_t huecos;
sem_t elementos;
sem_t sem_buffer;

// funcion del hilo productor
void* funcion_productor(void* arg) {
    FILE *archivo = fopen("archivoTexto.txt", "r");
    if (archivo == NULL) {
        perror("Error abriendo el txt");
        pthread_exit(NULL);
    }

    for (int i = 0; i < ITERACIONES; i++) {
        int c = fgetc(archivo);
        char letra = (c != EOF) ? (char)c : 'X'; // ponemos X si se acaba antes

        // control de velocidad
        if (i >= 30 && i < 60) sleep(1); 
        else if (i >= 60) sleep(rand() % 4);

        sem_wait(&huecos);
        sem_wait(&sem_buffer);

        // mete en buffer
        buffer[count] = letra;
        count++;
        printf("Prod mete '%c' | tam: %d\n", letra, count);

        sem_post(&sem_buffer);
        sem_post(&elementos);
    }

    fclose(archivo);
    pthread_exit(NULL);
}

// funcion del hilo consumidor
void* funcion_consumidor(void* arg) {
    for (int i = 0; i < ITERACIONES; i++) {
        // control de velocidad
        if (i < 30) sleep(1); 
        else if (i >= 60) sleep(rand() % 4);

        sem_wait(&elementos);
        sem_wait(&sem_buffer);

        // saca del buffer (LIFO)
        int num = buffer[count - 1];
        buffer[count - 1] = '-';
        count--;
        printf("\tCons saca '%d' | tam: %d\n", num, count);

        sem_post(&sem_buffer);
        sem_post(&huecos);
    }
    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));

    // inicializamos semaforos (el 0 indica que son para hilos)
    sem_init(&huecos, 0, N);
    sem_init(&elementos, 0, 0);
    sem_init(&sem_buffer, 0, 1);

    pthread_t hilo_p, hilo_c;

    printf("Arrancando hilos...\n");

    pthread_create(&hilo_p, NULL, funcion_productor, NULL);
    pthread_create(&hilo_c, NULL, funcion_consumidor, NULL);

    // esperamos a que acaben
    pthread_join(hilo_p, NULL);
    pthread_join(hilo_c, NULL);

    // limpieza
    sem_destroy(&huecos);
    sem_destroy(&elementos);
    sem_destroy(&sem_buffer);

    return 0;
}