#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define file_path_name "proxy.conf"
using namespace std;


char* getValueConf(char* valueName){
cout << "Lo que me llega " << valueName; cout.flush();
	FILE* fp;
	fp = fopen (file_path_name,"r");
	if (fp!=NULL){
		bool encontre	= false;		
		char* principioLinea;
		char c;		
		while (	c = getc()		

while (!feof(fp) && !encontre){
			principioLinea = (char*) malloc(strlen(valueName) + 1);
			memcpy(principioLinea, fp, strlen(valueName));
			memcpy(principioLinea + strlen(valueName), " ", 1);
//cout << "principioLinea: " << principioLinea; cout.flush();
			if (strcmp(valueName, principioLinea) == 1) {
				encontre = true;
			}
			free(principioLinea);
		}
		if (encontre) {
			if (strcmp(valueName, "hola") == 1) {
				char* ret = "3";			
				return ret;
			}
		}else{
			cout << "No se econtro configuracion para " << valueName; cout.flush();
			// return -1;
		}
	}else{
		cout << "No se econtro el archivo " << file_path_name; cout.flush();
	}
}

int main() {
		char* hola = "hola";
		char* tres = getValueConf(hola);
		cout << "Tres: " << tres; cout.flush();
}
