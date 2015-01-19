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

#include "platform/platform.h"
#include "SilverLiningSky.h"

#include "core/stream/bitStream.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "math/util/sphereMesh.h"
#include "math/mathUtils.h"
#include "math/util/matrixSet.h"
#include "scene/sceneRenderState.h"
#include "lighting/lightInfo.h"
#include "gfx/sim/gfxStateBlockData.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/sim/cubemapData.h"
#include "materials/shaderData.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "materials/sceneData.h"
#include <d3d9.h>
#include "gfx/D3D9/gfxD3D9Device.h"

#include "CloudLayerFactory.h"
// #include "MemAlloc.h"

#include <algorithm>    // std::sort
#include "gfx/util/gfxFrustumSaver.h"

ImplementEnumType( SilveLiningPrecipitationTypes,
	"PrecipitationTypes.\n" )

{ SilveLiningPrecipitationTypes::NONE, "NONE", "NONE" },
{ SilveLiningPrecipitationTypes::RAIN, "RAIN", "RAIN" },
{ SilveLiningPrecipitationTypes::DRY_SNOW, "DRY_SNOW", "DRY_SNOW", },
{ SilveLiningPrecipitationTypes::WET_SNOW, "WET_SNOW", "WET_SNOW" },
{ SilveLiningPrecipitationTypes::SLEET, "SLEET", "SLEET" },

EndImplementEnumType;

ConsoleDocClass( SilverLiningSky,
	"@brief Automatic skies, 3D clouds, and weather for any time and location."

	"@ingroup Atmosphere"
	);

SilverLining::Atmosphere* SilverLiningSky::atm = NULL;

const char* cloudTypesVector[NUM_CLOUD_TYPES] = { "CIRROCUMULUS",	"CIRRUS_FIBRATUS", "STRATUS", "CUMULUS_MEDIOCRIS", "CUMULUS_CONGESTUS", "CUMULUS_CONGESTUS_HI_RES", "CUMULONIMBUS_CAPPILATUS", 	"STRATOCUMULUS", "TOWERING_CUMULUS", "SANDSTORM" };

// Simulated visibility in meters, for fog effects.
#define kVisibility 30000.0

IMPLEMENT_CO_NETOBJECT_V1(SilverLiningSky);


SilverLiningSky::SilverLiningSky()
{		
	mNetFlags.set( Ghostable | ScopeAlways );
	mTypeMask |= EnvironmentObjectType | LightObjectType | StaticObjectType;

	mSunColor.set( 0, 0, 0, 1.0f );

	yawCam = 0.0f;

	mDirty = true;

	mLight = LightManager::createLightInfo();
	mLight->setType( LightInfo::Vector );

	ConfigureClouds();
	
	mSelectPrecipitationActive = SilveLiningPrecipitationTypes::NONE;
	mPrecipitationType = SilverLining::CloudLayer::NONE;
	mPrecipitationRate = 0;

	mYear = 0, mMonth = 1, mDay = 1;
	mHour = 12, mMin = 0;

	setTime( "2014 12 22 15 30 00" );

	mCastShadows = true;
	mLensFlare = true;
	mBrightness = 1.0f;
	mCrepuscularRays = 0.0f;

	// Set up wind blowing northeast at 50 meters/sec
	mWindSpeed = 50.0f;
	mWindDirection = 225.0f;
	mWindHandle = 0;
}

SilverLiningSky::~SilverLiningSky()
{	
	SAFE_DELETE( mLight );
}

bool SilverLiningSky::onAdd()
{
	PROFILE_SCOPE(SilverLiningSky_onAdd);

	// onNewDatablock for the server is called here
	// for the client it is called in unpackUpdate

	if ( !Parent::onAdd() )
		return false;

	setGlobalBounds();
	resetWorldBox();

	addToScene();

	if ( isClientObject() )
	{
		GFXTextureManager::addEventDelegate( this, &SilverLiningSky::_onTextureEvent ); 

		// Instantiate an Atmosphere object.
		atm = new SilverLining::Atmosphere("Your Company Name", "Your License Code");    
		InitializeAtm();
	}

	return true;
}

void SilverLiningSky::onRemove()
{
	removeFromScene();

	if ( isClientObject() )
	{
		if (atm)
		{
			delete atm;
			atm = 0;
		}
	}

	Parent::onRemove();
}

void SilverLiningSky::InitializeAtm()
{
	LPDIRECT3DDEVICE9 D3DDevice = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();

	int err = atm->Initialize(SilverLining::Atmosphere::DIRECTX9,
		".\\Resources\\", true, D3DDevice);

	if (err == SilverLining::Atmosphere::E_NOERROR)
	{
		// Tell SilverLining what your axis conventions are.
		// Set your frame of reference (call this before setting up clouds!)
		atm->SetUpVector(0, 0, 1);
		atm->SetRightVector(1, 0, 0);

		// The below settings will disable precipitation effects that depend on modifying the projection matrix,
		// such as pulling in the near clip plane and de-projecting particles to enforce minimum particle sizes. 
		// There is something about Torque's projection matrix that it just can't figure out it seems.
		atm->SetConfigOption("rain-use-depth-buffer", "yes"); 
		atm->SetConfigOption("snow-use-depth-buffer", "yes"); 
		atm->SetConfigOption("sleet-use-depth-buffer", "yes"); 
		atm->SetConfigOption("rain-minimum-pixels", "0"); 
		atm->SetConfigOption("sleet-minimum-pixels", "0"); 
		atm->SetConfigOption("snow-minimum-pixels", "0"); 
		atm->SetConfigOption("snow-near-clip", "0.2"); 
		atm->SetConfigOption("rain-near-clip", "0.2"); 
		atm->SetConfigOption("sleet-near-clip", "0.2"); 
		atm->SetConfigOption("enable-precipitation-visibility-effects", "no"); 

		atm->SetConfigOption("use-cloud-backdrops", "no"); 

		// Add some atmospheric perspective
		atm->GetConditions()->SetVisibility(kVisibility);

		// Add a little wind
		setWindSetting(15.0f, 180.0f);

		// Set our location (change this to your own latitude and longitude)
		SilverLining::Location loc;
		loc.SetAltitude(0);
		loc.SetLatitude(41.65f);
		loc.SetLongitude(0.86f);
		atm->GetConditions()->SetLocation(loc);

		atm->GetConditions()->SetMillisecondTimer(&timer);

		atm->EnableLensFlare(mLensFlare);

		//atm->SetRandomNumberGenerator(&random);

		SetSilverLiningTime();

		ConformPrecipitations();
	}
}

void SilverLiningSky::ConfigureClouds()
{
	for (S32 i = 0; i < NUM_CLOUD_TYPES; i++) {
		mCloudLayerTypeName[i] = cloudTypesVector[i];   
		mCloudLayerHandle[i] = 0;
		mCloudLayerEnabled[i] = false;
		mCloudLayerBaseAltitude[i] = 1000.0f;
		mCloudLayerThickness[i] = 90.0f;
		mCloudLayerBaseWidth[i] = 20000.0f;
		mCloudLayerDensity[i] = 0.5f;   
		mCloudLayerMustConform[i] = false; 
		mCloudLayerUpdate[i] = true; 
	}

	// Configure high cirrus clouds
	int i=CIRRUS_FIBRATUS; 
	mCloudLayerBaseAltitude[i] = 6000;
	mCloudLayerThickness[i] = 0;
	mCloudLayerBaseWidth[i] = 100000;
	mCloudLayerDensity[i] = 0.5f; 

	// Configure a cumulus congestus deck with 80% sky coverage.
	i=CUMULUS_CONGESTUS; 
	mCloudLayerBaseAltitude[i] = 1500;
	mCloudLayerThickness[i] = 100;
	mCloudLayerBaseWidth[i] = 30000;
	mCloudLayerDensity[i] = 0.8f; 

	// Sets up a solid stratus deck.
	i=STRATUS; 
	mCloudLayerBaseAltitude[i] = 1000;
	mCloudLayerThickness[i] = 600;
	mCloudLayerBaseWidth[i] = 0; //Is Infinite
	mCloudLayerDensity[i] = 0.5f; 

	// A thunderhead; note a Cumulonimbus cloud layer contains a single cloud.
	i=CUMULONIMBUS_CAPPILATUS; 
	mCloudLayerBaseAltitude[i] = 1000;
	mCloudLayerThickness[i] = 3000;
	mCloudLayerBaseWidth[i] = 5000;
	mCloudLayerDensity[i] = 0.5f; 

	// Cumulus mediocris are little, puffy clouds. Keep the density low for realism, otherwise
	// you'll have a LOT of clouds because they are small.
	i=CUMULUS_MEDIOCRIS; 
	mCloudLayerBaseAltitude[i] = 1000;
	mCloudLayerThickness[i] = 100;
	mCloudLayerBaseWidth[i] = 20000;
	mCloudLayerDensity[i] = 0.5f; 

	// Stratocumulus clouds are rendered with GPU ray-casting. On systems that can support it
	// (Shader model 3.0+) this enables very dense cloud layers with per-fragment lighting.
	i=STRATOCUMULUS; 
	mCloudLayerBaseAltitude[i] = 1000;
	mCloudLayerThickness[i] = 3000;
	mCloudLayerBaseWidth[i] = 30000;
	mCloudLayerDensity[i] = 1.0f; 

	// Sandstorms should be positioned at ground level. There is no need to set their
	// density or thickness.
	i=SANDSTORM; 
	mCloudLayerBaseAltitude[i] = 0;
	mCloudLayerBaseWidth[i] = 50000;
}

void SilverLiningSky::_onTextureEvent( GFXTexCallbackCode code ) 
{ 
	if(atm) { 
		if(code == GFXZombify) 
			atm->D3D9DeviceLost(); 
		else if(code == GFXResurrect) 
			atm->D3D9DeviceReset(); 
	} 
} 

void SilverLiningSky::_conformLights()
{
	if(!atm)
		return;

	atm->EnableLensFlare(mLensFlare);

	atm->GetSunOrMoonPosition(&mSunDir.x, &mSunDir.y, &mSunDir.z);   
	mLightDir = -mSunDir;

	mAmbientColor.set( 0, 0, 0, 0 );
	mSunColor.set( 0, 0, 0, 0 );

	atm->GetSunOrMoonColor(&mSunColor.red, &mSunColor.green, &mSunColor.blue);
	atm->GetAmbientColor(&mAmbientColor.red, &mAmbientColor.green, &mAmbientColor.blue);

	mSunColor.alpha = mAmbientColor.alpha = 1.0;	   

	mAmbientColor *= 0.5f;
	mSunColor *= 1.5f;

	mLight->setType(LightInfo::Vector);
	mLight->setCastShadows( mCastShadows );

	mLight->setDirection( mLightDir );
	mLight->setAmbient( mAmbientColor );   
	mLight->setColor( mSunColor );
	mLight->setBrightness( mBrightness );     

	mLight->setRange(kVisibility*kVisibility);	

	//atm->GetShadowMapObject()	

	SetSceneFog();
}

void SilverLiningSky::submitLights( LightManager *lm, bool staticLighting )
{
	if ( mDirty )
	{
		_conformLights();
		mDirty = false;
	}

	// The sun is a special light and needs special registration.
	lm->setSpecialLight( LightManager::slSunLightType, mLight );
}


void SilverLiningSky::inspectPostApply()
{
	mDirty = true;
	setMaskBits( 0xFFFFFFFF );
}

void SilverLiningSky::initPersistFields()
{	
	addGroup( "Misc" );

	addProtectedField( "DateTime", TypeString, Offset( mDateTime, SilverLiningSky ), &SilverLiningSky::ptSetTime, &defaultProtectedGetFn,
		"The horizontal angle of the sun measured clockwise from the positive Y world axis. This field is networked." );
	addField( "crepuscularRays", TypeF32, Offset( mCrepuscularRays, SilverLiningSky ),
		"Set to a value greater than 0 to draw crepuscular rays (AKA ""God rays"") after the clouds." );
	addField( "lensFlare", TypeBool, Offset( mLensFlare, SilverLiningSky ),
		"Enable or disable a big, flashy lens flare effect when the sun is visible in the scene." );

	endGroup( "Misc" );

	addGroup("Wind Effect");

	addField( "WindSpeed", TypeF32, Offset( mWindSpeed, SilverLiningSky ),
		"Set the wind velocity within this WindVolume, in meters per second." );
	addField( "WindDirection", TypeF32, Offset( mWindDirection, SilverLiningSky ),
		"Sets the wind direction, in degrees East from North. This is the direction the wind is"
		"coming from, not the direction it is blowing toward." );

	endGroup("Wind Effect");

	addGroup( "Lighting" );

	addField( "castShadows", TypeBool, Offset( mCastShadows, SilverLiningSky ),
		"Enables/disables shadows cast by objects due to SilverLiningSky light." );
	addField( "brightness", TypeF32, Offset( mBrightness, SilverLiningSky ),
		"The brightness of the ScatterSky's light object." );

	endGroup( "Lighting" );

	addGroup("Cloud Layers");
	addArray( "Clouds", NUM_CLOUD_TYPES );

	addProtectedField( "Type", TypeString, Offset( mCloudLayerTypeName, SilverLiningSky ), &SilverLiningSky::ptReadOnlySetFn, &defaultProtectedGetFn, NUM_CLOUD_TYPES,
		"Controls the texture repeat of this slot." );
	addProtectedField( "Enabled", TypeBool, Offset( mCloudLayerEnabled, SilverLiningSky ), &SilverLiningSky::ptSetClouds, &defaultProtectedGetFn, NUM_CLOUD_TYPES,
		"Controls the direction this slot scrolls." );
	addProtectedField( "BaseAltitude", TypeF32, Offset( mCloudLayerBaseAltitude, SilverLiningSky ), &SilverLiningSky::ptSetClouds, &defaultProtectedGetFn, NUM_CLOUD_TYPES,
		"Controls the speed this slot scrolls." );	
	addProtectedField( "BaseWidth", TypeF32, Offset( mCloudLayerBaseWidth, SilverLiningSky ), &SilverLiningSky::ptSetClouds, &defaultProtectedGetFn, NUM_CLOUD_TYPES,
		"Controls the speed this slot scrolls." );
	addProtectedField( "Thickness", TypeF32, Offset( mCloudLayerThickness, SilverLiningSky ), &SilverLiningSky::ptSetClouds, &defaultProtectedGetFn, NUM_CLOUD_TYPES,
		"Controls the speed this slot scrolls." );
	addProtectedField( "Density", TypeF32, Offset( mCloudLayerDensity, SilverLiningSky ), &SilverLiningSky::ptSetClouds, &defaultProtectedGetFn, NUM_CLOUD_TYPES,
		"Controls the speed this slot scrolls." );

	endArray( "Clouds" );
	endGroup("Cloud Layers");

	addGroup("Precipitation");
	addProtectedField("PrecipitationActive", TypeSilveLiningPrecipitationTypes, Offset(mSelectPrecipitationActive, SilverLiningSky), &SilverLiningSky::ptSetPrecipitationType, &defaultProtectedGetFn, 
		"The type of precipitation to simulate under this cloud layer - NONE, RAIN, WET_SNOW,"
		"DRY_SNOW, or SLEET.");
	addProtectedField("PrecipitationRate", TypeF32, Offset(mPrecipitationRate, SilverLiningSky), &SilverLiningSky::ptSetPrecipitationRate, &defaultProtectedGetFn, 
		"The simulated rate of precipitation, in millimeters per hour. Reasonable ranges"
		"might be between 1 for light rain or 20 for heavier rain. This value will be clamped to the value"
		"specified by rain-max-intensity, snow-max-intensity, or sleet-max-intensity in"
		"resources/SilverLining.config, which is 30 by default.");///< SoftSelectAction brush filtering
	endGroup("Precipitation");

	Parent::initPersistFields();
}

/// Set up wind blowing
void SilverLiningSky::setWindSetting( F32 windSpeed, F32 windDirection )
{
	mWindSpeed = windSpeed;
	mWindDirection = windDirection;

	// Add a little wind
	if(atm)
	{				
		if(mWindHandle != 0)
			mWindHandle = atm->GetConditions()->RemoveWindVolume(mWindHandle);
		SilverLining::WindVolume wv;
		wv.SetDirection(mWindDirection);
		wv.SetMinAltitude(0);
		wv.SetMaxAltitude(10000);
		wv.SetWindSpeed(mWindSpeed);
		mWindHandle = atm->GetConditions()->SetWind(wv);
	}
}

U32 SilverLiningSky::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
	U32 retMask = Parent::packUpdate(con, mask, stream);

	if ( stream->writeFlag( mask & TimeMask ) )
	{
		stream->write( mYear );
		stream->write( mMonth );
		stream->write( mDay );
		stream->write( mHour );
		stream->write( mMin );
	}

	if ( stream->writeFlag( mask & UpdateMask ) )
	{
		stream->write( mBrightness );
		stream->writeFlag( mCastShadows );
		stream->writeFlag( mLensFlare );

		stream->write( mCrepuscularRays );

		stream->write( mWindSpeed );
		stream->write( mWindDirection );

		for (S32 i = 0; i < NUM_CLOUD_TYPES; i++)
		{
			if ( stream->writeFlag( mCloudLayerUpdate[i] ) )
			{
				mCloudLayerUpdate[i] = false;

				if ( stream->writeFlag( mCloudLayerEnabled[i] ) )
				{
					stream->write( mCloudLayerBaseAltitude[i] );
					stream->write( mCloudLayerBaseWidth[i] );
					stream->write( mCloudLayerDensity[i] );
					stream->write( mCloudLayerThickness[i] );
				}

			}
		}
	}

	if ( stream->writeFlag( mask & PrecipitationUpdateMask ) )
	{
		stream->write(mPrecipitationType);		   
		stream->write(mPrecipitationRate);		   
	}   

	return retMask;
}

void SilverLiningSky::unpackUpdate(NetConnection *con, BitStream *stream)
{
	Parent::unpackUpdate(con, stream);

	if ( stream->readFlag() ) // TimeMask
	{
		stream->read( &mYear );
		stream->read( &mMonth );
		stream->read( &mDay );
		stream->read( &mHour );
		stream->read( &mMin );
		SetSilverLiningTime();
	}

	if ( stream->readFlag() ) // UpdateMask
	{
		stream->read( &mBrightness );
		mCastShadows = stream->readFlag();
		mLensFlare = stream->readFlag();

		stream->read( &mCrepuscularRays );

		F32 windSpeed, windDirection;
		stream->read( &windSpeed );
		stream->read( &windDirection );
		if(windSpeed != mWindSpeed || windDirection != mWindDirection)
			setWindSetting(windSpeed, windDirection);

		for (S32 i = 0; i < NUM_CLOUD_TYPES; i++)
		{
			if ( stream->readFlag() ) // mCloudLayers[i].update
			{
				mCloudLayerEnabled[i] = stream->readFlag();
				if(mCloudLayerEnabled[i])
				{
					stream->read( &mCloudLayerBaseAltitude[i] );
					stream->read( &mCloudLayerBaseWidth[i] );
					stream->read( &mCloudLayerDensity[i] );
					stream->read( &mCloudLayerThickness[i] );
				}
				mCloudLayerMustConform[i] = true;
			}
		}

		_conformLights();
		ConformCloudsLayers();
	}

	if ( stream->readFlag() ) // PrecipitationUpdateMask 
	{		
		stream->read(&mPrecipitationType);		   
		stream->read(&mPrecipitationRate);
		ConformPrecipitations();
	}


}

void SilverLiningSky::prepRenderImage( SceneRenderState *state )
{
	// Only render into diffuse and reflect passes.

	if( !state->isDiffusePass() &&
		!state->isReflectPass() )
		return;

	//Set Triton Matrices
	MatrixF view = GFX->getWorldMatrix();	
	MatrixF proj = GFX->getProjectionMatrix();
	view = view.transpose();
	proj = proj.transpose();
	matrixToDoubleArray(view, mMatModelView);
	matrixToDoubleArray(proj, mMatProjection);
	atm->SetCameraMatrix(mMatModelView);
	atm->SetProjectionMatrix(mMatProjection);

	// sky render instance.
	ObjectRenderInst *riSky = state->getRenderPass()->allocInst<ObjectRenderInst>();
	riSky->renderDelegate.bind( this, &SilverLiningSky::_renderSky );
	riSky->type = RenderPassManager::RIT_Object;
	state->getRenderPass()->addInst( riSky );

	ConformCloudsLayers();

	_conformLights();
}



void SilverLiningSky::_renderSky( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
	if (!atm)
		return;

	if ( overrideMat )
		return;

	// GFXTransformSaver is a handy helper class that restores
	// the current GFX matrices to their original values when
	// it goes out of scope at the end of the function
	GFXTransformSaver saver;

	//SetModelviewProjectionMatrix(state);	
	// Call DrawSky after scene has begun and modelview / projection matrices 
	// properly set for the camera position. This will draw the sky if you pass true.
	atm->DrawSky(true, false, 0, true, false, true);
	//_conformLights();

	atm->DrawObjects(true, true, true, mCrepuscularRays);	

	/*void* shadowTexID = 0; //on DirectX9 it is a IDirect3DTexture9 *
	SilverLining::Matrix4 lightMatrix, shadowMatrix;
	if ( atm->GetShadowMap(shadowTexID, &lightMatrix, &shadowMatrix) )
	{
	}*/
}

void SilverLiningSky::matrixToDoubleArray(const MatrixF& matrix, double out[])
{
	for(int i = 0; i < 16; i++) {
		out[i] = matrix[i];
	}
}


// For effects inside stratus decks, it's important to honor any requests from SilverLining
// for fog. SilverLining can also provide guidance for a good fog color to use to blend
// distant terrain into the sky.
void SilverLiningSky::SetSceneFog()
{
	// Need to light the fog, or it will glow in the dark...
	F32 ar, ag, ab;
	atm->GetSunOrMoonColor(&ar, &ag, &ab);

	FogData fog = getSceneManager()->getFogData();

	F32 density, r, g, b;

	// If you're inside a cloud, SilverLining will request that you set the fog accordingly.
	if (atm->GetFogEnabled())
	{
		atm->GetFogSettings(&density, &r, &g, &b);
		r *= ar;
		g *= ag;
		b *= ab;

		atm->GetConditions()->SetFog(density/100.0f, r, g, b);
		atm->SetHaze(r, g, b, 10, density/100.0f);

		fog.color = ColorF(r, g, b, 1.0);
		fog.density = density;
		getSceneManager()->setFogData( fog );
	}
	else // Otherwise, setting the fog to the average color of the sky at the horizon works well.
	{
		density = fog.density;	

		F32 yawdegrees = mRadToDeg(yawCam);
		atm->GetHorizonColor(yawdegrees, 1.0f, &r, &g, &b);				

		//density = (F32)(1.0 / kVisibility);

		atm->GetConditions()->SetFog(density/100.0f, r, g, b);
		atm->SetHaze(r, g, b, 10, density/100.0f);
		atm->GetConditions()->SetVisibility(kVisibility);

		//FogData fog = getSceneManager()->getFogData();
		fog.color = ColorF(r, g, b, 1.0);
		fog.density = density;
		getSceneManager()->setFogData( fog );
	}

}

void SilverLiningSky::_debugRender( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
}

// Static protected field set methods

bool SilverLiningSky::ptSetClouds( void *object, const char *index, const char *data )
{
	SilverLiningSky *sky = static_cast<SilverLiningSky*>( object );
	S32 cloudType = dAtoi(index);
	sky->setupClouds( cloudType );
	return true; // ok to set value
}

bool SilverLiningSky::ptSetTime( void *object, const char *index, const char *data )
{
	SilverLiningSky *sky = static_cast<SilverLiningSky*>( object );
	sky->setTime( data );
	return true;
}

//to expose C++ variabl as readonly
bool SilverLiningSky::ptReadOnlySetFn( void *object, const char *index, const char *data )
{
	return false;
}

bool SilverLiningSky::ptSetPrecipitationType( void *object, const char *index, const char *data )
{
	SilverLiningSky *sky = static_cast<SilverLiningSky*>( object );

	if(!dStrncmp(data, "NONE", 64))
		sky->SetPrecipitation(SilverLining::CloudLayer::NONE, -1.0f); 
	else if(!dStrncmp(data, "RAIN", 64))
		sky->SetPrecipitation(SilverLining::CloudLayer::RAIN, -1.0f); 
	else if(!dStrncmp(data, "DRY_SNOW", 64))
		sky->SetPrecipitation(SilverLining::CloudLayer::DRY_SNOW, -1.0f); 
	else if(!dStrncmp(data, "WET_SNOW", 64))
		sky->SetPrecipitation(SilverLining::CloudLayer::WET_SNOW, -1.0f); 
	else if(!dStrncmp(data, "SLEET", 64))
		sky->SetPrecipitation(SilverLining::CloudLayer::SLEET, -1.0f);  
	else
		Con::errorf("Precicitation Type not valid");

	return true; // ok to set value
}

bool SilverLiningSky::ptSetPrecipitationRate( void *object, const char *index, const char *data )
{
	SilverLiningSky *sky = static_cast<SilverLiningSky*>( object );
	F32 precipitationRate = dAtof(data);
	sky->SetPrecipitation(-1, precipitationRate); 

	return true; // ok to set value
}

void SilverLiningSky::_onSelected()
{
	Parent::_onSelected();
}

void SilverLiningSky::_onUnselected()
{
#ifdef TORQUE_DEBUG
	// Disable debug rendering on the light.
	if( isClientObject() )
		mLight->enableDebugRendering( false );
#endif

	Parent::_onUnselected();
}

void SilverLiningSky::setTime(const char *data)
{
	if(!isServerObject())
		return;

	mDateTime = data;

	S32 year, month, day, hour, min, sec; 

	S32 count = dSscanf( data, "%d %d %d %d %d %d", 
		&year, &month, &day,
		&hour, &min, &sec);

	if ( (count < 5) )
	{
		Con::printf("Failed to parse time information from '%s'", data);
		return;
	}

	if(	mYear == year && mMonth == month && mDay == day &&
		mHour == hour && mMin == min)
		return;

	mYear = year;
	mMonth = month;
	mDay = day;
	mHour = hour;
	mMin = min;

	mDirty = true;
	setMaskBits( TimeMask );

	inspectPostApply();
}

void SilverLiningSky::SetSilverLiningTime()
{
	if(!isClientObject())
		return;

	if(atm)
	{
		SilverLining::LocalTime tm;
		tm.SetYear(mYear);
		tm.SetMonth(mMonth);
		tm.SetDay(mDay);
		tm.SetHour(mHour);
		tm.SetMinutes(mMin);
		tm.SetSeconds(0);
		tm.SetObservingDaylightSavingsTime(true);
		tm.SetTimeZone(GMT);
		atm->GetConditions()->SetTime(tm);
	}

	mDirty = true;	// to update lights
}

void SilverLiningSky::ConformCloudsLayers()
{
	if(atm == NULL)
		return;

	for (S32 i = 0; i < NUM_CLOUD_TYPES; i++)
	{
		if(!mCloudLayerMustConform[i])
			continue;

		mCloudLayerMustConform[i] = false;

		// remove old cloud layer
		if(mCloudLayerHandle[i] > 0)
		{			
			atm->GetConditions()->RemoveCloudLayer(mCloudLayerHandle[i]);
			mCloudLayerHandle[i] = 0;
		}

		// is enabled?
		if(!mCloudLayerEnabled[i])
			continue;

		SilverLining::CloudLayer *cl = SilverLining::CloudLayerFactory::Create((CloudTypes)i);
		cl->SetBaseAltitude(mCloudLayerBaseAltitude[i]);
		cl->SetBaseLength(mCloudLayerBaseWidth[i]);
		cl->SetBaseWidth(mCloudLayerBaseWidth[i]);
		cl->SetThickness(mCloudLayerThickness[i]);
		cl->SetDensity(mCloudLayerDensity[i]);
		cl->SetLayerPosition(0, 0);

		cl->SetAlpha(0.8);

		switch (i)
		{
		case STRATUS:
			cl->SetIsInfinite(true);
			break;
		case CIRRUS_FIBRATUS:
			cl->SetThickness(0);
			cl->SetPrecipitation(SilverLining::CloudLayer::NONE, 0);
			break;
		case CUMULUS_MEDIOCRIS:
			cl->SetIsInfinite(true);
			break;
		case CUMULUS_CONGESTUS:
			cl->SetIsInfinite(true);
			cl->SetPrecipitation(SilverLining::CloudLayer::NONE, 0);
			cl->SetCloudAnimationEffects(0.1, false);
			cl->SetAlpha(0.8);
			cl->SetFadeTowardEdges(true);
			break;
		case CUMULONIMBUS_CAPPILATUS:
			cl->SetPrecipitation(SilverLining::CloudLayer::RAIN, 30);
			break;
		case STRATOCUMULUS:
			cl->SetAlpha(1.0);		
			cl->SetIsInfinite(true);
			cl->SetFadeTowardEdges(true);

			break;
		default:
			break;
		}

		// seed based on altitude
		srand((S32)mCloudLayerBaseAltitude[i]);
		cl->SeedClouds(*atm);

		mCloudLayerHandle[i] = atm->GetConditions()->AddCloudLayer(cl);

	}
}

void SilverLiningSky::setupClouds( S32 cloudType )
{	
	mCloudLayerUpdate[cloudType] = true;
	setMaskBits( UpdateMask );
}


void SilverLiningSky::SetPrecipitation(S32 precipitationType, F32 ratemmPerHour)
{
	if(!isServerObject())
		return;

	if(precipitationType>=0)
		mPrecipitationType = precipitationType;
	if(ratemmPerHour >= 0)
		mPrecipitationRate = ratemmPerHour;

	setMaskBits( PrecipitationUpdateMask );
}

void SilverLiningSky::ConformPrecipitations()
{
	if(!atm)
		return;

	atm->GetConditions()->SetPrecipitation( SilverLining::CloudLayer::NONE, 0.0f); //To clear all precipitation...
	if (mPrecipitationType != SilverLining::CloudLayer::NONE && mPrecipitationRate != 0)
		atm->GetConditions()->SetPrecipitation( mPrecipitationType, mPrecipitationRate);		
}

Point3F SilverLiningSky::getSunDir()
{
	bool isclient = isClientObject();
	return mSunDir;
}


// ConsoleMethods

ConsoleMethod( SilverLiningSky, SetPrecipitation, void, 3, 3, "SilverLiningSky.SetPrecipitation(PrecipitationType ratemmPerHour);")
{
	char strPrecipitationType[64];
	F32  ratemmPerHour = 0;

	S32 count = dSscanf( argv[2], "%s %f", 
		strPrecipitationType, &ratemmPerHour);

	if ( (count < 2) )
	{
		Con::printf("Failed to parse precipitation information from '%s'", argv[3]);
		return;
	}

	if(!dStrncmp(strPrecipitationType, "NONE", 64))
		object->SetPrecipitation(SilverLining::CloudLayer::NONE, 0); 
	else if(!dStrncmp(strPrecipitationType, "RAIN", 64))
		object->SetPrecipitation(SilverLining::CloudLayer::RAIN, ratemmPerHour); 
	else if(!dStrncmp(strPrecipitationType, "SNOW", 64))
		object->SetPrecipitation(SilverLining::CloudLayer::DRY_SNOW, ratemmPerHour); 
	else if(!dStrncmp(strPrecipitationType, "SLEET", 64))
		object->SetPrecipitation(SilverLining::CloudLayer::SLEET, ratemmPerHour);  
	else
		Con::errorf("Precicitation Type not valid");

}


DefineEngineFunction( GetSunDir, Point3F, (),,
	"Get the sun's world direction.\n"
	"@return the current direction of the sun\n" )
{	
	if(!SilverLiningSky::atm)
		return Point3F::Zero;

	F32 x,y,z;
	SilverLiningSky::atm->GetSunOrMoonPosition(&x, &y, &z);   
	return Point3F(x,-z,y);
}
