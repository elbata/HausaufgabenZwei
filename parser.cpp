#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

  bool http1Valid(char* entrada){
    char* result= strstr (entrada,"HTTP/1.1");
    if (result!=NULL){
      return true;
    }else{
	return false;
    }
  }

  char* getHttpMethod(char* httpCode){
    char* auxHttpMethod = (char*) malloc(1024);
    int httpResult = sscanf(httpCode,"%s %*s %*s",auxHttpMethod);
    char* httpMethod = (char*) malloc(strlen(auxHttpMethod) + 1);
    memcpy(httpMethod, auxHttpMethod, strlen(auxHttpMethod)); 
    memcpy(httpMethod + strlen(auxHttpMethod), "\0", 1); 
    free(auxHttpMethod);
    return httpMethod;
  }
  
  char* getUrl(char* httpCode){
      
      
      int inicio;
      int fin;
      string s(httpCode);
      inicio = s.find(" ");
      fin = s.find(" HTTP/");
      
      char* url;
      char* auxUrl = (char*) malloc(fin - inicio);
      char* auxUrlBorrar = auxUrl;
      int httpResult = sscanf(httpCode,"%*s %s %*s",auxUrl);
      char* tieneHttp = strstr(auxUrl,"http");
      if (tieneHttp != NULL){
	auxUrl = strstr (auxUrl,"/");
	auxUrl ++;
	auxUrl = strstr (auxUrl,"/");
	auxUrl ++;
	auxUrl = strstr (auxUrl,"/");
      }
      url = (char*) malloc(strlen(auxUrl) + 1);
      memcpy(url, auxUrl, strlen(auxUrl)); 
      memcpy(url + strlen(auxUrl), "\0", 1); 
      free(auxUrlBorrar);
      return url;
  }
        
  char* getHostName(char* httpCode){

    int inicio;
    int fin;
    string s(httpCode);
    inicio = s.find("Host:");
    inicio = s.find(" ", inicio);
    fin = s.find("\r\n", inicio);
	  
	  
    char* auxHostName = (char*) malloc(fin - inicio);
    char* hostName;
    char* tieneHost = strstr(httpCode,"Host:");
    if(tieneHost != NULL)
    { 
      sscanf(tieneHost,"Host: %s",auxHostName);
      hostName = (char*) malloc(strlen(auxHostName) + 1);
      memcpy(hostName, auxHostName, strlen(auxHostName)); 
      memcpy(hostName + strlen(auxHostName), "\0", 1); 
    }
    free(auxHostName);
    return hostName;
  }
  
  char* getUserAgent(char* httpCode){
    char* auxUserAgent;
    char* iter = strstr (httpCode, "\r\n");
    iter = iter + 1;
    iter = strstr (iter, "\r\n");
    iter = iter + 1;
    auxUserAgent = strtok (iter, "\r\n");
    char* userAgent = (char*) malloc(strlen(auxUserAgent) + 1);
    memcpy(userAgent, auxUserAgent, strlen(auxUserAgent)); 
    memcpy(userAgent + strlen(auxUserAgent), "\0", 1);
    return userAgent;
  }
  
  char* getHeadersRestantes(char* httpCode, int &size){

    string headers[14];
    headers[0] = "Allow";
    headers[1] = "Authorization";
    headers[2] = "Content-Legth";
    headers[3] = "Content-Type";
    headers[4] = "Date";
    headers[5] = "Expires";
    headers[6] = "From";
    headers[7] = "If-Modified-Since";
    headers[8] = "Last-Modified";
    headers[9] = "Location";
    headers[10] = "Pragma";
    headers[11] = "Referer";
    headers[12] = "Server";
    headers[13] = "WWW-Authenticate";
    
    string s(httpCode);
    
    
    printf("ADENTRO ME LLEGA ESTOOOOOO STRINGGGGG: %s",s.c_str());
    printf("ADENTRO ME LLEGA ESTOOOOOO CHAR*****: %s",httpCode);
    
    char* headersRestantes = NULL;
    int sizeHeadersRestantes = 0;
    char* auxHeadersRestantes;
    int sizeAuxHeadersRestantes = 0;
    int inicio;
    int fin;
    int i;

    for(i = 0; i <= 13; i++) {

      inicio = s.find(headers[i]);
      if (inicio != -1) {
	printf("%s",(headers[i]).c_str());
        fin = s.find("\r\n", inicio);
	cout << "Inicio: " << inicio << "\n"; cout.flush();
        cout << "Fin: " << fin << "\n" ; cout.flush();
        auxHeadersRestantes = (char*) malloc(sizeHeadersRestantes + (fin - inicio) + 2);
	cout << "1"; cout.flush();
        memcpy(auxHeadersRestantes, headersRestantes, sizeHeadersRestantes); 
	cout << "2"; cout.flush();
	memcpy(auxHeadersRestantes + sizeHeadersRestantes, httpCode + inicio, (fin - inicio));
	cout << "3"; cout.flush();
	memcpy(auxHeadersRestantes + sizeHeadersRestantes + (fin - inicio), "\r\n", 2);
	cout << "4"; cout.flush();
	sizeAuxHeadersRestantes = sizeHeadersRestantes + (fin - inicio) + 2;
	cout << "5"; cout.flush();
	
	if (headersRestantes != NULL) { free(headersRestantes); }
	cout << "6"; cout.flush();
        headersRestantes = (char*) malloc(sizeAuxHeadersRestantes);
	cout << "7"; cout.flush();
        memcpy(headersRestantes, auxHeadersRestantes, sizeAuxHeadersRestantes);
	cout << "8"; cout.flush();
        sizeHeadersRestantes = sizeAuxHeadersRestantes;
	cout << "9"; cout.flush();
	 
        free(auxHeadersRestantes);
      }else{
	      cout << "Inicio MENOS UNO: " << inicio << "\n"; cout.flush();
      }
    }
    size = sizeHeadersRestantes;
    return headersRestantes;
  }

//GET http://www.fing.edu.uy/system/application/images/pie_inf.jpg HTTP/1.1
//Host: www.fing.edu.uy
//User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.14) Gecko/20080404 Firefox/2.0.0.14