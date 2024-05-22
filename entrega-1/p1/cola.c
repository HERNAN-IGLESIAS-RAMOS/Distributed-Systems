#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cola.h"

void inicializar_cola(Cola *cola, int tam_cola)
{
    register int i;

    if (tam_cola < 1)
    {
        perror("Error: El tamaño de la cola debe ser mayor o igual que 1");
        exit(1);
    }
    if (cola == NULL)
    {
        perror("Error: El puntero a la cola es NULL en inicializar_cola");
        exit(2);
    }

    // A RELLENAR el resto de la inicialización de la cola
    cola->datos = (dato_cola **) malloc(tam_cola * sizeof(dato_cola *));
    if (!cola->datos) {
        printf("Error al asignar memoria para el array de datos\n");
        exit(50);
    }
    cola->head = 0;
    cola->tail = 0;
    cola->tam_cola= tam_cola;

    if(pthread_mutex_init(&cola->mutex_head, NULL)!=0){
        printf("Error en la inicialización del mutex para la cabeza de la cola\n");
        exit(51);
    }
    if(pthread_mutex_init(&cola->mutex_tail, NULL)!=0){
        printf("Error en la inicialización del mutex para la cola de la cola\n");
        exit(52);
    }
    if(sem_init(&cola->num_huecos, 0, tam_cola)!=0){
        printf("Error en la inicialización del semáforo para el número de huecos\n");
        exit(53);
    }
    if(sem_init(&cola->num_ocupados, 0, 0)!=0){
        printf("Error en la inicialización del semáforo para el número de ocupados\n");
        exit(54);
    }
}

void destruir_cola(Cola *cola)
{
    // Debe liberarse la memoria apuntada por cada puntero guardado en la cola
    // y la propia memoria donde se guardan esos punteros, así como
    // destruir los semáforos y mutexes

    // A RELLENAR
    if (cola == NULL){
        printf("El puntero cola no es válido\n");
        exit(55);
    }
    if (cola->datos != NULL){
        free(cola->datos);
    }

    if(pthread_mutex_destroy(&cola->mutex_head)!=0){
        printf("Error al destruir el mutex para la cabeza de la cola\n");
        exit(56);
    }
    if( pthread_mutex_destroy(&cola->mutex_tail)!=0){
        printf("Error al destruir el mutex para la cola de la cola\n");
        exit(57);
    }
    if(sem_destroy(&cola->num_huecos)!=0){
        printf("Error al destruir el semáforo para el número de huecos\n");
        exit(58);
    }
    if(sem_destroy(&cola->num_ocupados)!=0){
        printf("Error al destruir el semáforo para el número de ocupados\n");
        exit(59);
    }

}

void insertar_dato_cola(Cola *cola, dato_cola *dato)
{
    // A RELLENAR
    sem_wait(&cola->num_huecos);
    pthread_mutex_lock(&(cola->mutex_head));

    *((cola->datos)+(cola->head)) = dato;
    cola->head = (cola->head + 1) % cola->tam_cola;

    pthread_mutex_unlock(&(cola->mutex_head));
    sem_post(&(cola->num_ocupados));

}

dato_cola *obtener_dato_cola(Cola *cola)
{
    dato_cola *p;

    // A RELLENAR
    if (sem_wait(&(cola->num_ocupados)) != 0) {
        printf("Error en sem_wait a la hora de obtener un dato de la cola\n");
        exit(60);
    }
    if (pthread_mutex_lock(&(cola->mutex_tail)) != 0) {
        printf("Error en pthread_mutex_lock a la hora de obtener un dato de la cola\n");
        exit(61);
    }
    p = *((cola->datos)+(cola->tail));
    cola->tail = (cola->tail + 1) % cola->tam_cola;

    if (pthread_mutex_unlock(&(cola->mutex_tail)) != 0) {
        printf("Error en pthread_mutex_unlock a la hora de obtener un dato de la cola\n");
        exit(62);
    }

    if (sem_post(&(cola->num_huecos)) != 0) {
        printf("Error en sem_post a la hora de obtener un dato de la cola\n");
        exit(63);
    }
    return(p);
}
