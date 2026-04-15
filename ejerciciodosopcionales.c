// Adriana Sánchez-Bravo Cuesta y Alan Barreiro Martínez

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h> // Para el timestamp (ms) de los logs

#define N 10
#define TOTAL 80 // Total de intentos de consumo definidos por la práctica

// --- ESTRUCTURAS PARA EL OPCIONAL 1 Y 2 ---
typedef struct {
    int valor;
    int prioridad;   
    int origen;      
    time_t t_creacion; // (Opcional 2a): Almacena el segundo exacto de creación
    int caducidad;     // (Opcional 2a): Tiempo de vida (1-12s) asignado al nacer
    int activo;        // EXPLICACIÓN: Fundamental para solucionar el "Prod 0". 
                       // Indica si el hueco del buffer contiene un item real (1) o está libre (0).
} Item; 

Item buffer[N]; 
int num_elementos = 0; 
int final = 0;

// contadores de progreso y sumas
int producidos = 0;
int consumidos = 0;
int suma_ficheros[3] = {0, 0, 0}; 
int suma_cons = 0;

// Sincronización
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;    
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;    

FILE *ficheros[3]; 

// --- FUNCIONES AUXILIARES ---
long get_timestamp() { 
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// --- LÓGICA DE PRODUCCIÓN ---
int produce_item(int id, Item *it) { 
    int num;
    pthread_mutex_lock(&mutex); 
    if (fscanf(ficheros[id-1], "%d", &num) == 1) {
        it->valor = num;
        it->prioridad = id; 
        it->origen = id;
        it->t_creacion = time(NULL); // Marcamos el inicio de su vida
        it->caducidad = (rand() % 12) + 1; // Asignamos caducidad aleatoria
        it->activo = 1; // Marcamos como "ocupado" para que el consumidor lo vea
        pthread_mutex_unlock(&mutex);
        return 1;
    }
    pthread_mutex_unlock(&mutex);
    return -1; 
}

void insert_item(Item it) { 
    // Buscamos el primer hueco libre (no activo) en el buffer circular/estático
    for(int i = 0; i < N; i++) {
        if(buffer[i].activo == 0) {
            buffer[i] = it;
            num_elementos++;
            break;
        }
    }
}

// --- LÓGICA DE CONSUMO Y PRIORIDAD ---
Item remove_item_prioridad() { 
    int mejor_idx = -1;
    int min_prioridad = 999; 

    // EXPLICACIÓN: Recorremos todo el buffer buscando el ítem con mayor prioridad (menor número)
    // PERO solo consideramos aquellos que tengan 'activo == 1'.
    // Esto soluciona el error de "Prod 0", ya que antes el programa leía ceros de la memoria
    // inicializada pensando que eran ítems reales de un supuesto productor 0.
    for (int i = 0; i < N; i++) {
        if (buffer[i].activo == 1) { 
            if (buffer[i].prioridad < min_prioridad) {
                min_prioridad = buffer[i].prioridad;
                mejor_idx = i;
            }
        }
    }

    Item it = buffer[mejor_idx];
    buffer[mejor_idx].activo = 0; // Liberamos el hueco inmediatamente
    num_elementos--;
    return it;
}

void consume_item(Item it) { 
    suma_cons += it.valor;
    suma_ficheros[it.origen-1] += it.valor;
}

// --- HILOS ---
void* funcion_productor(void* arg) {
    int id = *(int*)arg; 
    Item it_aux;

    while (1) { 
        if (produce_item(id, &it_aux) == -1) break; 

        pthread_mutex_lock(&mutex); 
        while (num_elementos == N) { 
            pthread_cond_wait(&condp, &mutex);
        }

        insert_item(it_aux); 
        producidos++; 

        printf("[%ld ms] Productor %d (Prio %d) mete '%d' [Caducidad: %ds] (%d/%d)\n", 
                get_timestamp(), id, it_aux.prioridad, it_aux.valor, it_aux.caducidad, producidos, TOTAL);

        pthread_cond_signal(&condc); 
        pthread_mutex_unlock(&mutex); 
        
        sleep((rand() % 6) + 1); 
    }
    pthread_exit(NULL);
}

void* funcion_consumidor(void* arg) {
    while (consumidos < TOTAL) { 
        pthread_mutex_lock(&mutex); 
        while (num_elementos == 0) { 
            pthread_cond_wait(&condc, &mutex);
        }

        Item it = remove_item_prioridad(); 
        consumidos++; 

        // EXPLICACIÓN (Opcional 2b): Verificamos si la diferencia entre "ahora" y "creación"
        // es mayor que el tiempo de caducidad asignado por el productor.
        time_t t_actual = time(NULL);
        double tiempo_transcurrido = difftime(t_actual, it.t_creacion);

        if (tiempo_transcurrido > it.caducidad) {
            // Caso CADUCADO: Informamos y liberamos el mutex SIN sumar y SIN hacer sleep de procesado.
            printf("[%ld ms]\t!!! ITEM CADUCADO !!! Prod %d (Prio %d) valor '%d' tras %.0fs (Max: %ds)\n", 
                    get_timestamp(), it.origen, it.prioridad, it.valor, tiempo_transcurrido, it.caducidad);
            pthread_cond_signal(&condp); 
            pthread_mutex_unlock(&mutex);
        } 
        else {
            // Caso VÁLIDO: Informamos, liberamos mutex, sumamos y simulamos tiempo de procesado.
            printf("[%ld ms]\tConsumidor extrae de Prod %d (Prio %d) valor '%d' (%d/%d)\n", 
                    get_timestamp(), it.origen, it.prioridad, it.valor, consumidos, TOTAL);

            pthread_cond_signal(&condp); 
            pthread_mutex_unlock(&mutex); 

            consume_item(it); 
            sleep((rand() % 3) + 1); 
        }
    }
    pthread_exit(NULL);
}

// --- MAIN ---
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

    // Inicialización explícita del buffer como inactivo
    for(int i=0; i<N; i++) buffer[i].activo = 0;

    pthread_t hilos_p[3], h_cons;
    int id_p[3] = {1, 2, 3}; 

    for (int i = 0; i < 3; i++) pthread_create(&hilos_p[i], NULL, funcion_productor, &id_p[i]);
    pthread_create(&h_cons, NULL, funcion_consumidor, NULL);

    for (int i = 0; i < 3; i++) pthread_join(hilos_p[i], NULL);
    pthread_join(h_cons, NULL);

    printf("\n========================================\n");
    printf("Resultados (Filtrados por Caducidad):\n");
    printf("Suma Fichero 1: %d\nSuma Fichero 2: %d\nSuma Fichero 3: %d\n", suma_ficheros[0], suma_ficheros[1], suma_ficheros[2]);
    printf("Suma Total (Solo válidos): %d\n", suma_cons);
    printf("========================================\n");

    pthread_mutex_destroy(&mutex); 
    pthread_cond_destroy(&condp); pthread_cond_destroy(&condc);   
    for(int i=0; i<3; i++) fclose(ficheros[i]); 
    return 0;
}