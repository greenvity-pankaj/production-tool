/*********************************************************************

* File:     types.c $

*

* Description: Common types being used by XMLSTORE and others.

*

* Copyright (c) 2012 by Greenvity Communications.

*

********************************************************************/

#ifndef __TYPES_H__
#define __TYPES_H__



//typedef unsigned char bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t ;


typedef short int16_t;
typedef int int32_t ;


typedef uint8_t result_t;



typedef unsigned int process_event_t;


struct stimer {
    unsigned long start;
    unsigned long interval;
};

//typedef unsigned char u8_t;
/*REST method types*/
typedef enum {
	METHOD_GET		= (1 << 0),
	METHOD_PUT		= (1 << 1),
	METHOD_POST		= (1 << 2),
	METHOD_DELETE	= (1 << 3),
	METHOD_HEAD		= (1 << 4)
} method_t;

#define CCIF 
#define reentrant

//#define TRUE 1
//#define FALSE 0

#define PTR_CAST_DIFF char



/*Error codes*/
typedef enum {

    SUCCESS,

    FAIL,

    ERROR_INDEX_INVALID,

    ERROR_URI_INVALID,

    ERROR_METHOD_INVALID,

    ERROR_URI_NOT_LIST,

    ERROR_URI_LIST,

    ERROR_XML_NOT_FOUND,

    ERROR_INVALID_FS,

    ERROR_GET_RESOURCE_FAIL,

    ERROR_LIST_PARSE_FAIL,

    ERROR_DISCOVER_FAIL

} ERROR;

typedef enum
{
	REQUEST_TYPE_API,
	REQUEST_TYPE_HTTP
} REQUEST_TYPE;



#endif
