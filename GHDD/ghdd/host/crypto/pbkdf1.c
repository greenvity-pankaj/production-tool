
#ifdef MODULE
#include <linux/module.h>
#include <linux/netlink.h>
#else
#include <string.h>
#include <stdlib.h>
#endif
#include "sha2.h"
#include "pbkdf1.h"
#include "papdef.h"

eStatus pbkdf1(const unsigned char *pwd, size_t pwdLen, 
               const unsigned char *salt, size_t saltLen,
               unsigned int count, unsigned char * key, size_t keyLen)
{
	unsigned char digest[SHA256_DIGEST_LENGTH] = {0};

	int i;
	unsigned char *tmpStr;
	size_t tmpStrLen = saltLen + pwdLen;
	SHA256_CTX ctx256;
  
	//Step 1: If dkLen > Hash output length, return with error
	if (keyLen > SHA256_DIGEST_LENGTH) {
		return STATUS_FAILURE;  
	}
  
	//Create string = password + salt
#ifdef MODULE
	tmpStr = kmalloc(tmpStrLen, GFP_USER);
#else
	tmpStr = malloc(tmpStrLen);
#endif
	if (tmpStr == NULL) {
		return STATUS_FAILURE;
	}
  
	memcpy(tmpStr, pwd, pwdLen);
	memcpy(tmpStr + pwdLen, salt, saltLen);
  
	//Step 2: Calculate hash of Pwd + Salt
	SHA256_Init(&ctx256);
	SHA256_Update(&ctx256, tmpStr, tmpStrLen);
	SHA256_Final(digest, &ctx256);
#ifdef MODULE
	kfree(tmpStr);
#else
	kfree(tmpStr);
#endif  

	//Step 3: Calculate hash of previous hash repeatedly upto c times
	for(i = 0; i < count-1; i++) {
		SHA256_Init(&ctx256);
		SHA256_Update(&ctx256, digest, SHA256_DIGEST_LENGTH);
		SHA256_Final(digest, &ctx256);
	}
  
	memcpy(key, digest, keyLen);
    
	return STATUS_SUCCESS;
}
