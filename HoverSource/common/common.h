
#define STRICT
#define D3D_OVERLOADS

#define APP_GAME
//#define APP_EDITOR

#include "myenum.h"

#include <stdio.h>
#include <d3d.h>
#include <d3dxcore.h>
#include <d3dxmath.h>
#include <dinput.h>
#include <dsound.h>
#include <windows.h>
#include <pbt.h>
#include <mmsystem.h>
#include <tchar.h>

#include "d3dutilheaders/D3DApp.h"
#include "d3dutilheaders/D3DUtil.h"
#include "d3dutilheaders/D3DMath.h"
#include "d3dutilheaders/D3DTextr.h"
#include "myfile.h"
#include "myframe.h"
#include "myres.h"



#define TRACK_DIRECTORY "../../common/tracks/"
#define TEXTURE_DIRECTORY "../../common/textures/"
#define MODEL_DIRECTORY "../../common/models/"
#define SOUND_DIRECTORY "../../common/sounds/"

/*
#define TRACK_DIRECTORY "../common/tracks/"
#define TEXTURE_DIRECTORY "../common/textures/"
#define MODEL_DIRECTORY "../common/models/"
#define SOUND_DIRECTORY "../common/sounds/"
*/
/*
#define TRACK_DIRECTORY "tracks/"
#define TEXTURE_DIRECTORY "textures/"
#define MODEL_DIRECTORY "models/"
#define SOUND_DIRECTORY "sounds/"
*/
/*
#define TRACK_DIRECTORY ""
#define TEXTURE_DIRECTORY ""
#define MODEL_DIRECTORY ""
#define SOUND_DIRECTORY ""
*/


#define GAMESTATE_DEMORACE				0
#define GAMESTATE_PLAYERRACE			1
#define GAMESTATE_POSTRACE				2

const FLOAT RACE_INTRO_DURATION[ 3 ] = { 7.5f, 3.0f };

#define TRANSITIONSTATE_FADEIN		0
#define TRANSITIONSTATE_NORMAL		1
#define TRANSITIONSTATE_FADEOUT		2

#define FADEIN_DURATION						1.0f
#define FADEOUT_DURATION					0.5f
#define POSTRACE_FORCED_DURATION	5.0f

#define VP_MINZ		1.0f
#define VP_MAXZ		100.0f // 110.0f

#ifdef APP_GAME
#define VERTEXBUFFERSIZE					0x7FFF
#else
#define VERTEXBUFFERSIZE					0xFFFF
#endif

#define GRAVITY		55.0f // 45.5f

#define MAX_RACERS								8

#define NAVCOORD_NOCOORD					5001


class CRectRegion {
public:
	INT32 startX, startZ;
	INT32 endX, endZ;

	void Set( INT32 sx, INT32 sz, INT32 ex, INT32 ez );
	void Clip( INT32 sx, INT32 sz, INT32 ex, INT32 ez );
};

#define CAMERAMODE_NORMAL							0
#define CAMERAMODE_FIXEDPOSITION			1
#define CAMERAMODE_ORBITRACER					2
#define CAMERAMODE_REARVIEW						3

#define CAMERA_MAXVEL									300.0f
#define CAMERA_MIN_GROUND_CLEARANCE		3.0f

#define NUM_FIXED_CAMERA_POSITIONS		9
#define NUM_NORMAL_CAMERA_OFFSETS			4

#define CAMERA_SWITCH_DELAY						0.4f

#define CAMERA_LEADDIST								16.0f



struct CAMERA {

	DWORD				mode;
	DWORD				newMode;
	FLOAT				switchBaseTime;
	BOOL				bReadyToSwitch;

	INT32				racerNum;
	D3DVECTOR		targetOffset;
	D3DVECTOR		offset;
	DWORD				offsetNum;

	DWORD				fixedPositionNum;
	FLOAT				nearestDist;
	DWORD				newFixedPositionNum;

	D3DVECTOR		pos;
	D3DVECTOR		dir;
	FLOAT				yaw;
};




#include "input.h"
#include "textures.h"
#include "track.h"
#include "model.h"
#include "racer.h"
#include "sound.h"

//-----------------------------------------------------------------------------
// Name: class MyApplication
// Desc: Main class to run this application.
//-----------------------------------------------------------------------------
class CMyApplication {
	
	CMyFramework7*	m_pFramework;
	BOOL            m_bActive;
	BOOL            m_bReady;
	BOOL						m_bAttemptingInitialization;
	
//	DWORD           m_dwBaseTime;
//	DWORD           m_dwStopTime;
	
	HRESULT Initialize3DEnvironment();
	
	HRESULT Update();
	VOID    Cleanup3DEnvironment();
	VOID    DisplayFrameworkError( HRESULT, DWORD );

protected:

	BOOL            m_bFrameMoving;
	BOOL            m_bSingleStep;

	FLOAT						m_misc; // used to get output to ShowStats()
	BOOL						m_bVSync;

	DWORD						gameState;
	DWORD						newGameState;
	DWORD						transitionState;
	FLOAT						transitionTime;

	FLOAT						startTime;
	FLOAT						pauseTime;
	FLOAT						finishTime;
	INT32						finishRacePosition;
	FLOAT						elapsedTime;
	FLOAT						deltaTime;

	INT32						numLaps;

	CAMERA					cam;
	D3DVECTOR				fixedCameraPositions[ NUM_FIXED_CAMERA_POSITIONS ];

	FLOAT						mouseX;
	FLOAT						mouseY;

	INT32						playerRacerNum;
	BOOL						bPlayerPitchAssist;
	FLOAT						playerPitchAssistLevel;
    BOOL                        bAutopilot;
	DWORD						numRacers;
	CRacer					racers[ MAX_RACERS ];

	INT32						aiBehavior;

	BOOL						bRenderShadowTexture;
	BOOL						bTestShadow;
    BOOL                        bDisplayShadowTexture; // for debugging
	SHADOWTEXTURE		shadowTexture;
	D3DMATERIAL7		m_defaultMtrl;

	TRACK						track;
    BOOL                        bWireframeTerrain;
    BOOL                        bDisplayLiftLines;

	HWND                 m_hWnd;
	D3DEnum_DeviceInfo*  m_pDeviceInfo;
	LPDIRECTDRAW7        m_pDD;
	LPDIRECT3D7          m_pD3D;
	LPDIRECT3DDEVICE7    m_pd3dDevice;
	LPDIRECTDRAWSURFACE7 m_pddsRenderTarget;
	LPDIRECTDRAWSURFACE7 m_pddsRenderTargetLeft; // For stereo modes
	LPDIRECTDRAWSURFACE7 m_pddsDepthBuffer;
	DDSURFACEDESC2       m_ddsdRenderTarget;

	D3DVIEWPORT7				 m_vp;
	D3DMATRIX						 m_matProj;
	D3DMATRIX						 m_matView;

//	D3DMATRIX m_matView;
//	VOID    SetAppViewMatrix( D3DMATRIX mat )      { m_matView      = mat; }
//	VOID    SetViewParams( D3DVECTOR* vEyePt, D3DVECTOR* vLookatPt,
//	D3DVECTOR* vUpVec, FLOAT fEyeDistance );
	
	TCHAR*               m_strWindowTitle;
	BOOL                 m_bAppUseZBuffer;
	BOOL                 m_bAppUseStereo;
	BOOL                 m_bShowStats;
	HRESULT              (*m_fnConfirmDevice)(DDCAPS*, D3DDEVICEDESC7*);
	


	HRESULT RestoreSurfaces()      { return S_OK; }

	HRESULT	VideoOptions();
	HRESULT ForcedVideoOptions();

	virtual	HRESULT	Init() { return S_OK; }
	HRESULT InitDeviceObjects();
	HRESULT	InitRenderShadowTexture();
	BOOL		TestRenderShadowTexture();
	HRESULT InitShadowTexture();
	HRESULT	InitStats();
	HRESULT InitVehicleModels();
	HRESULT	InitTrack();
	HRESULT	InitTriRecur();
	VOID		InitFixedCameraPositions();
//	HRESULT	FinalizeTerrainLOD();

	HRESULT	UpdateGameState();
	HRESULT	RestartRace();

	HRESULT	InitRacers();
	VOID		InitRacerLiftThrustArrays();

	virtual	BOOL HandleKeyInputEvent( WPARAM wParam ) { return FALSE; }
	HRESULT GetUserInput( CRacer* );

	VOID		UpdateStreamingSoundBuffer();

	virtual	HRESULT	Move() { return S_OK; }

	WORD		GetNavCoordAt( INT32 x, INT32 z );

	VOID		GetAIInput( CRacer* );
	VOID		GetAIPitchRollInput( CRacer*, FLOAT, FLOAT );
	VOID		RacerTerrainCollide( CRacer* pRacer, FLOAT deltaTime );
	VOID		UpdateRacers();
	VOID		UpdateRacerNav( CRacer* );
	VOID		UpdateRaceState();

	FLOAT		DistToTerrain( D3DVECTOR vPosition, D3DVECTOR vDir, const FLOAT MAXDIST );

	HRESULT	CastRacerShadow( CRacer* );
	HRESULT	RenderToShadowTexture( CModel* pModel );
	HRESULT	ApplyShadowTexture( FLOAT, FLOAT, FLOAT );

	VOID		UpdateTerrainLighting( CRectRegion );

	VOID		SetCamera( FLOAT );
	VOID		SetVisibleTerrainCoords();

	HRESULT	RenderTerrain();
	BOOL		IsRacerVisible( CRacer* );
	HRESULT	RenderRacers();
	HRESULT Render();

	HRESULT	DisplayStats();
	HRESULT	DisplayNavMap();
	VOID		DisplayStatNavCoord( DWORD curr );
	VOID    ShowStats();
	VOID    OutputText( DWORD x, DWORD y, TCHAR* str );

	VOID		ReleaseTrack();
	VOID		ReleaseShadowTexture();
	VOID		DeleteDeviceObjects();
	virtual	VOID FinalCleanup() {}

	HRESULT	SetTerrainLOD( FLOAT ); // used only by editor

	// power management (APM) functions
	LRESULT OnQuerySuspend( DWORD dwFlags );
	LRESULT OnResumeSuspend( DWORD dwData );
	
public:

	// Functions to create, run, pause, and clean up the application
	HRESULT Create( HINSTANCE, TCHAR* );

	INT			Run();
	LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	VOID    Pause( BOOL bPause );

	HRESULT Change3DEnvironment();

	VOID		DisplayAlert( HRESULT* phr, CHAR* desc );

	LPDIRECT3DDEVICE7 GetD3DDevice() { return m_pd3dDevice; }
	D3DEnum_DeviceInfo*  GetDeviceInfo(){ return m_pDeviceInfo; }
	HWND GetWindowHandle(){ return m_hWnd; }
	BOOL GetActiveAndReady(){ return m_bActive && m_bReady; }
	CAMERA GetCam(){ return cam; }
	D3DVIEWPORT7 GetViewport(){ return m_vp; }

	CMyApplication();
};





#define SAFE_ARRAYDELETE( p )		if( p ) delete [] p

HRESULT SetViewMatrix( D3DMATRIX& mat, D3DVECTOR& vFrom,
                               D3DVECTOR& vAt, D3DVECTOR& vWorldUp );

FLOAT DistToCam( D3DVECTOR pos );

VOID MyAlert( HRESULT*, CHAR* );

// VOID OutputToLog( CHAR* msg );

WORD GetNumberOfBits( DWORD dwMask );


