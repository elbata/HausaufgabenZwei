#ifndef DATA_H
#define DATA_H

#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

using namespace std;

bool setServerIp(char*);
bool setServerPort(char*);
char* getServerIp();
int getServerPort();
char* getAdminIp();
int getAdminPort();
#endif
