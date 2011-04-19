

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

#define miPuerto 62000
#define MAX_QUEUE 30
#define MAX_BUFF_MSG 1024*1024
#define miIP  "127.0.0.1"

#define puertoAdministrador 63000
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

// Funcion que elimina el 'elem' de la lista listaDW
void removeElementDW(string elem){
  list<string>::iterator iter;
  iter=listaDW.begin();
  while((elem.compare(*iter)!=0))
  {
    iter++;
  }
  listaDW.erase(iter);
}

// Funcion que elimina el 'elem' de la lista listaUDW
void removeElementDUW(string elem){
  list<string>::iterator iter;
  iter=listaDUW.begin();
  while((elem.compare(*iter)!=0))
  {
    iter++;
  }
  listaDUW.erase(iter);
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
                    cout<<"error recv del administrador\n";cout.flush();
   		    cout<<"cerrando "<<socket_con_administrador;cout.flush();
                    if (close(socket_con_administrador)<0){
			perror("ERROR cerrando socket:\n");			
		    }
		    cout<<"cerrado\n";cout.flush();
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
	    cout<<"El mensaje que manda el administrador es:\n"<<mensajeAdmin;cout.flush();

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
		    listaDUW.push_front(palabra);
		    pthread_mutex_unlock( &mutexListDUW );
		    break;
	      case 2:
		    mensajeADevolver = "Notificado el addDW\r\n\0";
		    palabra=obtenerArgumento(mensajeAdmin);
		    pthread_mutex_lock( &mutexListDW );
		    listaDW.push_front(palabra);
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
		    mensajeADevolver = "Boludo, ingresaste cualquier cosa\r\n\0";
		    break;
	    }
	    char * mens = (char*)malloc(mensajeADevolver.size()+1);
 	    strcpy(mens,mensajeADevolver.c_str());
	    tamanioMensajeADevolver = strlen(mens);
	    //Mando lo mismo que me llego para probar un poco si funcionan los comandos
	    if (send(socket_con_administrador,mens,tamanioMensajeADevolver,MSG_NOSIGNAL) == -1){
                writeLog("ADMINISTRADOR: Error al mandar mensaje al cliente administrador.");
		cout<<"cerrando "<<socket_con_administrador;cout.flush();
                if (close(socket_con_administrador) <0){
			perror("ERROR cerrando socket\n");			
		}
		cout<<"cerrado\n";cout.flush();
		free(mens);
                pthread_exit(NULL);
            }
		free(mens);
	}
	cout<<"cerrando "<<socket_con_administrador;cout.flush();
	if (close(socket_con_administrador)<0){
		perror("ERROR cerrando socket\n");			
	}
	cout<<"cerrado\n";cout.flush();
  pthread_exit(NULL);
}

void * atencion_administrador(void * inutil) {

    //todo esto tendria que estar en una funcion
writeLog("ATENCION A ADMIN: Se inicia atencion a administrador.");

    cout<<"Esta es la consola del administrador\n";
    cout.flush();

    //Creo un socket para la conexion con el administrador
    socket_administrador = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_administrador == -1){
        writeLog("ATENCION A ADMIN: Error al crear socket con atencion de administrador.");
        pthread_exit(NULL);
    }

    //contendra la direccion IP y el numero de puerto local
    struct sockaddr_in proxy_addr;
    socklen_t proxy_addr_size = sizeof proxy_addr;
    proxy_addr.sin_family = AF_INET; //tipo de conexion
    proxy_addr.sin_port = htons(puertoAdministrador); //puerto al que me voy a conectar
    proxy_addr.sin_addr.s_addr = inet_addr(IPProxy); //direccion IP proxy

    //primitiva BIND
    //socket, puntero a sockaddr_in, tamaño de sockaddr_in
    if (bind (socket_administrador, (struct sockaddr*) & proxy_addr, proxy_addr_size) == -1){
        writeLog("ATENCION A ADMIN: Error al realizar bind del socket con atencion de administrador.");
	cout<<"cerrando "<<socket_administrador;cout.flush();
        if (close(socket_administrador)<0){
		perror("ERROR cerrando socket\n");			
	}
	cout<<"cerrado\n";cout.flush();
        pthread_exit(NULL);
    }

    //primitiva listen
    if (listen (socket_administrador, MAX_QUEUE) == -1 ){
        writeLog("ATENCION A ADMIN: Error al realizar listen sobre el socket con atencion de administrador.");
	cout<<"cerrando "<<socket_administrador;cout.flush();
        if (close(socket_administrador)<0){
		perror("ERROR cerrando socket\n");				
	}
	cout<<"cerrado\n";cout.flush();
        pthread_exit(NULL);
    }



    while (true){

	int socket_con_administrador;
	struct sockaddr_in admin_addr;
        socklen_t admin_addr_size = sizeof admin_addr;


        //espero por conexion de clientes
        socket_con_administrador = accept(socket_administrador, (struct sockaddr*) & admin_addr,& admin_addr_size);
        if (socket_con_administrador == -1){
          writeLog("ATENCION A ADMIN: Error al realizar accept sobre el socket con atencion de administrador.");
	  close(socket_administrador);
	  close(socket_con_administrador);
          pthread_exit(NULL);
        }
	pthread_t hijo_admin;
	int adminCreate;
	if(adminCreate=pthread_create(&hijo_admin,NULL,atender_varios_admins,(void*)socket_con_administrador)==0)
	{
		writeLog("ATENCION A ADMIN: Se creo un hilo de administrador con exito");
	}
	else
	{
		writeLog("ATENCION A ADMIN: Error al crear un hilo de administrador con exito");
	}

    }

    close (socket_administrador);
    pthread_exit(NULL);
}

void * atenderWeb(void * parametro){
      //para maquina virtual
      //int socket_to_browser = (int)parametro;
      //para fedora
      intptr_t socket_to_browser = (intptr_t)parametro;
      int sizeRecibido = MAX_BUFF_MSG;
      int datos_size = MAX_BUFF_MSG;
      //Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
      char* recibido;
      recibido = (char*) malloc(MAX_BUFF_MSG);
			//recibo un pedazo del mensaje
      sizeRecibido = recv(socket_to_browser, recibido, datos_size, 0);
			//string aux1 (recibido);
      if (sizeRecibido == -1){
	  writeLog("ATENCION WEB: Hubo error al recibir el encabezado por parte del cliente web.");
 	  cout<<"cerrando por error recv cabezal de fire"<<socket_to_browser;cout.flush();
	  if (close(socket_to_browser)<0){
		perror("ERROR cerrando socket:\n");				
	  }
	  cout<<"cerrado\n";cout.flush();
	  pthread_exit(NULL);
      }
	else{writeLog("Se recibio el encabezado por parte del cliente web con exito. El numero de hilo de la atencion a ese cliente es:"+ socket_to_browser);}
	//cout << "recibido: " << recibido; cout.flush();
      char* httpMethod = getHttpMethod(recibido);
	
	//cout<<"socket: "<<socket_to_browser<<"\n";

      bool metodoValido = validMethod(httpMethod); //llamar funcion comprobarMetodo despues de obtenerlo
      //aca hacer el if de si es valido
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
	//cout<<url;cout.flush();
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
	    
	    int sizeHL3 = strlen(userAgent) + 4;
	    char* hL3 = (char*) malloc(sizeHL3); //headerLine3
	    memcpy(hL3, userAgent, strlen(userAgent));
	    memcpy(hL3 + strlen(userAgent), "\r\n\r\n", 4); 
	    
	    free(userAgent);
	    
	    
	    //////////HEADER COMPLETO
	    
	    sizeAL = 0;
	    
	    aL = (char*) malloc(sizeHL1 + sizeHL2 + sizeHL3);
	    memcpy(aL, hL1, sizeHL1);
	    memcpy(aL + sizeHL1, hL2, sizeHL2);
	    memcpy(aL + sizeHL1 + sizeHL2, hL3, sizeHL3);
	    sizeAL = sizeHL1 + sizeHL2 + sizeHL3;
	    
	    int sizeHeader = sizeAL;
	    char* header = (char*) malloc(sizeHeader);
	    memcpy(header, aL, sizeAL);
	    
	    free(aL);
	    free(hL1);
	    free(hL2);
	    free(hL3);
	    //free(recibido);

	    //cout << "Esto es lo que mando:\n" << header; cout.flush();

	    //aca tengo q conectarme con el servidor posta, recibir la pagina y reenviarsela al cliente
	    struct sockaddr_in server_original;
	    server_original.sin_family = AF_INET; //tipo de conexion
	    server_original.sin_port = htons(80); //puerto del servidor
	    bzero ( &(server_original.sin_zero), 8);

	    struct hostent* host = gethostbyname(hostName);
	    
	    //free(hostName);

	    //controlo que el servidor que busco existe
	    if (host!=NULL){
                  int socket_to_server = socket(AF_INET, SOCK_STREAM, 0);
                  if (socket_to_server == -1){
                    writeLog("ATENCION WEB: Error al crear socket con el servidor al que se le mandara el pedido.");
			//free(//host);
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
		  if (connect(socket_to_server, res->ai_addr, res->ai_addrlen) == -1){
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
		  //if (send(socket_to_server,header,sizeHeader,0) == -1){
		  if (send(socket_to_server,header,sizeHeader,MSG_NOSIGNAL) == -1){
		      writeLog("ATENCION WEB: Error al realizar send con el servidor al que se le solicita el pedido.");
		      if (close(socket_to_browser)<0){
			perror("ERROR cerrando socket:\n");			
		      }
		      if (close(socket_to_server)<0){
			perror("ERROR cerrando socket\n");			
		      }
		//	free(host);
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
			    if (sizeRcvServ == -1){
				      writeLog("ATENCION WEB: Error al realizar recv con el servidor al que se le solicita el pedido.");
				      if (close(socket_to_server)<0){
					perror("ERROR cerrando socket\n");			
				      }
				      if (close(socket_to_browser)<0){
					perror("ERROR cerrando socket:\n");			
				      }
					//free(host);
				      pthread_exit(NULL);
			    }
			//    cout << rcvServ; cout.flush();
			    if (sizeRcvServ == 0 ){
				      //cout << "Termino de recibir, no mando nada"; cout.flush();
				      cout<<"cerrando socket,termine recibir servidor"<<socket_to_server;cout.flush();
				      if (close(socket_to_server)<0){
					perror("ERROR cerrando socket\n");			
				      }
					cout<<"cerrado\n";cout.flush();
				      //exit(-1);
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
			int enviados = send(socket_to_browser,bufferRcv,sizeBufferRcv,MSG_NOSIGNAL);
			if( enviados== -1){
				      writeLog("ATENCION WEB: Error al realizar send al cliente web.");
				      //close(socket_to_server);
					cout<<"cerrando error al enviar al fire"<<socket_to_browser;cout.flush();
				      if (close(socket_to_browser)<0){
					perror("ERROR cerrando socket\n");			
				      }
				      cout<<"cerrado\n"<<cout.flush();
				      free(host);
				      pthread_exit(NULL);
			}	
			      //cout<<"recibidos: "<<sizeBufferRcv;cout.flush();
			      cout<<"enviados: "<<enviados;cout.flush();
			free(bufferRcv);
		  }
		  //cierro la conexion
cout<<"cerrando "<<socket_to_browser;cout.flush();
		  if (close (socket_to_browser) < 0){
			perror("ERROR cerrando socket:\n");			
		}
cout<<"cerrado, termine de enviar sin problemas\n"<<cout.flush();
		  free(recibido);
		  //free(host);

		  pthread_exit(NULL);

	    }else{//fin if  servidor es valido
		//cierro el socket con el firefox
		//termino ejecucion, esto se hace cuando lo pasemos al otro archivo...
		cout<<"cerrando por servidor invalio"<<socket_to_browser;cout.flush();
		if (close(socket_to_browser)<0){
			perror("ERROR cerrando socket\n");			
		}
		cout<<"cerrado\n"<<cout.flush();
		free(recibido);
		free(header);
                pthread_exit(NULL);
	    }

	}else{//fin if url valida
	    cout<<"url invalido"<cout.flush();
	    char * merror=strdup("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n<h1>404 bloqueado por redes26,url invalida</h1>\r\n</body>\r\n</html>");	    
	    int cant = send(socket_to_browser,merror,strlen(merror),MSG_NOSIGNAL);
	    if (cant ==-1){
		cout<<"fallo el send\n";cout.flush();
	    }
	    free(merror);
	    cout<<"cerrando por url invalida"<<socket_to_browser;cout.flush();
            if (close(socket_to_browser)<0){
		perror("ERROR cerrando socket\n");			
            }
	    cout<<"cerrado\n"<<cout.flush();
	    free(recibido);
	    free(hostName);
            pthread_exit(NULL);
	}//fin else ulr valida
      }else{//fin metodo valido
	  free(httpMethod);
	  free(recibido);
	  //crear mensaje de error, mandarlo y (cerrar la conexion)
		cout<<"metodo invalido"<cout.flush();
	  char * merror=strdup("HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n<h1>404 bloqueado por redes26,metodo invalido</h1>\r\n</body>\r\n</html>");	    
	  int cant = send(socket_to_browser,merror,strlen(merror),MSG_NOSIGNAL);
	  if (cant ==-1){
	      cout<<"fallo el send\n";cout.flush();
	  }
	  free(merror);
     	  cout<<"cerrando por metodo valido"<<socket_to_browser;cout.flush();
	  if (close(socket_to_browser) <0){
		perror("ERROR cerrando socket:\n");			
	  }
	  cout.flush();cout<<"cerrado\n"<<cout.flush();
	  pthread_exit(NULL);
      }//fin else metodoValido

	      cout<<"estop no tendria q aparecer nunca\n";cout.flush();
	cout<<"cerrando cualquier fruta"<<socket_to_browser;cout.flush();
	  if (close(socket_to_browser)<0){
		perror("ERROR cerrando socket\n");			
	  }
	cout<<"cerrado frutasss\n"<<cout.flush();
	  pthread_exit(NULL);
      
   
}

/*
 *
 */
int main() {

    //writeLog("Se inicia el servidor proxy.");
    signal(SIGINT, signal_callback_handler);
    pthread_t ad;
    int alpedo=2;
    int resultPC;
    if (resultPC=pthread_create(&ad,NULL,atencion_administrador,(void*)alpedo)==0)
	{
	writeLog("SERVIDOR PROXY: Thread del administrador creado con exito.");
	}
    else
	{
	writeLog("SERVIDOR PROXY: Hubo error al crear el hilo del administrador, el numero de error es: " +resultPC);
	        pthread_exit(NULL);
	}

    booldenyPOST=false;
    booldenyGET=false;

    cout<<"HOLA SOY EL SERVIDOR PROXY, VENGO A FLOTAR\n";
    cout.flush();
    //Creo un socket para la conexion con el cliente
    server_socket = socket(AF_INET, SOCK_STREAM, 0);



    if (server_socket == -1){
        writeLog("SERVIDOR PROXY: Hubo error al crear el socket para la conexion con el cliente del servidor proxy");
        pthread_exit(NULL);
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
        writeLog("SERVIDOR PROXY: Hubo error al realizar bind entre el socket y el descriptor, para la conexion con el cliente del servidor proxy");
        if (close(server_socket)<0){
		perror("ERROR cerrando socket:\n");				
	}
        pthread_exit(NULL);
    }

    //primitiva listen
    if (listen (server_socket, MAX_QUEUE) == -1){
        writeLog("SERVIDOR PROXY: Hubo error al realizar la escucha sobre el socket para la conexion con el cliente del servidor proxy.");
	cout<<"cerrando por listen main"<<server_socket;cout.flush();
        if (close(server_socket)<0){
		perror("ERROR cerrando socket:\n");			
	}
	cout<<"cerrado\n";cout.flush();
        pthread_exit(NULL);
    }

    while (true){
        // inicializo estructuras para primitiva ACCEPT

        //Contendra la direccion IP y numero de puerto del cliente
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int socket_to_client;

        //espero por conexion de clientes
        socket_to_client = accept(server_socket, (struct sockaddr*) & client_addr,& client_addr_size);
        if (socket_to_client == -1){
          writeLog("SERVIDOR PROXY: Hubo error al realizar accept con el socket de clientes.");
          pthread_exit(NULL);
        }

	pthread_t hijo;

	pthread_create(&hijo,NULL,atenderWeb,(void*)socket_to_client);

    }
	cout<<"cerrando hola"<<server_socket;cout.flush();
    	if (close (server_socket)<0){
		perror("ERROR cerrando socket:\n");			
	}
	cout<<"cerrado chau\n";cout.flush();
}

