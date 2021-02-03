/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class at_fhlinz_imagej_LookUpSTORM */

#ifndef _Included_at_fhlinz_imagej_LookUpSTORM
#define _Included_at_fhlinz_imagej_LookUpSTORM
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setLookUpTable
 * Signature: ([DIDDDD)Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setLookUpTable___3DIDDDD
  (JNIEnv *, jobject, jdoubleArray, jint, jdouble, jdouble, jdouble, jdouble);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setLookUpTable
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setLookUpTable__Ljava_lang_String_2
  (JNIEnv *, jobject, jstring);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    releaseLookUpTable
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_releaseLookUpTable
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setEpsilon
 * Signature: (D)V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setEpsilon
  (JNIEnv *, jobject, jdouble);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setMaxIter
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setMaxIter
  (JNIEnv *, jobject, jint);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setThreshold
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setThreshold
  (JNIEnv *, jobject, jint);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    getLocs
 * Signature: ()[[D
 */
JNIEXPORT jobjectArray JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getLocs
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    numberOfDetectedLocs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_numberOfDetectedLocs
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    getAllLocs
 * Signature: ()[[D
 */
JNIEXPORT jobjectArray JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getAllLocs
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    numberOfAllLocs
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_numberOfAllLocs
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setImagePara
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setImagePara
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    getImageWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getImageWidth
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    getImageHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getImageHeight
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    feedImageData
 * Signature: ([SI)Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_feedImageData
  (JNIEnv *, jobject, jshortArray, jint);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    isLocFinish
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_isLocFinish
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    isReady
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_isReady
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_release
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_reset
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setVerbose
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setVerbose
  (JNIEnv *, jobject, jboolean);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setRenderImage
 * Signature: ([III)Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setRenderImage
  (JNIEnv *, jobject, jintArray, jint, jint);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    releaseRenderImage
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_releaseRenderImage
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    setRenderSigma
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setRenderSigma
  (JNIEnv *, jobject, jfloat);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    isRenderingReady
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_isRenderingReady
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    clearRenderingReady
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_clearRenderingReady
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    getRenderImageWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getRenderImageWidth
  (JNIEnv *, jobject);

/*
 * Class:     at_fhlinz_imagej_LookUpSTORM
 * Method:    getRenderImageHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getRenderImageHeight
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
