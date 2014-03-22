
#include "../common/common.h"
#include "editor.h"

const CHAR terrTypeTextureFilenames[][32] = { "dirt.bmp", "sand.bmp", "finishline.bmp" };
FLOAT terrTypeSlopeThresholds[3] = { 0.75, 0.75, 0.75 };
const BYTE toSlopeTerrType[] = { 0, 0, 0 };

LONG LOD_SETTING = 100;
     
CEdApplication::CEdApplication()
							:CMyApplication(){

	m_strWindowTitle	= TEXT( "Hover Terrain Editor" );

	modInProgress = FALSE;

	isEditing = TRUE;
	numRacers = 0;
	playerRacerNum = 0;
	bPlayerPitchAssist = FALSE;

	cursorZ = cursorX = 0;
	oldCursorX = cursorX;
	oldCursorZ = cursorZ;

	camDirection = 0;
	camPitch = 5.0f / 16 * g_PI;
	camDist = 60;

	currRadius = ED_DEFAULT_MOD_RADIUS;
	currPlateauRadius = ED_DEFAULT_MOD_PLATEAU_RADIUS;

	for(DWORD i=0; i < ED_NUM_UNDO_STEPS; i++){
		undoSteps[ i ].buffer = NULL;
	}
	modCount = 0;
	undoBuffersMemUsage = 0;

	terrBaseTypes = NULL;
	terrDisplayTypes = NULL;
}





HRESULT CEdApplication::Init(){

	HRESULT hr;	

	if( FAILED( hr = InitDirectInput( m_hWnd ) ) ){
		MyAlert(&hr, "Init() > InitDirectInput() failed" );
		return hr;
	}

/*
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
*/


	track.trackNum = 1;
	if( FAILED( EdInitTrack() ) ){
		return E_FAIL;
	}
	track.bgColor = D3DRGBA( 0.73, 0.69, 0.47, 1.0 ); // add to track file header
	track.bgColor &= 0xFFF8F8F8; // better 16bpp compatibility

	if( FAILED( InitVehicleModels() ) ){
		return E_FAIL;
	}


	// OutputToLog( "Done\n" );

	if( FAILED( AddStatsTexture() ) ){
		return E_FAIL;
	}

	numRacers = 0;

	numLaps = 3;

	InitFixedCameraPositions();

	//cam.mode = CAMERAMODE_NORMAL;

	// remember there is similar code to this, in myapp.cpp, after init()
	newGameState = GAMESTATE_DEMORACE;
	transitionState = TRANSITIONSTATE_FADEOUT;
	transitionTime = 0.0f;

	numLaps = 10;
	playerRacerNum = 0;
	racers[0].handicap = 0;
	cam.racerNum = 0; // playerRacerNum;

	return S_OK;
}




VOID CEdApplication::FinalCleanup(){

//	CreateGameTerrainTypesBitmap(); // needed as reference to make nav bitmap

	for(DWORD i=0; i < ED_NUM_UNDO_STEPS; i++){
		SAFE_DELETE( undoSteps[ i ].buffer );
	}

	ReleaseTrack();

	DestroyTextures();

	DestroyModels();

	//FreeDirectSound();

	FreeDirectInput();

}





//-----------------------------------------------------------------------------
// Name: Move()
// Desc: Called once per frame, the call is the entry point for animating
//			 the scene.
//-----------------------------------------------------------------------------
HRESULT CEdApplication::Move(){

	//UpdateStreamingSoundBuffer();

	if( !isEditing ){

		if( playerRacerNum != -1 ){
			GetUserInput( &racers[playerRacerNum] );
		}
		else{
			GetUserInput( NULL );
		}

		EdUpdateRacers();

		SetCamera( deltaTime );
	}
	else{

		racers[0].pos.x = cursorX;
		racers[0].pos.z = cursorZ;

		if( modInProgress ){
			DisplayMod();
		}

		EdSetCamera();
	}

//	SetCamera( deltaTime );

	SetVisibleTerrainCoords();

	if( isEditing ){
		UpdateTerrainTypeRuns( track.renderedTerrCoords, TRUE );
	}

	return S_OK;
}





VOID CEdApplication::EdUpdateRacers(){

	for(DWORD i=0; i < numRacers; i++){


		if( playerRacerNum != i ){
			GetAIInput( &racers[i] );
		}

	 	if( elapsedTime > 0.0f ){
			racers[i].ProcessInput( deltaTime );
		}
		racers[i].SetRotationMatrix();

		for(DWORD otherRacer=i+1; otherRacer < numRacers; otherRacer++){
			racers[i].RacerCollide( deltaTime, &racers[otherRacer] );
		}

		racers[i].Move( deltaTime, &track );
		RacerTerrainCollide( &racers[i], deltaTime );
		//UpdateRacerNav( &racers[i] );
		racers[i].UpdateSounds();
	}
}





VOID CEdApplication::EdSetCamera(){

	D3DMATRIX matProj, matSet, matInvSet;

	cam.pos = D3DVECTOR( 
		cursorX - cosf( camPitch ) * cosf( camDirection ) * camDist,
		track.terrCoords[cursorZ * track.width + cursorX].height * TERR_HEIGHT_SCALE + sinf( camPitch ) * camDist * TERR_SCALE,
		cursorZ - cosf( camPitch ) * sinf( camDirection ) * camDist );

	D3DVECTOR vLookatPt = D3DVECTOR(
		cursorX * TERR_SCALE,
		track.terrCoords[cursorZ * track.width + cursorX].height * TERR_HEIGHT_SCALE,
		cursorZ * TERR_SCALE );

	D3DVECTOR vUpVec = D3DVECTOR( 0.0f, 1.0f, 0.0f );

	SetViewMatrix(
		m_matView,
		cam.pos,
		vLookatPt,
		vUpVec );

	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &m_matView );

	cam.dir = Normalize( vLookatPt - cam.pos );
}



BOOL CEdApplication::HandleKeyInputEvent( WPARAM wParam ){
	
	FLOAT temp;

	switch (wParam){
	case 0x28: // DOWN ARROW
		if( cursorZ - 1 >= 0 ){
			cursorZ--;
		}
		break;
	case 0x26: // UP ARROW
		if( cursorZ + 1 <= track.depth - 1 ){
			cursorZ++;
		}
		break;
	case VK_NEXT:
		if( cursorZ - 8 >= 0 ){
			cursorZ -= 8;
		}
		break;
	case VK_PRIOR: // 0x12 // page up
		if( cursorZ + 8 <= track.depth - 1 ){
			cursorZ += 8;
		}
		break;
	case 0x25: // LEFT ARROW
		if( cursorX - 1 >= 0 ){
			cursorX--;
		}
		break;
	case 0x27: // RIGHT ARROW
		if( cursorX + 1 <= track.width - 1 ){
			cursorX++;
		}
		break;
	case VK_HOME:
		if( cursorX - 8 >= 0 ){
			cursorX -= 8;
		}
		break;
	case VK_END:
		if( cursorX + 8 <= track.width - 1 ){
			cursorX += 8;
		}
		break;
	case 0xBC: // ',' // LEFT ARROW BRACKET
		if( currSegment > 0 && modInProgress ){
			currSegment--;
		}
		else{
			//MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
	case 0xBE: // '.' // RIGHT ARROW BRACKET
		if( currSegment < ED_MAX_MOD_SEGMENTS && modInProgress ){
			currSegment++;
		}
		else{
			//MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
	case 0xDB: // '['
		if( currPlateauRadius > 0 && modInProgress ){
			currPlateauRadius--;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
	case 0xDD: // ']'
		if( modInProgress ){
			currPlateauRadius++;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;

	case 0x44: // 'd'
		if( camPitch < ED_MAX_PITCH ){
			camPitch += ED_PITCH_INC;
		}
		break;
	case 0x58: // 'x'
		if( camPitch > ED_MIN_PITCH ){
			camPitch -= ED_PITCH_INC;;
		}
		break;
	case 0x5A: // 'z'
		camDirection -= ED_DIRECTION_INC;
		break;
	case 0x43: // 'c'
		camDirection += ED_DIRECTION_INC;
		break;
	case 0x53: // s
		camDist += 5.0f;
		break;
	case 0x41: // a
		if( camDist > 5.0f ){
			camDist -= 5.0f;
		}
		break;
	
	case 0x68: // NUMPAD UP ARROW
		if( currHeight + 8 <= MAX_TERRAIN_HEIGHT && modInProgress ){
			currHeight += 8;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;

	case 0x62: // NUMPAD DOWN ARROW
		if( currHeight - 8 >= 0 && modInProgress ){
			currHeight -= 8;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
	case VK_ADD:
		if( currHeight + 1 <= MAX_TERRAIN_HEIGHT && modInProgress ){
			currHeight++;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;

	case VK_SUBTRACT:
		if( currHeight - 1 >= 0 && modInProgress ){
			currHeight--;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
	case 0x64: // NUMPAD LEFT ARROW
		if( currRadius > 1 && modInProgress ){
			currRadius--;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
	case 0x66: // NUMPAD RIGHT ARROW
		if( modInProgress ){
			currRadius++;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;
		
	case 0x4C: // 'l' // start mod (probably point or line)
		if( !modInProgress ){
			modInProgress = TRUE;
			currSegment = 0;
			currHeight = track.terrCoords[ cursorZ * track.width + cursorX ].height;
			currPlateauRadius = 0;
			modCount++;
			modRegion.Set( cursorX, cursorZ, cursorX,	cursorZ );
			SAFE_DELETE( undoSteps[modCount % ED_NUM_UNDO_STEPS].buffer );
			undoSteps[modCount % ED_NUM_UNDO_STEPS].buffer = NULL;
		}
		break;
	case 0x50: // 'P' // start mod, initialized as plateau
		if( !modInProgress ){
			modInProgress = TRUE;
			currSegment = 0;
			currHeight = track.terrCoords[ cursorZ * track.width + cursorX ].height;
			if( !currPlateauRadius ){
				currPlateauRadius = ED_DEFAULT_MOD_PLATEAU_RADIUS;
			}
			modCount++;
			modRegion.Set( cursorX, cursorZ, cursorX,	cursorZ );
			SAFE_DELETE( undoSteps[modCount % ED_NUM_UNDO_STEPS].buffer );
			undoSteps[modCount % ED_NUM_UNDO_STEPS].buffer = NULL;
		}
		break;
	case 0x1B: // ESC // undo
		if( UndoMod() ){
			UpdateTerrainLightingAndTypes( undoSteps[ modCount % ED_NUM_UNDO_STEPS ].region );
			modCount--;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		modInProgress = FALSE;
		break;
		
	case 0x20: // SPACE
		if( modInProgress ){
			currSegment++;
			modInProgress = FALSE;
			return TRUE;
		}
		else{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		break;

	case 0x72: // F3
		SaveTrack();
		CreateTerrainBitmap( terrDisplayTypes , "gametypes" , "track1types" );
		break;
/*
	case 0x73: // F4
		//CreateNavStuff( TRUE );
		//SaveTrack();
		CreateTerrainBitmap( terrDisplayTypes , "gametypes" , "track1types" );
		CreateNavMap();
		break;
*/
/*
	case 0x77: // F8
        
		temp = timeGetTime() * 0.001f;
		if( FAILED( CreateNavStuff() ) )
		{
			MessageBeep( MB_ICONEXCLAMATION );
		}
		m_misc = timeGetTime() * 0.001f - temp;
		SaveTrack();
		//CreateTerrainBitmap( terrDisplayTypes , "gametypes" , "track1types" );
		break;
*/
	case 0x74: // F5 // toggle play/edit mode
		if( isEditing ){
			if( modInProgress ){ // finalize a mod in progress
				currSegment++;
				modInProgress = FALSE;
			}
			UpdateTerrainTypeRuns( track.entireRegion, FALSE );
			isEditing = FALSE;
			numRacers=1;
			FLOAT height = track.terrCoords[cursorZ * track.width + cursorX].height * TERR_HEIGHT_SCALE;
			racers[0].Init( D3DVECTOR( cursorX, height + 5, cursorZ ), D3DVECTOR( 0,0,0 ),
				camDirection, 0, 0, 0, 0 );
			racers[0].currNavCoord = 0;
			racers[0].currLap = -1;
			cam.mode = CAMERAMODE_NORMAL;
			playerRacerNum = cam.racerNum = 0;
			cam.offset = D3DVECTOR(
				-50 * cosf( racers[cam.racerNum].yaw ),
				10,
				-50 * sinf( racers[cam.racerNum].yaw ) );
			StopBuffer();

			gameState = GAMESTATE_PLAYERRACE;
			transitionState = TRANSITIONSTATE_NORMAL;
			transitionTime = 0.0f;
		}
		else{
			isEditing = TRUE;
			numRacers=0;
			cursorX = racers[0].pos.x;
			cursorZ = racers[0].pos.z;
//			typeUnderCursor = terrDisplayTypes[cursorZ * track.width + cursorX];
//			oldCursorX = cursorX;
//			oldCursorZ = cursorZ;
		}
		break;

	case 0x75: // F6 // toggle player # between 0 and 1
		if( playerRacerNum == 0 ){
			playerRacerNum = 1;
		}
		else{
			playerRacerNum = 0;
		}
		break;
/*	
	case 0x76: // F7 // toggle cam mode
		if( cam.mode == CAMERAMODE_NORMAL ){
			cam.mode = CAMERAMODE_FACINGVEHICLE;
		}
		else{
			cam.mode = CAMERAMODE_NORMAL;
		}
		break;
*/
	case 0x56: // toggle v-sync
		m_bVSync = !m_bVSync;
		break;

	//case 0x43: // c // toggle cam offset
//		cam.offsetNum = ( cam.offsetNum + 1 ) % NUM_NORMAL_CAMERA_OFFSETS;
//		break;

    case 0x57: // w // toggle wireframe
        bWireframeTerrain = !bWireframeTerrain;
        break;

    case 0x54: // t // increase slope threshhold
        if( terrTypeSlopeThresholds[1] < 1.0f )
        {
            terrTypeSlopeThresholds[1] += 0.05f;
        }
        break;
    case 0x59: // y // decrease slope threshhold
        if( terrTypeSlopeThresholds[1] > 0.0f )
        {
            terrTypeSlopeThresholds[1] -= 0.05f;
        }
        break;
/*
    case 0x47: // g
        if( LOD_SETTING < 100000 )
        {
            LOD_SETTING += 100;
        }
        break;
    case 0x48: // h
        if( LOD_SETTING > -100 )
        {
            LOD_SETTING -= 100;
        }
        break;
*/
	case 0x47: // g
        SetTerrainLOD( LOD_SETTING );
        break;

	case 0x48: // h
        SetTerrainLOD( -100 );
        break;

	default:
		return FALSE; // let Windows handle other keys
	}

	if( modInProgress ){
		UpdateModInProgress();
	}

	return TRUE;
}




VOID CEdApplication::UpdateModInProgress(){
	
	modCoords[currSegment].x = cursorX;
	modCoords[currSegment].z = cursorZ;
	modCoords[currSegment].height = currHeight;
}




BOOL CEdApplication::UndoMod(){

	DWORD wz, wx, bz, bx;
	UNDOSTEP* pUndoStep;

	pUndoStep = &undoSteps[ modCount % ED_NUM_UNDO_STEPS ];
	if( pUndoStep->buffer != NULL ){

		DWORD bufferWidth = pUndoStep->region.endX - pUndoStep->region.startX + 1;
		for(bz=0, wz=pUndoStep->region.startZ;
		wz <= pUndoStep->region.endZ; bz++, wz++){
			for(bx=0, wx=pUndoStep->region.startX;
			wx <= pUndoStep->region.endX; bx++, wx++){
				track.terrCoords[ wz * track.width + wx ].height = 
					pUndoStep->buffer[ bz * bufferWidth + bx ];
			}
		}
	
		delete pUndoStep->buffer;
		pUndoStep->buffer = NULL;

		return TRUE;
	}
	return FALSE;
}




VOID CEdApplication::CreateUndoStep(){

	DWORD wz, wx, bz, bx;
	
	UNDOSTEP* pUndoStep;
	UNDOSTEP* pOldestUndoStep;

	pUndoStep = &undoSteps[ modCount % ED_NUM_UNDO_STEPS ];

	SAFE_DELETE( pUndoStep->buffer );

	pUndoStep->region = modRegion;

	pUndoStep->bufferSize = ( pUndoStep->region.endZ - pUndoStep->region.startZ + 1 ) *
		( pUndoStep->region.endX - pUndoStep->region.startX + 1 );

	if( NULL == ( pUndoStep->buffer = new WORD[pUndoStep->bufferSize] ) ){
		MyAlert( NULL, "CreateUndoStep() > memory allocation failed" );
		return;
	}

	DWORD bufferWidth = pUndoStep->region.endX - pUndoStep->region.startX + 1;
	for(bz=0, wz=pUndoStep->region.startZ;
	wz <= pUndoStep->region.endZ; bz++, wz++){
		for(bx=0, wx=pUndoStep->region.startX;
		wx <= pUndoStep->region.endX; bx++, wx++){
			pUndoStep->buffer[ bz * bufferWidth + bx ] = 
				track.terrCoords[ wz * track.width + wx ].height;
		}
	}

}




VOID CEdApplication::DisplayMod(){
	
	UNDOSTEP* pUndoStep;
	MODCOORD* pmc;
	CRectRegion unionRegion;
	DWORD i;
	DWORD numSegments = currSegment + 1;
	INT32 x,z;

	// undo mod from last mod update
	UndoMod();

	unionRegion = modRegion; // will be union of old & new modRegion

	// calc region affected by this mod
	modRegion.Set( cursorX, cursorZ, cursorX,	cursorZ );
	for(i=0; i < numSegments; i++){
		pmc = &modCoords[i];
		if( pmc->x - currRadius - currPlateauRadius < modRegion.startX ){
			modRegion.startX = pmc->x - currRadius - currPlateauRadius;
		}
		if( pmc->x + currRadius + currPlateauRadius > modRegion.endX ){
			modRegion.endX = pmc->x + currRadius + currPlateauRadius;
		}
		if( pmc->z - currRadius - currPlateauRadius < modRegion.startZ ){
			modRegion.startZ = pmc->z - currRadius - currPlateauRadius;
		}
		if( pmc->z + currRadius + currPlateauRadius > modRegion.endZ ){
			modRegion.endZ = pmc->z + currRadius + currPlateauRadius;
		}
	}

	modRegion.Clip( 0, 0, track.width - 1, track.depth - 1 );

	// create new undo step
	CreateUndoStep();

	if( numSegments == 1 ){
		DisplayPoint( modCoords[0] );
	}
	else{
		for(i=0; i < numSegments - 1; i++){
			DisplayLine( modCoords[i], modCoords[i+1] );
		}
	}

	if( modRegion.startX < unionRegion.startX ){
		unionRegion.startX = modRegion.startX;
	}
	if( modRegion.startZ < unionRegion.startZ ){
		unionRegion.startZ = modRegion.startZ;
	}
	if( modRegion.endX > unionRegion.endX ){
		unionRegion.endX = modRegion.endX;
	}
	if( modRegion.endZ > unionRegion.endZ ){
		unionRegion.endZ = modRegion.endZ;
	}

	UpdateTerrainLightingAndTypes( unionRegion );

}




VOID CEdApplication::DisplayPoint( MODCOORD p ){ 

	FLOAT dist, percent; 
	INT32 totalRadius = currRadius + currPlateauRadius;
	INT32 x,z;

	for(z=modRegion.startZ; z <= modRegion.endZ; z++){
		for(x=modRegion.startX; x <= modRegion.endX; x++){
			dist = sqrt( ( p.x - x ) * ( p.x - x ) + ( p.z - z ) * ( p.z - z ) );
			if( dist < totalRadius ){
				if( dist <= currPlateauRadius ){
					track.terrCoords[ z * track.width + x ].height = p.height;
				}
				else{
					dist -= currPlateauRadius;
					percent = ( cosf( dist / currRadius * g_PI ) + 1.0f ) / 2;
					track.terrCoords[ z * track.width + x ].height +=
						( p.height - track.terrCoords[ z * track.width + x ].height ) * percent;
				}
			}
		}
	}
}




VOID CEdApplication::DisplayLine( MODCOORD p1, MODCOORD p2 ){
	
	FLOAT dist, percent; 
	INT32 totalRadius = currRadius + currPlateauRadius;
	FLOAT length;
	FLOAT transX, transZ;
	FLOAT sinAngle, cosAngle;
	FLOAT interpolatedHeight;
	INT32 x,z;

	length = sqrt( ( p2.x - p1.x ) * ( p2.x - p1.x ) + ( p2.z - p1.z) * ( p2.z - p1.z ) );
	
	if( length == 0 ){
		DisplayPoint( p1 );
		return;
	}

	sinAngle = ( p2.z - p1.z ) / length;
	cosAngle = ( p2.x - p1.x ) / length;
	
	for(z=modRegion.startZ; z <= modRegion.endZ; z++){
		for(x=modRegion.startX; x <= modRegion.endX; x++){
					
			transX = ( x - p1.x ) * cosAngle + ( z - p1.z ) * sinAngle;
			transZ = ( z - p1.z ) * cosAngle - ( x - p1.x ) * sinAngle;
					
			if( transX > -totalRadius && transX < length + totalRadius ){
				if( transX < 0 ){
					dist = sqrt( transX * transX + transZ * transZ );
				}
				else if( transX > length ){
					dist = sqrt( ( transX - length ) * ( transX - length ) + transZ * transZ );
				}
				else{ // if between line endpoints
					dist = fabs( transZ );
				}
				
				if( dist < totalRadius ){
					interpolatedHeight = p1.height + ( p2.height - p1.height ) * ( transX / length );
					if( dist <= currPlateauRadius ){
						track.terrCoords[ z * track.width + x ].height = interpolatedHeight;
					}
					else{
						dist -= currPlateauRadius;
						percent = ( cosf( dist / currRadius * g_PI ) + 1.0f ) / 2;
						track.terrCoords[ z * track.width + x ].height +=
							( interpolatedHeight - track.terrCoords[ z * track.width + x ].height ) * percent;
					}
				}
			}
		}
	}
}


WORD SwitchEndian( WORD OldVal )
{
    WORD MaskOld = 0x8000;
    WORD MaskNew = 0x0001;
    WORD NewVal = 0;
    DWORD Bit;

    for( Bit = 0; Bit < 16; Bit++, MaskOld >>= 1, MaskNew <<= 1 )
    {
        NewVal |= ( OldVal & MaskOld ) ? MaskNew : 0;
    }
    return NewVal;
}











HRESULT CEdApplication::EdInitTrack(){

	CHAR filepath[256];
	HANDLE trackFile;
	DWORD numBytes, i, x, z;
	TRACKFILEHEADER header;
	CHAR desc[256]; // for error messages
	

	// get track.terrTypes and track dimensions from bitmap
	sprintf( filepath, "%strack%dtypes.bmp", TRACK_DIRECTORY, track.trackNum );
	
	HBITMAP hbm;
  if( !( hbm = (HBITMAP)LoadImage( NULL, filepath, IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE|LR_CREATEDIBSECTION ) ) ){
		sprintf( desc, "EdInitTrack() >  Could not load image:\n\n%s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}

	BITMAP bm;
	GetObject( hbm, sizeof(BITMAP), &bm );

	track.width = bm.bmWidth;
	track.depth = bm.bmHeight;
	track.numTerrCoords = track.width * track.depth;
	track.entireRegion.Set( 0, 0, track.width - 1, track.depth - 1 );

	if( track.width % 16 != 1 || track.depth % 16 != 1 ){
		MyAlert( NULL, "EdInitTrack() > Track dimensions derived from types bitmap are invalid" );
		return E_FAIL;
	}

	track.numTerrCoords = track.width * track.depth;

	track.triGroupsWidth = ( track.width - 1 ) >> TRIGROUP_WIDTHSHIFT;
	track.triGroupsDepth = ( track.depth - 1 ) >> TRIGROUP_WIDTHSHIFT;
	track.numTriGroups = track.triGroupsWidth * track.triGroupsDepth;

	// set terr types
	strcpy( track.terrTypeTextureFilenames[0], terrTypeTextureFilenames[0] );
	strcpy( track.terrTypeTextureFilenames[1], terrTypeTextureFilenames[1] );
	strcpy( track.terrTypeTextureFilenames[2], terrTypeTextureFilenames[2] );
	track.numTerrTypes = 3;

	track.sunlightDirection = ED_DEFAULT_SUNLIGHT_DIRECTION;
	track.sunlightPitch = ED_DEFAULT_SUNLIGHT_PITCH;

	if( NULL == ( track.terrTypeRuns = new TERRTYPENODE[ track.depth ] ) ){
		MyAlert( NULL, "EdInitTrack() > track.terrTypeRuns memory allocation failed" );
		return E_FAIL;
	}
	
	if( NULL == ( track.terrTypeNodes = new TERRTYPENODE[ MAX_TERRAINTYPE_NODES ] ) ){
		MyAlert( NULL, "EdInitTrack() > track.terrTypeNodes memory allocation failed" );
		return E_FAIL;
	}

	// real types and display types arrays are exclusive to editor
	if( NULL == ( terrBaseTypes = new BYTE[ track.numTerrCoords ] ) ){
		MyAlert( NULL, "EdInitTrack() > terrBaseTypes memory allocation failed" );
		return E_FAIL;
	}

	if( NULL == ( terrDisplayTypes = new BYTE[ track.numTerrCoords ] ) ){
		MyAlert( NULL, "EdInitTrack() > terrDisplayTypes memory allocation failed" );
		return E_FAIL;
	}

	for(z=0; z < track.depth; z++){
		CopyMemory( &terrBaseTypes[z * track.width],
			&((BYTE*)bm.bmBits)[z * bm.bmWidthBytes],	track.width );
	}

	DeleteObject( hbm );


	// allocate nav stuff
	if( NULL == ( track.navCoords = new NAVCOORD[ MAX_NAVCOORDS ] ) ){
		MyAlert( NULL, "EdInitTrack() > track.navCoords memory allocation failed" );
		return E_FAIL;
	}

	if( NULL == ( track.navCoordLookupNodes = new NAVCOORDLOOKUPNODE[ MAX_NAVCOORDLOOKUP_NODES ] ) ){
		MyAlert( NULL, "EdInitTrack() > track.navCoordLookupNodes memory allocation failed" );
		return E_FAIL;
	}

	if( NULL == ( track.navCoordLookupRuns = new NAVCOORDLOOKUPNODE[ track.depth ] ) ){
		MyAlert( NULL, "EdInitTrack() > track.navCoordLookupRuns memory allocation failed" );
		return E_FAIL;
	}

	// get track.terrCoords and some misc data from track file, if it exists
	sprintf( filepath, "%strack%d.dat", TRACK_DIRECTORY, track.trackNum );

	if( SUCCEEDED( trackFile = CreateFile( filepath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) ){

		if( !ReadFile( trackFile, &header, sizeof(TRACKFILEHEADER), &numBytes, NULL ) ){
			sprintf( desc, "EdInitTrack() > This file was successfully opened but it could not be read:\n\n%s", filepath );
			MyAlert( NULL, desc );
			return E_FAIL;
		}

		// copy header info
//		track.sunlightDirection = header.sunlightDirection;
//		track.sunlightPitch = header.sunlightPitch;

		DWORD oldTrackWidth = header.width;
		DWORD oldTrackDepth = header.depth;
		DWORD oldNumTerrCoords = oldTrackWidth * oldTrackDepth;

		for(i=0; i < MAX_RACERS; i++){
			track.racerStartPositions[i] = header.racerStartPositions[i];
		}

        cursorX = track.racerStartPositions[0].x;
        cursorZ = track.racerStartPositions[0].z;

		track.numTerrTypes = header.numTerrTypes;

		if( track.width != header.width || track.depth != header.depth ){
			MyAlert( NULL, "EdInitTrack() > Types bitmap dimensions do not match track header" );
			CloseHandle( trackFile );
			return E_FAIL;
		}

		track.numNavCoords = header.numNavCoords;
		numNavCoordLookupNodes = header.numNavCoordLookupNodes;

		// skip over un-used data
		DWORD offset = 0;
		offset += track.numTerrTypes * 32 * sizeof(CHAR); // terrType texture filenames

		offset += track.depth * sizeof(TERRTYPENODE); // track.terrTypeRuns
//		offset += oldTrackDepth * sizeof(TERRTYPENODE); // track.terrTypeRuns

		offset += header.numTerrTypeNodes * sizeof(TERRTYPENODE); // track.terrTypeNodes

//		offset += track.numNavCoords * sizeof(NAVCOORD);
//		offset += oldTrackDepth * sizeof(NAVCOORDLOOKUPNODE);
//		offset += header.numNavCoordLookupNodes * sizeof(NAVCOORDLOOKUPNODE);

		SetFilePointer( trackFile, offset, NULL, FILE_CURRENT ); // skip to navCoordData
/*
		// nav stuff: no nav coords, all lookups are zero
		track.numNavCoords = 0;
		numNavCoordLookupNodes = 0;
		for(z=0; z < track.depth; z++){
			track.navCoordLookupRuns[z].navCoord = NAVCOORD_NOCOORD;
			track.navCoordLookupRuns[z].next = 0;
			track.navCoordLookupRuns[z].nextStartX = track.width;
		}
*/

		// read nav stuff
		if( !ReadFile( trackFile, track.navCoords, track.numNavCoords * sizeof(NAVCOORD), &numBytes, NULL ) ){
			sprintf( desc, "InitTrack() > This file was successfully opened but it could not be read:\n\n%s", filepath );
			MyAlert( NULL, desc );
			return E_FAIL;
		}

		if( !ReadFile( trackFile,	track.navCoordLookupRuns,	track.depth * sizeof(NAVCOORDLOOKUPNODE),	&numBytes, NULL ) ){
//		if( !ReadFile( trackFile,	track.navCoordLookupRuns,	oldTrackDepth * sizeof(NAVCOORDLOOKUPNODE),	&numBytes, NULL ) ){
			sprintf( desc, "EdInitTrack() > This file was successfully opened but it could not be read:\n\n%s", filepath );
			MyAlert( NULL, desc );
			return E_FAIL;
		}

		if( !ReadFile( trackFile,	track.navCoordLookupNodes, header.numNavCoordLookupNodes * sizeof(NAVCOORDLOOKUPNODE), &numBytes,	NULL ) ){
			sprintf( desc, "EdInitTrack() > This file was successfully opened but it could not be read:\n\n%s", filepath );
			MyAlert( NULL, desc );
			return E_FAIL;
		}


		// coords (height-map)
		if( NULL == ( track.terrCoords = new TERRCOORD[ track.numTerrCoords ] ) ){
			MyAlert( NULL, "EdInitTrack() > track.terrCoords memory allocation failed" );
			return E_FAIL;
		}
	
		if( !ReadFile( trackFile, track.terrCoords,	track.numTerrCoords * sizeof(TERRCOORD), &numBytes,	NULL ) ){
			sprintf( desc, "EdInitTrack() > This file was successfully opened but it could not be read:\n\n%s", filepath );
			MyAlert( NULL, desc );
			CloseHandle( trackFile );
			return E_FAIL;
		}

/*
		// coords (height-map)
		TERRCOORD* oldTerrCoords;

		if( NULL == ( oldTerrCoords = new TERRCOORD[ oldNumTerrCoords ] ) ){
			MyAlert( NULL, "EdInitTrack() > oldTerrCoords memory allocation failed" );
			return E_FAIL;
		}

		DWORD blah=0;
	
		if( !ReadFile( trackFile, oldTerrCoords, oldNumTerrCoords * sizeof(TERRCOORD), &numBytes,	NULL ) ){
			sprintf( desc, "EdInitTrack() > This file was successfully opened but it could not be read:\n\n%s", filepath );
			MyAlert( NULL, desc );
			CloseHandle( trackFile );
			return E_FAIL;
		}

		if( NULL == ( track.terrCoords = new TERRCOORD[ track.numTerrCoords ] ) ){
			MyAlert( NULL, "EdInitTrack() > track.terrCoords memory allocation failed" );
			return E_FAIL;
		}


		FLOAT scale = (FLOAT)track.width / oldTrackWidth;
		FLOAT invScale = 1.0f / scale;

		for(DWORD newZ=0; newZ < track.depth; newZ++){
			for(DWORD newX=0; newX < track.width; newX++){

				FLOAT oldX = (FLOAT)newX * invScale;
				FLOAT oldZ = (FLOAT)newZ * invScale;

				DWORD oldIntX = (DWORD)oldX;
				DWORD oldIntZ = (DWORD)oldZ;

				if( oldIntX == oldTrackWidth - 1 ){
					oldIntX--;
				}
				if( oldIntZ == oldTrackDepth - 1 ){
					oldIntZ--;
				}

				FLOAT fracX = oldX - oldIntX;
				FLOAT fracZ = oldZ - oldIntZ;

				track.terrCoords[newZ * track.width + newX].height = scale *
					( (FLOAT)oldTerrCoords[oldIntZ * oldTrackWidth + oldIntX].height * ( 1.0f - fracZ ) * ( 1.0f - fracX ) +
					(FLOAT)oldTerrCoords[oldIntZ * oldTrackWidth + oldIntX + 1].height * ( 1.0f - fracZ ) * fracX +
					(FLOAT)oldTerrCoords[(oldIntZ + 1) * oldTrackWidth + oldIntX].height * fracZ * ( 1.0f - fracX ) +
					(FLOAT)oldTerrCoords[(oldIntZ + 1) * oldTrackWidth + oldIntX + 1].height * fracZ * fracX );
			}
		}

		delete oldTerrCoords;
*/
		// note: *not* reading triGroups data here

		CloseHandle( trackFile );
	}

	else{ // else if track file doesn't yet exist

		for(i=0; i < MAX_RACERS; i++){
			track.racerStartPositions[i] = D3DVECTOR( track.width / 2, 0, track.depth / 2 + i * 5 );
		}

		track.triGroupsWidth = ( track.width - 1 ) >> TRIGROUP_WIDTHSHIFT;
		track.triGroupsDepth = ( track.depth - 1 ) >> TRIGROUP_WIDTHSHIFT;
		track.numTriGroups = track.triGroupsWidth * track.triGroupsDepth;

		track.numTerrCoords = track.width * track.depth;

		// coords (height-map)
		if( NULL == ( track.terrCoords = new TERRCOORD[ track.numTerrCoords ] ) ){
			MyAlert( NULL, "EdInitTrack() > track.terrCoords memory allocation failed" );
			return E_FAIL;
		}
	
/*
		for(z=0; z < track.depth; z++){
			for(x=0; x < track.width; x++){
				track.terrCoords[z * track.width + x].height = ED_DEFAULT_TERRAIN_HEIGHT;
			}
		}
*/

        {
            #define Halt()              { __asm int 3 }
            #define ELEVDATA_WIDTH      (467)
            #define ELEVDATA_HEIGHT     (377)
            #define ELEVDATA_HSCALE     (8.0)
            #define ELEVDATA_VSCALE     (0.3)
            #define ELEVDATA_OFFZ       (0)
            #define ELEVDATA_OFFX       (0)
            //#define ELEVDATA_DESTWIDTH  (936)
            //#define ELEVDATA_DESTHEIGHT (1508)
            #define ELEVDATA_FILENAME   "\\elevdata\\O36117g2_raw.dem"
            #define ELEVDATA_TYPE       WORD

            HANDLE ElevDataFile;
            ELEVDATA_TYPE* ElevData;

          	sprintf( filepath, "%s%s", TRACK_DIRECTORY, ELEVDATA_FILENAME );

            ElevData = new ELEVDATA_TYPE[ ELEVDATA_WIDTH * ELEVDATA_HEIGHT ];

/*
	        if( FAILED( ElevDataFile = CreateFile( filepath,
		        GENERIC_READ,
		        0,
		        NULL,
		        OPEN_EXISTING,
		        FILE_ATTRIBUTE_NORMAL,
		        NULL ) ) )
            {
                MyAlert( NULL, "Can't find elev data file" );
                return E_FAIL;
            }

		    if( !ReadFile( ElevDataFile, ElevData, sizeof(DWORD) * 2, &numBytes, NULL ) )
            {
			    MyAlert( NULL, "Can't read elev data file" );
			    return E_FAIL;
            }

		    if( !ReadFile( ElevDataFile, ElevData, ELEVDATA_WIDTH * ELEVDATA_HEIGHT * sizeof(WORD), &numBytes, NULL ) )
            {
			    MyAlert( NULL, "Can't read elev data file" );
			    return E_FAIL;
            }
*/
            
            FILE* fptr;
            fptr = fopen( filepath, "rb" );
            int intVal;
            int i = 0;
            int result;

            if( fptr == 0 )
            {
                MyAlert( NULL, "can't open elev data file" );
                return E_FAIL;
            }
            /*
            fread( ElevData, sizeof(WORD), 4, fptr );
            fread( ElevData, sizeof(WORD), ELEVDATA_WIDTH * ELEVDATA_HEIGHT, fptr );
*/
            
		    for(z=0; z < ELEVDATA_HEIGHT; z++)
            {
			    for(x=0; x < ELEVDATA_WIDTH; x++)
                {
                    if( ( result = fscanf( fptr, "%d", &intVal ) ) != 1 )
                    {
                        Halt();
                    }
                    ElevData[(ELEVDATA_HEIGHT - z - 1) * ELEVDATA_WIDTH + 
                             (ELEVDATA_WIDTH - x - 1)] = intVal;
                }
            }

		    for(z=0; z < track.depth; z++){
			    for(x=0; x < track.width; x++){
                    /*
                    short val = ElevData[(z * track.depth / ELEVDATA_HEIGHT) * ELEVDATA_HEIGHT + (x * track.width / ELEVDATA_WIDTH)];
                    short switchedVal = SwitchEndian( val );
                    DWORD unsignedVal = switchedVal + 0x8000;
				    track.terrCoords[z * track.width + x].height = (WORD)unsignedVal * 0.01;
*/
                    if( z + ELEVDATA_OFFZ < ELEVDATA_HEIGHT * ELEVDATA_HSCALE &&
                        x + ELEVDATA_OFFX < ELEVDATA_WIDTH * ELEVDATA_HSCALE )
                    {
                        float vals[4];
                        unsigned int avgVal;
                        float scaledZ = (float)(z + ELEVDATA_OFFZ ) / ELEVDATA_HSCALE;
                        float scaledX = (float)(x + ELEVDATA_OFFX ) / ELEVDATA_HSCALE;
                        float fracZ, fracX;

                        vals[0] = ElevData[(int)( scaledZ ) * ELEVDATA_WIDTH +
                                           (int)( scaledX )];
                        vals[1] = ElevData[(int)( scaledZ ) * ELEVDATA_WIDTH +
                                           (int)( scaledX + 1.0 )];
                        vals[2] = ElevData[(int)( scaledZ + 1.0 ) * ELEVDATA_WIDTH +
                                           (int)( scaledX )];
                        vals[3] = ElevData[(int)( scaledZ + 1.0 ) * ELEVDATA_WIDTH +
                                           (int)( scaledX + 1.0 )];

                        fracZ = scaledZ - floor(scaledZ);
                        fracX = scaledX - floor(scaledX);

                        avgVal = ( vals[0] * ( 1.0 - fracZ ) * ( 1.0 - fracX ) +
                                   vals[1] * ( 1.0 - fracZ ) * ( fracX ) +
                                   vals[2] * ( fracZ ) * ( 1.0 - fracX ) +
                                   vals[3] * ( fracZ ) * ( fracX ) );

				        track.terrCoords[z * track.width + x].height = avgVal * ELEVDATA_HSCALE * ELEVDATA_VSCALE;
                    }
                    else
                    {
                        track.terrCoords[z * track.width + x].height = 10000;
                    }
			    }
		    }

            fclose( fptr );
            //CloseHandle( ElevDataFile );
        }

		// nav stuff: no nav coords, all lookups are zero
		track.numNavCoords = 0;
		numNavCoordLookupNodes = 0;
		for(z=0; z < track.depth; z++){
			track.navCoordLookupRuns[z].navCoord = NAVCOORD_NOCOORD;
			track.navCoordLookupRuns[z].next = 0;
			track.navCoordLookupRuns[z].nextStartX = track.width;
		}
	}

	// add editor-specific terrain type textures
	cursorType = track.numTerrTypes++;
	strcpy( track.terrTypeTextureFilenames[cursorType], ED_TEXTUREFILENAME_CURSOR );

	// add terrain type textures & allocate index arrays
	for(i=0; i < track.numTerrTypes; i++){
		if( FAILED( AddTexture( track.terrTypeTextureFilenames[i] ) ) ){
			return E_FAIL;
		}

		if( NULL == ( track.terrTypeIndices[i] = new WORD[ MAX_RENDERED_COORDS * 6 ] ) ){
			MyAlert( NULL, "EdInitTrack() > track.terrTypeIndices memory allocation failed" );
			return E_FAIL;
		}
		if( NULL == ( track.terrTypeBlendIndices[i] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 8 ) * 6] ) ){
			MyAlert( NULL, "EdInitTrack() > track.terrTypeBlendIndices memory allocation failed" );
			return E_FAIL;
		}
		if( NULL == ( track.terrTypeBlendOnIndices[i] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 16 ) * 6 ] ) ){
			MyAlert( NULL, "EdInitTrack() > track.terrTypeBlendOnIndices memory allocation failed" );
			return E_FAIL;
		}
	}


/*
	// set nav counts to zero, since they'll be created from scratch later
	track.numNavCoords = 0;
	numNavCoordLookupNodes = 0;


	if( NULL == ( track.navCoordLookupRuns = new NAVCOORDLOOKUPNODE[ track.depth ] ) ){
		MyAlert( NULL, "EdInitTrack() > navCoordLookupRuns memory allocation failed" );
		return E_FAIL;
	}
	
	if( NULL == ( track.navCoordLookupNodes = new NAVCOORDLOOKUPNODE[ MAX_NAVCOORDLOOKUP_NODES ] ) ){
		MyAlert( NULL, "EdInitTrack() > navCoordLookupNodes memory allocation failed" );
		return E_FAIL;
	}
*/

	// stuff for recursive tri processing
	if( NULL == ( track.triGroups = new TRIGROUP[ track.numTriGroups ] ) ){
		MyAlert( NULL, "EdInitTrack() > track.triGroups memory allocation failed" );
		return E_OUTOFMEMORY;
	}

	// initialize triGroups to max LOD
	FillMemory( track.triGroups, track.numTriGroups * sizeof(TRIGROUP), 0xFF );
	TRIGROUP* pTriGroup = track.triGroups;
	for(DWORD group=0; group < track.numTriGroups; group++, pTriGroup++){
		pTriGroup->data[TRIGROUP_NUM_DWORDS-1] &= 0x3FFFFFFF;
	}


	if( FAILED( InitTriRecur() ) ){
		return E_FAIL;
	}

	UpdateTerrainLightingAndTypes( track.entireRegion );

	// update terrain without ed stuff
	UpdateTerrainTypeRuns( track.entireRegion, FALSE );

	return S_OK;
}




HRESULT CEdApplication::SaveTrack(){

	CHAR filepath[256];
	HANDLE trackFile;
	DWORD numBytes;
	TRACKFILEHEADER header;
	CHAR desc[256]; // for error messages
	DWORD i;

	// update terrain without ed stuff
	numTerrTypeNodes = UpdateTerrainTypeRuns( track.entireRegion, FALSE );

	// set heights for racer start positions
	for(i=0; i < MAX_RACERS; i++){
		track.racerStartPositions[i].y = 1.0f + // height above ground
			track.terrCoords[(DWORD)track.racerStartPositions[i].z * track.width + (DWORD)track.racerStartPositions[i].x].height * TERR_HEIGHT_SCALE;
	}

	if( FAILED ( SetTerrainLOD( LOD_SETTING ) ) ){
		return E_FAIL;
	}

	// create new track file
	sprintf( filepath, "%strack%d.dat", TRACK_DIRECTORY, track.trackNum );

	if( ( trackFile = CreateFile( filepath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) == INVALID_HANDLE_VALUE ){
		sprintf( desc, "Ctrack::Save() >  Could not create file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}

	// fill header structure
	header.sunlightDirection = track.sunlightDirection;
	header.sunlightPitch = track.sunlightPitch;

	for(i=0; i < MAX_RACERS; i++){
		header.racerStartPositions[i] = track.racerStartPositions[i];
	}

	header.numTerrTypes = track.numTerrTypes - 1; // don't include cursor
	header.numTerrTypeNodes = numTerrTypeNodes;
	header.numNavCoords = track.numNavCoords;
	header.numNavCoordLookupNodes = numNavCoordLookupNodes;
	header.width = track.width;
	header.depth = track.depth;

	if( !WriteFile( trackFile, &header, sizeof(TRACKFILEHEADER), &numBytes, NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}


	// terrain types
	if( !WriteFile( trackFile, track.terrTypeTextureFilenames, header.numTerrTypes * 32 * sizeof(CHAR), &numBytes, NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}

	if( !WriteFile( trackFile, track.terrTypeRuns, track.depth * sizeof(TERRTYPENODE), &numBytes, NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}
	
	if( !WriteFile( trackFile, track.terrTypeNodes, numTerrTypeNodes * sizeof(TERRTYPENODE), &numBytes, NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}
	

	// nav coords
	if( !WriteFile( trackFile, track.navCoords, track.numNavCoords * sizeof(NAVCOORD), &numBytes, NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}
		
	if( !WriteFile( trackFile, track.navCoordLookupRuns, track.depth * sizeof(NAVCOORDLOOKUPNODE),	&numBytes, NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}
	
	if( !WriteFile( trackFile, track.navCoordLookupNodes, numNavCoordLookupNodes * sizeof(NAVCOORDLOOKUPNODE), &numBytes,	NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}


	// terr coords
	if( !WriteFile( trackFile, track.terrCoords, track.numTerrCoords * sizeof(TERRCOORD), &numBytes,	NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}

	// trigroups
	if( !WriteFile( trackFile, track.triGroups, track.numTriGroups * sizeof(TRIGROUP), &numBytes,	NULL ) ){
		sprintf( desc, "Ctrack::Save() > Could not write to file:\n\n%s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}


	CloseHandle( trackFile );

/*
	FillMemory( track.triGroups, track.numTriGroups * sizeof(TRIGROUP), 0xFF );
	TRIGROUP* pTriGroup = track.triGroups;
	for(DWORD group=0; group < track.numTriGroups; group++, pTriGroup++){
		pTriGroup->data[TRIGROUP_NUM_DWORDS-1] &= 0x3FFFFFFF;
	}
*/


	// restore max LOD
	FillMemory( track.triGroups, track.numTriGroups * sizeof(TRIGROUP), 0xFF );
	TRIGROUP* pTriGroup = track.triGroups;
	for(DWORD group=0; group < track.numTriGroups; group++, pTriGroup++){
		pTriGroup->data[TRIGROUP_NUM_DWORDS-1] &= 0x3FFFFFFF;
	}
	
	return S_OK;
}




HRESULT CEdApplication::CreateNavMap(){

	BYTE* navMap;
	FLOAT xRise, zRise, intensity;
	FLOAT run = 2 * TERR_SCALE;
	D3DVECTOR vGroundNormal;

	if( FAILED( navMap = new BYTE[ track.numTerrCoords ] ) ){
		return E_FAIL;
	}

	ZeroMemory( navMap, track.numTerrCoords * sizeof(BYTE) );

	for(INT32 z=1; z < track.depth - 1; z++){
		for(INT32 x=1; x < track.width - 1; x++){

			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;
			
			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			if( vGroundNormal.y < 0.3f ){
				intensity = 1.0f;
			}
			else if( vGroundNormal.y > 0.7f ){
				intensity = 0.0f;
			}
			else{
				intensity = 1.0f - ( vGroundNormal.y - 0.3f ) / 0.4f;
			}
			
			//intensity = ( 1.0f - max( 0.0f, ( vGroundNormal.y - 0.3f ) / 0.7f ) ) * 32;

			intensity *= 32;

			intensity = (DWORD)( intensity / 8 ) * 8;

			navMap[z * track.width + x] = intensity;
		}
	}

	CreateTerrainBitmap( navMap, "navmap_raw", "navmaptemplate" );

	delete navMap;

	return S_OK;
}



HRESULT CEdApplication::CreateTerrainBitmap( BYTE* data, CHAR* fileDesc, CHAR* templateFileDesc ){

	CHAR filepath[256];

	sprintf( filepath, "%s%s.bmp", TRACK_DIRECTORY, templateFileDesc );

	HANDLE existingFile, newFile;
	DWORD NumberOfBytesWritten;

	if( ( existingFile = CreateFile( filepath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) == INVALID_HANDLE_VALUE ){
		return E_FAIL;
	}

	DWORD fileSize = GetFileSize( existingFile, NULL );
	DWORD	bitmapWidthBytes = (DWORD)ceil( (FLOAT)track.width / 4 ) * 4;
	DWORD headerSize = fileSize - ( track.depth * bitmapWidthBytes );
	BYTE* fileData;
	if( NULL == ( fileData = new BYTE[ fileSize ] ) ){
		MyAlert( NULL, "CreateTerrainBitmap() > fileData mem alloc failed" );
		return E_FAIL;
	}

	if( !ReadFile( existingFile,
		fileData,
		fileSize,
		&NumberOfBytesWritten,
		NULL ) ){
		delete fileData;
		return E_FAIL;
	}

	CloseHandle( existingFile );

	for(DWORD z=0; z < track.depth; z++){
		CopyMemory( &fileData[ headerSize + z * bitmapWidthBytes ],
			&data[z * track.width], track.width );
	}

	sprintf( filepath, "%strack%d%s.bmp", TRACK_DIRECTORY, track.trackNum, fileDesc );

	if( ( newFile = CreateFile( filepath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) == INVALID_HANDLE_VALUE ){
		delete fileData;
		return E_FAIL;
	}

	if( !WriteFile( newFile,
		fileData,
		fileSize,     // number of bytes to write
		&NumberOfBytesWritten,  // number of bytes written
		NULL ) ){
		delete fileData;
		return E_FAIL;
	}

	CloseHandle( newFile );

	delete fileData;

	return S_OK;
}





	// connect nav coords (recursive)
	  // begin at start coord
	  // if has "forced connection" status, point to nearest fellow "forced connection"
	    // coord & process that coord, UNLESS that coord already points to THIS coord...
    // find nearest accesible coord...
	    // if nearest coord already points to THIS coord, find next-nearest
	    // if nearest coord has "branch" status, point to that coord and...
	      // find all other accessible branch coords, alt-point them to each other
	      // process all branch coords
	    // if nearest coord is normal, point to it and process it

FLOAT CEdApplication::DistToNavCoord( INT32 originX, INT32 originZ, WORD destCoord ){

	INT32 destX = track.navCoords[destCoord].x;
	INT32 destZ = track.navCoords[destCoord].z;

	return sqrt( ( destX - originX ) * ( destX - originX ) + 
		( destZ - originZ ) * ( destZ - originZ ) );
}




FLOAT CEdApplication::DistSqrdToNavCoord( INT32 originX, INT32 originZ, WORD destCoord ){

	INT32 destX = track.navCoords[destCoord].x;
	INT32 destZ = track.navCoords[destCoord].z;

	return ( destX - originX ) * ( destX - originX ) + 
		( destZ - originZ ) * ( destZ - originZ );
}




BOOL CEdApplication::IsNavCoordAccessible( INT32 originX, INT32 originZ,
																		WORD destCoord, BOOL goOffCliffs, BYTE* navData ){
	INT32 destX, destZ;
	BOOL isOnNormalTerrain = FALSE, isAtSlopeBase = FALSE;
	INT32 dx, dz, xInc, zInc;
	INT32 error=0;        // the discriminant i.e. error i.e. decision variable
  DWORD	i;

	BYTE* pNavData = &navData[originZ * track.width + originX];

	destX = track.navCoords[destCoord].x;
	destZ = track.navCoords[destCoord].z;
	
	dx = destX - originX;
	dz = destZ - originZ;

	if( dx >= 0 ){
		xInc = 1;
	} 
	else{
		xInc = -1;
		dx = -dx;
	}

	if( dz >= 0 ){
		zInc = track.width;
	}
	else{
		zInc = -track.width;
		dz = -dz;
	}

	if( dx > dz ){
		for(i=0; i <= dx; i++){

			if( *pNavData == NAVDATA_SLOPE && ( isAtSlopeBase || ( isOnNormalTerrain && !goOffCliffs ) ) ){
				return FALSE;
			}
			isAtSlopeBase = ( ( isOnNormalTerrain || isAtSlopeBase ) && *pNavData == NAVDATA_SLOPEBASE );
			isOnNormalTerrain = ( *pNavData == NAVDATA_NORMALTERRAIN );

			error += dz;

			if( error > dx ){
				error -= dx;
				pNavData += zInc;

				if( *pNavData == NAVDATA_SLOPE && ( isAtSlopeBase || ( isOnNormalTerrain && !goOffCliffs ) ) ){
					return FALSE;
				}
				isAtSlopeBase = ( ( isOnNormalTerrain || isAtSlopeBase ) && *pNavData == NAVDATA_SLOPEBASE );
				isOnNormalTerrain = ( *pNavData == NAVDATA_NORMALTERRAIN );
			}
			
			pNavData += xInc;
		}
	}
	else{
		for(i=0; i <= dz; i++){

			if( *pNavData == NAVDATA_SLOPE && ( isAtSlopeBase || ( isOnNormalTerrain && !goOffCliffs ) ) ){
				return FALSE;
			}
			isAtSlopeBase = ( ( isOnNormalTerrain || isAtSlopeBase ) && *pNavData == NAVDATA_SLOPEBASE );
			isOnNormalTerrain = ( *pNavData == NAVDATA_NORMALTERRAIN );

			error += dx;

			if( error > 0 ){
				error -= dz;
				pNavData += xInc;

				if( *pNavData == NAVDATA_SLOPE && ( isAtSlopeBase || ( isOnNormalTerrain && !goOffCliffs ) ) ){
					return FALSE;
				}
				isAtSlopeBase = ( ( isOnNormalTerrain || isAtSlopeBase ) && *pNavData == NAVDATA_SLOPEBASE );
				isOnNormalTerrain = ( *pNavData == NAVDATA_NORMALTERRAIN );
			} 

			pNavData += zInc;
		}
	}

	return TRUE;
}






VOID CEdApplication::ConnectNavCoord( WORD prev, WORD curr,
										 BYTE* navCoordProperties, BYTE* navData ){

	WORD coord, nearestCoord;
	FLOAT dist, distToNearestCoord;;

	track.navCoords[prev].next = curr;

	if( curr == 1 ){
		return; // reached finish line, so we're done
	}

	if( navCoordProperties[prev] == NAVDATA_BRANCHENDCOORD &&
		track.navCoords[curr].next == NAVCOORD_NOCOORD ){ // 1st time at branch end, don't continue
		track.navCoords[curr].next = 0; // change 'next' field to prevent this case next time
		return;
	}


	if( navCoordProperties[curr] == NAVDATA_FORCEDCONNECTIONCOORD &&
		navCoordProperties[prev] != NAVDATA_FORCEDCONNECTIONCOORD ){

		nearestCoord = 0;
		distToNearestCoord = 10000.0f;//MAX_FORCEDCONNECTIONDIST;
		for(coord=0; coord < track.numNavCoords; coord++){

			if( navCoordProperties[coord] == NAVDATA_FORCEDCONNECTIONCOORD &&
				coord != curr &&
				coord != prev &&
				track.navCoords[coord].next == NAVCOORD_NOCOORD &&
				//IsNavCoordAccessible( track.navCoords[curr].x, track.navCoords[curr].z, coord, TRUE, navData ) &&
				( dist = DistToNavCoord( track.navCoords[curr].x, track.navCoords[curr].z, coord ) ) < distToNearestCoord	){

				nearestCoord = coord;
				distToNearestCoord = dist;
			}
		}

		if( nearestCoord != 0 ){
			ConnectNavCoord( curr, nearestCoord, navCoordProperties, navData );
			return;
		} // if no other forced-connection coords nearby, connect to a regular point...
	}
	
	nearestCoord = NAVCOORD_NOCOORD;
	distToNearestCoord = 10000.0f;
	for(coord=0; coord < track.numNavCoords; coord++){

		if( IsNavCoordAccessible( track.navCoords[curr].x, track.navCoords[curr].z, coord, TRUE, navData ) &&
			coord != curr &&
			coord != prev &&
			( track.navCoords[coord].next == NAVCOORD_NOCOORD || track.navCoords[coord].next == 0 ||
			  ( navCoordProperties[curr] == NAVDATA_BRANCHENDCOORD && navCoordProperties[coord] != NAVDATA_BRANCHENDCOORD ) ) &&
			( dist = DistToNavCoord( track.navCoords[curr].x, track.navCoords[curr].z, coord ) ) < distToNearestCoord	&&
			( navCoordProperties[curr] != NAVDATA_BRANCHCOORD || // branches don't inter-connect
				navCoordProperties[coord] != NAVDATA_BRANCHCOORD ) &&
			( navCoordProperties[curr] != NAVDATA_BRANCHENDCOORD || // branch ends don't inter-connect
				navCoordProperties[coord] != NAVDATA_BRANCHENDCOORD ) &&
				navCoordProperties[coord] != NAVDATA_ALTSTARTCOORD ){

			nearestCoord = coord;
			distToNearestCoord = dist;
		}
	}

	if( nearestCoord == NAVCOORD_NOCOORD ){ 
		CHAR msg[256];
		sprintf( msg, "ConnectNavCoord() > Dead end at coord %d (%d,%d)", curr, track.navCoords[curr].x, track.navCoords[curr].z );
		MyAlert( NULL, msg );
		return;
	}

	if( navCoordProperties[nearestCoord] == NAVDATA_BRANCHCOORD ){

		DWORD currBranch = nearestCoord;
		for(coord=0; coord < track.numNavCoords; coord++){

			if( navCoordProperties[coord] == NAVDATA_BRANCHCOORD &&
				coord != currBranch &&
				IsNavCoordAccessible( track.navCoords[curr].x, track.navCoords[curr].z, coord, TRUE, navData ) &&
				track.navCoords[coord].alt == NAVCOORD_NOCOORD  ){

				track.navCoords[currBranch].alt = coord;
				currBranch = coord;
				ConnectNavCoord( curr, coord, navCoordProperties, navData );
			}
		}
	}

	// note: if branching, this will connect curr to 1st branch
	ConnectNavCoord( curr, nearestCoord, navCoordProperties, navData );
}



WORD CEdApplication::DistToStartCoord( WORD currCoord ){

	WORD shortestDist, tempDist;
	WORD nextCoord = track.navCoords[currCoord].next;

	if( currCoord == 0 ){
		return 0;
	}
	else if( currCoord == 1 ){ // reached finish coord, so go straight to start
		return DistToNavCoord( track.navCoords[currCoord].x, track.navCoords[currCoord].z, 0 );
	}

	if( nextCoord == NAVCOORD_NOCOORD ){
		return 10000;
	}

	shortestDist = DistToNavCoord( track.navCoords[currCoord].x, track.navCoords[currCoord].z,
		nextCoord ) + DistToStartCoord( nextCoord );

	while( track.navCoords[nextCoord].alt != NAVCOORD_NOCOORD ){

		nextCoord = track.navCoords[nextCoord].alt;

		tempDist = DistToNavCoord( track.navCoords[currCoord].x, track.navCoords[currCoord].z,
			nextCoord ) + DistToStartCoord( nextCoord );

		if( tempDist < shortestDist ){
			shortestDist = tempDist;
		}
	}

	return shortestDist;
}




HRESULT CEdApplication::CreateNavStuff(){

	NAVCOORD* newNavCoords;
	BYTE* navData;
	BYTE* pNavData;
	FLOAT xRise, zRise;
	D3DVECTOR vGroundNormal;
	CHAR filepath[256];
	HBITMAP hbm;
	BITMAP bm;
	BYTE* navCoordProperties;
	WORD* navCoordLookup; // should be WORD
	DWORD coord, oldCoord, nearestCoord, previousNearestCoord;
	FLOAT distSqrd, distSqrdToNearestCoord;
	INT32 x, z;
	DWORD numPositions, iter, position;
	D3DVECTOR temp;
	NAVCOORDLOOKUPNODE* pNode;
	DWORD numNewNavCoords;
	BOOL bUpdateLookups;

	if( NULL == ( navData = new BYTE[ track.numTerrCoords ] ) ){
		MyAlert( NULL, "Ctrack::CreateNavStuff() > navData memory allocation failed" );
		return E_OUTOFMEMORY;
	}

	sprintf( filepath, "%strack%dnav.bmp", TRACK_DIRECTORY, track.trackNum );

	if( !( hbm = (HBITMAP)LoadImage( NULL, filepath, IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE|LR_CREATEDIBSECTION ) ) ){
		delete navData;
		return E_FAIL;
	}

	GetObject( hbm, sizeof(BITMAP), &bm );

	for(z=0; z < track.depth; z++){
		CopyMemory( &navData[z * track.width],
			&((BYTE*)bm.bmBits)[z * bm.bmWidthBytes],	track.width );
	}

	DeleteObject( hbm );

	// get nav coords from file, store details in separate array
	  // nav coord details: forced connection, branch status
	
	if( FAILED( newNavCoords = new NAVCOORD[ MAX_NAVCOORDS ] ) ){
		MyAlert( NULL, "Ctrack::CreateNavStuff() > newNavCoords memory allocation failed" );
		delete navData;
		return E_OUTOFMEMORY;
	}

	ZeroMemory( newNavCoords, MAX_NAVCOORDS * sizeof(NAVCOORD) );
	numNewNavCoords = 2; // at least start & finish

	if( NULL == ( navCoordProperties = new BYTE[ MAX_NAVCOORDS ] ) ){
		MyAlert( NULL, "Ctrack::CreateNavStuff() > navCoordProperties memory allocation failed" );
		delete navData;
		delete newNavCoords;
		return E_OUTOFMEMORY;
	}
	ZeroMemory( navCoordProperties, MAX_NAVCOORDS * sizeof(BYTE) );

	pNavData = navData;
	numPositions=0;
	for(z=0; z < track.depth; z++){
		for(x=0; x < track.width; x++, pNavData++){

			if( *pNavData ){
				if( *pNavData == NAVDATA_STARTCOORD ){
					newNavCoords[0].x = x;
					newNavCoords[0].z = z;
				}
				else if( *pNavData == NAVDATA_FINISHCOORD ){
					newNavCoords[1].x = x;
					newNavCoords[1].z = z;
				}
				else if( *pNavData == NAVDATA_NORMALCOORD ||
								 *pNavData == NAVDATA_FORCEDCONNECTIONCOORD ||
								 *pNavData == NAVDATA_BRANCHCOORD ||
								 *pNavData == NAVDATA_BRANCHENDCOORD ||
								 *pNavData == NAVDATA_ALTSTARTCOORD ){
					newNavCoords[numNewNavCoords].x = x;
					newNavCoords[numNewNavCoords].z = z;
					navCoordProperties[numNewNavCoords] = *pNavData; 
					numNewNavCoords++;
				}
				else if( *pNavData == NAVDATA_RACERSTARTPOSITION ){
					track.racerStartPositions[numPositions].x = x;
					track.racerStartPositions[numPositions].z = z;
					numPositions++;
				}
			}
		}
	}

	if( numPositions != MAX_RACERS ){
		MyAlert( NULL, "Ctrack::CreateNavStuff() > Incorrect number of racer start positions" );
		delete navCoordProperties;
		delete newNavCoords;
		delete navData;
		return E_FAIL;
	}

	// sort racer start positions
	for(iter=0; iter < MAX_RACERS - 1; iter++){

		for(position=0; position < MAX_RACERS - 1; position++){

			if( DistToNavCoord( track.racerStartPositions[position + 1].x, track.racerStartPositions[position + 1].z, 0 ) <
				DistToNavCoord( track.racerStartPositions[position].x, track.racerStartPositions[position].z, 0 ) ){
				temp = track.racerStartPositions[position];
				track.racerStartPositions[position] = track.racerStartPositions[position + 1];
				track.racerStartPositions[position + 1] = temp;
			}
		}
	}

	
	// modify navData from terrain slope
	pNavData = navData;
	FLOAT run = 2 * TERR_SCALE;
	for(z=1; z < track.depth - 1; z++){
		pNavData = &navData[z * track.width + 1];
		for(x=1; x < track.width - 1; x++, pNavData++){

			zRise = ( track.terrCoords[( z - 1 ) * track.width + x].height - 
				track.terrCoords[( z + 1 ) * track.width + x].height ) * TERR_HEIGHT_SCALE;
			
			xRise = ( track.terrCoords[z * track.width + ( x - 1 )].height - 
				track.terrCoords[z * track.width + ( x + 1 )].height ) * TERR_HEIGHT_SCALE;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			if( vGroundNormal.y < MIN_PASSABLE_SLOPE_NORMAL_Y ){
				*pNavData = NAVDATA_SLOPE;

				if( vGroundNormal.x < 0 ){
					if( *( pNavData - 1 ) != NAVDATA_SLOPE ){
						*( pNavData - 1 ) = NAVDATA_SLOPEBASE;
					}
				}
				else if( vGroundNormal.x > 0 ){
					if( *( pNavData + 1 ) != NAVDATA_SLOPE ){
						*( pNavData + 1 ) = NAVDATA_SLOPEBASE;
					}
				}

				if( vGroundNormal.z < 0 ){
					if( *( pNavData - track.width ) != NAVDATA_SLOPE ){
						*( pNavData - track.width ) = NAVDATA_SLOPEBASE;
					}
				}
				else if( vGroundNormal.z > 0 ){
					if( *( pNavData + track.width ) != NAVDATA_SLOPE ){
						*( pNavData + track.width ) = NAVDATA_SLOPEBASE;
					}
				}
			}
		}
	}


	CreateTerrainBitmap( navData, "navoutput" , "track1nav" );


	// compare new navCoords to old; calc update region & create conversion table
	WORD* navCoordConversion;
	if( FAILED( navCoordConversion = new WORD[ track.numNavCoords ] ) ){
		MyAlert( NULL, "Ctrack::CreateNavStuff() > navCoordConversion memory allocation failed" );
		delete navCoordProperties;
		delete newNavCoords;
		delete navData;
		return E_OUTOFMEMORY;
	}
	ZeroMemory( navCoordConversion, track.numNavCoords * sizeof(WORD) );

	CRectRegion updateRegion;
	// set an invalid rectRegion, will be overridden if new/non-matching coords found
	updateRegion.Set( track.width, track.depth, -1, -1 );
	bUpdateLookups = FALSE;
	INT32 padding = 75;

	for(coord=0; coord < numNewNavCoords; coord++){

		x = newNavCoords[coord].x;
		z = newNavCoords[coord].z;

		for(oldCoord=0; oldCoord < track.numNavCoords; oldCoord++){
			if( track.navCoords[oldCoord].x == x && track.navCoords[oldCoord].z == z ){
				navCoordConversion[oldCoord] = coord;
				break;
			}
		}

		if( oldCoord == track.numNavCoords ){ // no matching old coord found, modify updateRegion
			bUpdateLookups = TRUE;
			if( x - padding < updateRegion.startX ){ 
				updateRegion.startX = x - padding; }
			if( x + padding > updateRegion.endX ){ 
				updateRegion.endX = x + padding; }
			if( z - padding < updateRegion.startZ ){ 
				updateRegion.startZ = z - padding; }
			if( z + padding > updateRegion.endZ ){ 
				updateRegion.endZ = z + padding; }
		}
	}		
	updateRegion.Clip( 0, 0, track.width - 1, track.depth - 1 );

	// overwrite existing old navCoords with new ones, and init 'next' & 'alt' field
	track.numNavCoords = numNewNavCoords;
	for(coord=0; coord < track.numNavCoords; coord++){
		track.navCoords[coord] = newNavCoords[coord];
		track.navCoords[coord].next = track.navCoords[coord].alt = NAVCOORD_NOCOORD;
	}


	// to begin traversal, connect end to start
	ConnectNavCoord( 1, 0, navCoordProperties, navData );
	// do alt start coords
	for(coord=0; coord < track.numNavCoords; coord++){
		if( navCoordProperties[coord] == NAVDATA_ALTSTARTCOORD ){
			ConnectNavCoord( coord, coord, navCoordProperties, navData );
		}
	}

	delete navCoordProperties;

	// fill distToStartCoord fields in navCoords
	for(coord=0; coord < track.numNavCoords; coord++){
		track.navCoords[coord].distToStartCoord = DistToStartCoord( coord );
	}

	// bUpdateLookups = FALSE;

	if( bUpdateLookups ){
		// update navCoordLookupRuns:

	  // create temporary navCoordLookup array
		if( NULL == ( navCoordLookup = new WORD[ track.numTerrCoords ] ) ){  // should be WORD
			MyAlert( NULL, "Ctrack::CreateNavStuff() > navCoordLookup memory allocation failed" );
			delete navData;
			delete newNavCoords;
			delete navCoordConversion;
			return E_OUTOFMEMORY;
		}

		// traverse existing lookupRuns, fill lookup array with *converted* coords
		INT32 currStartX;
		for(z=0; z < track.depth; z++){

			pNode = &track.navCoordLookupRuns[z];
			currStartX = 0;

			while( pNode->nextStartX < track.width ){
				for(x=currStartX; x < pNode->nextStartX; x++){
					navCoordLookup[z * track.width + x] = navCoordConversion[pNode->navCoord];
				}
				currStartX = pNode->nextStartX;
				pNode = &track.navCoordLookupNodes[pNode->next];;
			}

			for(x=currStartX; x < track.width; x++){
				navCoordLookup[z * track.width + x] = navCoordConversion[pNode->navCoord];
			}
		}

		delete navCoordConversion;

		// for every (x,z), find nearest coord that is "bi-directionally" accessible

		previousNearestCoord = 0;
		for(z=updateRegion.startZ; z < updateRegion.endZ; z++){ // track.depth
			for(x=updateRegion.startX; x < updateRegion.endX; x++){ // track.width

				nearestCoord = NAVCOORD_NOCOORD; // will indicate that you're "lost"
				distSqrdToNearestCoord = 1000000.0f;

				if( IsNavCoordAccessible( x, z, previousNearestCoord, TRUE, navData ) ){
					distSqrd = DistSqrdToNavCoord( x, z, previousNearestCoord );
					nearestCoord = previousNearestCoord;
					distSqrdToNearestCoord = distSqrd;
				}

				for(coord=0; coord < track.numNavCoords; coord++){

					if( ( distSqrd = DistSqrdToNavCoord( x, z, coord ) ) < distSqrdToNearestCoord &&
						IsNavCoordAccessible( x, z, coord, TRUE, navData )){
						nearestCoord = coord;
						distSqrdToNearestCoord = distSqrd;
					}
				}

				navCoordLookup[z * track.width + x] = previousNearestCoord = nearestCoord;
			}
		}

		BYTE* tempBytes = NULL;
		if( SUCCEEDED( tempBytes = new BYTE[ track.numTerrCoords ] ) ){
			for(DWORD i=0; i < track.numTerrCoords; i++){
				tempBytes[i] = (BYTE)( navCoordLookup[i] & 0x00FF );
			}
			CreateTerrainBitmap( tempBytes, "navlookupoutput" , "navlookupoutputtemplate" );
			delete tempBytes;
		}

		WORD nextAvailNode = 1; // waste 0th node so that 0 can indicate NULL

		// fill navCoordLookupRuns from lookup array
		for(z=0; z < track.depth; z++){
			pNode = &track.navCoordLookupRuns[z];
			pNode->navCoord = navCoordLookup[z * track.width];
			pNode->next = 0;
			for(x=0; x < track.width; x++){
				if( navCoordLookup[z * track.width + x] != pNode->navCoord ){
					if( nextAvailNode < MAX_NAVCOORDLOOKUP_NODES ){
						pNode->nextStartX = x;
						pNode->next = nextAvailNode;
						nextAvailNode++;
						pNode = &track.navCoordLookupNodes[pNode->next];
						pNode->navCoord = navCoordLookup[z * track.width + x];
						pNode->next = 0;
					}
				}
			}
			pNode->nextStartX = track.width;
		}

		numNavCoordLookupNodes = nextAvailNode;

		delete navCoordLookup;
	}

	delete navData;

	return S_OK;
}




WORD CEdApplication::UpdateTerrainTypeRuns( CRectRegion region, BOOL bIncludeEdStuff ){

	DWORD z, x;

//	CopyMemory( terrDisplayTypes, terrBaseTypes, track.numTerrCoords * sizeof(BYTE) );

//	if( modInProgress ){
//		UpdateTerrainLightingAndTypes( modRegion );
//	}

	terrDisplayTypes[oldCursorZ * track.width + oldCursorX] = typeUnderCursor;
	typeUnderCursor = terrDisplayTypes[cursorZ * track.width + cursorX];
	oldCursorX = cursorX;
	oldCursorZ = cursorZ;

	if( bIncludeEdStuff ){
		terrDisplayTypes[cursorZ * track.width + cursorX] = cursorType;
	}

	TERRTYPENODE* pCurrNode;
	WORD nextAvailNode = 1; // waste 0th node so that 0 can indicate NULL

	for(z=region.startZ; z <= region.endZ; z++){
		pCurrNode = &track.terrTypeRuns[z];
		pCurrNode->type = terrDisplayTypes[z * track.width];
		pCurrNode->next = 0;
		for(x=0; x < track.width; x++){
			if( terrDisplayTypes[z * track.width + x] != pCurrNode->type ){
				if( nextAvailNode < MAX_TERRAINTYPE_NODES ){
					pCurrNode->nextStartX = x;
					pCurrNode->next = nextAvailNode;
					nextAvailNode++;
					pCurrNode = &track.terrTypeNodes[pCurrNode->next];
					pCurrNode->type = terrDisplayTypes[z * track.width + x];
					pCurrNode->next = 0;
				}
			}
		}
		pCurrNode->nextStartX = track.width;
	}

	return nextAvailNode;
}




VOID CEdApplication::UpdateTerrainLightingAndTypes( CRectRegion region ){

	DWORD x,z;
	FLOAT xRise, zRise;
	FLOAT run = 2 * TERR_SCALE;
	BOOL updateUnderCursor;

	D3DVECTOR vSunlight = D3DVECTOR( // opposite of true direction, to match ground normals
		-cosf( track.sunlightPitch ) * cosf( track.sunlightDirection ),
		sinf( track.sunlightPitch ),
		-cosf( track.sunlightPitch ) * sinf( track.sunlightDirection ) );
	D3DVECTOR vGroundNormal;

	BYTE baseType;

	updateUnderCursor = oldCursorX >= region.startX && oldCursorZ >= region.startZ &&
		oldCursorX <= region.endX && oldCursorZ <= region.endZ;

	BOOL doMinZ = FALSE;
	BOOL doMaxZ = FALSE;
	BOOL doMinX = FALSE;
	BOOL doMaxX = FALSE;

	if( region.startZ == 0 ){
		doMinZ = TRUE;
		region.startZ = 1;
	}
	if( region.endZ == track.depth - 1 ){
		doMaxZ = TRUE;
		region.endZ = track.depth - 2;
	}
	if( region.startX == 0 ){
		doMinX = TRUE;
		region.startX = 1;
	}
	if( region.endX == track.width - 1 ){
		doMaxX = TRUE;
		region.endX = track.width - 2;
	}

	if( doMinZ ){
		z = 0;
		zRise = 0;

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[z * track.width + x].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}

		if( doMaxX ){
			x = track.width - 1;
			xRise = 0;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}

		for(x=region.startX; x <= region.endX; x++){

			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}
	}

	if( doMaxZ ){
		z = track.depth - 1;
		zRise = 0;

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}

		if( doMaxX ){
			x = track.width - 1;
			xRise = 0;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}

		for(x=region.startX; x <= region.endX; x++){

			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}
	}


	for(z=region.startZ; z <= region.endZ; z++){

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;

			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}

		if( doMaxX ){
			x = track.width - 1;
			xRise = 0;
			
			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;

			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}

		for(x=region.startX; x <= region.endX; x++){

			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;
			
			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, vGroundNormal );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}

			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			baseType = terrBaseTypes[z * track.width + x];

			if( vGroundNormal.y > terrTypeSlopeThresholds[baseType] ){
				terrDisplayTypes[z * track.width + x] = baseType;
			}
			else{
				terrDisplayTypes[z * track.width + x] = toSlopeTerrType[baseType];
			}
		}
	}	 

	if( updateUnderCursor ){
		typeUnderCursor = terrDisplayTypes[oldCursorZ * track.width + oldCursorX];
	}

}





VOID CEdApplication::Display2DStuff(){

	ShowStats();

    TCHAR buffer[80];
    sprintf( buffer, _T(" (%d,%d) %7.02f"), cursorX, cursorZ,
		(float)track.terrCoords[ cursorZ * track.width + cursorX ].height / TERR_SCALE);
    OutputText( 0, 28, buffer );
}
