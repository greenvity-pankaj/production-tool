/* 
 * File:   uim.h
 * Author: palmchip
 *
 * Created on September 29, 2011, 4:54 PM
 */

#ifndef UIM_H
#define	UIM_H

typedef struct uim
{
    int    opt;
#if defined(WIN32) || defined(_WIN32)
    HANDLE      uimThread;
#else
    pthread_t   uimThread;
#endif

}sUim, *psUim;

eStatus UIM_Init(sUim *uim);

void UIM_DisplayResult(sUim *uim, u8 reqType, void *resultParam);

#endif	/* UIM_H */

