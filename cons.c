//Nombres: 
//Programa del consumidor
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10
#define ITERACIONES 25

// variables globales compartidas por los hilos
int buffer[N];
int count= 0;
int suma=0;

// semaforos
sem_t huecos;
sem_t elementos;
sem_t sem_buffer;

//retira los numeros del buffer y los sustituye por 0
int remove_item( int pos){
    //coge el numero y lo cambia por 0
    int item= buffer[pos];
    sleep(1);
    buffer[pos]= 0;
    
    return item;
}

// funcion del hilo consumidor
void* consume(void* arg) {
    for (int i = 0; i < ITERACIONES; i++) {
        // control de velocidad
        if (i < 30) sleep(1); 
        else if (i >= 60) sleep(rand() % 4);

        sem_wait(&elementos);
        sem_wait(&sem_buffer);

        // saca del buffer (FIFO)
        int num = remove_item(i);

        printf("\tCons saca '%d' | tam: %d\n", num, count);

        suma+= num;

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

    pthread_t hilo_c;

    pthread_create(&hilo_c, NULL, consume, NULL);

    // esperamos a que acaben
    pthread_join(hilo_c, NULL);

    printf("La suma total es: %d", suma);

    // limpieza
    sem_destroy(&huecos);
    sem_destroy(&elementos);
    sem_destroy(&sem_buffer);

    return 0;
}