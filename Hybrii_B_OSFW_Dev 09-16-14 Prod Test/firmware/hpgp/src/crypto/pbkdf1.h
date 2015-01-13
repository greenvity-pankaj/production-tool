#ifndef _PBKDF1_H
#define _PBKDF1_H

#include "papdef.h"

eStatus pbkdf1(const unsigned char *pwd, size_t pwdLen, 
               const unsigned char *salt, size_t saltLen,
               unsigned int count, unsigned char * key, size_t keyLen);

#endif
