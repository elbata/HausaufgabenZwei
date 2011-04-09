/* 
 * File:   main.cpp
 * Author: redes26
 *
 * Created on April 7, 2011, 3:46 PM
 */

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

#define miPuerto 61000
#define MAX_QUEUE 30
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

//--------------Semaforos--------------------//
pthread_mutex_t mutexListDW = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListDUW = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexdenyPOST = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexdenyGET = PTHREAD_MUTEX_INITIALIZER;

int server_socket;

//funcion que comprueba si el m etodo es valido
bool comprobarMetodo (string m){
  //retorna true si el metodo es valido
  bool retorno = false;
  if (m.find("GET",0) != -1) {
      //verifico estado del booleano
      pthread_mutex_lock( &mutexdenyGET );      
      if (booldenyGET)
	  retorno = false;
      else
	  retorno = true;      
      pthread_mutex_unlock( &mutexdenyGET );
      
  }else   {
      if (m.find("POST",0) != -1) {
	//verifico estado del booleano
	pthread_mutex_lock( &mutexdenyPOST );      
	if (booldenyPOST)
	  retorno = false;
	else
	  retorno = true;
	pthread_mutex_unlock( &mutexdenyPOST );
      }
      else{
	if (m.find("HEAD",0) != -1) {
	  retorno = true;
	}
	else
	  retorno =false;
      }    
  }
  
  return retorno;
}


//funcion que comprueba si el url es valido
bool comprobarURL (string url){
  //devuelve true si la url  es valida
  bool encontrePalabra = false;
  list<string>::iterator iter;
  pthread_mutex_lock( &mutexListDUW );      
  for(iter=listaDUW.begin(); ((iter!=listaDUW.end()) && (encontrePalabra == false)); iter++){
	  if (url.find((*iter),0) !=-1){
	      encontrePalabra = true;
	  }	  
  }
  pthread_mutex_unlock( &mutexListDUW );      
  return (!encontrePalabra);
}


// Identificador del socket de conexión abierto por el servidor.
void * atender_varios_admins(void * param){
  
  	int socket_con_administrador = (*(int*)param);

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
	    mensajeAdmin.erase(0,strlen(mensajeAdmin.c_str()));
	    mensajeAdmin="\0";
	    
	    while (!terminoMensaje){
		recibidoDelAdmin = (char *) malloc(MAX_BUFF_MSG_ADM);
		
		//recibo un pedazo del mensaje
		sizeRecibidoAdmin = recv(socket_con_administrador, recibidoDelAdmin, datos_admin_size, 0);
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
		    mensajeADevolver = "Notificado el listDUW\r\n\0";
		    pthread_mutex_lock( &mutexListDUW );
		    mensajeADevolver+= printList(listaDUW);
		    pthread_mutex_unlock( &mutexListDUW );
		    break;
	      case 4: 
		    mensajeADevolver = "Notificado el listDW\r\n\0";
		    pthread_mutex_lock( &mutexListDW );
		    mensajeADevolver+= printList(listaDW);
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
	    char * mens = (char *)mensajeADevolver.c_str();
	    tamanioMensajeADevolver = strlen(mens);
	    //Mando lo mismo que me llego para probar un poco si funcionan los comandos
	    send(socket_con_administrador,mens,tamanioMensajeADevolver,0);
	    
	}

	close(socket_con_administrador);
  return NULL;
}

void * atencion_administrador(void *) {
    
    //todo esto tendria que estar en una funcion 
  
    cout<<"Esta es la consola del administrador\n";
    cout.flush();
    
    //Creo un socket para la conexion con el administrador
    int socket_administrador = socket(AF_INET, SOCK_STREAM, 0);


    //contendra la direccion IP y el numero de puerto local
    struct sockaddr_in proxy_addr;
    socklen_t proxy_addr_size = sizeof proxy_addr;
    proxy_addr.sin_family = AF_INET; //tipo de conexion
    proxy_addr.sin_port = htons(puertoAdministrador); //puerto al que me voy a conectar
    proxy_addr.sin_addr.s_addr = inet_addr(IPProxy); //direccion IP proxy
    
    //primitiva BIND
    //socket, puntero a sockaddr_in, tamaño de sockaddr_in
    bind (socket_administrador, (struct sockaddr*) & proxy_addr, proxy_addr_size);

    //primitiva listen
    listen (socket_administrador, MAX_QUEUE);

    

    while (true){
      
	int socket_con_administrador;
	struct sockaddr_in admin_addr;
        socklen_t admin_addr_size = sizeof admin_addr;

	
        //espero por conexion de clientes
        socket_con_administrador = accept(socket_administrador, (struct sockaddr*) & admin_addr,& admin_addr_size);
	pthread_t hijo_admin;
	pthread_create(&hijo_admin,NULL,atender_varios_admins,(void*)&socket_con_administrador);
	
	
    }

    close (socket_administrador);
    return NULL;
}

void * prueba(void* parametro){
	int socket_aux = (*(int*)parametro);
  	int sizeRecibido = MAX_BUFF_MSG;
	string buffer="";
	int datos_size = MAX_BUFF_MSG;
        // Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
	char * recibido;
	bool finCabezal = false;
	while (!finCabezal){
            recibido = (char *) malloc(MAX_BUFF_MSG);
	    //recibo un pedazo del mensaje
            sizeRecibido = recv(socket_aux,  recibido, datos_size, 0);
	    string aux1 (recibido);
	    //me fijo si termino el cabezal http, termina con doble salto de linea
	      buffer = buffer + char_string(recibido).substr(0,sizeRecibido);
	    size_t encontre = buffer.find("\r\n\r\n",0);
	    if (encontre !=-1) 
	      finCabezal = true;
	    //buffer+= aux1;
	  
            // libero memoria de recibido
            free(recibido);
        }
        cout<< "Soy el servidor proxy, me mandaste esta boludez:\n" << buffer;cout.flush();

        int comienzo = 0;
	string metodoHTTP="";
	string url="";
        int inicio = buffer.find("HTTP/1.1");
	int finMetodo;
	int finUrl;
	bool metvalido = true;
        if(inicio>0){
	  buffer.replace(inicio,8,"HTTP/1.0" );
	  int finMetodo = buffer.find("http",comienzo);
	  //cout<<"Fin metodo: "<<finMetodo<<"\n";cout.flush();
	  metodoHTTP = buffer.substr(0,finMetodo-1);
	  string hola = buffer.substr(0,12);cout<<hola;cout.flush();
	  if (comprobarMetodo(hola) ) {
	    
	    //cout<<"Metodo HTTP: "<<metodoHTTP<<"\n";cout.flush();
	    int posBarra = buffer.find("/",finMetodo+7);
	 //   cout<<"Posbarra: "<<posBarra<<"\n";cout.flush();
	    finUrl= buffer.find(" ",posBarra);
	    url=buffer.substr(finMetodo+7,finUrl-(finMetodo+7));
	   // cout<<"url: "<<url<<"\n";cout.flush();
	    buffer.erase(finMetodo,posBarra - finMetodo);
	    metvalido = true;
	  }
	  else {
	    cout<<"metodo invalido";cout.flush();
	    metvalido = false;
	  }
	}else{
	  finMetodo=buffer.find("/",comienzo);
	  //cout<<"Fin metodo: "<<finMetodo<<"\n";cout.flush();
	  finUrl= buffer.find(" ",finMetodo+1);
	  url=buffer.substr(finMetodo+1,finUrl-(finMetodo+1));
	  //cout<<"url: "<<url<<"\n";cout.flush();
	  metodoHTTP=buffer.substr(0,finMetodo-1);
	}
	
	if(metvalido && comprobarURL(url) ){
		
		//obtengo el nombre del host para realizar una busqueda dns del IP
		int posHost;
		posHost = buffer.find("Host: ",comienzo);
		int posFinHost = buffer.find("\r\n",posHost);
		string nombreHost;
		nombreHost = buffer.substr(posHost+6,(posFinHost)- (posHost+6));

		//aca elimino el user-agent del encabezado
		int posFinUserAgent= buffer.find("\r\n",posFinHost+1);
		buffer.erase(posFinUserAgent,buffer.length()-posFinUserAgent);
		
		buffer+="\r\n\r\n";
		
		//cout<<buffer;cout.flush();
		//aca tengo q conectarme con el servidor posta, recibir la pagina y reenviarsela al cliente
		struct sockaddr_in server_original;
		server_original.sin_family = AF_INET; //tipo de conexion
		server_original.sin_port = htons(80); //puerto del servidor

		int socknuevo = socket(AF_INET, SOCK_STREAM, 0);

		char * h = &nombreHost[0];
		struct hostent * host = gethostbyname (h);
		if (host != NULL){
		  int ** prueba = (int **)host->h_addr_list;
		  server_original.sin_addr.s_addr = **prueba; //direccion del servidor

		  //modifico el mensaje para q sea del tipo HTTP 1.0
		  int posUserAgent;
		  int posFinUser;

		  //mando el mensaje nuevo
		  char * mensaje = &buffer[0];
		  connect(socknuevo, (struct sockaddr *)&server_original,sizeof(struct sockaddr));
		  send(socknuevo,mensaje,strlen(mensaje),0);


		  // Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
		  // Cuando el "sizeRecibido" es menor que el máximo el mensaje se ha
		  // completado.
		  buffer="";
		  char * recibido2;
		  sizeRecibido = MAX_BUFF_MSG;
		  while (sizeRecibido !=0){
		      recibido2 = (char *) malloc(MAX_BUFF_MSG);
		      sizeRecibido = recv(socknuevo, recibido2, datos_size, 0);
		      //envio mensaje al cliente
		    // cout<<recibido2;cout.flush();
		      send(socket_aux,recibido2,sizeRecibido,0);
		      // libero memoria de recibido
		      free(recibido2);
		  }
		}

		close(socknuevo);

	}//fin de conexion con el servidor host, siendo metodo y url validos
	else{
	  //aca hay que mandar el mensaje de error correspondiente...
	  //los metemos en un archivo nuevo???
	  //ver posibles mensajes de error...
	  cout<<"entre al else\n";cout.flush();
	  string merror="HTTP/1.1 404 Not Found\r\n\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n  <h1>404 bloqueado por redes26</h1>\r\n</body>\r\n</html>\r\n\r\n";
	  char * mensajeError = (char*)merror.c_str();
	  int cant = send(socket_aux,mensajeError,strlen(mensajeError),0);
	  if (cant ==-1)
	      cout<<"fallo el send\n";cout.flush();

	}
	
        //cierro la conexion
        close (socket_aux);
	return NULL;
}

/*
 *
 */
int main() {
    
    pthread_t ad;
    int alpedo=2;
    
    if (pthread_create(&ad,NULL,atencion_administrador,(void*)alpedo)!=0)
    {
      cout<<"Error, no se creo la atencion a administrador";cout.flush();
    }
  
    booldenyPOST=false;
    booldenyGET=false;
    
    cout<<"HOLA SOY EL SERVIDOR PROXY, VENGO A FLOTAR\n";
    cout.flush();
    //Creo un socket para la conexion con el cliente
    server_socket = socket(AF_INET, SOCK_STREAM, 0);


    //contendra la direccion IP y el numero de puerto local
    struct sockaddr_in server_addr;
    socklen_t server_addr_size = sizeof server_addr;
    server_addr.sin_family = AF_INET; //tipo de conexion
    server_addr.sin_port = htons(miPuerto); //puerto por donde voy a atender
    server_addr.sin_addr.s_addr = inet_addr(miIP); //direccion IP mia

    //primitiva BIND
    //socket, puntero a sockaddr_in, tamaño de sockaddr_in
    if (bind (server_socket, (struct sockaddr*) & server_addr, server_addr_size)!=0)
    {
      cout<<"Error, no se linkeo el socket al descriptor";cout.flush();
    }

    //primitiva listen
    if (listen (server_socket, MAX_QUEUE)!=0)
    {
      cout<<"Error, no se pudo setear el listen sobre el socket";cout.flush();
    }



    while (true){
        //Contendra la direccion IP y numero de puerto del cliente
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int socket_to_client;
	
        //espero por conexion de clientes
        socket_to_client = accept(server_socket, (struct sockaddr*) & client_addr,& client_addr_size);
	if (socket_to_client!=0)
	{
	  cout<<"Error,no se pudo realizar la espera de conexiones de clientes";cout.flush();
	}
	
	pthread_t hijo;
	pthread_create(&hijo,NULL,prueba,(void*)&socket_to_client);

    }

    close (server_socket);
}

