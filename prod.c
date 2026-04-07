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
int contar = 0;

// semaforos
sem_t huecos;
sem_t elementos;
sem_t sem_buffer;

void* insert_item(int buffer[], int letra){
        // mete en buffer
        buffer[count] = letra;
        count++;
}

void* produce_item(FILE *archivo){
        for (int i = 0; i < ITERACIONES; i++) {
            int c = fgetc(archivo);
            //char letra = (c != EOF) ? (char)c : 'X'; // ponemos X si se acaba antes
            int letra = (rand()%99)+1;

            // control de velocidad
            if (i >= 30 && i < 60) sleep(1); 
            else if (i >= 60) sleep(rand() % 4);

            sem_wait(&huecos);
            sem_wait(&sem_buffer);

            insert_item(buffer, letra);

            contar = contar + letra;
            printf("Prod mete '%c' | tam: %d | cuentas: %d\n", letra, count, contar);

            sem_post(&sem_buffer);
            sem_post(&elementos);
        }
}



// funcion del hilo productor
int main() {
    
    FILE *archivo = fopen("archivoTexto.txt", "r");
    if (archivo == NULL) {
        perror("Error abriendo el txt");
        pthread_exit(NULL);
    }

    produce_item(archivo);

    fclose(archivo);
    pthread_exit(NULL);
}

