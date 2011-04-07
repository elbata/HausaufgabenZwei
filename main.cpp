/* 
 * File:   main.cpp
 * Author: fabian
 *
 * Created on 2 de abril de 2011, 01:25 PM
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

#define miPuerto 61000
#define MAX_QUEUE 30
#define MAX_BUFF_MSG 1024
#define miIP  "127.0.0.1"

using namespace std;

// Identificador del socket de conexión abierto por el servidor.
int server_socket;

// Funcion que convierte un char* a un String
string char_string( char* c ) {
	string s( c );
	return s;
}


/*
 * 
 */
int main() {
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
    bind (server_socket, (struct sockaddr*) & server_addr, server_addr_size);
    
    //primitiva listen
    listen (server_socket, MAX_QUEUE);



    while (true){


        // inicializo estructuras para primitiva ACCEPT

        //Contendra la direccion IP y numero de puerto del cliente
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int socket_to_client;

        int sizeRecibido = MAX_BUFF_MSG;
        char* recibido;
        string buffer = "";
        string temp;
        int datos_size = MAX_BUFF_MSG;
        buffer = "";

        //espero por conexion de clientes
        socket_to_client = accept(server_socket, (struct sockaddr*) & client_addr,& client_addr_size);

	sizeRecibido = MAX_BUFF_MSG;

        // Recibo el mensaje de a partes. El mensaje se va guardando en "buffer"
        // Cuando el "sizeRecibido" es menor que el máximo el mensaje se ha
        // completado.

        while (sizeRecibido >= MAX_BUFF_MSG){
            recibido = (char *) malloc(MAX_BUFF_MSG);
            sizeRecibido = recv(socket_to_client, recibido, datos_size, 0);
            buffer = buffer + char_string(recibido).substr(0,sizeRecibido);

            // libero memoria de recibido
            free(recibido);
        }
        //cout<< "Soy el servidor proxy, me mandaste esta boludez:\n" << buffer;

        int comienzo = 0;
        int inicio = buffer.find("HTTP/1.1");
        buffer.replace(inicio,9,"HTTP/1.0" );
        int inicio2 = buffer.find("GET http://");
        buffer.replace(inicio2,11,"GET " );
        //obtengo el nombre del host para realizar una busqueda dns del IP
        int posHost;
        posHost = buffer.find("Host: ",comienzo);
        int posFinHost = buffer.find("\r\n",posHost);
        string nombreHost;
        nombreHost = buffer.substr(posHost+6,(posFinHost)- (posHost+6));
      //  cout<<"buffer: "<<buffer;cout.flush();
       // cout<<"\nposhost: "<<posHost;cout.flush();
        //cout<<"\nposfinhost: "<<posFinHost;cout.flush();
        //cout<<"\nel nombre del host es: \n"<<nombreHost;cout.flush();


        //aca tengo q conectarme con el servidor posta, recibir la pagina y reenviarsela al cliente
        struct sockaddr_in server_original;
        server_original.sin_family = AF_INET; //tipo de conexion
        server_original.sin_port = htons(80); //puerto del servidor

        int socknuevo = socket(AF_INET, SOCK_STREAM, 0);

        char * h = &nombreHost[0];
        struct hostent * host = gethostbyname (h);
        int ** prueba = (int **)host->h_addr_list;
        server_original.sin_addr.s_addr = **prueba; //direccion del servidor

        //modifico el mensaje para q sea del tipo HTTP 1.0
        int posUserAgent;
        int posFinUser;
        posUserAgent = buffer.find("User-Agent:",comienzo);
        posFinUser = buffer.find("\r\n");
        int fin = buffer.length();
        buffer.erase(posFinUser,fin);
        buffer= buffer +"\n\n";

        //mando el mensaje nuevo
        char * mensaje = &buffer[0];
        cout<<mensaje;cout.flush();
       // int inicio = buffer.find("HTTP/1.1");
     //   buffer.replace(inicio,9,"HTTP/1.0" );
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
            
            buffer = buffer + char_string(recibido2).substr(0,sizeRecibido);

            // libero memoria de recibido
            free(recibido2);
        }


        close(socknuevo);
        cout<<buffer;cout.flush();
        const char *message;
        message = buffer.c_str() ;

       send(socket_to_client,message,strlen(message),0);
        
        //cierro la conexion
        close (socket_to_client);
    }

    close (server_socket);
}

