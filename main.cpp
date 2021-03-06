

//para lo q meti del parser

#include <stdio.h>

#include <unistd.h>




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
#include "log.h"
#include "data.h"
#include <algorithm>
#include <functional>

//#define miPuerto 62000
#define MAX_QUEUE 30
#define MAX_BUFF_MSG 4096
//#define miIP  "127.0.0.1"

//#define puertoAdministrador 63000
#define MAX_BUFF_MSG_ADM 100
//#define IPProxy  "127.0.0.1"

using namespace std;

//----------Estructura compartida------------//
bool booldenyPOST;
bool booldenyGET;

list<string>  listaDW;
list<string>  listaDUW;

int miPuerto;
char* miIP;
int puertoAdministrador;
char* IPProxy;


//--------------Semaforos--------------------//
pthread_mutex_t mutexListDW   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListDUW  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexdenyPOST = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexdenyGET  = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t getURLMUX  = PTHREAD_MUTEX_INITIALIZER;
int server_socket;
int socket_administrador;

//codigo para manejar el ctrl-c
void signal_callback_handler(int signum){
	   cout<<"Caught signal %d\n";cout.flush();
	   close(server_socket);
	   close(socket_administrador);
	   // Cleanup and close up stuff here

	   // Terminate program
	   exit(-1);
}

void addElementDW(string elem) {
  if (!(binary_search(listaDW.begin(), listaDW.end(), elem))) {
      listaDW.push_front(elem);
  }else{
      cout << "Ya existia " << elem; cout.flush();
  } 
}

void addElementDUW(string elem) {
  if (!(binary_search(listaDUW.begin(), listaDUW.end(), elem))) {
      listaDUW.push_front(elem);
    }else{
      cout << "Ya existia " << elem; cout.flush();
  } 
}

// Funcion que elimina el 'elem' de la lista listaDW
void removeElementDW(string elem){
  if (binary_search(listaDW.begin(), listaDW.end(), elem)) {
      listaDW.remove(elem);
  }else{
      cout << "No se encontro " << elem; cout.flush();
  }  
// list<string>::iterator iter;
//  iter=listaDW.begin();
 // for(iter=listaDW.begin(); (iter!=listaDW.end()); iter++) {
//	if (elem.compare(*iter)!=0) {
//		listaDW.erase(iter);
//	}
//  }
}

// Funcion que elimina el 'elem' de la lista listaUDW
void removeElementDUW(string elem){
  if (binary_search(listaDUW.begin(), listaDUW.end(), elem)) {
      listaDUW.remove(elem);
  }else{
      cout << "No se encontro " << elem; cout.flush();
  } 

//  list<string>::iterator iter;
//  for(iter=listaDUW.begin(); (iter!=listaDUW.end()); iter++) {
//	if (elem.compare(*iter)!=0) {
//		listaDUW.erase(iter);
//	}
//}
}

//funcion que comprueba si el m etodo es valido
bool validMethod (char* m){
  //retorna true si el metodo es valido
  bool retorno = false;
  char* resultSearch = strstr(m,"GET");
  if(resultSearch != NULL)
  {
      //verifico estado del booleano
      pthread_mutex_lock( &mutexdenyGET );
      if (booldenyGET)
	  retorno = false;
      else
	  retorno = true;
      pthread_mutex_unlock( &mutexdenyGET );

  }else{
      resultSearch=strstr(m,"POST");
      if (resultSearch!=NULL) {
	//verifico estado del booleano
	pthread_mutex_lock( &mutexdenyPOST );
	if (booldenyPOST)
	  retorno = false;
	else
	  retorno = true;
	pthread_mutex_unlock( &mutexdenyPOST );
   }else{
	resultSearch=strstr(m,"HEAD");
	if (resultSearch!=NULL)
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

// Identificador del socket de conexión abierto por el servidor.

void * atender_varios_admins(void * param){
	//para maquina virtual
  	//int socket_con_administrador = (int)param;
	//para fedora
	intptr_t socket_con_administrador = (intptr_t)param;
	writeLog("ADMINISTRADOR: se creo el hilo de atencion para el administrador con exito.");
	
	//variables para leer comandos del administrador
        int sizeRecibidoAdmin = MAX_BUFF_MSG_ADM;
        char* recibidoDelAdmin;
	string mensajeAdmin="";

        int datos_admin_size = MAX_BUFF_MSG_ADM;

	bool quitAdministrador = false;
	bool terminoMensaje = false;//variable que sirve para saber si me termino de llegar el mensaje, supuestamente cuando te llega un \n termino el mensaje

	//mientras no me llegue el mensaje de quit del administrador sigo con la conexion
	while (!quitAdministrador){
	    //vuelvo a inicializar la variable
	    //mensajeAdmin.erase(0,strlen(mensajeAdmin.c_str()));
	    mensajeAdmin="\0";

	    while (!terminoMensaje){
		recibidoDelAdmin = (char *) malloc(MAX_BUFF_MSG_ADM);

		//recibo un pedazo del mensaje
		sizeRecibidoAdmin = recv(socket_con_administrador, recibidoDelAdmin, datos_admin_size, 0);
                if (sizeRecibidoAdmin == -1){
                    printf("error recv del administrador\n");
   		    printf("cerrando %d ",socket_con_administrador);
		    writeLog("ADMINISTRADOR: error al recibir del administrador.");
                    if (close(socket_con_administrador)<0){
			perror("ERROR cerrando socket:\n");			
		    }
		    printf("cerrado\n");
                    pthread_exit(NULL);

                }

		string aux (recibidoDelAdmin);
		//cout<<aux;cout.flush();
		size_t encontre = aux.find("\r\n",0);
		if (encontre !=-1)
		    terminoMensaje = true;
		mensajeAdmin+= aux;

		// libero memoria de recibido
		free(recibidoDelAdmin);
	    }

	    //vuelvo a poner la variable en false
	    terminoMensaje = false;

	    //imprimo como prueba el mensajeAdmin
	    printf("El mensaje que manda el administrador es: %s \n",mensajeAdmin.c_str());

	    //comando recibido
	    string comand = mensajeAdmin.substr(0,10);//guardo en comand los primeros 10 caracteres del mensaje, creo q con eso alcanza para saber cual es, no hice la cuenta...

	    //mensaje a enviar al administrador
	    string mensajeADevolver ="";

	    size_t tamanioMensajeADevolver;

	    //me devuelve el tipo de mensaje que recibi
	    int comandoIngresado = procesarTipoMensaje(comand);

	    //aca habria que modificar la estructura que tenemos...
	    string palabra="";
	    //en base al comando recibido decido que hago y que mensaje mando cuando corresponda
	    switch(comandoIngresado){

	      case 1:
		    mensajeADevolver = "Notificado el addDUW\r\n\0";
		    palabra=obtenerArgumento(mensajeAdmin);
		    pthread_mutex_lock( &mutexListDUW );
		    addElementDUW(palabra);
		    pthread_mutex_unlock( &mutexListDUW );
		    break;
	      case 2:
		    mensajeADevolver = "Notificado el addDW\r\n\0";
		    palabra=obtenerArgumento(mensajeAdmin);
		    pthread_mutex_lock( &mutexListDW );
		    addElementDW(palabra);
		    pthread_mutex_unlock( &mutexListDW );
		    break;
	      case 3:
		    mensajeADevolver = "Notificado el listDUW\r\n";
		    pthread_mutex_lock( &mutexListDUW );
		    mensajeADevolver+= printList(listaDUW);
		    mensajeADevolver+="\0";
		    pthread_mutex_unlock( &mutexListDUW );
		    break;
	      case 4:
		    mensajeADevolver = "Notificado el listDW\r\n";
		    pthread_mutex_lock( &mutexListDW );
		    mensajeADevolver+= printList(listaDW);
		    mensajeADevolver+="\0";
		    pthread_mutex_unlock( &mutexListDW );
		    break;
	      case 5:
		    mensajeADevolver = "Notificado el deleteDUW\r\n\0";
		    palabra=obtenerArgumento(mensajeAdmin);
		    pthread_mutex_lock( &mutexListDUW );
		    removeElementDUW(palabra);
		    pthread_mutex_unlock( &mutexListDUW );
		    break;
	      case 6:
		    mensajeADevolver = "Notificado el deleteDW\r\n\0";
		    palabra=obtenerArgumento(mensajeAdmin);
		    pthread_mutex_lock( &mutexListDW );
		    removeElementDW(palabra);
		    pthread_mutex_unlock( &mutexListDW );
		    break;
	      case 7:
		    mensajeADevolver = "Notificado el denyPOST\r\n\0";
		    pthread_mutex_lock( &mutexdenyPOST );
		    booldenyPOST = true;
		    pthread_mutex_unlock( &mutexdenyPOST );
		    break;
	      case 8:
		    mensajeADevolver = "Notificado el allowPOST\r\n\0";
		    pthread_mutex_lock( &mutexdenyPOST );
		    booldenyPOST = false;
		    pthread_mutex_unlock( &mutexdenyPOST );
		    break;
	      case 9:
		    mensajeADevolver = "Notificado el denyGET\r\n\0";
		    pthread_mutex_lock( &mutexdenyGET );
		    booldenyGET = true;
		    pthread_mutex_unlock( &mutexdenyGET );
		    break;
	      case 10:
		    mensajeADevolver = "Notificado el allowGET\r\n\0";
		    pthread_mutex_lock( &mutexdenyPOST );
		    booldenyGET = false;
		    pthread_mutex_unlock( &mutexdenyPOST );
		    break;
	      case 11:
		    mensajeADevolver = "Te fuiste a la mierda\r\n\0";
		    quitAdministrador = true;
		    break;
	      case 12:
		    mensajeADevolver = "Comando invalido\r\n\0";
		    break;
	    }
	    char * mens = (char*)malloc(mensajeADevolver.size()+1);
 	    strcpy(mens,mensajeADevolver.c_str());
	    tamanioMensajeADevolver = strlen(mens);
	    //Mando lo mismo que me llego para probar un poco si funcionan los comandos
	    if (send(socket_con_administrador,mens,tamanioMensajeADevolver,MSG_NOSIGNAL) == -1){
                writeLog("ADMINISTRADOR: Error al mandar mensaje al cliente administrador.");
		printf("cerrando por error send de admin %d",socket_con_administrador);
                if (close(socket_con_administrador) <0){
			perror("ERROR cerrando socket\n");			
		}
		printf("cerrado\n");
		free(mens);
                pthread_exit(NULL);
            }
	    writeLog("ADMINISTRADOR: mensaje enviado con exito al administrador.");
		free(mens);
	}
	printf("cerrando %d",socket_con_administrador);
	if (close(socket_con_administrador)<0){
		perror("ERROR cerrando socket\n");			
	}
	cout<<"cerrado\n";cout.flush();
  pthread_exit(NULL);
}

void * atencion_administrador(void * inutil) {

    //todo esto tendria que estar en una funcion
    writeLog("ATENCION A ADMIN: Se inicia atencion a administrador.");



    //Creo un socket para la conexion con el administrador
    socket_administrador = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_administrador <0){
	printf("error al crear socket admin\n");
        writeLog("ATENCION A ADMIN: Error al crear socket con atencion de administrador.");
        return (NULL);
    }

    //contendra la direccion IP y el numero de puerto local
    struct sockaddr_in proxy_addr;
    socklen_t proxy_addr_size = sizeof proxy_addr;
    proxy_addr.sin_family = AF_INET; //tipo de conexion
    proxy_addr.sin_port = htons(puertoAdministrador); //puerto al que me voy a conectar
    proxy_addr.sin_addr.s_addr = inet_addr(IPProxy); //direccion IP proxy

    //primitiva BIND
    //socket, puntero a sockaddr_in, tamaño de sockaddr_in
    if (bind (socket_administrador, (struct sockaddr*) & proxy_addr, proxy_addr_size) <0){
        writeLog("ATENCION A ADMIN: Error al realizar bind del socket con atencion de administrador.");
	printf("cerrando por error bind admin %d",socket_administrador);
        if (close(socket_administrador)<0){
		perror("ERROR cerrando socket\n");			
	}
	printf("cerrado\n");
        return (NULL);
    }

    //primitiva listen
    if (listen (socket_administrador, MAX_QUEUE) <0 ){
        writeLog("ATENCION A ADMIN: Error al realizar listen sobre el socket con atencion de administrador.");
	printf("cerrando por error listen admin %d",socket_administrador);
        if (close(socket_administrador)<0){
		perror("ERROR cerrando socket\n");				
	}
	printf("cerrado\n");
        pthread_exit(NULL);
    }

    int socket_con_administrador;
    int adminCreate;
    while (true){


	struct sockaddr_in admin_addr;
        socklen_t admin_addr_size = sizeof admin_addr;


        //espero por conexion de clientes
        socket_con_administrador = accept(socket_administrador, (struct sockaddr*) & admin_addr,& admin_addr_size);
        if (socket_con_administrador <0){
          writeLog("ATENCION A ADMIN: Error al realizar accept sobre el socket con atencion de administrador.");
	  printf ("error accept administrador\n");
        }
        else{
	  pthread_t hijo_admin;
	  pthread_attr_t attr; 

	  pthread_attr_init  (&attr); 

	  pthread_attr_setdetachstate  (&attr,  PTHREAD_CREATE_DETACHED); 
	  adminCreate=pthread_create(&hijo_admin,&attr,atender_varios_admins,(void*)socket_con_administrador);
	  if (adminCreate <0){
		  writeLog("ATENCION A ADMIN: Error al crear un hilo de administrador con exito");
		  close(socket_con_administrador);
	  }

	}
    }

    close (socket_administrador);
    pthread_exit(NULL);
}

void * atenderWeb(void * parametro){
      //para maquina virtual
      //int socket_to_browser = (int)parametro;
      //para fedora
      writeLog("ATENCION WEB: se creo el hilo de atencion para el cliente web con exito.");
      intptr_t socket_to_browser = (intptr_t)parametro;
      int sizeRecibido = MAX_BUFF_MSG;
      int datos_size = MAX_BUFF_MSG;
      //Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
      char* recibido;
      char* auxRecibidoFinal;
      int sizeAuxRecibidoFinal = 0;
      char* recibidoFinal;
      int sizeRecibidoFinal = 0;
      bool termine = false;
//      while (!termine){
        recibido = (char*) malloc(MAX_BUFF_MSG);
			//recibo un pedazo del mensaje
        sizeRecibido = recv(socket_to_browser, recibido, datos_size, 0);
			//string aux1 (recibido);
	recibidoFinal = (char*) malloc(sizeRecibido + 1);
	memcpy(recibidoFinal, recibido, sizeRecibido);
	memcpy(recibidoFinal + sizeRecibido, "\0", 1);

	printf("GUILEEEEEEEEEEEEEEEEEEEEEEEee %s", recibidoFinal);
  //      if (sizeRecibido > 0) { 
		//cout<< "HAAAAAAAAAAAAAAAAAAA"; cout.flush();
		//cout<< strlen(recibidoFinal); cout.flush();
		//cout<< "HAAAAAAAAAAAAAAAAAAA"; cout.flush();
//	  auxRecibidoFinal = (char*) malloc(sizeRecibidoFinal + strlen(recibido));
//	  memcpy(auxRecibidoFinal, recibidoFinal, sizeRecibidoFinal);
//	  memcpy(auxRecibidoFinal + sizeRecibidoFinal, recibido, strlen(recibido));
//	  sizeAuxRecibidoFinal = sizeRecibidoFinal + strlen(recibido);
	  
//	  if (recibidoFinal != NULL) { free(recibidoFinal); }
//	  recibidoFinal = (char*) malloc(sizeAuxRecibidoFinal);
//	  memcpy(recibidoFinal, auxRecibidoFinal, sizeAuxRecibidoFinal);
//	  sizeRecibidoFinal = sizeAuxRecibidoFinal;
			
//	  free(auxRecibidoFinal);

  //        char* salto = strstr(recibidoFinal,"\r\n\r\n");
  //        if (salto != NULL){
 //           termine =true;
  //        }
//	}else{
//	  termine = true;
//	}      
  //    }
      if (sizeRecibido <= 0){
	  writeLog("ATENCION WEB: Hubo error al recibir el encabezado por parte del cliente web.");
 	  printf("cerrando por error recv cabezal de fire %d",socket_to_browser);
	  if (close(socket_to_browser)<0){
		perror("ERROR cerrando socket:\n");				
	  }
	  cout<<"cerrado\n";cout.flush();
	  pthread_exit(NULL);
      }
	else{writeLog("Se recibio el encabezado por parte del cliente web con exito.");}

      char* httpMethod = getHttpMethod(recibido);


      bool metodoValido = validMethod(httpMethod); //llamar funcion comprobarMetodo despues de obtenerlo

      if (metodoValido){
	
	//////////HEADER LINE 1
	writeLog("ATENCION WEB: El metodo recibido por el cliente es valido.");
	char* aL; //auxLine
	int sizeAL = 0;
	
	int sizeHL1 = strlen(httpMethod) + 1;
	char* hL1 = (char*) malloc(sizeHL1); //headerLine1
	memcpy(hL1, httpMethod, strlen(httpMethod));
	memcpy(hL1 + strlen(httpMethod), " ", 1);
	free(httpMethod);
	
	char* url = getUrl(recibido);
	aL = (char*) malloc(sizeHL1 + strlen(url) + 1);
	memcpy(aL, hL1, sizeHL1);
	free(hL1);
	memcpy(aL + sizeHL1, url, strlen(url));
	memcpy(aL + sizeHL1 + strlen(url), " ", 1);
	sizeAL = sizeHL1 + strlen(url) + 1;
	
	hL1 = (char*) malloc(sizeAL);
	memcpy(hL1, aL, sizeAL);
	sizeHL1 = sizeAL;
	free(aL);
	
	aL = (char*) malloc(sizeHL1 + 10);
	memcpy(aL, hL1, sizeHL1);
	memcpy(aL + sizeHL1, "HTTP/1.0\r\n", 10);
	sizeAL = sizeHL1 + 10;
	free(hL1);
	hL1 = (char*) malloc(sizeAL);
	memcpy(hL1, aL, sizeAL);
	sizeHL1 = sizeAL;
	free(aL);
	
	
	//obtengo el nombre del host para realizar una busqueda DNS del IP
	char* hostName = getHostName(recibido);
	
	
	bool urlValida = comprobarURL(url) && comprobarURL(hostName);//llamar a funcion comprobarUrl despues de obtenerla

	printf("\nurl: %s\n",url);
	free(url);
	if (urlValida){  
	    writeLog("ATENCION WEB: La url del cliente web es valida.");
	    //////////HEADER LINE 2
	    
	    int sizeAL = 0;
	    
	    int sizeHL2 = 6;
	    char* hL2 = (char*) malloc(sizeHL2); //headerLine2
	    memcpy(hL2, "Host: ", sizeHL2);
	    

	    
	    aL = (char*) malloc(sizeHL2 + strlen(hostName) + 2);
	    memcpy(aL, hL2, sizeHL2);
	    memcpy(aL + sizeHL2, hostName, strlen(hostName));
	    memcpy(aL + sizeHL2 + strlen(hostName), "\r\n", 2);
	    sizeAL = sizeHL2 + strlen(hostName) + 2;
	    free(hL2);
	    hL2 = (char*) malloc(sizeAL);
	    memcpy(hL2, aL, sizeAL);
	    sizeHL2 = sizeAL;
	    
	    free(aL);
	    
	    //cout << "headerLine2: " << hL2; cout.flush();
	    
	    //////////HEADER LINE 3
	    
	    char* userAgent=getUserAgent(recibido);
	    
	    int sizeHL3 = strlen(userAgent) + 2;
	    char* hL3 = (char*) malloc(sizeHL3); //headerLine3
	    memcpy(hL3, userAgent, strlen(userAgent));
	    memcpy(hL3 + strlen(userAgent), "\r\n", 2); 
	    
	    free(userAgent);
	    
	    
	    //////////HEADER COMPLETO
	    
	    sizeAL = 0;
	    
	    //printf("ANTES DE RESTANTES HEADERS%s\n",recibidoFinal);
	    
	    int sizeHeadersRestantes = 0;
	    char* headersRestantes = getHeadersRestantes(recibidoFinal,sizeHeadersRestantes);
	    //printf("LARGOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOo%d\n",sizeHeadersRestantes);
	    //printf("3\n");
	    aL = (char*) malloc(sizeHL1 + sizeHL2 + sizeHL3 + sizeHeadersRestantes + 2);
	    //printf("3\n");
	    memcpy(aL, hL1, sizeHL1);
	    //printf("3\n");
	    memcpy(aL + sizeHL1, hL2, sizeHL2);
	    //printf("3\n");
	    memcpy(aL + sizeHL1 + sizeHL2, hL3, sizeHL3);
	    //printf("3\n");
	    memcpy(aL + sizeHL1 + sizeHL2 + sizeHL3, headersRestantes, sizeHeadersRestantes);
	    //printf("3\n");
	    memcpy(aL + sizeHL1 + sizeHL2 + sizeHL3 + sizeHeadersRestantes, "\r\n", 2);
	    //printf("3\n");
	    sizeAL = sizeHL1 + sizeHL2 + sizeHL3 + sizeHeadersRestantes + 2;
	    //printf("3\n");

            int sizeHeader;
	    char* header;

            if (esPost(recibido)) {
              int sizeBody = 0;
	      char* body = getPostBody(recibidoFinal,sizeRecibido,sizeBody);

              sizeHeader = sizeAL + sizeBody + 1;
	      header = (char*) malloc(sizeHeader);
	      memcpy(header, aL, sizeAL);
              memcpy(header + sizeAL, body, sizeBody);
	      memcpy(header + sizeAL + sizeBody, "\0", 1);
            }else{
              sizeHeader = sizeAL + 1;
	      header = (char*) malloc(sizeHeader);
	      memcpy(header, aL, sizeAL);
	      memcpy(header + sizeAL, "\0", 1);
            }

	    
	    //printf("RECIBIDO FINAL%s\n",recibidoFinal);
	    //printf("HEADERS RESTANTES%s\n",headersRestantes);
	    printf("TODO JUNTO%s\n",header);

	    
	    free(aL);
	    free(hL1);
	    free(hL2);
	    free(hL3);
	    //free(recibido);
	    

/////////////////////////////////////////////////////////////////////
	    string recibidoAImprimir = "ATENCION WEB: recibido\n";
	    string recibidoString (recibidoFinal);
	    recibidoAImprimir+=recibidoString;
	    writeLog(recibidoAImprimir);
/////////////////////////////////////////////////////////////////////

	    string stringAEnviar = "ATENCION WEB: enviado\n";
	    string stringAux (header);
	    stringAEnviar+=stringAux;
	    writeLog(stringAEnviar);
/////////////////////////////////////////////////////////////////////

	    //aca tengo q conectarme con el servidor posta, recibir la pagina y reenviarsela al cliente
	    struct sockaddr_in server_original;
	    server_original.sin_family = AF_INET; //tipo de conexion
	    server_original.sin_port = htons(80); //puerto del servidor
	    bzero ( &(server_original.sin_zero), 8);

	    struct hostent* host = gethostbyname(hostName);
	    

	    //controlo que el servidor que busco existe
	    if (host!=NULL){
                  int socket_to_server = socket(AF_INET, SOCK_STREAM, 0);
                  if (socket_to_server == -1){
                    writeLog("ATENCION WEB: Error al crear socket con el servidor al que se le mandara el pedido.");
		    free(header);
		    free(hostName);
                    pthread_exit(NULL);
                  }
		  int** prueba = (int**)host->h_addr_list;
		  server_original.sin_addr.s_addr = **prueba; //direccion del servidor


		  //obtenemos la direccion con getaddrinfo
		  struct addrinfo hints, *res;
		  memset(&hints, 0, sizeof hints);
		  hints.ai_family = AF_INET;
		  hints.ai_socktype = SOCK_STREAM;
		  getaddrinfo(hostName, "80", &hints, &res);
		  
				

		  //modifico el mensaje para q sea del tipo HTTP 1.0

		  //mando el mensaje nuevo
		  if (connect(socket_to_server, res->ai_addr, res->ai_addrlen) <0){
		    writeLog("ATENCION WEB: Error al realizar connect con el servidor al que se le solicita el pedido.");
		    if (close(socket_to_browser)<0){
			perror("ERROR cerrando socket:\n");			
		    }
		    if (close(socket_to_server)<0){
			perror("ERROR cerrando socket:\n");			
		    }
			//free(host);
		    free(hostName);
		    free(header);
		    pthread_exit(NULL);
		  }

		  if (send(socket_to_server,header,sizeHeader,MSG_NOSIGNAL) <0){
		      writeLog("ATENCION WEB: Error al realizar send con el servidor al que se le solicita el pedido.");
			printf("error al enviar al servidor web");
		      if (close(socket_to_browser)<0){
			perror("ERROR cerrando socket:\n");			
		      }
		      if (close(socket_to_server)<0){
			perror("ERROR cerrando socket\n");			
		      }
		      free(header);
		      free(hostName);
		      pthread_exit(NULL);
		  }
		  free(header);
		  free(hostName);

		  //Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
		  //Cuando el "sizeRecvServ" es menor que el máximo el mensaje se ha
		  //completado.
		  char* bufferRcv = (char*) malloc(1);
			int sizeBufferRcv = 0;
		  char* rcvServ;
		  int sizeRcvServ = MAX_BUFF_MSG;
			char* auxBufferRcv;
			int sizeAuxBufferRcv = 0;
		  while (sizeRcvServ != 0){
			    rcvServ = (char*) malloc(MAX_BUFF_MSG);
			    sizeRcvServ = recv(socket_to_server, rcvServ, datos_size, 0);
			    //envio mensaje al cliente
			    if (sizeRcvServ <0){
				      writeLog("ATENCION WEB: Error al realizar recv con el servidor al que se le solicita el pedido.");
				      if (close(socket_to_server)<0){
					perror("ERROR cerrando socket\n");			
				      }
				      if (close(socket_to_browser)<0){
					perror("ERROR cerrando socket:\n");			
				      }
				      pthread_exit(NULL);
			    }

			    if (sizeRcvServ == 0 ){

				      printf("cerrando socket,termine recibir servidor %d",socket_to_server);
				      if (close(socket_to_server)<0){
					perror("ERROR cerrando socket\n");			
				      }
					printf("cerrado\n");

			    }else{
				  auxBufferRcv = (char*) malloc(sizeBufferRcv + sizeRcvServ);
				  memcpy(auxBufferRcv, bufferRcv, sizeBufferRcv);
				  memcpy(auxBufferRcv + sizeBufferRcv, rcvServ, sizeRcvServ);
				  sizeAuxBufferRcv = sizeBufferRcv + sizeRcvServ;
				  if (bufferRcv != NULL) { free(bufferRcv); }
				  bufferRcv = (char*) malloc(sizeAuxBufferRcv);
				  memcpy(bufferRcv, auxBufferRcv, sizeAuxBufferRcv);
				  sizeBufferRcv = sizeAuxBufferRcv;
				  free(auxBufferRcv);
				  sizeAuxBufferRcv = 0;
			    }
			    if (rcvServ!= NULL){
				free(rcvServ); //libero memoria de recibido
			    }
		  }//fin while sizeRecvServ !=0
		  if (sizeBufferRcv!=0){


//////////////////////////////////////////

  list<string>::iterator iter;

  //pido el semaforo	
  pthread_mutex_lock( &mutexListDW );
  for(iter=listaDW.begin(); (iter!=listaDW.end()); iter++){

	    char* auxPalabra = (char*) malloc((*iter).size()+1);
	    strcpy (auxPalabra, (*iter).c_str());
            bool encontre = true;
	    while (encontre){
		char * posicion = strstr(bufferRcv,auxPalabra);
		if (posicion == NULL){
		    encontre = false;
		}
		else{
		    int largo = strlen(auxPalabra);
		    int iterador=1;
		    while (iterador<=largo){
		        (*posicion)='*';
		        posicion++;
		        iterador++;
		    }
		}
	    }
	    free(auxPalabra);
  }
  //libero el semaforo
  pthread_mutex_unlock( &mutexListDW );

//////////////////////////////////////////

			int total = 0;
			int bytesleft = sizeBufferRcv;
			int n;
			    while(total < sizeBufferRcv) {
				n = send(socket_to_browser, bufferRcv + total, bytesleft, MSG_NOSIGNAL);
				if (n <0) {
				  close(socket_to_browser);				  
				}
				total += n;
				bytesleft -= n;
			    }
			int enviados = total; 
			if( enviados <0){
				      writeLog("ATENCION WEB: Error al realizar send al cliente web.");

				      printf("cerrando error al enviar al fire %d\n",socket_to_browser);
				      if (close(socket_to_browser)<0){
					perror("ERROR cerrando socket\n");			
				      }
				      printf("cerrado\n");
				      free(host);
				      pthread_exit(NULL);
			}	
			      cout<<"recibidos: "<<sizeBufferRcv;cout.flush();
			      printf("enviados: %d\n",enviados);
			free(bufferRcv);

			if (close (socket_to_browser) < 0){
			      perror("ERROR cerrando socket:\n");			
			}

			free(recibido);

			pthread_exit(NULL);
		  }
		

	    }else{//fin if  servidor es valido
		//cierro el socket con el firefox
		//termino ejecucion, esto se hace cuando lo pasemos al otro archivo...
		writeLog("ATENCION WEB: el servidor es invalido.");
		printf("servidor invalido\n");
		if (close(socket_to_browser)<0){
			perror("ERROR cerrando socket\n");			
		}
		free(recibido);
		free(header);
                pthread_exit(NULL);
	    }

	}else{//fin if url valida
	    printf("url invalido\n");
	    writeLog("ATENCION WEB: la url es invalida.");
	    char * merror=strdup("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n<h1>404 bloqueado por redes26,url invalida</h1>\r\n</body>\r\n</html>");	    
	    int cant = send(socket_to_browser,merror,strlen(merror),MSG_NOSIGNAL);
	    if (cant ==-1){
		printf("fallo el send de url invalida\n");
	    }
	    free(merror);	    
            if (close(socket_to_browser)<0){
		perror("ERROR cerrando socket\n");			
            }
	    free(recibido);
	    free(hostName);
            pthread_exit(NULL);
	}//fin else ulr valida
      }else{//fin metodo valido
	  free(httpMethod);
	  free(recibido);
	  //crear mensaje de error, mandarlo y (cerrar la conexion)
	  printf("metodo invalido\n");
	  writeLog("ATENCION WEB: el metodo es invalido.");
	  char * merror=strdup("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n<h1>404 bloqueado por redes26,metodo invalido</h1>\r\n</body>\r\n</html>");	    
	  int cant = send(socket_to_browser,merror,strlen(merror),MSG_NOSIGNAL);
	  if (cant ==-1){
	      printf("fallo el send de metodo invalido\n");
	  }
	  free(merror);
	  if (close(socket_to_browser) <0){
		perror("ERROR cerrando socket:\n");			
	  }
	  pthread_exit(NULL);
      }//fin else metodoValido

	  printf("estop no tendria q aparecer nunca\n");
	  printf("cerrando cualquier fruta %d\n",socket_to_browser);
	  if (close(socket_to_browser)<0){
		perror("ERROR cerrando socket\n");			
	  }
	  printf("cerrado frutasss\n");
	  pthread_exit(NULL);
      
   
}

/*
 *
 */
int main(int argc, char *argv[] ) {

	if (argc==3)
	{
		setServerIp(argv[1]);
		setServerPort(argv[2]);
	}
	else if (argc==2)
	{
		setServerIp(argv[1]);
	}

	miPuerto=getServerPort();
	miIP=getServerIp();
	puertoAdministrador=getAdminPort();
	IPProxy=getAdminIp();

	//writeLog("Se inicia el servidor proxy.");
	signal(SIGINT, signal_callback_handler);
	pthread_t ad;
	int inutilizable=2;
      

	booldenyPOST=false;
	booldenyGET=false;

	cout<<"Se inicio el servidor proxy\n";
	cout.flush();
	//Creo un socket para la conexion con el cliente
	server_socket = socket(AF_INET, SOCK_STREAM, 0);



	if (server_socket == -1){
	    writeLog("SERVIDOR PROXY: Hubo error al crear el socket para la conexion con el cliente del servidor proxy");
	    return (-1);
	}

	//contendra la direccion IP y el numero de puerto local
	struct sockaddr_in server_addr;
	socklen_t server_addr_size = sizeof server_addr;
	server_addr.sin_family = AF_INET; //tipo de conexion
	server_addr.sin_port = htons(miPuerto); //puerto por donde voy a atender
	server_addr.sin_addr.s_addr = inet_addr(miIP); //direccion IP mia

	//primitiva BIND
	//socket, puntero a sockaddr_in, tamaño de sockaddr_in
	if (bind (server_socket, (struct sockaddr*) & server_addr, server_addr_size) == -1){
	    printf("error bind main\n");
	    writeLog("SERVIDOR PROXY: Hubo error al realizar bind entre el socket y el descriptor, para la conexion con el cliente del servidor proxy");
	    if (close(server_socket)<0){
		    perror("ERROR por bind main cerrando socket:\n");				
	    }
	    return (-1);
	}

	//primitiva listen
	if (listen (server_socket, MAX_QUEUE) == -1){
	    writeLog("SERVIDOR PROXY: Hubo error al realizar la escucha sobre el socket para la conexion con el cliente del servidor proxy.");
	    printf("cerrando por error listen main %d\n",server_socket);
	    if (close(server_socket)<0){
		    perror("ERROR cerrando socket:\n");			
	    }
	    printf("cerrado\n");
	    return (-1);
	}
	int resultPC;
        resultPC=pthread_create(&ad,NULL,atencion_administrador,(void*)inutilizable);
	if (resultPC<0){
		writeLog("SERVIDOR PROXY: Hubo error al crear el hilo del administrador.");
	        return (-1);
	}

        int socket_to_client;
	while (true){
	  // inicializo estructuras para primitiva ACCEPT

	  //Contendra la direccion IP y numero de puerto del cliente
	  struct sockaddr_in client_addr;
	  socklen_t client_addr_size = sizeof client_addr;


	  //espero por conexion de clientes
	  socket_to_client = accept(server_socket, (struct sockaddr*) & client_addr,& client_addr_size);
	  if (socket_to_client <0){
	    writeLog("SERVIDOR PROXY: Hubo error al realizar accept con el socket de clientes.");
	    printf("error accept web\n");		
	  }else{
	  
	  pthread_t hijo;
	  pthread_attr_t attr; 

	  pthread_attr_init  (&attr); 

	  pthread_attr_setdetachstate  (&attr,  PTHREAD_CREATE_DETACHED); 
	  if (pthread_create(&hijo,&attr,atenderWeb,(void*)socket_to_client) <0){
	    writeLog("SERVIDOR PROXY: Hubo error al crear el thread de atencioin web.");
	    close(socket_to_client);
	  }
		}

	}	
	cout<<"cerrando mal"<<server_socket;cout.flush();
    	if (close (server_socket)<0){
		perror("ERROR cerrando socket:\n");			
	}
	printf("cerrado chau\n");
}

