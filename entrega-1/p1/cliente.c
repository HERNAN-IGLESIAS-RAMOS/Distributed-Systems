// Archivos de cabecera para manipulación de sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include "util.h"

#define TAMLINEA 1024
#define SINASIGNAR -1
#define MAXHILOSCLIENTE 10

// tipo de datos que recibiran los hilos cliente
struct datos_hilo
{
    unsigned char id;
    char *nom_fichero_consultas;
    struct sockaddr *dserv;
};

typedef struct datos_hilo datos_hilo;

//
// VARIABLES GLOBALES
//

// IP del proceso srvdns
char *ip_srvdns;

// Puerto en el que espera el proceso srvdns
int puerto_srvdns;

// Numero de hilos lectores
int nhilos;

// Es o no orientado a conexion
unsigned char es_stream = CIERTO;

// nombre del fichero fuente de consultas
char *fich_consultas;

char *hilos_file_names[MAXHILOSCLIENTE] = {
    "sal00.dat",
    "sal01.dat",
    "sal02.dat",
    "sal03.dat",
    "sal04.dat",
    "sal05.dat",
    "sal06.dat",
    "sal07.dat",
    "sal08.dat",
    "sal09.dat"};

void procesa_argumentos(int argc, char *argv[])
{
    if (argc < 6)
    {
        fprintf(stderr, "Forma de uso: %s ip_srvdns puerto_srvdns {t|u} nhilos fich_consultas\n", argv[0]);
        exit(1);
    }
    // Verificación de los argumentos e inicialización de las correspondientes variables globales.
    // Puedes usar las funciones en util.h

    // A RELLENAR
    char* ip = strdup(argv[1]);
    int validaip=valida_ip(ip);
    if(validaip==1){
      ip_srvdns=argv[1];
    }else{
      printf("La dirección IP proporcionada no cumple el formato\n");
      exit(2);
    }

    char* puerto_cadena=argv[2];
    int validapuerto=valida_numero(puerto_cadena);
    if(validapuerto==1){
        puerto_srvdns=atoi(argv[2]);
        if(puerto_srvdns<1024 || puerto_srvdns>65535){
            printf("El número de puerto ha de estar entre 1024 y 65535\n");
            exit(3);
        }
    }else{
        printf("El puerto debe de ser un número positivo\n");
        exit(4);
    }


    char* tipo_socket=argv[3];

    if(strcmp(argv[3],"t")==0){
        es_stream=CIERTO;
    }else if(strcmp(argv[3],"u")==0){
        es_stream=FALSO;
    }else{
        printf("Tercer parámetro es incorrecto: [t|u]\n");
        exit(5);
    }

    char* nhilos_cadena=argv[4];
    int validanhilos=valida_numero(nhilos_cadena);
    if(validanhilos==1){
        nhilos=atoi(argv[4]);
        if(nhilos<1 || nhilos>MAXHILOSCLIENTE){
            printf("El número de hilos tiene que estar entre 1 y 10\n");
            exit(10);
        }
    }
    else{
        printf("El número de hilos debe de ser un número positivo\n");
        exit(11);
    }

    fich_consultas = argv[5];
    FILE *file_cons = fopen(fich_consultas, "r");
    if (!file_cons) {
        fprintf(stderr, "No se pudo abrir el fichero de log: %s\n", fich_consultas);
        exit(8);
    }
    fclose(file_cons);
}

void salir_bien(int s)
{
    exit(0);
}

void *hilo_lector(datos_hilo *p)
{
    int enviados, recibidos;
    char buffer[TAMLINEA];
    char respuesta[TAMLINEA];
    char *s;
    int sock_dat;
    FILE *fpin;
    FILE *fpout;

    if ((fpin = fopen(p->nom_fichero_consultas, "r")) == NULL)
    {
        perror("Error: No se pudo abrir el fichero de consultas");
        pthread_exit(NULL);
    }
    if ((fpout = fopen(hilos_file_names[p->id], "w")) == NULL)
    {
        fclose(fpin); // cerramos el handler del fichero de consultas
        perror("Error: No se pudo abrir el fichero de resultados");
        pthread_exit(NULL);
    }
    do
    {
        bzero(buffer, TAMLINEA);
        s = fgets(buffer, TAMLINEA, fpin);

        if (s != NULL)
        {
            if (es_stream)
            {
                // Enviar el mensaje leído del fichero a través de un socket TCP
                // y leer la respuesta del servidor
                // A RELLENAR
		if ((sock_dat = socket(PF_INET, SOCK_STREAM, 0)) < 0){
             		 printf("Error al crear el socket TCP");
              		 exit(40);
            }
	        if (connect(sock_dat, p->dserv, sizeof(struct sockaddr_in)) < 0){
               		printf("Error al conectar el socket");
              		exit(41);
            }
                if (write(sock_dat, buffer, TAMLINEA) == -1){
              		printf("Error al escribir en el socket");
              		exit(42);
            }
		recibidos = recv(sock_dat, respuesta, TAMMSG, 0);
		if(recibidos ==-1){
			printf("Error al recibir la conexion\n");
			exit(43);
		}
            }
            else
            {
                // Enviar el mensaje leído del fichero a través de un socket UDP
                // y leer la respuesta del servidor
                // A RELLENAR
		if ((sock_dat = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
              		printf("Error al crear el socket UDP");
              		exit(44);
            }
            	if (sendto(sock_dat, buffer, TAMLINEA, 0, p->dserv, sizeof(struct sockaddr_in)) < 0){
              		printf("Error al enviar el socket");
              		exit(45);
            }
		int tamaio = sizeof(struct sockaddr_in);
		recibidos=recvfrom(sock_dat,respuesta,TAMMSG,0,p->dserv,&tamaio);
		if(recibidos<0){
			printf("Error al recibir los datos\n");
			exit(46);
		}
		if(recibidos==0){
			printf("Conexion terminada por el otro extremo \n");
			break;
		}
            }
            close(sock_dat);
            // Volcar la petición y la respuesta, separadas por ":" en
            // el fichero de resultados
            // A RELLENAR
	    buffer[strlen(buffer)-1]='\0';
            fprintf(fpout, "%s:%s\n", buffer, respuesta);
        }
    } while (s);
    // Terminado el hilo, liberamos la memoria del puntero y cerramos ficheros
    fclose(fpin);
    fclose(fpout);
    free(p);
    return NULL;
}

int main(int argc, char *argv[])
{
    register int i;

    pthread_t *th;
    datos_hilo *q;

    struct sockaddr_in d_serv;

    // handler de archivo
    FILE *fp;

    signal(SIGINT, salir_bien);
    procesa_argumentos(argc, argv);

    // Comprobar si se puede abrir el fichero, para evitar errores en los hilos
    if ((fp = fopen(fich_consultas, "r")) == NULL)
    {
        perror("Error: No se pudo abrir el fichero de consultas");
        exit(6);
    }
    else
        fclose(fp); // cada hilo lo abrirá para procesarlo

    if ((th = (pthread_t *)malloc(sizeof(pthread_t) * nhilos)) == NULL)
    {
        fprintf(stderr, "No se pudo reservar memoria para los objetos de datos de hilo\n");
        exit(7);
    }

    // Inicializar la estructura de dirección del servidor que se pasará a los hilos
    // A RELLENAR
    memset(&d_serv, 0, sizeof(struct sockaddr_in));
    d_serv.sin_family = AF_INET;
    d_serv.sin_addr.s_addr = inet_addr(ip_srvdns);
    d_serv.sin_port = htons(puerto_srvdns);

    for (i = 0; i < nhilos; i++)
    {
        // Preparar el puntero con los parámetros a pasar al hilo
        // A RELLENAR
        q = (datos_hilo *)malloc(sizeof(datos_hilo));
	q->id = i;
	q->nom_fichero_consultas = fich_consultas;
        q->dserv = (struct sockaddr *) &d_serv;

        // Crear el hilo
        // A RELLENAR
        if (pthread_create(&th[i], NULL, (void *(*)(void *))hilo_lector, (void *) q) != 0)
        {
            fprintf(stderr, "No se pudo crear el hilo lector %d\n", i);
            exit(9);
        }
    }

    // Esperar a que terminen todos los hilos
    for (i = 0; i < nhilos; i++)
    {
        pthread_join(th[i], NULL);
    }
}
