#include "operaciones.h"

//Mensajes posibles del administrador
#define addDUW "addDUW"
#define addDW "addDW"
#define listDUW "listDUW"
#define listDW "listDW"
#define deleteDUW "deleteDUW"
#define deleteDW "deleteDW"
#define denyPOST "denyPOST"
#define allowPOST "allowPOST"
#define denyGET "denyGET"
#define allowGET "allowGET"
#define quit "quit"

using namespace std;

list<string>::iterator iter;

// Funcion que retorna todos los elementos de una lista de strings, en un string, separado cada elemento por \n
string printList(list<string> l) {
	string result="";
 	for(iter=l.begin(); iter!=l.end(); iter++){
 		result+=*iter + "\n";
	}
	return result;
}

// Funcion que convierte un char* a un String
string char_string( char* c ) {
	string s( c );
	return s;
}

//devuelve el argumento de la linea de entrada
string obtenerArgumento(string linea){
  size_t posArg=0;
  posArg= linea.find(" ") + 1;
  string argumento= linea.substr(posArg,linea.length()-posArg);
  size_t posFin= argumento.find("\r\n");
  if (posFin>0)
  {
    argumento= argumento.substr(0,posFin);
	argumento+="\0";
  }
  return argumento;
}

//Funcion auxiliar para determinar la naturaleza del mensaje del administrador
int procesarTipoMensaje(string entrada){
  
  string comandos[11];
  comandos[0] = addDUW;
  comandos[1] = addDW;
  comandos[2] = listDUW;
  comandos[3] = listDW;
  comandos[4] = deleteDUW;
  comandos[5] = deleteDW;
  comandos[6] = denyPOST;
  comandos[7] = allowPOST;
  comandos[8] = denyGET;
  comandos[9] = allowGET;
  comandos[10] = quit;
  
  int i;
  for (i = 0; i <= 11; i++){
    if (i != 11) {
      if (entrada.find(comandos[i],0) !=-1)
	return i+1;
    }else{
      return i + 1;
    }
  }
} 
