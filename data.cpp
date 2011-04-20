// basic file operations
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define file_path_name "./values.config"
using namespace std;


bool setServerIp (char* ip)
{
	int server_port;
	char* admin_ip=(char*) malloc(16);
	int admin_port;
	FILE * file;
  	file = fopen (file_path_name,"r");
  	if (file!=NULL)
  	{
		fscanf(file,"%*s %*s %*s %d %*s %s %*s %d",&server_port,admin_ip,&admin_port);
		fclose(file);		
		file= fopen(file_path_name,"w");
		fprintf (file, "SERVER_IP\t%s\nSERVER_PORT\t%d\nADMIN_IP\t%s\nADMIN_PORT\t%d\n",ip,server_port,admin_ip,admin_port);
		fclose(file);
		free(admin_ip);
		return 0;
  	}else{
		cout<<"No se pudo abrir el archivo config.\n";cout.flush();
		return -1;
	}
}

bool setServerPort (char* port)
{
	FILE * file;
  	file = fopen (file_path_name,"r");
	char* server_ip=(char*) malloc(16);
	char* admin_ip=(char*) malloc(16);
	int admin_port;
  	if (file!=NULL)
  	{
		fscanf(file,"%*s %s %*s %*d %*s %s %*s %d",server_ip,admin_ip,&admin_port);
		fclose(file);		
		file= fopen(file_path_name,"w");
		fprintf (file, "SERVER_IP\t%s\nSERVER_PORT\t%s\nADMIN_IP\t%s\nADMIN_PORT\t%d\n",server_ip,port,admin_ip,admin_port);
		fclose(file);
		free(admin_ip);
		free(server_ip);
		return 0;
  	}else{
		cout<<"No se pudo abrir el archivo config.\n";cout.flush();
		return -1;
	}
}

int getServerPort()
{
	FILE * file;
  	file = fopen (file_path_name,"r");
	int server_port;
	if (file!=NULL)
  	{
		fscanf(file,"%*s %*s %*s %d %*s %*s %*s %*d",&server_port);
		fclose(file);
	}
return server_port;
}

char* getServerIp()
{
	FILE * file;
  	file = fopen (file_path_name,"r");
	char* server_ip=(char*) malloc(16);
	if (file!=NULL)
  	{
		fscanf(file,"%*s %s %*s %*d %*s %*s %*s %*d",server_ip);
		fclose(file);
	}
return server_ip;
}

int getAdminPort()
{
	FILE * file;
  	file = fopen (file_path_name,"r");
	int admin_port;
	if (file!=NULL)
  	{
		fscanf(file,"%*s %*s %*s %*d %*s %*s %*s %d",&admin_port);
		fclose(file);
	}
return admin_port;
}

char* getAdminIp()
{
	FILE * file;
  	file = fopen (file_path_name,"r");
	char* admin_ip=(char*) malloc(16);
	if (file!=NULL)
  	{
		fscanf(file,"%*s %*s %*s %*d %*s %s %*s %*d",admin_ip);
		fclose(file);
	}
return admin_ip;
}
