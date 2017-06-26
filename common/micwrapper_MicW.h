/* DO NOT EDIT THIS FILE - it is machine generated */

/* Header for class micwrapper_MicW */

#ifndef _Included_micwrapper_MicW
#define _Included_micwrapper_MicW
#if defined (__ANDROID__) || defined(ROBOVM)
#include <jni.h>
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     micwrapper_MicW
 * Method:    Start
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_micwrapper_MicW_Start
  (JNIEnv *, jclass, jint);

/*
 * Class:     micwrapper_MicW
 * Method:    Stop
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_micwrapper_MicW_Stop
  (JNIEnv *, jclass);

/*
 * Class:     micwrapper_MicW
 * Method:    GetReadReady
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_micwrapper_MicW_GetReadReady
  (JNIEnv *, jclass);

/*
 * Class:     micwrapper_MicW
 * Method:    Read
 * Signature: ([FI)I
 */
JNIEXPORT jint JNICALL Java_micwrapper_MicW_Read
  (JNIEnv *, jclass, jfloatArray, jint);

#ifdef __cplusplus
}
#endif
#else

#ifdef __cplusplus
extern "C" {
#endif
  
int _IPhoneMicStart();

void _IPhoneMicStop();

int _IPhoneMicGetReadReady();

int _IPhoneMicRead(float *pfBuf, int length);
  
#ifdef __cplusplus
}
#endif

#endif // APPLE
#endif
