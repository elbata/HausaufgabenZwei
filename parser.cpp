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
    char* auxHttpMethod= (char*) malloc(1024);
    int httpResult= sscanf(httpCode,"%s %*s %*s",auxHttpMethod);
    char* httpMethod = (char*) malloc(strlen(auxHttpMethod) + 1);
    memcpy(httpMethod, auxHttpMethod, strlen(auxHttpMethod)); 
    memcpy(httpMethod + strlen(auxHttpMethod), "\0", 1); 
    free(auxHttpMethod);
    return httpMethod;
  }
  
  char* getUrl(char* httpCode){
      
      char* url;
      char* auxUrl = (char*) malloc(1024);
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
    char* auxHostName = (char*) malloc(1024);
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
  