// basic file operations
#include <iostream>
#include <time.h>
#include <stdio.h>

#define file_path_name "./ejemplo.txt"
using namespace std;


bool writeLog (string message)
{
	FILE * file;
  	file = fopen (file_path_name,"a");
  	if (file!=NULL)
  	{
		time_t t;
		time(&t);
		fputs("-------------------------------------------------------------\n",file);
		fputs(ctime(&t),file);
		fputs(message.c_str(),file);
		fputs("\n",file);
		fputs("-------------------------------------------------------------\n",file);
		fclose(file);
		return 0;
  	}else{
		cout<<"No se pudo abrir el archivo log.\n";cout.flush();
		return -1;
	}
}
