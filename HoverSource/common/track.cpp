
#include "common.h"



#pragma inline_recursion(on)

// globals for ProcessTri()

const INT32 triRecur_incX[ 8 ] = { 1,  1,  0, -1, -1, -1,  0,  1 };
const INT32 triRecur_incZ[ 8 ] = { 0,  1,  1,  1,  0, -1, -1, -1 };
BYTE triRecur_bits[ TRIGROUP_NUM_BITS ];
INT32 triRecur_incRenderPos[ TRIGROUP_NUM_DEPTHS ][ 8 ];
INT32 triRecur_incCoordPos[ TRIGROUP_NUM_DEPTHS ][ 8 ];
INT32 triRecur_incWorldX[ TRIGROUP_NUM_DEPTHS ][ 8 ];
INT32 triRecur_incWorldZ[ TRIGROUP_NUM_DEPTHS ][ 8 ];
FLOAT triRecur_incTextureU[ TRIGROUP_NUM_DEPTHS ][ 8 ];
FLOAT triRecur_incTextureV[ TRIGROUP_NUM_DEPTHS ][ 8 ];
FLOAT triRecur_incShadowTextureU[ TRIGROUP_NUM_DEPTHS ][ 8 ];
FLOAT triRecur_incShadowTextureV[ TRIGROUP_NUM_DEPTHS ][ 8 ];
DWORD triRecur_numVertices;
TERRTLVERTEX* triRecur_transformedVertices;
WORD* triRecur_pTypeIndex[ MAX_TERRAIN_TYPES ];
DWORD triRecur_currType;
TERRLVERTEX* triRecur_pVertex;
WORD* triRecur_indexAtRenderPos = NULL;
BYTE* triRecur_typeAtRenderPos = NULL;
WORD* triRecur_pTerrTypeBlendIndex[ MAX_TERRAIN_TYPES ];
WORD* triRecur_pTerrTypeBlendOnIndex[ MAX_TERRAIN_TYPES ];
TERRCOORD* triRecur_terrCoords;
BYTE* triRecur_splitCoords = NULL;
FLOAT triRecur_triGroupMaxError;



DWORD g_numTerrTypeNodes;

void Advance( TERRTYPENODE* &pNode, TERRTYPENODE* nodes ){

	pNode = &nodes[ pNode->next ];
}





HRESULT CMyApplication::InitTrack(){

	CHAR filepath[256];
	HANDLE trackFile;
	DWORD numBytes, i;
	TRACKFILEHEADER header;
	CHAR desc[256]; // for error messages

	sprintf( filepath, "%strack%d.dat", TRACK_DIRECTORY, track.trackNum );

	if( ( trackFile = CreateFile( filepath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) == INVALID_HANDLE_VALUE ){
		sprintf( desc, "Could not open file: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}

	if( !ReadFile( trackFile, &header, sizeof(TRACKFILEHEADER), &numBytes, NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}


	// copy header info
	track.sunlightDirection = header.sunlightDirection;
	track.sunlightPitch = header.sunlightPitch;

	for(i=0; i < MAX_RACERS; i++){
		track.racerStartPositions[i] = header.racerStartPositions[i];
	}

	track.numTerrTypes = header.numTerrTypes;
	track.numNavCoords = header.numNavCoords;
	track.width = header.width;
	track.depth = header.depth;

	// stuff derived from header info
	track.triGroupsWidth = ( track.width - 1 ) >> TRIGROUP_WIDTHSHIFT;
	track.triGroupsDepth = ( track.depth - 1 ) >> TRIGROUP_WIDTHSHIFT;
	track.numTriGroups = track.triGroupsWidth * track.triGroupsDepth;

	track.numTerrCoords = track.width * track.depth;
	track.entireRegion.Set( 0, 0, track.width - 1, track.depth - 1 );

	g_numTerrTypeNodes = header.numTerrTypeNodes;

	// terrain types
	if( !ReadFile( trackFile, track.terrTypeTextureFilenames, track.numTerrTypes * 32 * sizeof(CHAR), &numBytes, NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}

	for(i=0; i < track.numTerrTypes; i++){

		// add textures
		if( FAILED( AddTexture( track.terrTypeTextureFilenames[i] ) ) ){
			return E_FAIL;
		}

		if( NULL == ( track.terrTypeIndices[i] = new WORD[ MAX_RENDERED_COORDS * 6 ] ) ){
			MyAlert( NULL, "Out of memory" );
			return E_FAIL;
		}
		if( NULL == ( track.terrTypeBlendIndices[i] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 8 ) * 6] ) ){
			MyAlert( NULL, "Out of memory" );
			return E_FAIL;
		}
		if( NULL == ( track.terrTypeBlendOnIndices[i] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 16 ) * 6 ] ) ){
			MyAlert( NULL, "Out of memory" );
			return E_FAIL;
		}
	}

	if( NULL == ( track.terrTypeRuns = new TERRTYPENODE[ track.depth ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}

	if( !ReadFile( trackFile, track.terrTypeRuns, track.depth * sizeof(TERRTYPENODE), &numBytes, NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}
	
	if( NULL == ( track.terrTypeNodes = new TERRTYPENODE[ header.numTerrTypeNodes ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}

	if( !ReadFile( trackFile, track.terrTypeNodes, header.numTerrTypeNodes * sizeof(TERRTYPENODE), &numBytes, NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}
	

	// nav coords
	if( NULL == ( track.navCoords = new NAVCOORD[ track.numNavCoords ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}

	if( !ReadFile( trackFile, track.navCoords, track.numNavCoords * sizeof(NAVCOORD), &numBytes, NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}
	
	if( NULL == ( track.navCoordLookupRuns = new NAVCOORDLOOKUPNODE[ track.depth ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}
	
	if( !ReadFile( trackFile,	track.navCoordLookupRuns,	track.depth * sizeof(NAVCOORDLOOKUPNODE),	&numBytes, NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}

	if( NULL == ( track.navCoordLookupNodes = new NAVCOORDLOOKUPNODE[ header.numNavCoordLookupNodes ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}
	
	if( !ReadFile( trackFile,	track.navCoordLookupNodes, header.numNavCoordLookupNodes * sizeof(NAVCOORDLOOKUPNODE), &numBytes,	NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}

	// calc track length
	DWORD secondCoord = track.navCoords[0].next;
	INT32 originX = track.navCoords[secondCoord].x;
	INT32 originZ = track.navCoords[secondCoord].z;
	INT32 destX = track.navCoords[0].x;
	INT32 destZ = track.navCoords[0].z;

	track.trackLength = sqrt( ( destX - originX ) * ( destX - originX ) + 
		( destZ - originZ ) * ( destZ - originZ ) ) + 
		track.navCoords[secondCoord].distToStartCoord;


	// terr coords
	if( NULL == ( track.terrCoords = new TERRCOORD[ track.numTerrCoords ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}
	
	if( !ReadFile( trackFile, track.terrCoords,	track.numTerrCoords * sizeof(TERRCOORD), &numBytes,	NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		return E_FAIL;
	}


	// trigroups
	if( NULL == ( track.triGroups = new TRIGROUP[ track.numTriGroups ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_OUTOFMEMORY;
	}
//	ZeroMemory( track.triGroups, track.numTriGroups * sizeof(TRIGROUP) );

	if( !ReadFile( trackFile, track.triGroups, track.numTriGroups * sizeof(TRIGROUP), &numBytes,	NULL ) ){
		sprintf( desc, "This file was successfully opened but it could not be read: %s", filepath );
		MyAlert( NULL, desc );
		CloseHandle( trackFile );
		return E_FAIL;
	}

	CloseHandle( trackFile );


	if( FAILED( InitTriRecur() ) ){
		return E_FAIL;
	}

	UpdateTerrainLighting( track.entireRegion );

	return S_OK;
}





HRESULT CMyApplication::InitTriRecur(){

	DWORD scale,depth,i;

	ZeroMemory( triRecur_bits, TRIGROUP_NUM_BITS * sizeof(BYTE) );

	for(depth=0, scale=8; depth < TRIGROUP_NUM_DEPTHS; depth++){ 

		if( ( depth % 2 ) == 1 ){
			scale /= 2;
		}

		for(i=0; i < 8; i++){
			triRecur_incCoordPos[depth][i] = ( triRecur_incZ[i] * track.width + triRecur_incX[i] ) * scale;

			triRecur_incWorldX[depth][i] = triRecur_incX[i] * TERR_SCALE * scale;
			triRecur_incWorldZ[depth][i] = triRecur_incZ[i] * TERR_SCALE * scale;

			triRecur_incTextureU[depth][i] = (FLOAT)triRecur_incX[i] * TERR_TEXTURE_SCALE * scale;
			triRecur_incTextureV[depth][i] = (FLOAT)triRecur_incZ[i] * TERR_TEXTURE_SCALE * scale;

			triRecur_incShadowTextureU[depth][i] = (FLOAT)triRecur_incX[i] * TERR_SHADOWTEXTURE_SCALE * scale;
			triRecur_incShadowTextureV[depth][i] = (FLOAT)triRecur_incZ[i] * TERR_SHADOWTEXTURE_SCALE * scale;
		}
	}

	// arrays for triGroups traversal
	if( NULL == ( triRecur_indexAtRenderPos = new WORD[ MAX_RENDERED_COORDS ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_OUTOFMEMORY;
	}	

	if( NULL == ( triRecur_typeAtRenderPos = new BYTE[ MAX_RENDERED_COORDS ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_OUTOFMEMORY;
	}

	return S_OK;
}





VOID CMyApplication::ReleaseTrack(){

	for(DWORD i=0; i < track.numTerrTypes; i++){
		SAFE_ARRAYDELETE( track.terrTypeIndices[i] );
		SAFE_ARRAYDELETE( track.terrTypeBlendIndices[i] );
		SAFE_ARRAYDELETE( track.terrTypeBlendOnIndices[i] );
	}
	SAFE_ARRAYDELETE( track.terrTypeRuns );
	SAFE_ARRAYDELETE( track.terrTypeNodes );

	SAFE_ARRAYDELETE( triRecur_indexAtRenderPos );
	SAFE_ARRAYDELETE( triRecur_typeAtRenderPos );

	SAFE_ARRAYDELETE( track.navCoords );
	SAFE_ARRAYDELETE( track.navCoordLookupRuns );
	SAFE_ARRAYDELETE( track.navCoordLookupNodes );

	SAFE_ARRAYDELETE( track.terrCoords );
}




// splitTri & related funcs don't scale height

FLOAT TriGroupError( DWORD depth, DWORD branch, DWORD dataIndex, DWORD direction,
										FLOAT leftHeight, FLOAT topHeight, FLOAT rightHeight, DWORD pos ){

	FLOAT error;
	FLOAT newTopHeight = ( leftHeight + rightHeight ) / 2;

	error = fabs( newTopHeight - triRecur_terrCoords[pos].height );

	if( depth < TRIGROUP_MAX_DEPTH - 1 ){

		DWORD leftDataIndex = dataIndex + ( 2 << depth ) + branch;

		DWORD leftDirection = ( direction + 5 ) & 7;

		error += TriGroupError( depth + 1, branch << 1, leftDataIndex, leftDirection,
			topHeight, newTopHeight, leftHeight,
			pos - triRecur_incCoordPos[depth][leftDirection] );
			
		DWORD rightDirection = ( direction + 3 ) & 7;

		error += TriGroupError( depth + 1, ( branch << 1 ) + 1, leftDataIndex + 1, rightDirection,
			rightHeight, newTopHeight, topHeight,
			pos - triRecur_incCoordPos[depth][rightDirection] );
	}

	return error;
}



BOOL SplitTriIfForced( DWORD depth, DWORD branch, DWORD dataIndex, DWORD direction,
											 FLOAT leftHeight, FLOAT topHeight, FLOAT rightHeight, DWORD pos ){

	BOOL isSplit = FALSE;
	FLOAT newTopHeight;

	if( triRecur_splitCoords[pos] ){

		isSplit = TRUE;

		newTopHeight = triRecur_terrCoords[pos].height;
	}
	else{
		newTopHeight = ( leftHeight + rightHeight ) / 2;
	}

	DWORD leftDataIndex = dataIndex + ( 2 << depth ) + branch;

	DWORD leftDirection = ( direction + 5 ) & 7;

	if( depth < TRIGROUP_MAX_DEPTH - 1){

		if( SplitTriIfForced( depth + 1, branch << 1, leftDataIndex, leftDirection,
			topHeight, newTopHeight, leftHeight,
			pos - triRecur_incCoordPos[depth][leftDirection] ) ){
			isSplit = TRUE;
		}

		DWORD rightDirection = ( direction + 3 ) & 7;

		if( SplitTriIfForced( depth + 1, ( branch << 1 ) + 1, leftDataIndex + 1, rightDirection,
			rightHeight, newTopHeight, topHeight,
			pos - triRecur_incCoordPos[depth][rightDirection] ) ){
			isSplit = TRUE;
		}
	}

	if( isSplit ){
		triRecur_bits[dataIndex] = 1;
		triRecur_splitCoords[pos] = TRUE;
		return TRUE;
	}

	return FALSE;
}




BOOL SplitTri( DWORD depth, DWORD branch, DWORD dataIndex, DWORD direction,
							FLOAT leftHeight, FLOAT topHeight, FLOAT rightHeight, DWORD pos ){

	BOOL isSplit = FALSE;
	FLOAT newTopHeight;

	if( triRecur_splitCoords[pos] || 
		TriGroupError( depth, branch, dataIndex, direction,
			leftHeight, topHeight, rightHeight, pos ) > triRecur_triGroupMaxError ){

		isSplit = TRUE;

		newTopHeight = triRecur_terrCoords[pos].height; //pTerrCoord->height;
	}
	else{
		newTopHeight = ( leftHeight + rightHeight ) / 2;
	}

	DWORD leftDataIndex = dataIndex + ( 2 << depth ) + branch;

	DWORD leftDirection = ( direction + 5 ) & 7;

	if( depth < TRIGROUP_MAX_DEPTH - 1 ){

		if( SplitTri( depth + 1, branch << 1, leftDataIndex, leftDirection,
			topHeight, newTopHeight, leftHeight,
			pos - triRecur_incCoordPos[depth][leftDirection] ) ){
			isSplit = TRUE;
		}

		DWORD rightDirection = ( direction + 3 ) & 7;

		BOOL rightIsSplit = SplitTri( depth + 1, ( branch << 1 ) + 1, leftDataIndex + 1, rightDirection,
			rightHeight, newTopHeight, topHeight,
			pos - triRecur_incCoordPos[depth][rightDirection] );

		if( rightIsSplit ){ // if right splits, go back and check for forced splits on left
			SplitTriIfForced( depth + 1, branch << 1, leftDataIndex, leftDirection,
				topHeight, newTopHeight, leftHeight,
				pos - triRecur_incCoordPos[depth][leftDirection] );
			isSplit = TRUE;
		}
	}

	if( isSplit ){
		triRecur_bits[dataIndex] = 1;
		triRecur_splitCoords[pos] = TRUE;
		return TRUE;
	}

	return FALSE;
}



HRESULT CMyApplication::SetTerrainLOD( FLOAT maxError ){

	DWORD tgz, tgx, cz, cx, i;
	

	ZeroMemory( track.triGroups, track.numTriGroups * sizeof(TRIGROUP) );
	
	if( NULL == ( triRecur_splitCoords = new BYTE[ track.numTerrCoords ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_OUTOFMEMORY;
	}

	ZeroMemory( triRecur_splitCoords, track.numTerrCoords * sizeof(BYTE) );

// set all tris on type borders to be split
	TERRTYPENODE* pTopNode;
	TERRTYPENODE* pBottomNode;
	BYTE oldTopType, newTopType;
	BYTE oldBottomType, newBottomType;

	for(cz=0; cz < track.depth - 1; cz++){

		pTopNode = &track.terrTypeRuns[cz + 1];
		pBottomNode = &track.terrTypeRuns[cz];

		oldTopType = newTopType = pTopNode->type;
		oldBottomType = newBottomType = pBottomNode->type;
		cx = 1;

		if( newTopType == newBottomType ){
			if( pTopNode->nextStartX < pBottomNode->nextStartX ){
				cx = pTopNode->nextStartX;
				Advance( pTopNode, track.terrTypeNodes );
				newTopType = pTopNode->type;
			}
			else if( pBottomNode->nextStartX < pTopNode->nextStartX ){
				cx = pBottomNode->nextStartX;
				Advance( pBottomNode, track.terrTypeNodes );
				newBottomType = pBottomNode->type;
			}
			else{ // runs are changing at same x value
				cx = pTopNode->nextStartX;
				Advance( pTopNode, track.terrTypeNodes );
				Advance( pBottomNode, track.terrTypeNodes );
				newTopType = pTopNode->type;
				newBottomType = pBottomNode->type;
			}
		}
		else{
			if( cx == pTopNode->nextStartX ){
				oldTopType = newTopType;
				Advance( pTopNode, track.terrTypeNodes );
				newTopType = pTopNode->type;
			}
			if( cx == pBottomNode->nextStartX ){
				oldBottomType = newBottomType;
				Advance( pBottomNode, track.terrTypeNodes );
				newBottomType = pBottomNode->type;
			}
		}


		while( cx < track.width ){

			if( oldBottomType != newBottomType ){
				triRecur_splitCoords[cz * track.width + cx - 1] = 1;
				triRecur_splitCoords[cz * track.width + cx] = 1;
			}

			if( oldBottomType != oldTopType ){
				triRecur_splitCoords[cz * track.width + cx - 1] = 1;
				triRecur_splitCoords[(cz + 1) * track.width + cx - 1] = 1;
			}

			if( newBottomType != newTopType ){
				triRecur_splitCoords[cz * track.width + cx] = 1;
				triRecur_splitCoords[(cz + 1) * track.width + cx] = 1;
			}

			if( oldTopType != newTopType ){
				triRecur_splitCoords[(cz + 1) * track.width + cx - 1] = 1;
				triRecur_splitCoords[(cz + 1) * track.width + cx] = 1;
			}
			
			oldTopType = newTopType;
			oldBottomType = newBottomType;
			cx++;

			if( newTopType == newBottomType ){
				if( pTopNode->nextStartX < pBottomNode->nextStartX ){
					oldTopType = newTopType;
					cx = pTopNode->nextStartX;
					Advance( pTopNode, track.terrTypeNodes );
					newTopType = pTopNode->type;
				}
				else if( pBottomNode->nextStartX < pTopNode->nextStartX ){
					oldBottomType = newBottomType;
					cx = pBottomNode->nextStartX;
					Advance( pBottomNode, track.terrTypeNodes );
					newBottomType = pBottomNode->type;
				}
				else{ // runs are changing at same x value
					oldTopType = newTopType;
					oldBottomType = newBottomType;
					cx = pTopNode->nextStartX;
					Advance( pTopNode, track.terrTypeNodes );
					Advance( pBottomNode, track.terrTypeNodes );
					newTopType = pTopNode->type;
					newBottomType = pBottomNode->type;
				}
			}
			else{
				if( cx == pTopNode->nextStartX ){
					oldTopType = newTopType;
					Advance( pTopNode, track.terrTypeNodes );
					newTopType = pTopNode->type;
				}
				if( cx == pBottomNode->nextStartX ){
					oldBottomType = newBottomType;
					Advance( pBottomNode, track.terrTypeNodes );
					newBottomType = pBottomNode->type;
				}
			}
		}
	}

	triRecur_terrCoords = track.terrCoords;
	triRecur_triGroupMaxError = maxError;

	for(tgz=1, cz=TRIGROUP_WIDTH; tgz < track.triGroupsDepth - 1;
		tgz++, cz += TRIGROUP_WIDTH){ // skip border groups

		for(tgx=1, cx=TRIGROUP_WIDTH; tgx < track.triGroupsWidth - 1;
			tgx++, cx += TRIGROUP_WIDTH){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				DWORD mask = 0x00000001;
				for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( track.triGroups[ tgz * track.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
				}
			}

			SplitTri( 0, 0, 0, 5,
				track.terrCoords[ ( cz ) * track.width + cx + TRIGROUP_WIDTH ].height,
				track.terrCoords[ ( cz ) * track.width + cx ].height,
				track.terrCoords[ ( cz + TRIGROUP_WIDTH ) * track.width + cx ].height,
				( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX );

			SplitTri( 0, 1, 1, 1,
				track.terrCoords[ ( cz + TRIGROUP_WIDTH ) * track.width + cx ].height,
				track.terrCoords[ ( cz + TRIGROUP_WIDTH ) * track.width + cx + TRIGROUP_WIDTH ].height,
				track.terrCoords[ ( cz ) * track.width + cx + TRIGROUP_WIDTH ].height,
				( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX );

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				DWORD mask = 0x00000001;
				for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
					if( triRecur_bits[i * 32 + bit] ){
						track.triGroups[ tgz * track.triGroupsWidth + tgx ].data[i] |= mask;
					}
				}
			}

		}
	}


	for(DWORD pass=0; pass < 4; pass++){// 4 more passes to check for forced splits

		for(tgz=0, cz=0; tgz < track.triGroupsDepth; tgz++, cz += TRIGROUP_WIDTH){ // include border groups

			for(tgx=0, cx=0; tgx < track.triGroupsWidth; tgx++, cx += TRIGROUP_WIDTH){
		
				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						triRecur_bits[i * 32 + bit] = ( track.triGroups[ tgz * track.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
					}
				}

				SplitTriIfForced( 0, 0, 0, 5,
					track.terrCoords[ ( cz ) * track.width + cx + TRIGROUP_WIDTH ].height,
					track.terrCoords[ ( cz ) * track.width + cx ].height,
					track.terrCoords[ ( cz + TRIGROUP_WIDTH ) * track.width + cx ].height,
					( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX );

				SplitTriIfForced( 0, 1, 1, 1,
					track.terrCoords[ ( cz + TRIGROUP_WIDTH ) * track.width + cx ].height,
					track.terrCoords[ ( cz + TRIGROUP_WIDTH ) * track.width + cx + TRIGROUP_WIDTH ].height,
					track.terrCoords[ ( cz ) * track.width + cx + TRIGROUP_WIDTH ].height,
					( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX );

				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						if( triRecur_bits[i * 32 + bit] ){
							track.triGroups[ tgz * track.triGroupsWidth + tgx ].data[i] |= mask;
						}
					}
				}

			}
		}
	}

	delete [] triRecur_splitCoords;

	return S_OK;
}





/*
HRESULT CMyApplication::InitTerrainTypes(){

	TERRTYPENODE* pCurrNode;
	WORD nextAvailNode = 1; // waste 0th node so that 0 can indicate NULL
	BYTE* pBitmapData;

	HBITMAP hbm;

	CHAR filePath[256];
	strcpy( filePath, currTrackFilePath );
	strcat( filePath, "types.bmp" );

  if( !( hbm = (HBITMAP)LoadImage( NULL, filePath, IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE|LR_CREATEDIBSECTION ) ) ){
		return E_FAIL;
	}

	BITMAP bm;
	GetObject( hbm, sizeof(BITMAP), &bm );
	CloseHandle( hbm );

//	if( bm.bmWidthBytes != bm.bmWidth ){
//		return E_FAIL;
//	}

	pBitmapData = (BYTE*)bm.bmBits;

	track.terrTypeRuns = new TERRTYPENODE[ track.depth ];

	for(WORD z=0; z < track.depth; z++){
		pCurrNode = &track.terrTypeRuns[ z ];
		pCurrNode->type = pBitmapData[ z * bm.bmWidthBytes ];
		pCurrNode->next = 0;
		for(WORD x=0; x < track.width; x++){
			if( pBitmapData[ z * bm.bmWidthBytes + x ] != pCurrNode->type ){
				pCurrNode->nextStartX = x;
				pCurrNode->next = nextAvailNode;
				nextAvailNode++;
				if( nextAvailNode > MAX_TERRAIN_TYPE_NODES ){
					return E_FAIL;
				}
				pCurrNode = &track.terrTypeNodes[ pCurrNode->next ];
				pCurrNode->type = pBitmapData[ z * bm.bmWidthBytes + x ];
				pCurrNode->next = 0;
			}
		}
		pCurrNode->nextStartX = track.width;
	}


	for(DWORD i=0; i < terr.numTypes; i++){

		track.terrTypeIndices[i] = new WORD[ MAX_RENDERED_COORDS * 6 ];

		track.terrTypeBlendIndices[ i ] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 8 ) * 6];

		track.terrTypeBlendOnIndices[ i ] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 16 ) * 6 ];
	}


	return S_OK;
}
*/



WORD CMyApplication::GetNavCoordAt( INT32 x, INT32 z ){

	NAVCOORDLOOKUPNODE* pNode = &track.navCoordLookupRuns[z];

	while( pNode->nextStartX <= x ){
		pNode = &track.navCoordLookupNodes[pNode->next];
	}

	return pNode->navCoord;
}





VOID CMyApplication::UpdateTerrainLighting( CRectRegion region ){

	DWORD z,x;
	FLOAT xRise, zRise;
	const FLOAT run = 2 * TERR_SCALE;

	D3DVECTOR vSunlight = D3DVECTOR( // this is actually the opposite of it's true direction
		-cosf( track.sunlightPitch ) * cosf( track.sunlightDirection ),
		sinf( track.sunlightPitch ),
		-cosf( track.sunlightPitch ) * sinf( track.sunlightDirection ) );
	D3DVECTOR vGroundNormal;

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
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.startX = 1;
		}

		if( doMaxX ){
			x = track.width - 1;
			xRise = 0;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.endX = track.width - 2;
		}

		for(x=region.startX; x <= region.endX; x++){

			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );
		}

		region.startZ = 1;
	}

	if( doMaxZ ){
		z = track.depth - 1;
		zRise = 0;

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.startX = 1;
		}

		if( doMaxX ){
			x = track.width - 1;
			xRise = 0;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.endX = track.width - 2;
		}

		for(x=region.startX; x <= region.endX; x++){

			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );
		}

		region.endZ = track.depth - 2;
	}

	for(z=region.startZ; z <= region.endZ; z++){

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;

			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.startX = 1;
		}

		if( doMaxX ){
			x = track.width - 1;
			xRise = 0;
			
			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;

			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.endX = track.width - 2;
		}

		for(x=region.startX; x <= region.endX; x++){

			zRise = ( track.terrCoords[ ( z - 1 ) * track.width + x ].height - 
				track.terrCoords[ ( z + 1 ) * track.width + x ].height ) * TERR_HEIGHT_SCALE;
			
			xRise = ( track.terrCoords[ z * track.width + ( x - 1 ) ].height - 
				track.terrCoords[ z * track.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			// vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			track.terrCoords[ z * track.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );
		}
	}	 
}




inline TERRLVERTEX ProjectCoord( INT32 x, INT32 y, INT32 z, FLOAT tu, FLOAT tv, BYTE lightIntensity ){
	TERRLVERTEX	vertex;

	vertex.x = x;
	vertex.y = y;
	vertex.z = z;

	vertex.color = ( lightIntensity << 16 ) | ( lightIntensity << 8 ) | ( lightIntensity );

	vertex.tu = tu;
	vertex.tv = tv;

	return vertex;
}



inline VOID	UpdateIndexPtr( WORD* &pTerrCurrTypeIndex, INT32 ux,
													  TERRTYPENODE* &pCurrTerrTypeNode, WORD** pTerrTypeIndex,
														TERRTYPENODE* terrTypeNodes){
	if( ux == pCurrTerrTypeNode->nextStartX ){
		pTerrTypeIndex[ pCurrTerrTypeNode->type ] = pTerrCurrTypeIndex;
		pCurrTerrTypeNode = &terrTypeNodes[ pCurrTerrTypeNode->next ];
		pTerrCurrTypeIndex = pTerrTypeIndex[ pCurrTerrTypeNode->type ];
	}
}




int _weird = 0;
int _scale = 64;

inline VOID ProcessTri( DWORD depth, DWORD branch, DWORD dataIndex, DWORD direction,
											 WORD leftIndex, WORD topIndex, WORD rightIndex,
											 DWORD renderPos,
											 TERRCOORD* pTerrCoord,
											 DWORD wx, DWORD wz, FLOAT tu, FLOAT tv ){
	WORD newTopIndex;
	WORD** ppIndex;

	if( triRecur_bits[dataIndex] ){

		if( 0 == ( newTopIndex = triRecur_indexAtRenderPos[renderPos] ) ){
			triRecur_pVertex->x = wx;
			triRecur_pVertex->y = pTerrCoord->height * TERR_HEIGHT_SCALE;
			triRecur_pVertex->z = wz;

            // temp
            {
                if( _weird )
                {
                    float offx = ( (float)( pTerrCoord->height / _scale % 9 ) - 4.0 ) * 0.25;
                    float offz = ( (float)( ( wx * wz / _scale / _scale ) % 9 ) - 4.0 ) * 0.25;

                    triRecur_pVertex->x += offx;
                    triRecur_pVertex->z += offz;
                }
            }

			triRecur_pVertex->color = 0xFF000000 | ( pTerrCoord->lightIntensity << 16 ) | ( pTerrCoord->lightIntensity << 8 ) | ( pTerrCoord->lightIntensity );

//			triRecur_pVertex->specular = 0;

			triRecur_pVertex->tu = tu;
			triRecur_pVertex->tv = tv;

			newTopIndex = triRecur_indexAtRenderPos[renderPos] = triRecur_numVertices;
			triRecur_numVertices++;
			triRecur_pVertex++;
		}

		DWORD leftDataIndex = dataIndex + ( 2 << depth ) + branch;

		DWORD leftDirection = ( direction + 5 ) & 7;

		ProcessTri( depth + 1, branch << 1, leftDataIndex, leftDirection, 
			topIndex, newTopIndex, leftIndex,
			renderPos - triRecur_incRenderPos[depth][leftDirection],
			pTerrCoord - triRecur_incCoordPos[depth][leftDirection],
			wx - triRecur_incWorldX[depth][leftDirection],
			wz - triRecur_incWorldZ[depth][leftDirection],
			tu - triRecur_incTextureU[depth][leftDirection],
			tv - triRecur_incTextureV[depth][leftDirection] );

		DWORD rightDirection = ( direction + 3 ) & 7;

		ProcessTri( depth + 1, ( branch << 1 ) + 1, leftDataIndex + 1, rightDirection,
			rightIndex, newTopIndex, topIndex,
			renderPos - triRecur_incRenderPos[depth][rightDirection],
			pTerrCoord - triRecur_incCoordPos[depth][rightDirection],
			wx - triRecur_incWorldX[depth][rightDirection],
			wz - triRecur_incWorldZ[depth][rightDirection],
			tu - triRecur_incTextureU[depth][rightDirection],
			tv - triRecur_incTextureV[depth][rightDirection] );
	}
	else{

		if( depth == TRIGROUP_MAX_DEPTH ){ // type blends only occur on smallest tris
			DWORD topType = triRecur_typeAtRenderPos[renderPos];
			DWORD leftType = triRecur_typeAtRenderPos[renderPos + triRecur_incRenderPos[TRIGROUP_MAX_DEPTH - 2][(direction + 3) & 7]];
			DWORD rightType = triRecur_typeAtRenderPos[renderPos + triRecur_incRenderPos[TRIGROUP_MAX_DEPTH - 2][(direction + 5) & 7]];

//			DWORD topType = depth % 6;
//			DWORD leftType = depth % 6;
//			DWORD rightType = depth % 6;
			
			if( leftType != topType ){
				// add tri w/ type of left vertex
				triRecur_pTerrTypeBlendIndex[leftType][0] = topIndex;
				triRecur_pTerrTypeBlendIndex[leftType][1] = leftIndex; 
				triRecur_pTerrTypeBlendIndex[leftType][2] = rightIndex; 
				triRecur_pTerrTypeBlendIndex[leftType] += 3;

				if( leftType == rightType ){ // turn on alpha of both left & right if same type
					triRecur_pTerrTypeBlendOnIndex[leftType][0] = leftIndex; 
					triRecur_pTerrTypeBlendOnIndex[leftType][1] = rightIndex; 
					triRecur_pTerrTypeBlendOnIndex[leftType] += 2;
				}
				else{
					triRecur_pTerrTypeBlendOnIndex[leftType][0] = leftIndex; 
					triRecur_pTerrTypeBlendOnIndex[leftType]++;
				}
			}
			if( rightType != topType && rightType != leftType ){
				// add tri w/ type of right vertex
				triRecur_pTerrTypeBlendIndex[rightType][0] = topIndex;
				triRecur_pTerrTypeBlendIndex[rightType][1] = leftIndex; 
				triRecur_pTerrTypeBlendIndex[rightType][2] = rightIndex; 
				triRecur_pTerrTypeBlendIndex[rightType] += 3;

				triRecur_pTerrTypeBlendOnIndex[rightType][0] = rightIndex; 
				triRecur_pTerrTypeBlendOnIndex[rightType]++;
			}
		}

		ppIndex = &triRecur_pTypeIndex[triRecur_typeAtRenderPos[renderPos]];

		//ppIndex = &triRecur_pTypeIndex[depth % 6];

		(*ppIndex)[0] = topIndex;
		(*ppIndex)[1] = leftIndex;
		(*ppIndex)[2] = rightIndex;

		(*ppIndex) += 3;

	}
}



HRESULT CMyApplication::RenderTerrain(){

	DWORD tgcz, tgcx, tgrz, tgrx, triGroupNum; // coord-based and render-based trigroup positions
	DWORD cz, cx, rz, rx; 
	TRIGROUP* pTriGroup = track.triGroups;
	DWORD depth, scale, bit, i;
	DWORD coordRenderWidth, groupRenderWidth;
	TERRLVERTEX *terrVertices;
	TERRTLVERTEX* terrTransformedVertices;
	WORD* pIndex;
	HRESULT hr;
	DWORD mask;
	TERRCOORD* pTerrCoord;
	DWORD wx, wz; 
	FLOAT tu, tv;
	TERRTYPENODE* pNode;
	DWORD currStartX;


	D3DMATRIX matWorld;
	D3DUtil_SetIdentityMatrix(matWorld);
	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );

	groupRenderWidth = track.renderedTriGroups.endX - track.renderedTriGroups.startX + 1;
	coordRenderWidth = track.renderedTerrCoords.endX - track.renderedTerrCoords.startX + 1;

	for(depth=0, scale=8; depth < TRIGROUP_NUM_DEPTHS; depth++){ 

		if( ( depth % 2 ) == 1 ){
			scale /= 2;
		}

		for(i=0; i < 8; i++){
			triRecur_incRenderPos[depth][i] = ( triRecur_incZ[i] * coordRenderWidth + triRecur_incX[i] ) * scale;
		}
	}

//// local type map
	for(cz=track.renderedTerrCoords.startZ, rz=0; cz <= track.renderedTerrCoords.endZ; cz++, rz++){

		pNode = &track.terrTypeRuns[cz];

		while( pNode->nextStartX <= track.renderedTerrCoords.startX ){ Advance( pNode, track.terrTypeNodes ); }

		currStartX = track.renderedTerrCoords.startX;

		while( pNode->nextStartX < track.renderedTerrCoords.endX ){
			for(i=currStartX; i < pNode->nextStartX; i++){
				triRecur_typeAtRenderPos[rz * coordRenderWidth - track.renderedTerrCoords.startX + i] =
					pNode->type;
			}
			currStartX = pNode->nextStartX;
			Advance( pNode, track.terrTypeNodes );
		}

		FillMemory( &triRecur_typeAtRenderPos[ rz * coordRenderWidth + currStartX - track.renderedTerrCoords.startX ],
			track.renderedTerrCoords.endX - currStartX + 1,
			pNode->type );
	}
//////////

	if( FAILED( track.terrVB->Lock( DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS | DDLOCK_WAIT,	
		(VOID**)&terrVertices, NULL ) ) ){
		return E_FAIL;
	}

	for(i=0; i < track.numTerrTypes; i++){
		triRecur_pTypeIndex[i] = track.terrTypeIndices[i];
		triRecur_pTerrTypeBlendIndex[i] = track.terrTypeBlendIndices[i];
		triRecur_pTerrTypeBlendOnIndex[i] = track.terrTypeBlendOnIndices[i];
	}

	triRecur_numVertices = 0;
	triRecur_pVertex = terrVertices;

	ZeroMemory( triRecur_indexAtRenderPos, MAX_RENDERED_COORDS * sizeof(WORD) );

	
	rz = 0;
	wz = track.renderedTerrCoords.startZ * TERR_SCALE;
	pTerrCoord = &track.terrCoords[ track.renderedTerrCoords.startZ * track.width + track.renderedTerrCoords.startX ];
	tv = (FLOAT)track.renderedTerrCoords.startZ * TERR_TEXTURE_SCALE;
	for(cz=track.renderedTerrCoords.startZ; cz <= track.renderedTerrCoords.endZ;
		cz += TRIGROUP_WIDTH,	rz += TRIGROUP_WIDTH, wz += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
		tv += ( TERR_TEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

		rx = 0;
		wx = track.renderedTerrCoords.startX * TERR_SCALE;
		tu = (FLOAT)track.renderedTerrCoords.startX * TERR_TEXTURE_SCALE;
		for(cx=track.renderedTerrCoords.startX; cx <= track.renderedTerrCoords.endX;
			cx += TRIGROUP_WIDTH, rx += TRIGROUP_WIDTH,	wx += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
			tu += ( TERR_TEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

			pTerrCoord = &track.terrCoords[ cz * track.width + cx ];

			triRecur_pVertex->x = wx;
			triRecur_pVertex->y = pTerrCoord->height * TERR_HEIGHT_SCALE;
			triRecur_pVertex->z = wz;

			triRecur_pVertex->color = 0xFF000000 | ( pTerrCoord->lightIntensity << 16 ) | ( pTerrCoord->lightIntensity << 8 ) | ( pTerrCoord->lightIntensity );

			triRecur_pVertex->tu = tu;
			triRecur_pVertex->tv = tv;

			triRecur_indexAtRenderPos[ rz * coordRenderWidth + rx ] = triRecur_numVertices;
			triRecur_numVertices++;
			triRecur_pVertex++;
		}
	}

	// fill groupCornerIndices // track.width & height are 1 excess!!
	triGroupNum = track.renderedTriGroups.startZ * track.triGroupsWidth + track.renderedTriGroups.startX;
	cz = track.renderedTerrCoords.startZ;
	rz = 0;
	for(tgcz=track.renderedTriGroups.startZ, tgrz=0; tgcz <= track.renderedTriGroups.endZ;
		tgcz++, tgrz++,	cz += TRIGROUP_WIDTH, rz+= TRIGROUP_WIDTH, triGroupNum += track.triGroupsWidth - groupRenderWidth){

		cx = track.renderedTerrCoords.startX;
		rx = 0;
		for(tgcx=track.renderedTriGroups.startX, tgrx=0; tgcx <= track.renderedTriGroups.endX;
			tgcx++, tgrx++, cx += TRIGROUP_WIDTH, rx+= TRIGROUP_WIDTH, triGroupNum++){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				mask = 0x00000001;
				for(bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( track.triGroups[ triGroupNum ].data[i] & mask )? TRUE : FALSE;
				}
			}
			
			ProcessTri( 0, 0, 0, 5,
				( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
				( tgrz ) * ( groupRenderWidth + 1 ) + tgrx, 
				( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
				( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
				&track.terrCoords[ ( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX ],
				( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
				( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
				(FLOAT)( cx + TRIGROUP_CENTERZ ) * TERR_TEXTURE_SCALE,
				(FLOAT)( cz + TRIGROUP_CENTERX ) * TERR_TEXTURE_SCALE );

			ProcessTri( 0, 1, 1, 1,
				( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
				( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
				( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
				( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
				&track.terrCoords[ ( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX ],
				( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
				( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
				(FLOAT)( cx + TRIGROUP_CENTERZ ) * TERR_TEXTURE_SCALE,
				(FLOAT)( cz + TRIGROUP_CENTERX ) * TERR_TEXTURE_SCALE );
		}
	}

	if( FAILED( track.terrVB->Unlock() ) ){
		return E_FAIL;
	}

	while( ( hr = track.terrTransformedVB->ProcessVertices( D3DVOP_TRANSFORM | D3DVOP_CLIP,
		0,
		triRecur_numVertices,
		track.terrVB,
		0,
		m_pd3dDevice,
		0 ) ) == DDERR_SURFACEBUSY ){}

	if( FAILED( hr ) ){
		MyAlert(&hr, "Unable to process terrain vertices" );
		return E_FAIL;
	}

	m_misc = triRecur_numVertices;

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_COLORVERTEX, TRUE ); 

//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

    if( bWireframeTerrain )
    {
        m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME );
    }

	INT32 myMaxIndices = 65535; // 63; // 255; //1023; // 2046; // 4095; // 8190; //16383; // 32766; // 65535
	INT32 numRemainingIndices;

	//m_misc = triRecur_numVertices;

	for(i=0; i < track.numTerrTypes; i++){

		if( FAILED( hr = m_pd3dDevice->SetTexture( 0, (GetTexture( track.terrTypeTextureFilenames[i] ))->m_pddsSurface ) ) ){
			MyAlert(&hr, "Unable to set texture for terrain rendering" );
			return E_FAIL;
		}

		numRemainingIndices = triRecur_pTypeIndex[i] - track.terrTypeIndices[i];
		pIndex = track.terrTypeIndices[i];

		while( numRemainingIndices > myMaxIndices ){

			if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
				track.terrTransformedVB,
				0,
				triRecur_numVertices,
				pIndex,
				myMaxIndices, 0 ) ) ){
				MyAlert(&hr, "Unable to render terrain (1)" );
				return E_FAIL;
			}

			numRemainingIndices -= myMaxIndices;
			pIndex += myMaxIndices;
		}

        if( numRemainingIndices > 0 )
        {
		    if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
			    track.terrTransformedVB,
			    0,
			    triRecur_numVertices,
			    pIndex,
			    numRemainingIndices, 0 ) ) ){
			    MyAlert(&hr, "Unable to render terrain (2)" );
			    return E_FAIL;
		    }
        }
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );

    // temp
    if( !_weird )
    {
	for(i=0; i < track.numTerrTypes; i++){
		if( FAILED( hr = m_pd3dDevice->SetTexture( 0, (GetTexture( track.terrTypeTextureFilenames[i] ))->m_pddsSurface ) ) ){
			MyAlert(&hr, "Unable to set texture for terrain rendering" );
			return E_FAIL;
		}

		if( FAILED( track.terrTransformedVB->Lock( DDLOCK_WRITEONLY | DDLOCK_WAIT,	
			(VOID**)&terrTransformedVertices, NULL ) ) ){
			return E_FAIL;
		}

		for(pIndex = track.terrTypeBlendIndices[i]; pIndex < triRecur_pTerrTypeBlendIndex[i]; pIndex++){
			*((BYTE *)(&terrTransformedVertices[ *pIndex ].color)+3) = 0;// &= 0x00FFFFFF;
		}
		for(pIndex = track.terrTypeBlendOnIndices[i]; pIndex < triRecur_pTerrTypeBlendOnIndex[i]; pIndex++){
//			terrTransformedVertices[ *pIndex ].color |= 0xFF000000;
			*((BYTE *)(&terrTransformedVertices[ *pIndex ].color)+3) = 0xFF;// &= 0x00FFFFFF;
		}

		if( FAILED( track.terrTransformedVB->Unlock() ) ){
			return E_FAIL;
		}


        numRemainingIndices = triRecur_pTerrTypeBlendIndex[ i ] - track.terrTypeBlendIndices[ i ];

        if( numRemainingIndices > 0 )
        {
		    HRESULT hr;
		    if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
			    track.terrTransformedVB,
			    0,
			    triRecur_numVertices,
			    track.terrTypeBlendIndices[ i ],
			    numRemainingIndices, 0 ) ) ){
			    MyAlert(&hr, "Unable to render terrain (3)" );
			    return E_FAIL;
		    }
        }
	}
    }

    if( bWireframeTerrain )
    {
        m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID );
    }

//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_COLORVERTEX, FALSE ); 

	return S_OK;
}

	





inline VOID ShadowProcessTri( DWORD depth, DWORD branch, DWORD dataIndex, DWORD direction,
											 WORD leftIndex, WORD topIndex, WORD rightIndex,
											 DWORD renderPos,
											 TERRCOORD* pTerrCoord,
											 DWORD wx, DWORD wz, FLOAT tu, FLOAT tv ){
	WORD newTopIndex;
	WORD** ppIndex;

	if( triRecur_bits[dataIndex] ){

		newTopIndex = triRecur_indexAtRenderPos[renderPos];
		triRecur_transformedVertices[newTopIndex].tu = tu;
		triRecur_transformedVertices[newTopIndex].tv = tv;

		DWORD leftDataIndex = dataIndex + ( 2 << depth ) + branch;

		DWORD leftDirection = ( direction + 5 ) & 7;

		ShadowProcessTri( depth + 1, branch << 1, leftDataIndex, leftDirection, 
			topIndex, newTopIndex, leftIndex,
			renderPos - triRecur_incRenderPos[depth][leftDirection],
			pTerrCoord - triRecur_incCoordPos[depth][leftDirection],
			wx - triRecur_incWorldX[depth][leftDirection],
			wz - triRecur_incWorldZ[depth][leftDirection],
			tu - triRecur_incShadowTextureU[depth][leftDirection],
			tv - triRecur_incShadowTextureV[depth][leftDirection] );

		DWORD rightDirection = ( direction + 3 ) & 7;

		ShadowProcessTri( depth + 1, ( branch << 1 ) + 1, leftDataIndex + 1, rightDirection,
			rightIndex, newTopIndex, topIndex,
			renderPos - triRecur_incRenderPos[depth][rightDirection],
			pTerrCoord - triRecur_incCoordPos[depth][rightDirection],
			wx - triRecur_incWorldX[depth][rightDirection],
			wz - triRecur_incWorldZ[depth][rightDirection],
			tu - triRecur_incShadowTextureU[depth][rightDirection],
			tv - triRecur_incShadowTextureV[depth][rightDirection] );
	}
	else{

		triRecur_pTypeIndex[0][0] = topIndex;
		triRecur_pTypeIndex[0][1] = leftIndex;
		triRecur_pTypeIndex[0][2] = rightIndex;

		triRecur_pTypeIndex[0] += 3;

		// texture blends stuff here
	}
}




HRESULT CMyApplication::ApplyShadowTexture( FLOAT centerX, FLOAT centerZ, FLOAT radius ){

	INT32 tgcz, tgcx, tgrz, tgrx, triGroupNum; // coord-based and render-based trigroup positions
	INT32 cz, cx, rz, rx; 
	TRIGROUP* pTriGroup = track.triGroups;
	DWORD numVertices;
	DWORD depth, scale, bit, i, j;
	INT32 coordRenderWidth, groupRenderWidth, groupShadedWidth;
	TERRLVERTEX *terrVertices;
	TERRTLVERTEX* terrTransformedVertices;
	WORD* pIndex;
	HRESULT hr;
	DWORD mask;
	TERRCOORD* pTerrCoord;
	INT32 wx, wz; 
	FLOAT tu, tv;
	WORD* pIndexAtRenderPos;
	CRectRegion shadedTriGroups, shadedCoords;
	DWORD index;

	groupRenderWidth = track.renderedTriGroups.endX - track.renderedTriGroups.startX + 1;
	coordRenderWidth = track.renderedTerrCoords.endX - track.renderedTerrCoords.startX + 1;

	shadedTriGroups.Set(
		(INT32)( centerX - radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerZ - radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerX + radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerZ + radius ) >> TRIGROUP_WIDTHSHIFT );

	shadedTriGroups.Clip( track.renderedTriGroups.startX,
		track.renderedTriGroups.startZ,
		track.renderedTriGroups.endX,
		track.renderedTriGroups.endZ );

	shadedCoords.Set( shadedTriGroups.startX << TRIGROUP_WIDTHSHIFT,
		shadedTriGroups.startZ << TRIGROUP_WIDTHSHIFT,
		( ( shadedTriGroups.endX + 1 ) << TRIGROUP_WIDTHSHIFT ),
		( ( shadedTriGroups.endZ + 1 ) << TRIGROUP_WIDTHSHIFT ));

	groupShadedWidth = shadedTriGroups.endX - shadedTriGroups.startX + 1;

	for(depth=0, scale=8; depth < TRIGROUP_NUM_DEPTHS; depth++){ 

		if( ( depth % 2 ) == 1 ){
			scale /= 2;
		}

		for(i=0; i < 8; i++){
			triRecur_incRenderPos[depth][i] = ( triRecur_incZ[i] * coordRenderWidth + triRecur_incX[i] ) * scale;
		}
	}


	if( FAILED( track.terrTransformedVB->Lock( DDLOCK_WRITEONLY | DDLOCK_WAIT,	
		(VOID**)&triRecur_transformedVertices, NULL ) ) ){
		return E_FAIL;
	}

	//triRecur_numVertices = 0;
	triRecur_pTypeIndex[0] = track.terrTypeIndices[0]; // borrowing terrain type 0's index array

	
	rz = shadedCoords.startZ - track.renderedTerrCoords.startZ;
	wz = shadedCoords.startZ * TERR_SCALE;
	pTerrCoord = &track.terrCoords[ shadedCoords.startZ * track.width + shadedCoords.startX ];
	tv = ( (FLOAT)shadedCoords.startZ - centerZ / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f;
	for(cz=shadedCoords.startZ; cz <= shadedCoords.endZ;
		cz += TRIGROUP_WIDTH,	rz += TRIGROUP_WIDTH, wz += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
		tv += ( TERR_SHADOWTEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

		rx = shadedCoords.startX - track.renderedTerrCoords.startX;
		wx = shadedCoords.startX * TERR_SCALE;
		tu = ( (FLOAT)shadedCoords.startX - centerX / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f;
		for(cx=shadedCoords.startX; cx <= shadedCoords.endX;
			cx += TRIGROUP_WIDTH, rx += TRIGROUP_WIDTH,	wx += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
			tu += ( TERR_SHADOWTEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

			//pTerrCoord = &track.terrCoords[cz * track.width + cx];

			//triRecur_pVertex->x = wx;
			//triRecur_pVertex->y = pTerrCoord->height * TERR_HEIGHT_SCALE;
			//triRecur_pVertex->z = wz;

			//triRecur_pVertex->color = ( pTerrCoord->lightIntensity << 16 ) | ( pTerrCoord->lightIntensity << 8 ) | ( pTerrCoord->lightIntensity );
			index = triRecur_indexAtRenderPos[rz * coordRenderWidth + rx];
			triRecur_transformedVertices[index].tu = tu;
			triRecur_transformedVertices[index].tv = tv;

			//triRecur_indexAtRenderPos[rz * coordRenderWidth + rx] = triRecur_numVertices;
			//triRecur_numVertices++;
		}
	}

	triGroupNum = shadedTriGroups.startZ * track.triGroupsWidth + shadedTriGroups.startX;
	cz = shadedCoords.startZ;
	rz = shadedCoords.startZ - track.renderedTerrCoords.startZ;
	tgrz = shadedTriGroups.startZ - track.renderedTriGroups.startZ;
	for(tgcz=shadedTriGroups.startZ; tgcz <= shadedTriGroups.endZ;
		tgcz++, tgrz++,	cz += TRIGROUP_WIDTH, rz+= TRIGROUP_WIDTH, triGroupNum += track.triGroupsWidth - groupShadedWidth){

		cx = shadedCoords.startX;
		rx = shadedCoords.startX - track.renderedTerrCoords.startX;
		tgrx = shadedTriGroups.startX - track.renderedTriGroups.startX;
		for(tgcx=shadedTriGroups.startX; tgcx <= shadedTriGroups.endX;
			tgcx++, tgrx++, cx += TRIGROUP_WIDTH, rx+= TRIGROUP_WIDTH, triGroupNum++){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				mask = 0x00000001;
				for(bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( track.triGroups[ triGroupNum ].data[i] & mask )? TRUE : FALSE;
				}
			}
			
			if( ( (INT32)( centerZ - radius + 0.5f ) - cz ) + ( (INT32)( centerX - radius + 0.5f ) - cx ) <
				TRIGROUP_CENTERX + TRIGROUP_CENTERZ ){
				ShadowProcessTri( 0, 0, 0, 5,
					( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
					( tgrz ) * ( groupRenderWidth + 1 ) + tgrx, 
					( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
					( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
					&track.terrCoords[ ( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX ],
					( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
					( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
					( (FLOAT)( cx + TRIGROUP_CENTERX ) - centerX / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f,
					( (FLOAT)( cz + TRIGROUP_CENTERZ ) - centerZ / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f );
			}

			if( ( (INT32)( centerZ + radius + 0.5f ) - cz ) + ( (INT32)( centerX + radius + 0.5f ) - cx ) >
				TRIGROUP_CENTERX + TRIGROUP_CENTERZ ){
				ShadowProcessTri( 0, 1, 1, 1,
					( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
					( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
					( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
					( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
					&track.terrCoords[ ( cz + TRIGROUP_CENTERZ ) * track.width + cx + TRIGROUP_CENTERX ],
					( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
					( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
					( (FLOAT)( cx + TRIGROUP_CENTERX ) - centerX / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f,
					( (FLOAT)( cz + TRIGROUP_CENTERZ ) - centerZ / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f);
			}
		}
	}


	if( FAILED( track.terrTransformedVB->Unlock() ) ){
		return E_FAIL;
	}

	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );

    if( !bDisplayShadowTexture )
    {
	    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO );
	    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR );
	    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
    }

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGCOLOR, 0x00000000 );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );

	if( FAILED( hr = m_pd3dDevice->SetTexture( 0, shadowTexture.pddsSurface ) ) ){
		MyAlert(&hr, "Unable to set texture for shadow rendering" );
		return E_FAIL;
	}

	if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
		track.terrTransformedVB,
		0,
		triRecur_numVertices,
		track.terrTypeIndices[0],
		(triRecur_pTypeIndex[0] - track.terrTypeIndices[0]), 0 ) ) ){
		MyAlert(&hr, "Unable to render shadows" );
		return E_FAIL;
	}


	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGCOLOR,		 track.bgColor );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );

    if( !bDisplayShadowTexture )
    {
  	    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
	    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );
	    m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
    }


    if( bDisplayShadowTexture )
    {
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	FALSE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ); 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_POINT ); 
		
        TERRTLVERTEX rect[4];
        for( DWORD i=0; i < 4; i++ )
        {
            rect[i].color = 0xFFFFFFFF;
            rect[i].sz = 0.0f;
            rect[i].rhw = 1.0f;
            rect[i].specular = 0.0f;
        }

		rect[0].sx = 0.0f;
		rect[0].sy = 0.0f;
        rect[0].tu = 0.375f;
        rect[0].tv = 0.625f;

		rect[1].sx = 0.0f;
		rect[1].sy = 255;
        rect[1].tu = 0.625f;
        rect[1].tv = 0.625f;

		rect[2].sx = 255;
		rect[2].sy = 0.0;//m_vp.dwHeight;
        rect[2].tu = 0.375f;
        rect[2].tv = 0.375f;

		rect[3].sx = 255;
		rect[3].sy = 255;
        rect[3].tu = 0.625f;
        rect[3].tv = 0.375f;
		
		if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, TERRTLVERTEXDESC, 
			rect, 4, NULL ) ) ){
			MyAlert( &hr, "Unable to display shadow texture" );
			return hr;
		}
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	 TRUE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ); 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_LINEAR ); 
    }

	return S_OK;
}




FLOAT CMyApplication::DistToTerrain( D3DVECTOR vPosition, D3DVECTOR vDir, const FLOAT MAXDIST ){

	FLOAT heightAboveGround;
	FLOAT cx,cz,offx,offz,dist,hl,hr,dl,dr,testX,testZ,xDist,zDist,A,B,C;
	DWORD index;
	
	cx = floor( vPosition.x );
	cz = floor( vPosition.z );
	offx = vPosition.x - cx;
	offz = vPosition.z - cz;
	
	index = cz * track.width + cx;
	hl = (FLOAT)track.terrCoords[index].height * TERR_HEIGHT_SCALE;
	hr = (FLOAT)track.terrCoords[index + 1].height * TERR_HEIGHT_SCALE;
	dl = (FLOAT)track.terrCoords[index + track.width].height * TERR_HEIGHT_SCALE - hl;
	dr = (FLOAT)track.terrCoords[index + track.width + 1].height * TERR_HEIGHT_SCALE - hr;
		
	if( ( heightAboveGround = vPosition.y - ( ( offz * ( dr - dl ) + ( hr - hl ) ) * offx +	offz * dl +	hl ) ) <= 0 ){
		return heightAboveGround;
	}
	
	while( TRUE ){
		
		A = vDir.x * vDir.z * ( dr - dl );
		B = vDir.z * dl + 
			vDir.z * offx * ( dr - dl ) +
			vDir.x * offz * ( dr - dl ) +
			vDir.x * ( hr - hl ) -
			vDir.y;
		C = -vPosition.y +
			( offz * ( dr - dl ) + ( hr - hl ) ) * offx +
			offz * dl +
			hl;
		
		if( A != 0 ){
			if( 4*A*C < B*B ){
				if( B < 0 ){
					A = -A;
					B = -B;
					C = -C;
				}
			dist = (-B - sqrt(B*B - 4*A*C)) / (2*A); // find first root
				if( dist >= 0 ){ 
					testX = offx + vDir.x * dist;
					testZ = offz + vDir.z * dist;
					if( testX >= -g_EPSILON && testX <= ( 1.0f + g_EPSILON ) &&
						testZ >= -g_EPSILON && testZ <= ( 1.0f + g_EPSILON ) ){
						break; // first root valid, so we're done
					}
				}
				// first root invalid, so try second
				dist = 2*C / (-B - sqrt(B*B - 4*A*C));
				if( dist >= 0 ){
					testX = offx + vDir.x * dist;
					testZ = offz + vDir.z * dist;
					if( testX >= -g_EPSILON && testX <= ( 1.0f + g_EPSILON ) &&
						testZ >= -g_EPSILON && testZ <= ( 1.0f + g_EPSILON ) ){
						break; // second root valid
					}
				}
			}
			// else no solutions
		}
		else{
			dist = -C / B; // A == 0, so only one possible solution
			if( dist >= 0 ){
				testX = offx + vDir.x * dist;
				testZ = offz + vDir.z * dist;
				if( testX >= -g_EPSILON && testX <= ( 1.0f + g_EPSILON ) &&
					testZ >= -g_EPSILON && testZ <= ( 1.0f + g_EPSILON ) ){
					break;
				}
			}
		}
			
		// no valid solutions, try a different cell
			
		// relying on idea that vDir.z ~ 1/vDir.x (and vice-versa)
		if( vDir.x > 0 ){
			xDist = ( 1 - offx ) * fabs( vDir.z );
		}
		else{
			xDist = offx * fabs( vDir.z );
		}
		if( vDir.z > 0 ){
			zDist = ( 1 - offz ) * fabs( vDir.x );
		}
		else{
			zDist = offz * fabs( vDir.x );
		}
		
		if( xDist < zDist ){
			if( vDir.x < 0 ){
				cx--;
				if( floor(vPosition.x) - cx - 1 > MAXDIST * -vDir.x ||
					cx < 0 ){
					dist = MAXDIST;
					break;
				}
			}
			else{
				cx++;
				if( cx - floor(vPosition.x) - 1 > MAXDIST * vDir.x ||
					cx >= track.width ){
					dist = MAXDIST;
					break;
				}
			}
		}
		else{
			if( vDir.z < 0 ){
				cz--;
				if( floor(vPosition.z) - cz - 1 > MAXDIST * -vDir.z ||
					cz < 0 ){
					dist = MAXDIST;
					break;
				}
			}
			else{
				cz++;
				if( cz - floor(vPosition.z) - 1 > MAXDIST * vDir.z ||
					cz >= track.depth ){
					dist = MAXDIST;
					break;
				}
			}
		}
			
		offx = vPosition.x - cx;
		offz = vPosition.z - cz;
			
		index = cz * track.width + cx;
		hl = (FLOAT)track.terrCoords[index].height * TERR_HEIGHT_SCALE;
		hr = (FLOAT)track.terrCoords[index + 1].height * TERR_HEIGHT_SCALE;
		dl = (FLOAT)track.terrCoords[index + track.width].height * TERR_HEIGHT_SCALE - hl;
		dr = (FLOAT)track.terrCoords[index + track.width + 1].height * TERR_HEIGHT_SCALE - hr;
	}
		
	if( dist < 0 ){
		DWORD blah=0;
	}

	return dist;
}












