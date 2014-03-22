
#include "common.h"


struct BasicVertexT
{
    D3DVALUE x, y, z;      // The untransformed, 3D position for the vertex
    D3DCOLOR color;        // The vertex color
};

#define D3DFVF_BASICVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

/*
const INT32 racerHullSectionHues[ MAX_RACERS ][ MAX_VEHICLE_HULL_SECTIONS ] =
{   0,  35,
   85, 150,
  40,   40 };

const FLOAT racerHullSectionLights[ MAX_RACERS ][ MAX_VEHICLE_HULL_SECTIONS ] =
{ 1.0, 1.0,
  1.0, 1.0,
	1.0, 0.3 };
*/
const D3DCOLORVALUE racerMeshDiffuseColors[ MAX_RACERS ][ MAX_MODEL_MESHES - 1 ] =
{
	0.5, 0.5, 0.5, 1,  1.0, 0.75,0.0, 1,  0.15,0.15,0.15,1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
	0.5, 0.5, 0.5, 1,  0.0, 0.65,0.55,1,  0.3, 0.3, 0.3, 1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
  0.5, 0.5, 0.5, 1,  0.1, 0.1, 0.35,1,  0.6, 0.6, 0.6, 1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
	0.5, 0.5, 0.5, 1,  0.4, 0.0, 0.0, 1,  0.6, 0.6, 0.6, 1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
  0.5, 0.5, 0.5, 1,  1.0, 0.25,0.0, 1,  0.3, 0.3, 0.3, 1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
	0.5, 0.5, 0.5, 1,  1.0, 1.0, 1.0, 1,  0.3, 0.3, 0.3, 1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
  0.5, 0.5, 0.5, 1,  0.1, 0.1, 0.1, 1,  0.6, 0.6, 0.6, 1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1,
  0.5, 0.5, 0.5, 1,  0.6, 0.75,0.0, 1,  0.15,0.15,0.15,1,  0.5, 0.5, 0.5, 1,  0.2, 0.2, 0.2, 1
};


const D3DCOLORVALUE racerShieldColors[ MAX_RACERS ] =
{ 1.0,1.0,1.0,0,
  1.0,1.0,1.0,0,
  1.0,1.0,1.0,0,
  1.0,1.0,1.0,0,
  1.0,1.0,1.0,0,
  1.0,1.0,1.0,0,
  1.0,1.0,1.0,0,
	1.0,0.0,0.5,0 };

D3DVECTOR liftThrusterPositions[ RACER_NUM_LIFT_THRUSTERS ] = { 
	D3DVECTOR( 1.0f,    0.0f,    0.0f ),
	D3DVECTOR( 0.7071f, 0.0f, 0.7071f ),
	D3DVECTOR( 0.0f,    0.0f,    1.0f ),
	D3DVECTOR(-0.7071f, 0.0f, 0.7071f ),
	D3DVECTOR(-1.0f,    0.0f,    0.0f ),
	D3DVECTOR(-0.7071f, 0.0f,-0.7071f ),
	D3DVECTOR( 0.0f,    0.0f,   -1.0f ),
	D3DVECTOR( 0.7071f, 0.0f,-0.7071f ),
	D3DVECTOR( 0, 0, 0 ) };
D3DVECTOR liftThrusterDirections[ RACER_NUM_LIFT_THRUSTERS ]; 
FLOAT liftThrusterRollEffect[ RACER_NUM_LIFT_THRUSTERS ];
FLOAT liftThrusterPitchEffect[ RACER_NUM_LIFT_THRUSTERS ];
FLOAT liftThrusterVelEffect[ RACER_NUM_LIFT_THRUSTERS ];


const FLOAT vehicleModelFileScales[ NUM_VEHICLE_MODELS ] = { 
	1.75f / 20.0f };

const D3DVECTOR vehicleModelRadii[ NUM_VEHICLE_MODELS ] = {
	D3DVECTOR( 0.9625f, 0.4f, 0.825f ) }; // x & z exagerrated by 10%
	
const BOOL isVehicleModelSymmetrical[ NUM_VEHICLE_MODELS ] = { 
	TRUE };




CRacer::CRacer(){

	for(DWORD mesh=0; mesh < MAX_MODEL_MESHES; mesh++){
		meshVertexEmissiveColors[mesh] = NULL;
		meshVertexShield[mesh] = NULL;
	}
}




HRESULT CMyApplication::InitVehicleModels(){

	CHAR name[32];

	for(DWORD model=0; model < NUM_VEHICLE_MODELS; model++){

		sprintf( name, "vehicle%d.x", model );

		if( FAILED( AddModel( name,	vehicleModelFileScales[model],
			isVehicleModelSymmetrical[model] ) ) ){
			return E_FAIL;
		}
	}

	return S_OK;
}


VOID CMyApplication::InitRacerLiftThrustArrays(){

	D3DVECTOR vDir;
	D3DVECTOR vDirOrigin = D3DVECTOR( 0.0f, RACER_LIFTTHRUST_CONE, 0.0f );

	for(DWORD thruster=0; thruster < RACER_NUM_LIFT_THRUSTERS; thruster++){

		liftThrusterDirections[thruster] = Normalize( liftThrusterPositions[thruster] - vDirOrigin );

		liftThrusterRollEffect[thruster] = RACER_LIFTTHRUST_MAXPITCHROLLACCEL * -liftThrusterPositions[thruster].z;
		liftThrusterPitchEffect[thruster] = RACER_LIFTTHRUST_MAXPITCHROLLACCEL * liftThrusterPositions[thruster].x;
		liftThrusterVelEffect[thruster] = 1.0f;
	}
}





HRESULT CMyApplication::InitRacers(){


	FLOAT dirX = track.navCoords[0].x - track.navCoords[1].x;
	FLOAT dirZ = track.navCoords[0].z - track.navCoords[1].z;
	FLOAT racerStartYaw = atan2( dirZ, dirX );

	for(DWORD i=0; i < numRacers; i++){

		if( FAILED( racers[i].Init(
			track.racerStartPositions[i],
			D3DVECTOR(0,0,0),
			racerStartYaw,
			0,
			0,
			0,
			i ) ) ){
			return E_FAIL;
		}
	}

	InitRacerLiftThrustArrays();

	return S_OK;
}




HRESULT CRacer::Init( D3DVECTOR iPos, D3DVECTOR iVel,
									FLOAT iYaw, FLOAT iPitch, FLOAT iRoll, FLOAT iLookPitch,
									DWORD iRacerNum ){
	CHAR str[10];
	DWORD i, iv;

	pos = iPos;
	vel = iVel;

	yaw = iYaw;
	pitch = iPitch;
	roll = iRoll;
	lookPitch = iLookPitch;

	ZeroMemory( &input, sizeof(RACERINPUT) );

	currLap = -1;
	currNavCoord = 2;
	targetNavCoord = 0;

	racerNum = iRacerNum;

	racePosition = racerNum;

	DWORD vehicleModelNum = racerNum % NUM_VEHICLE_MODELS;

	sprintf( vehicleModelName, "vehicle%d.x", vehicleModelNum );

	vehicleModelRadius = vehicleModelRadii[vehicleModelNum];
	vehicleModelMaxRadius = max( vehicleModelRadius.x, max( vehicleModelRadius.y, vehicleModelRadius.z ) );

	thrustIntensity = 0.0f;

	CModel* pVehicleModel = GetModel( vehicleModelName );
	MODELMESH* pMesh = pVehicleModel->meshes;
	for(i=0; i < pVehicleModel->numMeshes; i++, pMesh++){
/*
		SAFE_ARRAYDELETE( meshVertexEmissiveColors[i] );
		SAFE_ARRAYDELETE( meshVertexShield[i] );

		meshVertexEmissiveColors[i] = new D3DCOLOR[ pVehicleModel->meshes[i].numVertices ];
		meshVertexShield[i] = new FLOAT[ pVehicleModel->meshes[i].numVertices ];
		if( meshVertexEmissiveColors[i] == NULL || meshVertexShield[i] == NULL ){
			MyAlert( NULL, "Out of memory" );
			return E_FAIL;
		}
		for(iv=0; iv < pMesh->numVertices; iv++){
			meshVertexShield[i][iv] = 0.0f;
			//meshVertexEmissiveColors[i][iv] = racerShieldColors[racerNum]; // alpha initialized to 0
		}
*/
		meshVertexEmissiveColors[i] = NULL;
		meshVertexShield[i] = NULL;

		meshStridedData[i].position.lpvData =						&pMesh->vertices[0].x;
		meshStridedData[i].normal.lpvData =							&pMesh->vertices[0].nx;
		meshStridedData[i].diffuse.lpvData =						meshVertexEmissiveColors[i];
		meshStridedData[i].textureCoords[0].lpvData =		&pMesh->vertices[0].tu;
		meshStridedData[i].position.dwStride =					sizeof(D3DVERTEX);
		meshStridedData[i].normal.dwStride =						sizeof(D3DVERTEX);
		meshStridedData[i].diffuse.dwStride =						sizeof(D3DCOLOR);
		meshStridedData[i].textureCoords[0].dwStride =	sizeof(D3DVERTEX);
	}

	return S_OK;
}





VOID CMyApplication::UpdateRacers(){

	for(DWORD i=0; i < numRacers; i++){

		if( playerRacerNum != i || bAutopilot ){
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
		UpdateRacerNav( &racers[i] );
        #ifdef APP_GAME
		    racers[i].UpdateSounds();
        #endif
		//racers[i].UpdateShield( deltaTime );

	}
}





VOID CMyApplication::UpdateRacerNav( CRacer* pRacer ){

	DWORD coord;
	WORD tempDist, shortestDist;
	D3DVECTOR velAdjustedPos;
	FLOAT random;
	DWORD temp;

	pRacer->previousNavCoord = pRacer->currNavCoord;

	// update currNavCoord if lookup is valid
	if( ( temp = GetNavCoordAt( pRacer->pos.x, pRacer->pos.z ) ) != NAVCOORD_NOCOORD ){
		pRacer->currNavCoord = temp;
	}

	if( pRacer->previousNavCoord == 1 && pRacer->currNavCoord == 0 ){
		pRacer->currLap++;
	}
	else if( pRacer->previousNavCoord == 0 && pRacer->currNavCoord == 1 ){
		pRacer->currLap--;
	}

	DWORD cx = floor( pRacer->pos.x );
	DWORD cz = floor( pRacer->pos.z );
	FLOAT offx = pRacer->pos.x - cx;
	FLOAT offz = pRacer->pos.z - cz;
	DWORD index = cz * track.width + cx;
	FLOAT heightAboveGround;
	FLOAT hl = (FLOAT)track.terrCoords[index].height * TERR_HEIGHT_SCALE;
	FLOAT hr = (FLOAT)track.terrCoords[index + 1].height * TERR_HEIGHT_SCALE;
	FLOAT dl = (FLOAT)track.terrCoords[index + track.width].height * TERR_HEIGHT_SCALE - hl;
	FLOAT dr = (FLOAT)track.terrCoords[index + track.width + 1].height * TERR_HEIGHT_SCALE - hr;

	// if not in mid-jump, and if 'next' is valid, then update target coord
	if( RACER_AI_NAV_GROUNDCLEARANCE > ( heightAboveGround = pRacer->pos.y - ( ( offz * ( dr - dl ) + ( hr - hl ) ) * offx +	offz * dl +	hl ) ) &&
		track.navCoords[pRacer->currNavCoord].next != NAVCOORD_NOCOORD ){ 

		pRacer->targetNavCoord = coord = track.navCoords[pRacer->currNavCoord].next;

		velAdjustedPos = pRacer->pos + pRacer->vel * RACER_AI_PATHING_VELOCITYWEIGHT;

//		shortestDist = track.navCoords[coord].distToStartCoord + 
//			RACER_AI_PATHING_LAZINESS * sqrt( ( track.navCoords[coord].x - velAdjustedPos.x ) * ( track.navCoords[coord].x - velAdjustedPos.x ) + 
//			( track.navCoords[coord].z - velAdjustedPos.z ) * ( track.navCoords[coord].z - velAdjustedPos.z ) );
		shortestDist = sqrt( ( track.navCoords[coord].x - velAdjustedPos.x ) * ( track.navCoords[coord].x - velAdjustedPos.x ) + 
			( track.navCoords[coord].z - velAdjustedPos.z ) * ( track.navCoords[coord].z - velAdjustedPos.z ) );
		
		while( track.navCoords[coord].alt != NAVCOORD_NOCOORD ){
			
			coord = track.navCoords[coord].alt;
			
			tempDist = sqrt( ( track.navCoords[coord].x - velAdjustedPos.x ) * ( track.navCoords[coord].x - velAdjustedPos.x ) + 
				( track.navCoords[coord].z - velAdjustedPos.z ) * ( track.navCoords[coord].z - velAdjustedPos.z ) );
			
			random = sinf( elapsedTime * 8.0f ) * RACER_AI_PATHING_RANDOMNESS;
			
			if( tempDist + random < shortestDist ){
				shortestDist = tempDist;
				pRacer->targetNavCoord = coord;
			}
		}
	}

	pRacer->distToStartCoord = track.navCoords[pRacer->targetNavCoord].distToStartCoord + 
		sqrt( ( track.navCoords[pRacer->targetNavCoord].x - pRacer->pos.x ) * ( track.navCoords[pRacer->targetNavCoord].x - pRacer->pos.x ) + 
		( track.navCoords[pRacer->targetNavCoord].z - pRacer->pos.z ) * ( track.navCoords[pRacer->targetNavCoord].z - pRacer->pos.z ) );
}




VOID CMyApplication::UpdateRaceState(){

	DWORD sortedRacers[ MAX_RACERS ], temp;
	CRacer* pRacerA;
	CRacer* pRacerB;
	DWORD position, iter, racer;
	DWORD positionAIOnly, numAIRacers;

	for(position=0; position < numRacers; position++){
		sortedRacers[position] = position;
	}

	for(iter=0; iter < numRacers - 1; iter++){ // bubble sort

		for(position=0; position < numRacers - 1; position++){

			pRacerA = &racers[sortedRacers[position]];
			pRacerB = &racers[sortedRacers[position+1]];

			if( pRacerB->currLap > pRacerA->currLap ||
				( pRacerB->currLap == pRacerA->currLap &&
				pRacerB->distToStartCoord < pRacerA->distToStartCoord ) ){

				temp = sortedRacers[position];
				sortedRacers[position] = sortedRacers[position+1];
				sortedRacers[position+1] = temp;
			}
		}
	}

	if( playerRacerNum != -1 ){
		numAIRacers = numRacers - 1;
	}
	else{
		numAIRacers = numRacers;
	}

	positionAIOnly=0;
	pRacerB = &racers[playerRacerNum];
	for(position=0; position < numRacers; position++){
		pRacerA = &racers[sortedRacers[position]];
		pRacerA->racePosition = position;

		if( pRacerA->racerNum != playerRacerNum ){
			pRacerA->handicap = (FLOAT)positionAIOnly / ( numAIRacers - 1 ) * RACER_AI_HANDICAP_POSITION;

			INT32 lead = ( pRacerA->currLap * track.trackLength - pRacerA->distToStartCoord ) -
				( pRacerB->currLap * track.trackLength - pRacerB->distToStartCoord );

			if( lead > RACER_AI_HANDICAP_LEADDIST ){
				pRacerA->handicap += (FLOAT)positionAIOnly / ( numAIRacers - 1 ) * RACER_AI_HANDICAP_LEAD;
			}

			//pRacerA->handicap = (FLOAT)lead / track.trackLength;

			positionAIOnly++;
		}
	}
    racers[playerRacerNum].handicap = 0.0f;
}





FLOAT g_relPitch, g_relPitchYDiff;

VOID CMyApplication::GetAIInput( CRacer* pRacer ){

	FLOAT velMag;
	FLOAT targetDir, velDir, idealYaw;
	FLOAT dirError, yawError, scaledYawError;

	ZeroMemory( &pRacer->input, sizeof(RACERINPUT) );

	// yaw
	targetDir = atan2( track.navCoords[pRacer->targetNavCoord].z - pRacer->pos.z, 
		track.navCoords[pRacer->targetNavCoord].x - pRacer->pos.x );

	if( aiBehavior != AIBEHAVIOR_RACE && Magnitude( pRacer->pos - racers[playerRacerNum].pos ) < 75.0f ){
		if( aiBehavior == AIBEHAVIOR_HUNT ){
			targetDir = atan2( racers[playerRacerNum].pos.z - pRacer->pos.z, 
				racers[playerRacerNum].pos.x - pRacer->pos.x ); 
		}
		else{
			targetDir = atan2( pRacer->pos.z - racers[playerRacerNum].pos.z, 
				pRacer->pos.x - racers[playerRacerNum].pos.x );
		}
	}

	if( pRacer->currLap >= 0 ){
		targetDir += cosf( ( -elapsedTime + 0.35 ) * g_PI * ( 0.5f + pRacer->handicap * 0.5 ) ) * RACER_AI_HANDICAP_SWERVE * pRacer->handicap;
	}

	velMag = Magnitude( pRacer->vel );

	if( velMag > 0.0f ){

		velDir = atan2( pRacer->vel.z, pRacer->vel.x );

		dirError = velDir - targetDir;

		if( dirError < -g_PI*3/16 ){
			dirError = -g_PI*3/16;
		}
		else if( dirError > g_PI*3/16 ){
			dirError = g_PI*3/16;
		}

		dirError *= min( velMag / RACER_REAL_MAXVEL, 1.0f );

		idealYaw = targetDir - dirError;

		if( idealYaw > g_PI ){
			idealYaw -= g_PI * 2.0f;
		}
		else if( idealYaw < -g_PI ){
			idealYaw += g_PI * 2.0f;
		}
	}
	else{

		idealYaw = targetDir;
	}

	yawError = idealYaw - pRacer->yaw; // using ideal yaw

	if( yawError > g_PI ){
		yawError -= g_PI * 2.0f;
	}
	else if( yawError < -g_PI ){
		yawError += g_PI * 2.0f;
	}

	// scaledYawError = yawError / ( RACER_MAXTURNRATE * deltaTime );
	scaledYawError = yawError;

	if( scaledYawError > 1.0f ){
		pRacer->input.turn = 1.0f;
	}
	else if( scaledYawError < -1.0f ){
		pRacer->input.turn = -1.0f;
	}
	else{
		pRacer->input.turn = scaledYawError;
	}

	GetAIPitchRollInput( pRacer, targetDir, 
		RACER_AI_MAX_INCLINATION * ( 1.0f - pRacer->handicap ) );

	pRacer->input.accel = TRUE;

/*
	pRacer->input.turn = 0.01f;
	pRacer->input.accel = 0.0f;
	pRacer->input.pitch = 0.01f;
	pRacer->input.roll = 0.01f;
*/
}





VOID CMyApplication::GetAIPitchRollInput( CRacer* pRacer, FLOAT targetDir, 
																				 FLOAT maxRelInclination ){
	FLOAT yawError;
	INT32 terrPosX, terrPosZ;
	D3DVECTOR vDiag1, vDiag2, vGroundNormal;
	FLOAT groundRoll;
	FLOAT idealRelInclination;
	FLOAT relPitchYDiff, relPitch, idealRelPitch, pitchError, scaledPitchError;
	FLOAT relRollYDiff, relRoll, idealRelRoll, rollError, scaledRollError;


	// pitch & roll
	yawError = targetDir - pRacer->yaw;


	DWORD cx = floor( pRacer->pos.x );
	DWORD cz = floor( pRacer->pos.z );
	FLOAT offx = pRacer->pos.x - cx;
	FLOAT offz = pRacer->pos.z - cz;
	DWORD index = cz * track.width + cx;
	FLOAT heightAboveGround;
	FLOAT hl = (FLOAT)track.terrCoords[index].height * TERR_HEIGHT_SCALE;
	FLOAT hr = (FLOAT)track.terrCoords[index + 1].height * TERR_HEIGHT_SCALE;
	FLOAT dl = (FLOAT)track.terrCoords[index + track.width].height * TERR_HEIGHT_SCALE - hl;
	FLOAT dr = (FLOAT)track.terrCoords[index + track.width + 1].height * TERR_HEIGHT_SCALE - hr;

	// if not in mid-jump, update target coord
	heightAboveGround = pRacer->pos.y - ( ( offz * ( dr - dl ) + ( hr - hl ) ) * offx +	offz * dl +	hl );

	vDiag1 = D3DVECTOR( 1.0f, hr + dr - hl, 1.0f );
	vDiag2 = D3DVECTOR( 1.0f, hr - ( hl + dl ), -1.0f );

	vGroundNormal = Normalize( CrossProduct( vDiag1, vDiag2 ) );

	if( DotProduct( vGroundNormal, pRacer->vel ) > RACER_AI_MAX_DOWNWARD_VELOCITY &&
		heightAboveGround < RACER_AI_INCLINATION_GROUNDCLEARANCE ){
		idealRelInclination = maxRelInclination;
	}
	else{
		idealRelInclination = 0.0f;
	}

	groundRoll = asin( DotProduct( vGroundNormal, 
		D3DVECTOR( cosf( pRacer->yaw + ( 0.25f * g_PI ) ), 0, sinf( pRacer->yaw + ( 0.25f * g_PI ) ) ) ) );

	idealRelPitch = -idealRelInclination * cosf( yawError ) * min( 1.0f, cosf( pRacer->roll ) * cosf( pRacer->roll ) * cosf( pRacer->roll ) * 3.0f );

	pitchError = idealRelPitch - pRacer->pitch;

	idealRelRoll = idealRelInclination * sinf( yawError );

	rollError = idealRelRoll + groundRoll - pRacer->roll;


	if( pitchError < 0 ){
		if( pitchError < -RACER_AI_PITCHROLL_EPSILON ){
			pitchError += RACER_AI_PITCHROLL_EPSILON;
		}
		else{
			pitchError = 0;
		}
	}
	else{
		if( pitchError > RACER_AI_PITCHROLL_EPSILON ){
			pitchError -= RACER_AI_PITCHROLL_EPSILON;
		}
		else{
			pitchError = 0;
		}
	}

	scaledPitchError = pitchError / ( RACER_MAXPITCHRATE * deltaTime );

	if( scaledPitchError > 1.0f ){
		pRacer->input.pitch = 1.0f;
	}
	else if( scaledPitchError < -1.0f ){
		pRacer->input.pitch = -1.0f;
	}
	else{
		pRacer->input.pitch = scaledPitchError;
	}


	if( rollError < 0 ){
		if( rollError < -RACER_AI_PITCHROLL_EPSILON ){
			rollError += RACER_AI_PITCHROLL_EPSILON;
		}
		else{
			rollError = 0;
		}
	}
	else{
		if( rollError > RACER_AI_PITCHROLL_EPSILON ){
			rollError -= RACER_AI_PITCHROLL_EPSILON;
		}
		else{
			rollError = 0;
		}
	}

	scaledRollError = rollError / ( RACER_MAXROLLRATE * deltaTime );

	if( scaledRollError > 1.0f ){
		pRacer->input.roll = 1.0f;
	}
	else if( scaledRollError < -1.0f ){
		pRacer->input.roll = -1.0f;
	}
	else{
		pRacer->input.roll = scaledRollError;
	}
}




VOID CRacer::ProcessInput( FLOAT deltaTime ){

#if 0
	if( input.turn > 10.0f * g_PI * deltaTime ){
		input.turn = 10.0f * g_PI * deltaTime;
	}
	else if( input.turn < -10.0f * g_PI * deltaTime ){
		input.turn = -10.0f * g_PI * deltaTime;
	}
	yaw += input.turn;
#endif

	if( input.turn ){
        yaw += input.turn * RACER_MAXTURNRATE * deltaTime;
    }

	if( yaw > g_PI ){ yaw -= 2 * g_PI; }
	else if( yaw < -g_PI ){ yaw += 2 * g_PI; }

	lookPitch += input.lookPitch;

	if( lookPitch < -RACER_MAXLOOKPITCH ){ lookPitch = -RACER_MAXLOOKPITCH;}
	else if( lookPitch > RACER_MAXLOOKPITCH ){ lookPitch = RACER_MAXLOOKPITCH; }

	FLOAT totalInclination = sqrt( input.pitch * input.pitch + input.roll * input.roll );
	if( totalInclination > 1.0f ){
		FLOAT scale = 1.0f / totalInclination;
		input.pitch *= scale;
		input.roll *= scale;
	}

	if( input.pitch ){
		pitch += input.pitch * RACER_MAXPITCHRATE * deltaTime;
	}

	if( input.roll ){
		roll += input.roll * RACER_MAXROLLRATE * deltaTime;
	}

	// roll & pitch checked here, before calcing lift thrust, *and* afterwards
	if( roll < -RACER_MAXROLL ){ roll = -RACER_MAXROLL;}
	else if( roll > RACER_MAXROLL ){ roll = RACER_MAXROLL; }

	if( pitch < -RACER_MAXPITCH ){ pitch = -RACER_MAXPITCH;}
	else if( pitch > RACER_MAXPITCH ){ pitch = RACER_MAXPITCH; }

	if( input.accel ){
		vel += RACER_MAXACCEL * D3DVECTOR( cosf( yaw ) * cosf( pitch ),
			sinf( pitch ),
			sinf( yaw ) * cosf( pitch ) ) * deltaTime;
		
		thrustIntensity += RACER_EXHAUST_WARMUP_RATE * deltaTime;
		if( thrustIntensity > 1.0f ){
			thrustIntensity = 1.0f;
		}
	}
	else{
		thrustIntensity -= RACER_EXHAUST_COOLDOWN_RATE * deltaTime;
		if( thrustIntensity < 0.0f ){
			thrustIntensity = 0.0f;
		}
	}

}





VOID CRacer::UpdateSounds(){

	// update thrust position, vel, & volume
	
	if( DistToCam( pos ) <= SOUND_MAXDISTANCE ){
		FLOAT velMag = Magnitude( vel ) / RACER_REAL_MAXVEL;
		AdjustSoundInstance3D( "thrust.wav", racerNum, pos, vel );
		AdjustSoundInstance2D( "thrust.wav", racerNum,
			velMag + 1.0f, SOUND_NOADJUST, min( 1.0f, ( velMag * 0.5f + 0.5f ) * thrustIntensity ) );
		PlaySoundInstance( "thrust.wav", racerNum );
		
		// update thrust position, vel, & volume
		AdjustSoundInstance3D( "lift.wav", racerNum, pos, vel );
		AdjustSoundInstance2D( "lift.wav", racerNum,
			liftIntensity, SOUND_NOADJUST, SOUND_NOADJUST );
		PlaySoundInstance( "lift.wav", racerNum );
	}
	else{
		StopSoundInstance( "thrust.wav", racerNum );
		StopSoundInstance( "lift.wav", racerNum );
	}

	if( IsSoundInstancePlaying( "crash1.wav", racerNum ) ){
		AdjustSoundInstance3D( "crash1.wav", racerNum, pos, vel );
	}
	if( IsSoundInstancePlaying( "crash2.wav", racerNum ) ){
		AdjustSoundInstance3D( "crash2.wav", racerNum, pos, vel );
	}
}





VOID CRacer::SetRotationMatrix(){ 

	D3DMATRIX matTemp;

	D3DUtil_SetIdentityMatrix(matRot);

	// Produce and combine the rotation matrices.
	D3DUtil_SetRotateXMatrix(matTemp, roll);     // roll
	D3DMath_MatrixMultiply(matRot,matRot,matTemp);
	D3DUtil_SetRotateZMatrix(matTemp, pitch);    // pitch
	D3DMath_MatrixMultiply(matRot,matRot,matTemp);
	D3DUtil_SetRotateYMatrix(matTemp, -yaw);  // yaw -- negative because world-y's sign is screwy
	D3DMath_MatrixMultiply(matRot,matRot,matTemp);
}






VOID CRacer::RacerCollide( FLOAT deltaTime, CRacer* pOtherRacer ){

	D3DVECTOR sep = pOtherRacer->pos - pos;
	FLOAT dist = vehicleModelMaxRadius + pOtherRacer->vehicleModelMaxRadius;

//	pOtherRacer->pitch = RACER_MAXPITCH;

	if( abs( sep.x ) < dist &&
			abs( sep.z ) < dist &&
			abs( sep.y ) < dist ){

		D3DVECTOR normSep, negNormSep, relNormSep, otherRelNormSep;
		FLOAT relRadius, otherRelRadius;
		FLOAT	sepDist = Magnitude( sep );
		FLOAT overlapDist, collisionSpeed;

		normSep.x = sep.x / sepDist;
		normSep.y = sep.y / sepDist;
		normSep.z = sep.z / sepDist;

		D3DMATRIX matTemp = -1 * matRot;
		D3DMath_VectorMatrixMultiply( relNormSep, normSep, matTemp );

		relRadius = Magnitude( relNormSep * vehicleModelRadius );

		matTemp = -1 * pOtherRacer->matRot;
		negNormSep = -1 * normSep;
		D3DMath_VectorMatrixMultiply( otherRelNormSep, negNormSep, matTemp );

		otherRelRadius = Magnitude( otherRelNormSep * pOtherRacer->vehicleModelRadius );

		if( ( overlapDist = ( relRadius + otherRelRadius ) - sepDist ) > 0 ){

//			pos -= normSep * overlapDist;

			pos -= normSep * ( overlapDist / 2 );
			
			pOtherRacer->pos += normSep * ( overlapDist / 2 );

			collisionSpeed = DotProduct( vel, normSep ) + DotProduct( pOtherRacer->vel, negNormSep );

			if( collisionSpeed > 0.0f ){

				collisionSpeed *= RACER_COLLISION_ELASTICITY;

				vel += negNormSep * collisionSpeed;

				pOtherRacer->vel += normSep * collisionSpeed;

                #ifdef APP_GAME
				if( DistToCam( pos ) <= SOUND_MAXDISTANCE ){
					FLOAT volume = min( 1.0f, pow( collisionSpeed / RACER_REAL_MAXVEL * 3.0f, 0.15 ) );
				
					if( volume > 0.8f ){

						if( IsSoundInstancePlaying( "crash1.wav", racerNum ) ){
							AdjustSoundInstance2D( "crash2.wav", racerNum,
								SOUND_NOADJUST, SOUND_NOADJUST, volume );
							PlaySoundInstance( "crash2.wav", racerNum );
						}
						else{
							AdjustSoundInstance2D( "crash1.wav", racerNum,
								SOUND_NOADJUST, SOUND_NOADJUST, volume );
							PlaySoundInstance( "crash1.wav", racerNum );
						}

						if( IsSoundInstancePlaying( "crash2.wav", pOtherRacer->racerNum ) ){
							AdjustSoundInstance2D( "crash1.wav", pOtherRacer->racerNum,
								SOUND_NOADJUST, SOUND_NOADJUST, volume );
							PlaySoundInstance( "crash1.wav", pOtherRacer->racerNum );
						}
						else{
							AdjustSoundInstance2D( "crash2.wav", pOtherRacer->racerNum,
								SOUND_NOADJUST, SOUND_NOADJUST, volume );
							PlaySoundInstance( "crash2.wav", pOtherRacer->racerNum );
						}
					}
				}
                #endif
			}
		}
	}
}




BasicVertexT lineVerts[RACER_NUM_LIFT_THRUSTERS*2];

VOID CMyApplication::RacerTerrainCollide( CRacer* pRacer, FLOAT deltaTime ){

	D3DVECTOR vDir, vRelPosition, vPosition, vRelDamagePosition;
	FLOAT dist;
	DWORD index;
	FLOAT pitchCorrection = 0;
	FLOAT rollCorrection = 0;
	BOOL collisionOccurred = FALSE;
	FLOAT heightAboveGround;
	FLOAT positionCorrectionDist = 0;
	FLOAT newPositionCorrectionDist;
	D3DVECTOR vPositionCorrection, vGroundNormal, vNewGroundNormal, vDiag1, vDiag2;
	D3DVECTOR normalizedVel;

	if( pRacer->vel.x || pRacer->vel.y || pRacer->vel.z ){
		normalizedVel = Normalize( pRacer->vel );
	}
	else{
		normalizedVel = D3DVECTOR( 0.0f, 0.0f, 0.0f );
	}

	for(DWORD thruster=0; thruster < RACER_NUM_LIFT_THRUSTERS; thruster++){

		// liftThrusterDirections should already be normalized
		D3DMath_VectorMatrixMultiply( vDir, liftThrusterDirections[thruster], pRacer->matRot );		

		// craft points down it's x-axis, so x & z are switched
		vRelPosition.x = liftThrusterPositions[thruster].x * pRacer->vehicleModelRadius.z;
		vRelPosition.y = liftThrusterPositions[thruster].y * pRacer->vehicleModelRadius.y;
		vRelPosition.z = liftThrusterPositions[thruster].z * pRacer->vehicleModelRadius.x;
		D3DMath_VectorMatrixMultiply( vPosition, vRelPosition, pRacer->matRot );
		vPosition.x += pRacer->pos.x;
		vPosition.y += pRacer->pos.y;
		vPosition.z += pRacer->pos.z;
		vPosition /= TERR_SCALE;


//		vPosition = D3DVECTOR( 0.5, 5.0, 0.6137 ); 
//		vDir = D3DVECTOR( 1.5, -3.0, -1.5 );
//		vDir = Normalize(vDir);
		dist = DistToTerrain( vPosition, vDir, RACER_LIFTTHRUST_MAXDIST );

        if( bDisplayLiftLines && pRacer->racerNum == playerRacerNum )
        {
            FLOAT intensity;

            lineVerts[thruster*2].x = vPosition.x;
            lineVerts[thruster*2].y = vPosition.y;
            lineVerts[thruster*2].z = vPosition.z;

            lineVerts[thruster*2+1].x = vPosition.x + vDir.x * dist;
            lineVerts[thruster*2+1].y = vPosition.y + vDir.y * dist;
            lineVerts[thruster*2+1].z = vPosition.z + vDir.z * dist;
        
            intensity = ( 1.0f / dist ) * 0.75f +
				( 1.0f - ( dist / RACER_LIFTTHRUST_MAXDIST ) ) * 0.25f;

            D3DXCOLOR blue( 0xFF00FFFF );
            D3DXCOLOR black( 0xFF000000 );
            D3DXCOLOR result;
   			D3DXColorLerp( &result, &black, &blue, intensity );
            lineVerts[thruster*2].color = (DWORD)result;
   			//D3DXColorLerp( &result, &black, &red, intensity );
            lineVerts[thruster*2+1].color = (DWORD)result;
        }

		if( dist < 0 ){ // if < 0, then DistToTerrain actually returned heightAboveGround

			heightAboveGround = dist;
			// find thruster which is farthest below surface (requires largest correction)

			index = floor( vPosition.z ) * track.width + floor( vPosition.x );
			FLOAT slope1 = (FLOAT)( track.terrCoords[index + track.width + 1].height - 
				track.terrCoords[index].height ) * TERR_HEIGHT_SCALE;
			FLOAT slope2 = (FLOAT)( track.terrCoords[index + 1].height -
				track.terrCoords[index + track.width].height ) * TERR_HEIGHT_SCALE;

			vDiag1 = D3DVECTOR( 1.0f, slope1, 1.0f );
			vDiag2 = D3DVECTOR( 1.0f, slope2, -1.0f );

			vNewGroundNormal = Normalize( CrossProduct( vDiag1, vDiag2 ) );

			if( ( newPositionCorrectionDist = -heightAboveGround * vNewGroundNormal.y ) >
				positionCorrectionDist ){
				positionCorrectionDist = newPositionCorrectionDist;
				vGroundNormal = vNewGroundNormal;
				vRelDamagePosition = vRelPosition;
			}

			dist = 0;
		}

		// update lift intensity with actual dist before capping it
		if( thruster == 8 ){ // center thruster
			pRacer->liftIntensity = ( 1.0f / dist ) * 0.25f +
				( 1.0f - ( dist / RACER_LIFTTHRUST_MAXDIST ) ) * 0.75f;
			if( pRacer->liftIntensity < 0.0f ){
				pRacer->liftIntensity = 0.0f;
			}
			else if( pRacer->liftIntensity > 1.0f ){
				pRacer->liftIntensity = 1.0f;
			}
		}

		if( dist < RACER_LIFTTHRUST_MAXDIST ){

			if( dist < RACER_LIFTTHRUST_MINDIST ){ dist = RACER_LIFTTHRUST_MINDIST; }
			FLOAT accel = ( 1.0f / ( dist * dist ) ) * RACER_LIFTTHRUST_MAXACCEL * deltaTime;

			accel *= ( 1.0f + DotProduct( normalizedVel, vDir ) * RACER_LIFTTHRUST_SHOCKABSORB );
			//vel += -vDir * ( RACER_LIFTTHRUST_MAX_DIST - dist ) * ( RACER_LIFTTHRUST_MAX_DIST - dist ) * RACER_LIFTTHRUST_MAXACCEL;
			pRacer->vel += -vDir * accel * liftThrusterVelEffect[thruster];
			rollCorrection += accel * liftThrusterRollEffect[thruster];
			pitchCorrection += accel * liftThrusterPitchEffect[thruster];
		}

		//liftThrustDistances[thruster] = dist;
	}

	if( positionCorrectionDist ){
		pRacer->pos += vGroundNormal * positionCorrectionDist;

		FLOAT mag = DotProduct( pRacer->vel, vGroundNormal );
		if( mag < 0 ){
			pRacer->vel -= vGroundNormal * ( mag * TERRAIN_COLLISION_ELASTICITY );
			
            #ifdef APP_GAME
			if( DistToCam( pRacer->pos ) <= SOUND_MAXDISTANCE ){
				FLOAT volume = min( 1.0f, pow( -mag / RACER_REAL_MAXVEL * 3.0f, 0.2 ) );
				
				if( volume > 0.85f ){
					if( IsSoundInstancePlaying( "crash2.wav", pRacer->racerNum ) ||
						( ( (DWORD)( ( elapsedTime + pRacer->racerNum ) * 5.0f ) % 2 ) &&
						  !IsSoundInstancePlaying( "crash1.wav", pRacer->racerNum ) ) ){
						AdjustSoundInstance2D( "crash1.wav", pRacer->racerNum,
							SOUND_NOADJUST, SOUND_NOADJUST, volume );
						PlaySoundInstance( "crash1.wav", pRacer->racerNum );
					}
					else{
						AdjustSoundInstance2D( "crash2.wav", pRacer->racerNum,
							SOUND_NOADJUST, SOUND_NOADJUST, volume );
						PlaySoundInstance( "crash2.wav", pRacer->racerNum );
					}
				}
			}
            #endif
			//InflictShieldDamage( vRelDamagePosition, -mag / RACER_REAL_MAXVEL );
		}
	}

	pRacer->pitch += pitchCorrection * cosf(pRacer->roll);
	//pRacer->yaw += pitchCorrection * sinf(pRacer->roll);
	pRacer->roll += rollCorrection;
}




VOID CRacer::InflictShieldDamage( D3DVECTOR& vRelPosition, FLOAT intensity ){

	CModel* pVehicleModel = GetModel( vehicleModelName );
	MODELMESH* pMesh = pVehicleModel->meshes;
	D3DVERTEX* pVert;
	FLOAT dist, damage;

	for(DWORD mesh=0; mesh < pVehicleModel->numMeshes; mesh++, pMesh++){

		pVert = pMesh->vertices;
		for(DWORD vert=0; vert < pMesh->numVertices; vert++, pVert++){

			dist = Magnitude( D3DVECTOR( pVert->x - vRelPosition.x,
																   pVert->y - vRelPosition.y,
																	 pVert->z - vRelPosition.z ) );
			if( dist < 1.0 ){
				dist = 1.0;
			}

			damage = intensity * 6.0f / dist;
			
			//damage = intensity * 4.0f / Magnitude( *(D3DVECTOR*)(&pVert->x) - vRelPosition );

			meshVertexShield[mesh][vert] += damage;
		}
	}
}



VOID CRacer::Move( FLOAT deltaTime, TRACK* pTrack ){

	// roll & pitch checked here *and* before calcing lift thrust
	if( roll < -RACER_MAXROLL ){ roll = -RACER_MAXROLL;}
	else if( roll > RACER_MAXROLL ){ roll = RACER_MAXROLL; }

	if( pitch < -RACER_MAXPITCH ){ pitch = -RACER_MAXPITCH;}
	else if( pitch > RACER_MAXPITCH ){ pitch = RACER_MAXPITCH; }

	vel.y -= GRAVITY * deltaTime;
	//vel *= exp( RACER_DRAG * deltaTime );
	vel *= 1.0f - RACER_DRAG * deltaTime;

	pos += vel * deltaTime;

//	pitch *= exp( RACER_PITCHADJUST * deltaTime );

  if( pos.x / TERR_SCALE < 5 && vel.x < 0 ){
		pos.x = 5 * TERR_SCALE;
		vel.x = 0;
	}
  else if( pos.x / TERR_SCALE > pTrack->width - 6 && vel.x > 0 ){
		pos.x = ( pTrack->width - 6 ) * TERR_SCALE;
		vel.x = 0;
	}
  if( pos.z / TERR_SCALE < 5 && vel.z < 0 ){
		pos.z = 5 * TERR_SCALE;
		vel.z = 0;
	}
  else if( pos.z / TERR_SCALE > pTrack->depth - 6 && vel.z > 0 ){
		pos.z = ( pTrack->depth - 6 ) * TERR_SCALE;
		vel.z = 0;
	}

	if( pos.y > 4000 && vel.y > 0 ){
		pos.y = 4000;
		vel.y = 0;
	}
}



/*
VOID CRacer::InflictDamage( D3DVECTOR& source, FLOAT mag ){

	CModel* pVehicleModel = GetModel( vehicleModelName );
	MODELMESH* pMesh = pVehicleModel->meshes;
	DWORD vert;
	for(DWORD mesh=0; mesh < pVehicleModel->numMeshes; mesh++, pMesh++){
		for(vert=0; vert < pMesh->numVertices; vert++){
			meshVertexShield[mesh][vert] += -mag / RACER_MAXVEL;
		}
	}

}
*/



VOID CRacer::UpdateShield( FLOAT deltaTime ){

	CModel* pVehicleModel = GetModel( vehicleModelName );
	MODELMESH* pMesh = pVehicleModel->meshes;
	FLOAT shield;

	for(DWORD i=0; i < pVehicleModel->numMeshes; i++, pMesh++){
		for(DWORD iv=0; iv < pMesh->numVertices; iv++){

			shield = meshVertexShield[i][iv];
			if( shield > 1 ){
				shield = 1.0f;
			}
			//meshVertexEmissiveColors[i][iv] &= 0x00ffffff;
			//meshVertexEmissiveColors[i][iv] |= (INT)(shield * 255.0f) << 24;

			meshVertexEmissiveColors[i][iv]	= D3DRGB( 
				//racerMeshDiffuseColors[racerNum][i].r * ( 1 - shield ) +
				racerShieldColors[racerNum].r * shield,
				//racerMeshDiffuseColors[racerNum][i].g * ( 1 - shield ) +
				racerShieldColors[racerNum].g * shield,
				//racerMeshDiffuseColors[racerNum][i].b * ( 1 - shield ) +
				racerShieldColors[racerNum].b * shield );

			meshVertexShield[i][iv] -= RACER_SHIELD_FADE_RATE * deltaTime;
			if( meshVertexShield[i][iv] < 0 ){
				meshVertexShield[i][iv] = 0;
			}
		}
	}
}




HRESULT CMyApplication::CastRacerShadow( CRacer* pRacer ){

	FLOAT centerX, centerZ, radius;
	CRectRegion shadedTriGroups;

	centerX = pRacer->pos.x;
	centerZ = pRacer->pos.z;
	radius = pRacer->vehicleModelMaxRadius * 1.05f;

	// this code is duplicated in ApplyShadowTexture()
	shadedTriGroups.Set(
		(INT32)( centerX - radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerZ - radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerX + radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerZ + radius ) >> TRIGROUP_WIDTHSHIFT );

	if( shadedTriGroups.startX <= track.renderedTriGroups.endX &&
		shadedTriGroups.startZ <= track.renderedTriGroups.endZ &&
		shadedTriGroups.endX >= track.renderedTriGroups.startX &&
		shadedTriGroups.endZ >= track.renderedTriGroups.startZ ){

		if( bRenderShadowTexture ){
			m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &pRacer->matRot );

			if( FAILED( RenderToShadowTexture( GetModel( pRacer->vehicleModelName ) ) ) ){
				return E_FAIL;
			}
		}

		if( FAILED( ApplyShadowTexture( centerX, centerZ, radius ) ) ){
			return E_FAIL;
		}
	}

	return S_OK;
}				



BOOL CMyApplication::IsRacerVisible( CRacer* pRacer ){

	D3DMATRIX matSet;
	D3DVECTOR vWorld, vScreen;

	D3DMath_MatrixMultiply( matSet, m_matView, m_matProj );

	D3DMath_VectorMatrixMultiply( vScreen, pRacer->pos, matSet );

	// 10%-width margin of error
	return vScreen.x > -1.1f && vScreen.x < 1.1f &&
		vScreen.y > -1.1f && vScreen.y < 1.1f &&
		vScreen.z > -0.1f && vScreen.z < 1.1f;
}





HRESULT CRacer::Render( LPDIRECT3DDEVICE7 pd3dDevice ){


	D3DMATRIX matWorld; // World matrix being constructed.
	D3DMATERIAL7 mtrl;
	CModel* pVehicleModel = GetModel( vehicleModelName );
	MODELMESH* pMesh = pVehicleModel->meshes;
	HRESULT hr;


	D3DUtil_SetTranslateMatrix(matWorld, pos);
/*
	D3DUtil_SetIdentityMatrix(matRot);

	// Produce and combine the rotation matrices.
	D3DUtil_SetRotateXMatrix(matTemp, roll);     // roll
	D3DMath_MatrixMultiply(matRot,matRot,matTemp);
	D3DUtil_SetRotateZMatrix(matTemp, pitch);    // pitch
	D3DMath_MatrixMultiply(matRot,matRot,matTemp);
	D3DUtil_SetRotateYMatrix(matTemp, -yaw);  // yaw -- negative because world-y's sign is screwy
	D3DMath_MatrixMultiply(matRot,matRot,matTemp);
*/
		
	// Apply the rotation matrices to complete the world matrix.
	D3DMath_MatrixMultiply( matWorld, matRot, matWorld );
	
	pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );

//	pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
	pd3dDevice->SetRenderState( D3DRENDERSTATE_SPECULARENABLE, TRUE );
//	pd3dDevice->SetRenderState( D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1 );
//	pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );

	for(DWORD i=0; i < pVehicleModel->numMeshes; i++, pMesh++){

		if( pMesh->textureName == NULL ){ // no texture
			pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
		}
		else{
			pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
			if( FAILED( hr = pd3dDevice->SetTexture( 0, (GetTexture( pMesh->textureName ))->m_pddsSurface ) ) ){
				MyAlert(&hr, "Unable to set texture for racer rendering" );
				return E_FAIL;
			}
		}

		if( i == 0 ){ // exhaust mesh, by convention
			mtrl.diffuse.r = mtrl.diffuse.g = mtrl.diffuse.b = mtrl.diffuse.a = 0;
			mtrl.ambient = mtrl.specular = mtrl.diffuse;
			mtrl.power = 0;

			mtrl.emissive.r = VEHICLE_EXHAUST_COLOR.r * thrustIntensity;
			mtrl.emissive.g = VEHICLE_EXHAUST_COLOR.g * thrustIntensity;
			mtrl.emissive.b = VEHICLE_EXHAUST_COLOR.b * thrustIntensity;
			mtrl.emissive.a = 1.0f;
		}
		else{
			mtrl.diffuse = racerMeshDiffuseColors[racerNum][i - 1];
			mtrl.diffuse.a = 1;

			mtrl.ambient = mtrl.diffuse;

			mtrl.specular = pMesh->specularColor;
			mtrl.power = pMesh->specularPower;

			mtrl.emissive.r = mtrl.emissive.g = mtrl.emissive.b = mtrl.emissive.a = 0;
		}

		if( FAILED( hr = pd3dDevice->SetMaterial( &mtrl ) ) ){
			MyAlert(&hr, "Unable to set material for racer rendering" );
			return E_FAIL;
		}

/*
		if( FAILED( hr = pd3dDevice->DrawIndexedPrimitiveStrided( D3DPT_TRIANGLELIST, MODELVERTEXDESC, 
			&meshStridedData[i], pMesh->numVertices, 
			pMesh->indices, pMesh->numIndices, NULL ) ) ){
			MyAlert(&hr, "CRacer:Render() > DrawIndexedPrimitiveStrided() failed" );
			return E_FAIL;
		}
*/


		//DrawIndexedPrimitiveStrided( 

		if( FAILED( pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, D3DFVF_VERTEX,
			pMesh->vertices, pMesh->numVertices,
			pMesh->indices, pMesh->numIndices, NULL ) ) ){
			MyAlert( NULL, "Unable to render racers" );
			return E_FAIL;
		}

	}

//	pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );
	pd3dDevice->SetRenderState( D3DRENDERSTATE_SPECULARENABLE, FALSE );

	return S_OK;
}



HRESULT CMyApplication::RenderRacers(){

	HRESULT hr;

	DWORD racer;
	D3DVECTOR racerCenters[ MAX_RACERS ];
	D3DVALUE racerRadii[ MAX_RACERS ];
	DWORD racerVisibilities[ MAX_RACERS ];

	for(racer=0; racer < numRacers; racer++){
	
		racerCenters[racer] = racers[racer].pos;
		racerRadii[racer] = racers[racer].vehicleModelMaxRadius;
	}

	m_pd3dDevice->ComputeSphereVisibility( racerCenters, racerRadii, numRacers, 0, racerVisibilities );

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );

	for(racer=0; racer < numRacers; racer++){

		if( 0 == ( racerVisibilities[racer] & D3DSTATUS_CLIPINTERSECTIONALL ) ){

			if( FAILED( racers[racer].Render( m_pd3dDevice ) ) ){
				return E_FAIL;
			}
		}
		
		if( FAILED( CastRacerShadow( &racers[racer] ) ) ){
			return E_FAIL;
		}
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );

	if( FAILED( hr = m_pd3dDevice->SetMaterial( &m_defaultMtrl ) ) ){
		MyAlert(&hr, "Unable to restore material after rendering racers" );
		return E_FAIL;
	}

    if( bDisplayLiftLines && playerRacerNum != -1 )
    {
        D3DMATRIX matWorld;
        D3DUtil_SetIdentityMatrix( matWorld );
	    m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );

	    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );

        if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_LINELIST, D3DFVF_BASICVERTEX,
            lineVerts, RACER_NUM_LIFT_THRUSTERS*2, 0 ) ) )
        {
		    MyAlert(&hr, "Unable to render racer lift vectors" );
            return E_FAIL;
        }

    	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    }

	return S_OK;
}
