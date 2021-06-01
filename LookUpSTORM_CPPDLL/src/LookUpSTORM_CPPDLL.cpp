/****************************************************************************
 *
 * MIT License
 *
 * Copyright (C) 2021 Fabian Hauser
 *
 * Author: Fabian Hauser <fabian.hauser@fh-linz.at>
 * University of Applied Sciences Upper Austria - Linz - Austria
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ****************************************************************************/
#include "Common.h"

#ifdef JNI_EXPORT_LUT

#include "LookUpSTORM_CPPDLL.h"
#include "Controller.h"

#include <fstream>

using namespace LookUpSTORM;

struct PrimitiveArray
{
	jarray array;
	void* ptr;
	JNIEnv* env;
};

static PrimitiveArray _JLookUpTableHelper = { nullptr };

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setLookUpTable___3DIDDDD
(JNIEnv* env, jobject obj, jdoubleArray lookup, jint windowSize, jdouble dLat, jdouble dAx, jdouble rangeLat, jdouble rangeAx)
{
	const int size = env->GetArrayLength(lookup);
	if (env->ExceptionCheck()) {
		std::cerr << "LookUpSTORM_CPPDLL: ExceptionCheck triggered (1)!" << std::endl;
		return false;
	}

	Java_at_fhlinz_imagej_LookUpSTORM_releaseLookUpTable(env, obj);

	jboolean iscopy = false;
	jdouble* data = (jdouble*)(env->GetPrimitiveArrayCritical(lookup, &iscopy));
	if (env->ExceptionCheck()) {
		std::cerr << "LookUpSTORM_CPPDLL: ExceptionCheck triggered (2)!" << std::endl;
		return false;
	} else if (data == nullptr) {
		std::cerr << "LookUpSTORM_CPPDLL: Could not get primitive array for lookup table!" << std::endl;
		return false;
	}

	Fitter& f = Controller::inst()->fitter();
	if (!f.setLookUpTable(data, size, iscopy, windowSize, dLat, dAx, rangeLat, rangeAx)) {
		std::cerr << "LookUpSTORM_CPPDLL: Load LUT: Could not set LUT!" << std::endl;
		env->ReleasePrimitiveArrayCritical(lookup, data, JNI_ABORT);
		return false;
	}
	else {
		env->ReleasePrimitiveArrayCritical(lookup, data, JNI_COMMIT);
	}
	if (iscopy) {
		env->ReleasePrimitiveArrayCritical(lookup, data, JNI_ABORT);
		if (env->ExceptionCheck()) {
			std::cerr << "LookUpSTORM_CPPDLL: ExceptionCheck triggered (3)!" << std::endl;
			return false;
		}
	}
	else {
		_JLookUpTableHelper.array = lookup;
		_JLookUpTableHelper.ptr = data;
		_JLookUpTableHelper.env = env;
	}

	Controller::inst()->renderer().setSettings(f.minAx(), f.maxAx(), f.deltaAx(), 1.f);
	Controller::inst()->reset();
	return true;
}

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setLookUpTable__Ljava_lang_String_2
(JNIEnv* env, jobject, jstring jFileName)
{
	const char* fileName = env->GetStringUTFChars(jFileName, nullptr);
	std::ifstream file(fileName, std::ios::binary);
	env->ReleaseStringUTFChars(jFileName, fileName);
	if (!file.is_open())
		return false;

	struct HeaderLUT {
		char id[8];
		size_t dataSize;
		size_t indices;
		size_t windowSize;
		double dLat;
		double dAx;
		double rangeLat;
		double rangeAx;
	} hdr{};

	file.read((char*)&hdr, sizeof(hdr));
	if (strncmp(hdr.id, "LUTDSMLM", 8) != 0) {
		std::cerr << "LookUpSTORM_CPPDLL: Load LUT: ID of file is not correct" << std::endl;
		return false;
	}
	const size_t size = hdr.dataSize / sizeof(double);
	double* data = new double[size];
	file.read((char*)data, hdr.dataSize);
	file.close();
	Fitter& f = Controller::inst()->fitter();
	if (!f.setLookUpTable(data, size, true, hdr.windowSize, hdr.dLat, hdr.dAx, hdr.rangeLat, hdr.rangeAx))
		return false;
	Controller::inst()->renderer().setSettings(f.minAx(), f.maxAx(), f.deltaAx(), 1.f);
	Controller::inst()->reset();
	return true;
}

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_releaseLookUpTable
(JNIEnv* env, jobject)
{
	_JLookUpTableHelper.env = env;
	if ((_JLookUpTableHelper.env != nullptr) && (_JLookUpTableHelper.array != nullptr)) {
		_JLookUpTableHelper.env->ReleasePrimitiveArrayCritical(_JLookUpTableHelper.array, _JLookUpTableHelper.ptr, JNI_ABORT);
		if (_JLookUpTableHelper.env->ExceptionCheck()) {
			_JLookUpTableHelper.env->ExceptionDescribe();
			_JLookUpTableHelper.env->ExceptionClear();
		}
		_JLookUpTableHelper = { nullptr };
	}
	Fitter& f = Controller::inst()->fitter();
	f.release();
	return true;
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setEpsilon
(JNIEnv*, jobject, jdouble eps)
{
	Controller::inst()->fitter().setEpsilon(eps);
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setMaxIter
(JNIEnv*, jobject, jint maxIter)
{
	Controller::inst()->fitter().setMaxIter(maxIter);
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setThreshold
(JNIEnv*, jobject, jint threshold)
{
	Controller::inst()->setThreshold(threshold);
}

JNIEXPORT jobjectArray JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getLocs
(JNIEnv* env, jobject)
{
	// https://gamedev.stackexchange.com/questions/96947/jni-multidimensional-array-as-return-value
	jobjectArray result;

	std::list<Molecule>& mols = Controller::inst()->detectedMolecues();

	const jclass doubleArray1DClass = env->FindClass("[D");

	result = env->NewObjectArray(mols.size(), doubleArray1DClass, nullptr);
	auto it = mols.begin();
	for (size_t i = 0; i < mols.size(); ++i, ++it) {
		jdoubleArray arr = env->NewDoubleArray(7);
		env->SetDoubleArrayRegion(arr, 0, 7, it->data);
		env->SetObjectArrayElement(result, i, arr);
	}

	return result;
}

JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_numberOfDetectedLocs
(JNIEnv*, jobject)
{
	return static_cast<jint>(Controller::inst()->numberOfDetectedLocs());
}

JNIEXPORT jobjectArray JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getAllLocs
(JNIEnv* env, jobject)
{
	// https://gamedev.stackexchange.com/questions/96947/jni-multidimensional-array-as-return-value
	jobjectArray result;

	std::list<Molecule>& mols = Controller::inst()->allMolecues();

	const jclass doubleArray1DClass = env->FindClass("[D");

	result = env->NewObjectArray(mols.size(), doubleArray1DClass, nullptr);
	auto it = mols.begin();
	for (size_t i = 0; i < mols.size(); ++i, ++it) {
		jdoubleArray arr = env->NewDoubleArray(7);
		env->SetDoubleArrayRegion(arr, 0, 7, it->data);
		env->SetObjectArrayElement(result, i, arr);
	}

	return result;
}

JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_numberOfAllLocs
(JNIEnv*, jobject)
{
	return static_cast<jint>(Controller::inst()->allMolecues().size());
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setImagePara
(JNIEnv*, jobject, jint width, jint height)
{
	Controller::inst()->setImageSize(width, height);
}

JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getImageWidth
(JNIEnv*, jobject)
{
	return Controller::inst()->imageWidth();
}

JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getImageHeight
(JNIEnv*, jobject)
{
	return Controller::inst()->imageHeight();
}

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_processFrame
(JNIEnv* env, jobject, jshortArray jImage, jint width, jint height, jint frame, 
	jintArray jRenderImage, jint renderWidth, jint renderHeight)
{
	jboolean iscopy = false;
	int len = env->GetArrayLength(jImage);
	jshort* img = (jshort*)env->GetPrimitiveArrayCritical(jImage, &iscopy);
	ImageU16 image(width, height, (uint16_t*)img, false);
	if (img == nullptr) {
		std::cerr << "LookUpSTORM_CPPDLL: processFrame: Image error!" << std::endl;
		return 0;
	} else if (len != (width * height)) {
		return 0;
	}
	if ((width != Controller::inst()->imageWidth()) || (width != Controller::inst()->imageWidth()))
		Controller::inst()->setImageSize(width, height);

	Controller::inst()->renderer().setSize(renderWidth, renderHeight, double(renderWidth) / width, double(renderHeight) / height);

	Controller::inst()->processImage(image, frame);

	env->ReleasePrimitiveArrayCritical(jImage, img, JNI_ABORT);

	len = env->GetArrayLength(jRenderImage);
	jint* renderImg = (jint*)env->GetPrimitiveArrayCritical(jRenderImage, &iscopy);
	if (renderImg == nullptr) {
		std::cerr << "LookUpSTORM_CPPDLL: processFrame: Render image error!" << std::endl;
		return 0;
	}
	ImageU32 renderImage(renderWidth, renderHeight, (uint32_t*)renderImg, false);
	if (Controller::inst()->renderToImage(renderImage, frame)) {
		// copy back the content and free the elems buffer
		env->ReleasePrimitiveArrayCritical(jRenderImage, renderImg, 0);
	}
	else {
		// copy back the content and free the elems buffer
		env->ReleasePrimitiveArrayCritical(jRenderImage, renderImg, JNI_ABORT);
	}

	return 1;
}

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_isReady
(JNIEnv*, jobject)
{
	return Controller::inst()->isReady();
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_release
(JNIEnv*, jobject)
{
	Controller::inst()->reset();
	Controller::inst()->fitter().release();
	Controller::inst()->renderer().release();
	Controller::release();
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_reset
(JNIEnv*, jobject)
{
	Controller::inst()->reset();
	Controller::inst()->renderer().clear();
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setVerbose
(JNIEnv*, jobject, jboolean verbose)
{
	Controller::inst()->setVerbose(verbose);
}

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_releaseRenderImage
(JNIEnv* env, jobject)
{
	Controller::inst()->renderer().release();
	return true;
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_setRenderSigma
(JNIEnv*, jobject, jfloat sigma)
{
	Controller::inst()->renderer().setSigma(sigma);
}

JNIEXPORT jboolean JNICALL Java_at_fhlinz_imagej_LookUpSTORM_isRenderingReady
(JNIEnv*, jobject)
{
	return Controller::inst()->isSMLMImageReady();
}

JNIEXPORT void JNICALL Java_at_fhlinz_imagej_LookUpSTORM_clearRenderingReady
(JNIEnv*, jobject)
{
	Controller::inst()->clearSMLMImageReady();
}

JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getRenderImageWidth
(JNIEnv*, jobject)
{
	return Controller::inst()->renderer().imageWidth();
}

JNIEXPORT jint JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getRenderImageHeight
(JNIEnv*, jobject)
{
	return Controller::inst()->renderer().imageHeight();
}

JNIEXPORT jstring JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getVersion
(JNIEnv* env, jobject)
{
	return env->NewStringUTF(VERSION_STR);
}

typedef struct _JNI_POSREC {
	jclass cls;
	jmethodID constructortorID;
	jfieldID frameID;
	jfieldID xID;
	jfieldID yID;
	jfieldID zID;
	jfieldID intensityID;
	jfieldID backgroundID;
	jfieldID crlb_xID;
	jfieldID crlb_yID;
	jfieldID crlb_zID;
} JNI_POSREC;

bool LoadJniPosRec(JNI_POSREC* jniPosRec, JNIEnv* env)
{
	jniPosRec->cls = env->FindClass("at/fhlinz/imagej/Molecule");
	if (jniPosRec == nullptr) {
		std::cerr << "LookUpSTORM_CPPDLL: Could not load JNI class description of Molecule!" << std::endl;
		return false;
	}

	jniPosRec->constructortorID = env->GetMethodID(jniPosRec->cls, "<init>", "()V");
	jniPosRec->frameID = env->GetFieldID(jniPosRec->cls, "frame", "I");
	jniPosRec->xID = env->GetFieldID(jniPosRec->cls, "x", "D");
	jniPosRec->yID = env->GetFieldID(jniPosRec->cls, "y", "D");
	jniPosRec->zID = env->GetFieldID(jniPosRec->cls, "z", "D");
	jniPosRec->intensityID = env->GetFieldID(jniPosRec->cls, "intensity", "D");
	jniPosRec->backgroundID = env->GetFieldID(jniPosRec->cls, "background", "D");
	jniPosRec->crlb_xID = env->GetFieldID(jniPosRec->cls, "crlb_x", "D");
	jniPosRec->crlb_yID = env->GetFieldID(jniPosRec->cls, "crlb_y", "D");
	jniPosRec->crlb_zID = env->GetFieldID(jniPosRec->cls, "crlb_z", "D");

	return true;
}

JNIEXPORT jobjectArray JNICALL Java_at_fhlinz_imagej_LookUpSTORM_getFittedMolecules
(JNIEnv* env, jobject, jdouble pixelSize, jdouble adu, jdouble gain, jdouble baseline)
{
	JNI_POSREC jniPosRec;
	LoadJniPosRec(&jniPosRec, env);
	
	std::list<Molecule>& mols = Controller::inst()->allMolecues();

	const jclass doubleArray1DClass = env->FindClass("[D");

	jobjectArray result = env->NewObjectArray(mols.size(), jniPosRec.cls, nullptr);
	
	double crlb[5];
	auto it = mols.begin();
	for (size_t i = 0; i < mols.size(); ++i, ++it) {
		const double photons = Controller::inst()->calculatePhotons(*it, adu, gain);
		const double bg = std::max(0.0, (it->background - baseline) * adu / gain);

		jobject jPosRec = env->NewObject(jniPosRec.cls, jniPosRec.constructortorID);
		env->SetIntField(jPosRec, jniPosRec.frameID, (jint)it->frame);
		env->SetDoubleField(jPosRec, jniPosRec.xID, (jdouble)(it->x * pixelSize));
		env->SetDoubleField(jPosRec, jniPosRec.yID, (jdouble)(it->y * pixelSize));
		env->SetDoubleField(jPosRec, jniPosRec.zID, (jdouble)it->z);
		env->SetDoubleField(jPosRec, jniPosRec.intensityID, (jdouble)photons);
		env->SetDoubleField(jPosRec, jniPosRec.backgroundID, (jdouble)bg);
		if (Controller::inst()->calculateCRLB(*it, crlb, adu, gain, baseline, pixelSize) &&
			std::isfinite(crlb[2]) && std::isfinite(crlb[3]) && std::isfinite(crlb[4])) {
			env->SetDoubleField(jPosRec, jniPosRec.crlb_xID, (jdouble)crlb[2]);
			env->SetDoubleField(jPosRec, jniPosRec.crlb_yID, (jdouble)crlb[3]);
			env->SetDoubleField(jPosRec, jniPosRec.crlb_zID, (jdouble)crlb[4]);
		}
		env->SetObjectArrayElement(result, i, jPosRec);
	}

	return result;
}

#endif // JNI_EXPORT