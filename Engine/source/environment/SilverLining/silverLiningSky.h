//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _SILVERLININGSKY_H_
#define _SILVERLININGSKY_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif
#ifndef _RENDERPASSMANAGER_H_
#include "renderInstance/renderPassManager.h"
#endif
#ifndef _LIGHTINFO_H_
#include "lighting/lightInfo.h"
#endif
#ifndef _LIGHTFLAREDATA_H_
#include "T3D/lightFlareData.h"
#endif
#ifndef _TRESPONSECURVE_H_
#include "math/util/tResponseCurve.h"
#endif

#ifndef ATMOSPHERE_H
#include "Atmosphere.h"
#endif

#ifndef CLOUD_TYPES_H
#include "CloudTypes.h"
#endif

#ifndef _CLOUDLAYER_H_
#include "CloudLayer.h"
#endif


class LightInfo;

class MyMillisecondTimer : public SilverLining::MillisecondTimer
{
public:
	virtual unsigned long SILVERLINING_API GetMilliseconds() const
	{
		return SilverLining::MillisecondTimer::GetMilliseconds() * 2;
	}
};

class SilverLiningSky : public SceneObject, public ISceneLight
{
   typedef SceneObject Parent;

public:

   enum MaskBits
   {
      UpdateMask = Parent::NextFreeMask,
	  TimeMask = Parent::NextFreeMask << 1,
	  PrecipitationUpdateMask = Parent::NextFreeMask << 2,
	  NextFreeMask = Parent::NextFreeMask << 3
   };

   SilverLiningSky();
   ~SilverLiningSky();

   static SilverLining::Atmosphere* atm;

   //Matrices
   MatrixSet *mMatrixSet;
   double mMatModelView[16], mMatProjection[16];

   // SimObject
   bool onAdd();

   void InitializeAtm();

   void matrixToDoubleArray(const MatrixF& matrix, double out[]);

   void onRemove();

   // ISceneLight
   virtual void submitLights( LightManager *lm, bool staticLighting );
   virtual LightInfo* getLight() { return mLight; }

   // ConsoleObject
   DECLARE_CONOBJECT(SilverLiningSky);
   void inspectPostApply();
   static void initPersistFields();

   // Network
   U32  packUpdate  ( NetConnection *conn, U32 mask, BitStream *stream );
   void unpackUpdate( NetConnection *conn,           BitStream *stream );

   void prepRenderImage( SceneRenderState* state );

   void setTime(const char *data);

   Point3F getSunDir();

	void SetPrecipitation(S32 precipitationType, F32 ratemmPerHour);

protected:

   void _renderSky( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );

   void _renderObjects( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );
   void _debugRender( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );

   void _conformLights();
   void SetSceneFog();

   // static protected field set methods
   static bool ptSetClouds( void *object, const char *index, const char *data );
   static bool ptSetTime( void *object, const char *index, const char *data );
   static bool ptReadOnlySetFn( void *object, const char *index, const char *data );

   // SimObject.
   virtual void _onSelected();
   virtual void _onUnselected();

   void SetSilverLiningTime();
   void ConformCloudsLayers();
   void setupClouds( S32 cloudType );
   void ConformPrecipitations();
   void _onTextureEvent( GFXTexCallbackCode code );
protected:

#define CURVE_COUNT 5

	double pView[16], pProj[16];

   VectorF mLightDir;
   VectorF mSunDir;

   S32 mYear, mMonth, mDay;
   S32 mHour, mMin;

   F32 yawCam; // for average color  fog calc.

   S32 mPrecipitationType;
   F32 mPrecipitationRate;

   MyMillisecondTimer timer;

   ColorF mAmbientColor;   ///< Not a field
   ColorF mSunColor;       ///< Not a field
   ColorF mFogColor;       ///< Not a field

   LightInfo *mLight;

   bool mCastShadows;
   bool mDirty;

   GFXStateBlockRef mZEnableSB;

   // Fields...
   StringTableEntry mDateTime;

   StringTableEntry mCloudLayerTypeName[NUM_CLOUD_TYPES];   
   S32 mCloudLayerHandle[NUM_CLOUD_TYPES];
   bool mCloudLayerEnabled[NUM_CLOUD_TYPES];
   F32 mCloudLayerBaseAltitude[NUM_CLOUD_TYPES];
   F32 mCloudLayerThickness[NUM_CLOUD_TYPES];
   F32 mCloudLayerBaseWidth[NUM_CLOUD_TYPES];
   F32 mCloudLayerDensity[NUM_CLOUD_TYPES];   
   bool mCloudLayerMustConform[NUM_CLOUD_TYPES]; 
   bool mCloudLayerUpdate[NUM_CLOUD_TYPES]; 
};

#endif // _SILVERLININGSKY_H_
