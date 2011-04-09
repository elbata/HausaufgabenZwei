#ifndef OPERACIONES_H
#define OPERACIONES_H

#include <cstdlib>
#include <stdlib.h>
#include <map>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <list>
#include "operaciones.h"



using namespace std;

#define pagina_error ="HTTP/1.1 404 Not Found\r\n<html>\r\n<head>\r\n</head>\r\n<body>\r\n  <h1>404 Pagina no encontrada</h1>\r\n</body>\r\n</html>\r\n\r\n"



string printList(list<string>);
string char_string( char*);
string obtenerArgumento(string);
int procesarTipoMensaje(string entrada);

#endif
