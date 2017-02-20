#include <string.h>
#include "sha2.h"
#include "papdef.h"
#include <stdlib.h>
#include "pbkdf1.h"

eStatus pbkdf1(const unsigned char *pwd, size_t pwdLen, const unsigned char *salt, size_t saltLen,
               unsigned int count, unsigned char * key, size_t keyLen)
{
  unsigned char digest[SHA256_HASH_BYTES] = {0};

  int i;
  unsigned char *tmpStr;
  size_t tmpStrLen = saltLen + pwdLen;
  
  //Step 1: If dkLen > Hash output length, return with error
  if (keyLen > SHA256_HASH_BYTES)
  {
    return STATUS_FAILURE;  
  }
  
  //Create string = password + salt
  tmpStr = malloc(tmpStrLen);
  if (tmpStr == NULL)
  {
    return STATUS_FAILURE;
  }
  
  memcpy(tmpStr, pwd, pwdLen);
  memcpy(tmpStr + pwdLen, salt, saltLen);
  
  //Step 2: Calculate hash of Pwd + Salt
  sha256(digest, tmpStr, tmpStrLen * 8);

  free(tmpStr);
  
  //Step 3: Calculate hash of previous hash repeatedly upto c times
  for(i = 0; i < count-1; i++)
  {
    sha256(digest, digest, SHA256_HASH_BITS);
  }
  
  memcpy(key, digest, keyLen);
    
  return STATUS_SUCCESS;
}
