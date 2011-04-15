#include <cstdlib>
#include <sys/socket.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <list>
#include "operaciones.h"
#include "parser.h"
#include <stdio.h>


#define miPuerto 61000
#define MAX_BUFF_MSG 1024*1024
#define miIP  "127.0.0.1"

#define puertoAdministrador 62000
#define MAX_QUEUE 30
#define MAX_BUFF_MSG_ADM 100
#define IPProxy  "127.0.0.1"

using namespace std;

//----------Estructura compartida------------//
bool booldenyPOST;
bool booldenyGET;

list<string>  listaDW;
list<string>  listaDUW;

//--------------Semaforos--------------------//
pthread_mutex_t mutexListDW = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListDUW = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexdenyPOST = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexdenyGET = PTHREAD_MUTEX_INITIALIZER;

int server_socket;

//funcion que comprueba si el m etodo es valido
bool validMethod (char* m){
  //retorna true si el metodo es valido
  bool retorno = false;
  int resultSearch=sscanf(m,"GET");
  if(resultSearch> 0)
  {
      //verifico estado del booleano
      pthread_mutex_lock( &mutexdenyGET );      
      if (booldenyGET)
	  retorno = false;
      else
	  retorno = true;      
      pthread_mutex_unlock( &mutexdenyGET );
      
  }else{
      resultSearch=sscanf(m,"POST");
      if (resultSearch > 0) {
	//verifico estado del booleano
	pthread_mutex_lock( &mutexdenyPOST );      
	if (booldenyPOST)
	  retorno = false;
	else
	  retorno = true;
	pthread_mutex_unlock( &mutexdenyPOST );
   }else{
	resultSearch= sscanf(m,"HEAD");
	if (resultSearch> 0)
	  retorno = true;
	else
	  retorno =false;
      }    
  }
  return retorno;
}


//funcion que comprueba si el url es valido
bool comprobarURL (char* url){
  //devuelve true si la url  es valida
  string urlStr (url);
  bool encontrePalabra = false;
  list<string>::iterator iter;
  pthread_mutex_lock( &mutexListDUW );      
  for(iter=listaDUW.begin(); ((iter!=listaDUW.end()) && (encontrePalabra == false)); iter++){
	  if (urlStr.find((*iter),0) !=-1){
	      encontrePalabra = true;
	  }	  
  }
  pthread_mutex_unlock( &mutexListDUW );      
  return (!encontrePalabra);
}

int main(int argc, char** argv) {

    booldenyPOST = false;
    booldenyGET  = false;

    cout<<"HOLA SOY EL SERVIDOR PROXY, VENGO A FLOTAR\n";
    cout.flush();
    //Creo un socket para la conexion con el cliente
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket==-1){
        cout << "Error al crear el socket"; cout.flush();
        exit(-1);
    }

    //contendra la direccion IP y el numero de puerto local
    struct sockaddr_in server_addr;
    socklen_t server_addr_size = sizeof server_addr;
    server_addr.sin_family = AF_INET;                 //tipo de conexion
    server_addr.sin_port = htons(miPuerto);           //puerto por donde voy a atender
    server_addr.sin_addr.s_addr = inet_addr(miIP);    //direccion IP mia
    bzero ( &(server_addr.sin_zero), 8);
    //primitiva BIND
    //socket, puntero a sockaddr_in, tamaño de sockaddr_in
    if (bind (server_socket, (struct sockaddr*) & server_addr, server_addr_size) == -1){
        cout << "Error bind"; cout.flush();
        close(server_socket);
        exit(-1);
    }

    //primitiva listen
    if (listen (server_socket, MAX_QUEUE) == -1){
        cout << "Error listen"; cout.flush();
        close(server_socket);
        exit(-1);
    }

    while (true){
			
    	// inicializo estructuras para primitiva ACCEPT

      //Contendra la direccion IP y numero de puerto del cliente
      struct sockaddr_in client_addr;
      socklen_t client_addr_size = sizeof client_addr;
      int socket_to_client;

			//cout << "aca llego000"; cout.flush();
      //espero por conexion de clientes
      socket_to_client = accept(server_socket, (struct sockaddr*) & client_addr,& client_addr_size);
      if (socket_to_client == -1){
          cout << "Error accept"; cout.flush();
          exit(-1);
      }
      
  //pthread_t hijo;
      //cout << "acaaaaaaaa"; cout.flush();
      //int socket_to_client = (*(int*)parametro);
      int sizeRecibido = MAX_BUFF_MSG;
      int datos_size = MAX_BUFF_MSG;
      string buffer="";
      //Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
      char* recibido;
      //bool finCabezal = false;
      //while (!finCabezal){
			    recibido = (char*) malloc(MAX_BUFF_MSG);
					      //recibo un pedazo del mensaje
			    sizeRecibido = recv(socket_to_client, recibido, datos_size, 0);
					      //string aux1 (recibido);
			    if (sizeRecibido == -1){
				cout << "Error recibir cabezal"; cout.flush();
				close(socket_to_client);
				exit(-1);
			    }
					      //me fijo si termino el cabezal http, termina con doble salto de linea
					      //size_t encontre = aux1.find("\r\n\r\n",0);
					      //if (encontre !=-1)
						      //finCabezal = true;
					      //buffer+= aux1;
			    //libero memoria de recibido
			      //free(recibido);
      //}
      cout << "Soy el servidor proxy, me mandaste esta boludez:\n" << recibido; cout.flush();
                      
      //Primer paso comprobarMetodo, no importa la url ni nada, en caso de error hay q cerrar la conexion y terminar la ejecucion (terminar la ejecucion solo cuando lo pasemosal otro archivo)
      //Segundo, si el metodo es valido se comprueba la url, si no es valida mandar mensaje de error al firefox, cerrar la conexoin y terminar la ejecucion (idem a lo de arriba con lo de terminar la ejecucion)
      //Tercero,si todo lo anterior esta bien se ejecuta el codigo q esta abajo, hay q hacer cambios, por ejemplo los controles del metodo y  url hacerlos los pasos anteriores y no aca, lo q si hay q hacer
      //es extraer bien el host y las partes para enviar el mensaje http al servidor.
      
      char* httpMethod= getHttpMethod(recibido);
      char* url=getUrl(recibido);
      char* headerLine1= (char*) malloc(1024);
      headerLine1[0]='\0';
      strcat(headerLine1,httpMethod);
      strcat(headerLine1," ");
      strcat(headerLine1,url);
      strcat(headerLine1," ");
      strcat(headerLine1,"HTTP/1.0\r\n");
      cout << "headerLine1: " << headerLine1<<"\n"; cout.flush();
      //aca hacer el if de si es valido
      if(validMethod(httpMethod) && comprobarURL(url)){
	    //obtengo el nombre del host para realizar una busqueda DNS del IP
	    char* hostName= getHostName(recibido);
	    char* headerLine2= (char*)malloc(1024);
	    headerLine2[0]='\0';
	    strcat(headerLine2,"Host: ");
	    strcat(headerLine2,hostName);
	    strcat(headerLine2,"\r\n\r\n");
	    strcat(headerLine1,headerLine2);
	    cout << "Esto es lo que mando:\n" << headerLine1; cout.flush();
	    //aca tengo q conectarme con el servidor posta, recibir la pagina y reenviarsela al cliente
	    struct sockaddr_in server_original;
	    server_original.sin_family = AF_INET; //tipo de conexion
	    server_original.sin_port = htons(80); //puerto del servidor
	    bzero ( &(server_original.sin_zero), 8);
	    int socknuevo = socket(AF_INET, SOCK_STREAM, 0);
	    if (socknuevo == -1){
	      cout << "Error socknuevo"; cout.flush();
	      exit(-1);
	    }
	    struct hostent* host = gethostbyname(hostName);
	    
	    //controlo que el servidor que busco existe
	    if (host!=NULL){
		  int** prueba = (int**)host->h_addr_list;
		  server_original.sin_addr.s_addr = **prueba; //direccion del servidor

		  //modifico el mensaje para q sea del tipo HTTP 1.0

		  //mando el mensaje nuevo
		  char* mensaje = &buffer[0];
		  if (connect(socknuevo, (struct sockaddr*) &server_original, sizeof(struct sockaddr)) == -1){
		    cout << "Error connect"; cout.flush();
		    close(socknuevo);
		    close(socket_to_client);
		    exit(-1);
		  }
		  if (send(socknuevo,mensaje,strlen(mensaje),0) == -1){
		      cout << "Error send al server"; cout.flush();
		      close(socknuevo);
		      close(socket_to_client);
		      exit(-1);
		  }

		  //Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
		  //Cuando el "sizeRecibido" es menor que el máximo el mensaje se ha
		  //completado.
		  buffer="";
		  char* recibido2;
		  sizeRecibido = MAX_BUFF_MSG;
		  while (sizeRecibido != 0){
			    recibido2 = (char*) malloc(MAX_BUFF_MSG);
			    sizeRecibido = recv(socknuevo, recibido2, datos_size, 0);
			    //envio mensaje al cliente
			    if (sizeRecibido == -1){
				      cout << "Error recv del server"; cout.flush();
				      close(socknuevo);
				      close(socket_to_client);
				      exit(-1);
			    }
			    cout << recibido; cout.flush();
			    if (sizeRecibido == 0 ){
				      cout << "Termino de recibir, no mando nada"; cout.flush();
				      close(socknuevo);
				      close(socket_to_client);
				      //exit(-1);
			    }else{
				      if(send(socket_to_client,recibido2,sizeRecibido,0) == -1){
						      cout << "Error send al fire"; cout.flush();
						      close(socknuevo);
						      close(socket_to_client);
						      exit(-1);
				      }
			    }
			    free(recibido2); //libero memoria de recibido
		  }//fin while sizeRecibido !=0
		  close(socknuevo);
		  //} //fin de conexion con el servidor host, siendo metodo y url validos
		  //cierro la conexion
		  close (socket_to_client);
		  
		  
	    }else{//fin if  servidor es validos
		//cierro el socket con el firefox
		//termino ejecucion, esto se hace cuando lo pasemos al otro archivo...
	    }
      }
	  
    /*}else{//fin metodo valido
	//crear mensaje de error, mandarlo y (cerrar la conexion)
      
    }//fin else metodoValido*/
    
    }//fin while true
	close (server_socket);
}

