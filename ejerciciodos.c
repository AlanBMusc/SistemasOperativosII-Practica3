// Adriana Sánchez-Bravo Cuesta y Alan Barreiro Martínez

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
// #include <semaphore.h> // ELIMINADA: Se sustituye por mutex y variables de condición

#define N 10
#define TOTAL 80 // CAMBIADA: De 9 a 80 para cumplir el apartado 2

int buffer[N];
int num_elementos = 0; 
int principio = 0;
int final = 0;

// contadores de progreso
int producidos = 0;
int consumidos = 0;

// contadores de suma
int suma_prod = 0;
int suma_cons = 0;

// Sincronización (Mutex y Variables de Condición)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // NUEVA: Sustituye sem_buffer
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;    // NUEVA: Sustituye huecos
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;    // NUEVA: Sustituye elementos

FILE *f;

// --- FUNCIONES OBLIGATORIAS (Apartado 1.3) ---

int produce_item() { // NUEVA: Función requerida
    int num;
    if (fscanf(f, "%d", &num) == 1) {
        suma_prod += num;
        return num;
    }
    return -1; // Fin de fichero
}

void insert_item(int item) { // NUEVA: Función requerida
    buffer[final] = item;
    final = (final + 1) % N;
    num_elementos++;
}

int remove_item() { // NUEVA: Función requerida
    int item = buffer[principio];
    buffer[principio] = 0; // Reemplazar por 0 como pide el enunciado
    principio = (principio + 1) % N;
    num_elementos--;
    return item;
}

void consume_item(int item) { // NUEVA: Función requerida
    suma_cons += item;
}

// funcion del productor
void* funcion_productor(void* arg) {
    int id = *(int*)arg; 

    for (int i = 0; i < TOTAL; i++) { // CAMBIADA: while(1) por bucle de iteraciones
        int num = produce_item(); // CAMBIADA: Se usa la función obligatoria
        if (num == -1) break; // NUEVA: Por si el archivo es más corto que TOTAL

        pthread_mutex_lock(&mutex); // CAMBIADA: sem_wait(&sem_buffer) por mutex_lock
        while (num_elementos == N) { // CAMBIADA: sem_wait(&huecos) por while + cond_wait
            pthread_cond_wait(&condp, &mutex);
        }

        insert_item(num); // CAMBIADA: Lógica interna por llamada a función
        producidos++; 

        printf("Productor %d mete '%d' (%d/%d)\n", id, num, producidos, TOTAL);

        pthread_cond_signal(&condc); // CAMBIADA: sem_post(&elementos) por cond_signal
        pthread_mutex_unlock(&mutex); // CAMBIADA: sem_post(&sem_buffer) por mutex_unlock
        
        // --- CONTROL DE VELOCIDAD (Apartado 2.1.3) ---
        if (i < 30) usleep(20000);  // NUEVA: Productor rápido (0.02s)
        else if (i < 60) usleep(200000); // NUEVA: Productor lento (0.2s)
        else usleep(100000); // NUEVA: Normal
    }

    pthread_exit(NULL);
}

// funcion del consumidor
void* funcion_consumidor(void* arg) {
    int id = *(int*)arg;

    for (int i = 0; i < TOTAL; i++) { // CAMBIADA: while(1) por bucle de iteraciones
        pthread_mutex_lock(&mutex); // CAMBIADA: sem_wait(&sem_buffer) por mutex_lock
        while (num_elementos == 0) { // CAMBIADA: sem_wait(&elementos) por while + cond_wait
            pthread_cond_wait(&condc, &mutex);
        }

        int num = remove_item(); // CAMBIADA: Lógica interna por llamada a función
        consumidos++; 

        printf("\tConsumidor %d saca '%d' (%d/%d)\n", id, num, consumidos, TOTAL);

        pthread_cond_signal(&condp); // CAMBIADA: sem_post(&huecos) por cond_signal
        pthread_mutex_unlock(&mutex); // CAMBIADA: sem_post(&sem_buffer) por mutex_unlock

        consume_item(num); // CAMBIADA: Lógica interna por llamada a función
        
        // --- CONTROL DE VELOCIDAD (Apartado 2.1.3 - Inverso) ---
        if (i < 30) usleep(200000); // NUEVA: Consumidor lento para que el buffer se llene
        else if (i < 60) usleep(20000); // NUEVA: Consumidor rápido para que se vacíe
        else usleep(100000); // NUEVA: Normal
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

    // ELIMINADAS: sem_init (huecos, elementos, sem_buffer) ya no se usan
    // Los mutex y cond_var ya están inicializados de forma estática arriba

    // creamos los hilos
    pthread_t hilos_p[num_p];
    pthread_t hilos_c[num_c];
    int id_p[num_p];
    int id_c[num_c];

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
    printf("Suma productor= %d\n", suma_prod);
    printf("Total consumidos: %d\n", consumidos);
    printf("Suma del consumidor= %d\n", suma_cons);

    // ELIMINADAS: sem_destroy (huecos, elementos, sem_buffer)
    pthread_mutex_destroy(&mutex); // NUEVA
    pthread_cond_destroy(&condp);   // NUEVA
    pthread_cond_destroy(&condc);   // NUEVA
    fclose(f);

    return 0;
}