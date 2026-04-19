// Adriana Sánchez-Bravo Cuesta y Alan Barreiro Martínez

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h> //para el timestamp

#define N 10


typedef struct {
    int valor;
    int prioridad;   
    int origen;     
    int activo;  
} Item; 

//memoria compartida
Item buffer[N]; 
int num_elementos = 0; 
int final = 0;

//contadores de progreso y sumas
int producidos = 0;
int consumidos = 0;
int suma_ficheros[3] = {0, 0, 0}; 
int suma_cons = 0;

int productores_vivos = 3;

//sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;    
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;    

FILE *ficheros[3]; 


//cada llamada lee un número del archivo de texto y lo mete en la estructura
int produce_item(int id, Item *it) { 
    int num;
    if (fscanf(ficheros[id-1], "%d", &num) == 1) {
        it->valor = num;
        it->prioridad = id; 
        it->origen = id;
        it->activo = 1;
        return 1;
    }
    return -1; 
}

//coloca el entero leido
void insert_item(Item it) { 
    //se busca el primer hueco libre en el buffer 
    for(int i = 0; i < N; i++) {
        if(buffer[i].activo == 0) {
            buffer[i] = it;
            num_elementos++;
            break;
        }
    }
}

//quita el elemento del buffer por la prioridad de los procesos
Item remove_item_prioridad() { 
    int mejor_idx = -1;
    int min_prioridad = 999; 

    //recorrer todo el buffer buscando los numeros con mayor prioridad
    for (int i = 0; i < N; i++) {
        //tienen que estar activos para no leer 0s
        if (buffer[i].activo == 1) { 
            if (buffer[i].prioridad < min_prioridad) {
                min_prioridad = buffer[i].prioridad;
                mejor_idx = i;
            }
        }
    }

    Item it = buffer[mejor_idx];
    buffer[mejor_idx].activo = 0; //liberar el hueco poniendo activo a 0
    num_elementos--;
    return it;
}

//realiza las sumas
void consume_item(Item it) {
    //suma del consumidor
    suma_cons += it.valor;
    //suma de cada fichero
    suma_ficheros[it.origen-1] += it.valor;
}

//funcion del hilo productor 
void* funcion_productor(void* arg) {
    int id = *(int*)arg; 
    Item it_aux;

    //bucle infinito hasta que acaben los productores
    while (1) { 
        //se calcula el sleep aleatorio
        sleep((rand() % 6) + 1); 
        
        //se bloquea el acceso al buffer
        pthread_mutex_lock(&mutex); 

        //si el buffer esta lleno se bloquea hasta que el consumidor lance una señal
        while (num_elementos == N) { 
            pthread_cond_wait(&condp, &mutex);
        }

        //lee el numero del fichero 
        //si es el final del archivo devuelve -1 y el productor acaba
        if (produce_item(id, &it_aux) == -1) {
            //el contador de productores que estan en funcionamiento baja para avisar al
            //consumidor si todos acaban
            productores_vivos--;   
            
            //se lanza una señal para desbloquear todos los consumidores, aunque en este
            //caso solo se trata de uno
            pthread_cond_broadcast(&condc); 
            
            //se desbloquea el acceso al buffer y se sale del if
            pthread_mutex_unlock(&mutex);  
            break;
        }

        //se añade la estructura con el numero al buffer
        insert_item(it_aux); 
        producidos++; 

        printf("Productor %d (Prio %d) mete '%d (%d)' \n", 
                id, it_aux.prioridad, it_aux.valor, producidos);

        pthread_cond_signal(&condc); 
        pthread_mutex_unlock(&mutex); 
        
    }
    pthread_exit(NULL);
}

//funcion del hilo consumidor
void* funcion_consumidor(void* arg) {

    while (1) { 
        //bloquea el acceso a memoria
        pthread_mutex_lock(&mutex); 

        //si el buffer esta vacio y siguen habiendo productores vivos espera con 
        //una variable de condicion
        while (num_elementos == 0 && productores_vivos > 0) { 
            pthread_cond_wait(&condc, &mutex);
        }

        //si el buffer esta vacio y no hay productores activos, libera el acceso a memoria
        //y sale del bucle
        if (num_elementos == 0 && productores_vivos == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        //quita el numero del buffer y aumenta el contador de elementos consumidos
        Item it = remove_item_prioridad(); 
        consumidos++; 

        printf("\tConsumidor extrae de Prod %d (Prio %d) valor '%d' (%d)\n", 
                it.origen, it.prioridad, it.valor, consumidos);

        pthread_cond_signal(&condp); 
        pthread_mutex_unlock(&mutex); 

        //se calcula el sleep aleatorio
        sleep((rand() % 3) + 1); 

        //se realizan las sumas
        consume_item(it);
        
    }
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) { 
    
    if (argc < 4) { 
        printf("Uso: %s <f1.txt> <f2.txt> <f3.txt>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    for(int i = 0; i < 3; i++) {
        ficheros[i] = fopen(argv[i+1], "r");
        if (ficheros[i] == NULL) { return 1; }
    }

    // Inicialización del buffer como inactivo
    for(int i=0; i<N; i++) buffer[i].activo = 0;

    pthread_t hilos_p[3], h_cons;
    int id_p[3] = {1, 2, 3}; 

    //se crean los 3 hilos productores
    for (int i = 0; i < 3; i++) pthread_create(&hilos_p[i], NULL, funcion_productor, &id_p[i]);
    //se crea el hilo consumidor
    pthread_create(&h_cons, NULL, funcion_consumidor, NULL);

    //se espera a que todos acaben
    for (int i = 0; i < 3; i++) pthread_join(hilos_p[i], NULL);
    pthread_join(h_cons, NULL);

    //se imprimen los resultados
    printf("Resultados:\n");
    printf("Suma Fichero 1: %d\nSuma Fichero 2: %d\nSuma Fichero 3: %d\n", suma_ficheros[0], suma_ficheros[1], suma_ficheros[2]);
    printf("Suma Total: %d\n", suma_cons);

    //se libera todo
    pthread_mutex_destroy(&mutex); 
    pthread_cond_destroy(&condp); pthread_cond_destroy(&condc);   
    for(int i=0; i<3; i++) fclose(ficheros[i]); 
    return 0;
}
