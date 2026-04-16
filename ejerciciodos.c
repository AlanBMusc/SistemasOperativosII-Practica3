// Adriana Sánchez-Bravo Cuesta y Alan Barreiro Martínez

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#define N 10
#define TOTAL 80 

//buffer compartido
int buffer[N];
int num_elementos = 0; 
int principio = 0;
int final = 0;

//contadores de progreso
int producidos = 0;
int consumidos = 0;

//numero de hilos
int num_p, num_c;

//contadores de suma
int suma_prod = 0;
int suma_cons = 0;

int fin= 0;

//mutex 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//variables de condicion
pthread_cond_t condp = PTHREAD_COND_INITIALIZER; 
pthread_cond_t condc = PTHREAD_COND_INITIALIZER; 

FILE *f;

//cada llamada lee un número del archivo de texto y hace suma
int produce_item(){
    int num;
    //se comprueba si se llegó al final
    if (fscanf(f, "%d", &num) == 1) {
        suma_prod += num;
        return num;
    }
    return -1; // Fin de fichero
}

//colocar entero leido
void insert_item(int item) {
    buffer[final] = item;
    final = (final + 1) % N;
    num_elementos++;
}

//retirar del buffer entero y reemplazarlo por 0
int remove_item() {
    int item = buffer[principio];
    buffer[principio] = 0; 
    principio = (principio + 1) % N;
    num_elementos--;
    return item;
}

//calcula suma enteros
void consume_item(int item) { 
    suma_cons += item;
}

// funcion del productor
void* funcion_productor(void* arg) {
    int id = *(int*)arg; 

    //cambiar bucle infinito por bucle hasta el maximo de iteraciones definido
    for (int i = 0; i < TOTAL; i++) { 

        //se bloquea el buffer con un mutex
        pthread_mutex_lock(&mutex); 

        //cuando se llega al maximo de elementos en el buffer se desbloquea el mutex y
        //se bloquea el hilo hasta que un consumidor quite algun elemento
        while (num_elementos == N) {
            pthread_cond_wait(&condp, &mutex);
        }

        //se lee el numero y se guarda en la variable num
        int num = produce_item(); 
        //si se llego al final del archivo se sale del bucle
        if (num == -1){
            if (!fin){
                fin= 1;

                for (int i = 0; i < num_c; i++) {
                    while (num_elementos == N)
                        pthread_cond_wait(&condp, &mutex);

                    insert_item(-1);
                }
                pthread_cond_broadcast(&condc);
            }
            pthread_mutex_unlock(&mutex);
            break;
        } 

        //se añade al buffer el numero leido
        insert_item(num); 
        producidos++; 

        printf("Productor %d mete '%d' (%d/%d)\n", id, num, producidos, TOTAL);

        //si hay algun hilo consumidor bloqueado, lo desbloquea para que siga consumiendo
        pthread_cond_broadcast(&condc);
        //se desbloquea la memoria compartida despues de que el hilo acabe
        pthread_mutex_unlock(&mutex); 
        
        // sleeps para las carreras criticas
        if (i < 30) usleep(20000);  
        else if (i < 60) usleep(200000); 
        else usleep(100000);
    }

    pthread_exit(NULL);
}

// funcion del consumidor
void* funcion_consumidor(void* arg) {
    int id = *(int*)arg;

    for (int i = 0; i < TOTAL; i++) { 
        //se bloquea el buffer ya que el consumidor entro
        pthread_mutex_lock(&mutex); 
        //si el buffer esta vacio, se espera a q algun productor lo llene
        //con una variable de condicion
        while (num_elementos == 0 ) {
            if(fin){
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            } 
            pthread_cond_wait(&condc, &mutex);
        }

        //se retira el numero del buffer y se cambia por 0
        int num = remove_item(); 
        
        if(num == -1){            
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        consumidos++; 
        printf("\tConsumidor %d saca '%d' (%d/%d)\n", id, num, consumidos, TOTAL);

        //si algun productor estaba bloqueado se libera
        pthread_cond_signal(&condp); 
        //se desbloquea el acceso a memoria compartida 
        pthread_mutex_unlock(&mutex); 

        //se hace la suma de los consumidores
        consume_item(num); 
        
        // velocidades segun el hilo 
        if (i < 30) usleep(200000);
        else if (i < 60) usleep(20000); 
        else usleep(100000); 
    }

    pthread_exit(NULL);
}

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

    //se crean los hilos
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

    //se espera a que todos terminen
    for (int i = 0; i < num_p; i++) pthread_join(hilos_p[i], NULL);
    for (int i = 0; i < num_c; i++) pthread_join(hilos_c[i], NULL);

    printf("\nTotal producidos: %d\n", producidos);
    printf("Suma productor= %d\n", suma_prod);
    printf("Total consumidos: %d\n", consumidos);
    printf("Suma del consumidor= %d\n", suma_cons);

    pthread_mutex_destroy(&mutex); 
    pthread_cond_destroy(&condp);   
    pthread_cond_destroy(&condc);   
    fclose(f);

    return 0;
}
