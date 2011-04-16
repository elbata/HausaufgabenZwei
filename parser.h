#ifndef PARSER_H
#define PARSER_H

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

bool http1Valid(char*);
char* getHttpMethod(char*);
char* getUrl(char*);
char* getHostName(char*);
char* getUserAgent(char*);
#endif
