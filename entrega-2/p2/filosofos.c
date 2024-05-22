/*****************************************************************************

                               filosofos.c

En esta version distribuida del problema de los filosofos requiere de un
maestro que controle los recursos compartidos (palillos).

*****************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h> // Fase 2
#include <semaphore.h> // Fase 4
// Funciones para simplificar la API de sockets. Se declaran aquí
// pero no se implementan. El alumno puede copiar su implementación
// de otros ejercicios, o implementarlas él mismo

sem_t sem_mutex; //Fase 4
int MAX_CONCURRENT_EATERS = 2; //Fase 4


// Crea un socket y si no hay errores lo retorna
int CrearSocketClienteTCP(void);

// Conecta a la IP y puertos dados. La IP se le pasa como cadena
// Si hay errores termina el programa
void ConectarConServidor(int s, char *ip, int puerto);

// Las dos siguientes funciones equivalen a send() y recv() pero
// si detectan un error terminan el programa
int Enviar(int s, char *buff, int longitud);
int Recibir(int s, char *buff, int longitud);

// Cierra el socket
void CerrarSocket(int s);

#define MAX_BOCADOS 10
#define LOCK 1
#define UNLOCK 2

//Prototipos de las funciones
void filosofo(int numfilo,char *ip,int puerto,int maxfilo);
int controlMutex(char *ipmaster, int puerto, char op, int nummutex1, int nummutex2);

void log_debug(char *msg) // FASE 2
{
    struct timespec t;
    clock_gettime(_POSIX_MONOTONIC_CLOCK, &t);

    printf("[%ld.%09ld] %s", t.tv_sec, t.tv_nsec, msg);
}

void main(int argc, char *argv[])
{
   int numfilo;
   int puerto;
   int maxfilo;

   //Comprobacion del número de argumentos
   if (argc<5)
   {
      fprintf(stderr,"Forma de uso: %s numfilo ip_maestro puerto maxfilo\n",
              argv[0]);
    exit(1);
   }
   /*** Lectura de valores de los argumentos ***/
   numfilo=atoi(argv[1]);
   if ((numfilo<0) || (numfilo>4))
   {
      fprintf(stderr,"El numero de filosofo debe ser >=0 y <=4\n");
    exit(2);
   }
   puerto=atoi(argv[3]);
   if ((puerto<0) || (puerto>65535))
   {
      fprintf(stderr,"El numero de puerto debe ser >=0 y <=65535\n");
    exit(3);
   }
   maxfilo=atoi(argv[4]);
   if (maxfilo<0)
   {
      fprintf(stderr,"El numero de filosofos debe ser mayor que 0\n");
    exit(3);
   }
   /*********************************************/
   //lanzamiento del filósofo
   sem_init(&sem_mutex, 0, MAX_CONCURRENT_EATERS); //Fase 4
   filosofo(numfilo,argv[2],puerto,maxfilo);
   exit(0);
}

//función que implementa las tareas a realizar por el filósofo
void filosofo(int numfilo,char *ip,int puerto,int maxfilo)
{
  int veces=0;
  char msg[100]; //Fase 2
  sprintf(msg, "Filosofo %d sentado a la mesa.\n", numfilo); //Fase 2
  log_debug(msg);  //Fase 2
  while (veces<MAX_BOCADOS)
  {
       // mientras el acuse de la solicitud de palillo derecho sea 0
       // seguimos intentándolo
       while (controlMutex(ip, puerto, LOCK, numfilo, (numfilo + 1) % maxfilo)==0); //Fase 3
       // ya tenemos ambos palillos y por tanto podemos comer
       sem_wait(&sem_mutex); //Fase 4
       sprintf(msg, "L %d %d\n", numfilo, (numfilo + 1) % maxfilo); //Fase 3
       log_debug(msg); //Fase 3
       sleep(0.5); //Fase 3
       sprintf(msg, "El filosofo %d toma palillos %d, %d y esta comiendo.\n", numfilo, numfilo, (numfilo + 1) % maxfilo); //Fase 2
       log_debug(msg);  //Fase 2
       sleep(3);
       // mientras el acuse de liberación de palillo izquierdo sea 0
       // seguimos intentando
       //liberar el palillo
       while (controlMutex(ip, puerto, UNLOCK, numfilo, (numfilo + 1) % maxfilo) == 0); //Fase 3
       //el filosofo ha soltado ambos palillos y puede dedicarse a pensar
       sem_post(&sem_mutex); //Fase 4
       sprintf(msg, "U %d %d\n", numfilo, (numfilo + 1) % maxfilo); //Fase 3
       log_debug(msg); //Fase 3
       sleep(0.5); //Fase 3
       sprintf(msg, "El filosofo %d deja palillos %d, %d y esta pensando.\n", numfilo, numfilo, (numfilo + 1) % maxfilo); //Fase 2
       log_debug(msg);  //Fase 2
       sleep(5);
       // incrementamos el número de veces que el filósofo
       // ha podido completar el ciclo
       veces++;
  }
  //el filósofo ha completado el número de bocados y se levanta de la mesa
  sprintf(msg, "El filosofo %d se ha levantado de la mesa.\n", numfilo);  //Fase 2
  log_debug(msg); //Fase 2
}

//esta función le permite al filósofo solicitar-liberar un recurso
//gestionado por el coordinador
int controlMutex(char *ipmaster, int puerto, char op, int nummutex1, int nummutex2)
{
   //Necesita de las funciones de libsokets
   int sock;
   char buffer[10];
   char *ptr;

   //crea el socket de conexion con el maestro de recursos
   sock=CrearSocketClienteTCP();
   //abre conexión con el maestro de recursos
   ConectarConServidor(sock, ipmaster, puerto);
   //en buffer confecciona el mensaje en función de la operación
   //a realizar y el palillo a solicitar (LOCK) o liberar (UNLOCK)
   sprintf(buffer,"%c",'\0');
   switch (op){
     case LOCK:
     sprintf(buffer,"L %d %d",nummutex1,nummutex2);
     break;
     case UNLOCK:
     sprintf(buffer,"U %d %d",nummutex1,nummutex2);
     break;
    }
    //se envía el mensaje al maestro de recursos
    Enviar(sock,buffer,strlen(buffer));
    //se inicializa el buffer para la recepción
    sprintf(buffer,"%c",'\0');
    //se espera síncronamente la respuesta del mensaje
    Recibir(sock,buffer,10);
    //se obtiene en ptr un puntero al primer token del mensaje
    ptr=strtok(buffer," \t\n");
    //se cierra el socket de comunicación con el servidor
    CerrarSocket(sock);
    //se retorna el valor entero del token recibido
    //1-OK, 0-NO OK
    return(atoi(ptr));
}


int CrearSocketClienteTCP()  //FASE 1
{
    int ret;
    int sock;
    ret=socket(PF_INET, SOCK_STREAM,0);
    if (ret==-1){
        perror("Error el crear el socket cliente TCP.");
        exit(1);
    }
    sock=ret;
    return sock;
}

void ConectarConServidor(int s, char* ip, int puerto) //FASE 1
{
    struct sockaddr_in dir;
    dir.sin_family=AF_INET;
    dir.sin_port=htons(puerto);
    //dir.sin_addr.s_addr=htonl(*ip);
    inet_pton(AF_INET, ip, &dir.sin_addr);
    if(connect(s, (struct sockaddr* )&dir, (socklen_t) sizeof(dir))<0){
            perror("Error al conectar el socket.");
        exit(1);
    }


}

int Enviar(int sock, char *buff, int longitud) //FASE 1
{
  int ret;
  ret=send(sock, buff, longitud, 0);
  if (ret==-1) {
    perror("Error al enviar.");
    exit(-1);
  }
  return ret;
}

int Recibir(int sock, char *buff, int longitud) //FASE 1
{
  int ret;
  ret=recv(sock, buff, longitud, 0);
  if (ret==-1) {
    perror("Error al recibir.");
    exit(-1);
  }
  return ret;
}

void CerrarSocket(int sock) //FASE 1
{
  if (close(sock)==-1) {
    perror("Error al cerrar el socket.");
  }
}
