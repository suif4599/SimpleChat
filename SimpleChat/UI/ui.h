#ifndef __UI_H__
#define __UI_H__

#include <stdio.h>
#include "../../ERROR/error.h"
#include "../../config.h"

char *readLine(int n);
#ifdef _WIN32
int InitializeUserInterface(); // Initialize the user interface, return 0 if success, otherwise return -1
int moveUpFirst();
#endif



#endif