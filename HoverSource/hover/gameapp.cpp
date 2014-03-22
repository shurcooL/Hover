

#include "../common/common.h"
#include "hover.h"


CGameApplication::CGameApplication()
								:CMyApplication(){

	m_strWindowTitle	= TEXT( "Hover" );
}




HRESULT CGameApplication::Init(){

	HRESULT hr;	

	// OutputToLog( "Initializing DirectInput..." );

	if( FAILED( hr = InitDirectInput( m_hWnd ) ) ){
		MyAlert(&hr, "Unable to initialize DirectInput" );
		return hr;
	}

	// OutputToLog( "Done\n" );

	// OutputToLog( "Initializing DirectSound..." );

	if( FAILED( hr = InitDirectSound( m_hWnd ) ) ){
		MyAlert(&hr, "Unable to initialize DirectSound" );
		return hr;
	}

	if( FAILED( AddSound( "thrust.wav", MAX_RACERS, TRUE, 35.0f, 
		DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY ) ) ){
		return E_FAIL;
	}

	if( FAILED( AddSound( "lift.wav", MAX_RACERS, TRUE, 25.0f,
		DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY ) ) ){
		return E_FAIL;
	}

	if( FAILED( AddSound( "crash1.wav", MAX_RACERS, FALSE, 25.0f,
		DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME ) ) ){
		return E_FAIL;
	}

	if( FAILED( AddSound( "crash2.wav", MAX_RACERS, FALSE, 25.0f,
		DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME ) ) ){
		return E_FAIL;
	}

	if( FAILED( LoadWaveFile( "hypnotic.wav" ) ) ){
		return E_FAIL;
	}
	AdjustStreamingSoundBuffer( 0.0f );

	// OutputToLog( "Done\n" );

	// OutputToLog( "Loading track..." );

	track.trackNum = 1;
	if( FAILED( InitTrack() ) ){
		return E_FAIL;
	}
	track.bgColor = D3DRGBA( 0.73, 0.69, 0.47, 1.0 ); // add to track file header
	track.bgColor &= 0xFFF8F8F8; // better 16bpp compatibility

	// OutputToLog( "Done\n" );

	// OutputToLog( "Loading vehicle models..." );

	if( FAILED( InitVehicleModels() ) ){
		return E_FAIL;
	}

	// OutputToLog( "Done\n" );

	if( FAILED( AddStatsTexture() ) ){
		return E_FAIL;
	}

	numRacers = MAX_RACERS;

	numLaps = 3;

	aiBehavior = AIBEHAVIOR_RACE;

	InitFixedCameraPositions();

	//cam.mode = CAMERAMODE_NORMAL;

	// remember there is similar code to this, in myapp.cpp, after init()
	newGameState = GAMESTATE_DEMORACE;
	transitionState = TRANSITIONSTATE_FADEOUT;
	transitionTime = 0.0f;
	
	return S_OK;
}





BOOL CGameApplication::HandleKeyInputEvent( WPARAM wParam ){
	
	switch (wParam){

	case 0x1B: // ESC // undo

		if( transitionState == TRANSITIONSTATE_NORMAL ){
			newGameState = GAMESTATE_DEMORACE;
			transitionState = TRANSITIONSTATE_FADEOUT;
			transitionTime = FADEOUT_DURATION;
		}
		break;


///// REMOVE!!! ///////////
///////////////////////////

	case 0x72: // F3
		// Toggle frame movement
					m_bFrameMoving = !m_bFrameMoving;
						
					if( m_bFrameMoving ){
						startTime += timeGetTime() * 0.001f - pauseTime;
						pauseTime = timeGetTime() * 0.001f;
					}
					else
						pauseTime = timeGetTime() * 0.001f;
					break;
						
	case 0x73: // F4
	// Single-step frame movement
					if( FALSE == m_bFrameMoving )
						startTime += timeGetTime() * 0.001f - ( pauseTime + 0.02f );
						
					pauseTime	 = timeGetTime() * 0.001f;
					m_bFrameMoving = FALSE;
					m_bSingleStep  = TRUE;
					break;
////////////////////////////
///////////////////////////

//	case 0x20: // SPACE
//		break;

//	case 0x72: // F3
//		break;

//	case 0x73: // F4
//		break;
/*		
	case 0x56: // v // toggle v-sync
		m_bVSync = !m_bVSync;
		break;

	case 0x53: // s // toggle shadow mode & override test
		// Once into Change3DEnvironment() (below), shadow release won't be done correctly
		ReleaseShadowTexture();
		shadowTexture.pddsSurface = NULL;

		bTestShadow = FALSE;
		bRenderShadowTexture = !bRenderShadowTexture;
		Change3DEnvironment();
		break;
*/
	case 0x41: // a // toggle pitch assist
		// bPlayerPitchAssist = !bPlayerPitchAssist;
        bAutopilot = !bAutopilot;
		break;

	case 0x42: // b
		aiBehavior = ( aiBehavior + 1 ) % 3;
		break;

	case 0x43: // c // toggle cam offset
		cam.offsetNum = ( cam.offsetNum - 1 + NUM_NORMAL_CAMERA_OFFSETS ) % NUM_NORMAL_CAMERA_OFFSETS;
		break;

    case 0x57: // w // toggle wireframe
        bWireframeTerrain = !bWireframeTerrain;
        break;

    case 0x53: // s // toggle display shadow texture
        bDisplayShadowTexture = !bDisplayShadowTexture;
        break;

    case 0x4C: // l
        bDisplayLiftLines = !bDisplayLiftLines;
        break;

	default:

		if( transitionState == TRANSITIONSTATE_NORMAL ){
			if( gameState == GAMESTATE_DEMORACE ||
				( gameState == GAMESTATE_POSTRACE && elapsedTime - finishTime > POSTRACE_FORCED_DURATION ) ){
				newGameState = GAMESTATE_PLAYERRACE;
				transitionState = TRANSITIONSTATE_FADEOUT;
				transitionTime = FADEOUT_DURATION;
				break;
			}
		}

		return FALSE; // let Windows handle other keys
	}

	return TRUE;
}





//-----------------------------------------------------------------------------
// Name: FrameMove()
//-----------------------------------------------------------------------------
HRESULT CGameApplication::Move(){

	if( playerRacerNum != -1 && !bAutopilot ){
		GetUserInput( &racers[playerRacerNum] );
	}
	else{
		GetUserInput( NULL );
	}

	UpdateRaceState();

	UpdateStreamingSoundBuffer();

	UpdateRacers();

	SetCamera( deltaTime ); // updates listener, too

	SetVisibleTerrainCoords();

	return S_OK;
}





VOID CGameApplication::FinalCleanup(){

	ReleaseTrack();

	DestroyTextures();

	DestroyModels();

	FreeDirectSound();

	FreeDirectInput();
}
