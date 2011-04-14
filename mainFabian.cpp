
#include <cstdlib>

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

#define miPuerto 61000
#define MAX_BUFF_MSG 1024*1024
#define miIP  "127.0.0.1"

#define puertoAdministrador 62000
#define MAX_QUEUE 30
#define MAX_BUFF_MSG_ADM 100
#define IPProxy  "127.0.0.1"

//----------Estructura compartida------------//
bool booldenyPOST;
bool booldenyGET;

using namespace std;

int server_socket;

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
      cout << "Soy el servidor proxy, me mandaste esta boludez:\n" << buffer; cout.flush();
      buffer=recibido;
      int comienzo = 0;
      string metodoHTTP="";
      string url="";
      bool isHttp1= true;//http1Valid(recibido);
      int finMetodo;
      int finUrl;
                      
      //Primer paso comprobarMetodo, no importa la url ni nada, en caso de error hay q cerrar la conexion y terminar la ejecucion (terminar la ejecucion solo cuando lo pasemosal otro archivo)
      //Segundo, si el metodo es valido se comprueba la url, si no es valida mandar mensaje de error al firefox, cerrar la conexoin y terminar la ejecucion (idem a lo de arriba con lo de terminar la ejecucion)
      //Tercero,si todo lo anterior esta bien se ejecuta el codigo q esta abajo, hay q hacer cambios, por ejemplo los controles del metodo y  url hacerlos los pasos anteriores y no aca, lo q si hay q hacer
      //es extraer bien el host y las partes para enviar el mensaje http al servidor.
      
      bool metodoValido = true; //llamar funcion verificarMetodo despues de obtenerlo
      //aca hacer el if de si es valido
      if (metodoValido){
	
	bool urlValida = true;//llamar a funcion verificarUrl despues de obtenerla
	if (urlValida){
	  if(isHttp1){//esto no se bien q es....
	    char* httpCode1= strtok(recibido,"HTTP/1.1");
	    cout<<"HTTPCODE!"<<httpCode1;cout.flush();
	    int inicio = buffer.find("HTTP/1.1");
	    buffer.replace(inicio,8,"HTTP/1.0" );
	    int finMetodo = buffer.find("http://",comienzo);
	    cout << "Fin Metodo: " << finMetodo << "\n"; cout.flush();
	    metodoHTTP = buffer.substr(0,finMetodo-1);
	    cout << "Metodo HTTP: " << metodoHTTP << "\n"; cout.flush();
	    int posBarra = buffer.find("/",finMetodo+7);
	    cout << "PosBarra: " << posBarra << "\n"; cout.flush();
	    finUrl = buffer.find(" ",posBarra);
	    url = buffer.substr(finMetodo+7,finUrl-(finMetodo+7));
	    cout << "URL: " << url << "\n"; cout.flush();
	    buffer.erase(finMetodo,posBarra - finMetodo);
	  }/*else{
	    finMetodo = buffer.find("/",comienzo);
	    cout << "Fin metodo: " << finMetodo << "\n"; cout.flush();
	    finUrl = buffer.find(" ",finMetodo+1);
	    url = buffer.substr(finMetodo+1,finUrl-(finMetodo+1));
	    cout << "url: " << url << "\n"; cout.flush();
	    metodoHTTP = buffer.substr(0,finMetodo-1);
	  }*/

	  //if(comprobarMetodo(metodoHTTP) && comprobarURL(url)){

	    //obtengo el nombre del host para realizar una busqueda DNS del IP
	    int posHost = buffer.find("Host: ",comienzo);
	    int posFinHost = buffer.find("\r\n",posHost);
	    string nombreHost;
	    nombreHost = buffer.substr(posHost+6,(posFinHost)-(posHost+6));

	    //aca elimino el user-agent del encabezado
	    int posFinUserAgent = buffer.find("\r\n",posFinHost+1);
	    buffer.erase(posFinUserAgent,buffer.length()-posFinUserAgent);

	    buffer += "\r\n\r\n";

	    cout << "Esto es lo que mando:\n" << buffer; cout.flush();
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
	    char* h = &nombreHost[0];
	    struct hostent* host = gethostbyname(h);
	    
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
		  
		  
	    }else{//fin if  servidor es valido
		//cierro el socket con el firefox
		//termino ejecucion, esto se hace cuando lo pasemos al otro archivo...
		close(socket_to_client);
	    }
	    
	}else{//fin if url valida
	    //lo de siempre mandar
	  
	}//fin else ulr valida
	  
    }else{//fin metodo valido
	//crear mensaje de error, mandarlo y (cerrar la conexion)
      
    }//fin else metodoValido
    
    }//fin while true
	close (server_socket);
}

