#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define N 10

int buffer[N];
int num_elementos = 0; 
int principio= 0;
int final= 0;

//numero de hilos
int num_p, num_c;
int fin_prod= 0;
// contadores de progreso
int producidos = 0;
int consumidos = 0;

// contadores de lsa sumas
int suma_prod= 0;
int suma_cons= 0;

// semaforos
sem_t huecos;
sem_t elementos;
sem_t sem_buffer;

FILE *f;

//cada llamada lee un número del archivo de texto y hace suma
int produce_item(){
    int num;
    int comprobar= fscanf(f, "%d", &num);
    if(comprobar != 1){
        return -1;
    }
    suma_prod+= num;
    return num;
}

//colocar entero leido
void* insert_item(void* arg){
    int id = *(int*)arg; 

    while (1) {
        sem_wait(&huecos);
        sem_wait(&sem_buffer);
        
        int num= produce_item();
        if (num == -1) {
                        if (!fin_prod) {
                fin_prod = 1;
                sem_post(&sem_buffer);
                sem_post(&huecos);
 
                for (int i = 0; i < num_c; i++) {
                    sem_wait(&huecos);
                    sem_wait(&sem_buffer);
 
                    buffer[final] = -1;
                    final = (final + 1) % N;
                    num_elementos++;
 
                    sem_post(&sem_buffer);
                    sem_post(&elementos);
                }
            } else {
                sem_post(&sem_buffer);
                sem_post(&huecos);
            }
            break;
        }
        
        
        buffer[final] = num;
        final= (final+1) %N;
        num_elementos++;
        producidos++; 

        printf("Productor %d mete '%d' (%d)\n", id, num, producidos);

        sem_post(&sem_buffer);
        sem_post(&elementos);
        
        usleep(100000); // 0.1s
    }
    pthread_exit(NULL);
}

//retirar del buffer entero y reemplazarlo por 0
int remove_item(){
    int num = buffer[principio];
    if(num== -1){
        return -1;
    }
    buffer[principio] = 0;
    principio= (principio +1) %N;
    num_elementos--;
    consumidos++; 
    return num;
}

//calcula suma enteros
void* consume_item(void* arg){
    int id = *(int*)arg;

    while (1) {
        sem_wait(&elementos);
        sem_wait(&sem_buffer);

        int num= remove_item();
        if(num== -1){
            num_elementos--;
            principio = (principio + 1) % N;
 
            sem_post(&sem_buffer);
            sem_post(&huecos);
            break;
        }
        printf("\tConsumidor %d saca '%d' (%d)\n", id, num, consumidos);
        
        // contar
        suma_cons+= num;

        sem_post(&sem_buffer);
        sem_post(&huecos);
        
        usleep(150000); 
    }

    pthread_exit(NULL);
}

//main
int main() {
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
        pthread_create(&hilos_p[i], NULL, insert_item, &id_p[i]);
    }

    for (int i = 0; i < num_c; i++) {
        id_c[i] = i + 1;
        pthread_create(&hilos_c[i], NULL, consume_item, &id_c[i]);
    }

    // esperamos a que todos terminen
    for (int i = 0; i < num_p; i++) pthread_join(hilos_p[i], NULL);
    for (int i = 0; i < num_c; i++) pthread_join(hilos_c[i], NULL);

    printf("\nTotal producidos: %d\n", producidos);
    printf("Suma productor= %d\n", suma_prod);
    printf("Total consumidos: %d\n", consumidos);
    printf("Suma del consumidor= %d\n", suma_cons);

    // limpiamos todo
    sem_destroy(&huecos);
    sem_destroy(&elementos);
    sem_destroy(&sem_buffer);
    fclose(f);

    return 0;
}  