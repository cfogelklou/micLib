//
//  AppDelegate_cpp.h
//  remoteio_test
//
//  Created by Chris Fogelklou on 28/05/15.
//  Copyright (c) 2015 Applicaudia. All rights reserved.
//

#ifndef __remoteio_test__AppDelegate_cpp__
#define __remoteio_test__AppDelegate_cpp__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MIC_tag{
  void *pSelf;
} MIC_t;

MIC_t * MIC_Start( void * pSelf );
    
void MIC_Stop( MIC_t * pMic );
    
#ifdef __cplusplus
}
#endif


#endif /* defined(__remoteio_test__AppDelegate_cpp__) */
