
#include "common.h"



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
WORD triRecur_numVertices;
TERRTLVERTEX* triRecur_transformedVertices;
WORD* triRecur_pTypeIndex[ MAX_TERRAIN_TYPES ];
DWORD triRecur_currType;
TERRLVERTEX* triRecur_pVertex;
WORD* triRecur_indexAtRenderPos;
BYTE* triRecur_typeAtRenderPos;
WORD* triRecur_pterrTypeBlendIndex[ MAX_TERRAIN_TYPES ];
WORD* triRecur_pterrTypeBlendOnIndex[ MAX_TERRAIN_TYPES ];
TERRCOORD* triRecur_terrCoords;
BYTE* triRecur_splitCoords;
FLOAT triRecur_triGroupMaxError;




void Advance( TERRTYPENODE* &pNode, TERRTYPENODE* nodes ){
	pNode = &nodes[ pNode->next ];
}



HRESULT CMyApplication::InitTrackInfo(){

	CHAR filePath[256];
	strcpy( filePath, currTrackFilePath );
	strcat( filePath, "info.dat" );

	HANDLE newFile;
	DWORD NumberOfBytesWritten;

	if( ( newFile = CreateFile( filePath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) == INVALID_HANDLE_VALUE ){
		return E_FAIL;
	}

	TRACKFILEINFO trackInfo;

	if( !ReadFile( newFile,
		&trackInfo,
		sizeof(trackInfo),     // number of bytes to write
		&NumberOfBytesWritten,  // number of bytes written
		NULL ) ){
		return E_FAIL;
	}

	terr.width = trackInfo.terrWidth;
	terr.depth = trackInfo.terrDepth;
	terr.area = terr.width * terr.depth;

	terr.triGroupsWidth = ( terr.width - 1 ) >> TRIGROUP_WIDTHSHIFT;
	terr.triGroupsDepth = ( terr.depth - 1 ) >> TRIGROUP_WIDTHSHIFT;
	terr.triGroupsArea = terr.triGroupsWidth * terr.triGroupsDepth;

	sunlightDirection = trackInfo.sunlightDirection;
	sunlightPitch = trackInfo.sunlightPitch;

	terr.numTypes = trackInfo.numTerrTypes;

	CopyMemory( &terr.textureFileNames[ 0 ][ 0 ],
		&(trackInfo.realTerrTextureFileNames[ 0 ][ 0 ]),
		sizeof(trackInfo.realTerrTextureFileNames) );

	return S_OK;
}



// commit mem for indices, verts, & type-node space
HRESULT CMyApplication::InitStaticTerrainStuff(){

	if( NULL == ( terr.typeNodes = new TERRTYPENODE[MAX_TERRAIN_TYPE_NODES] ) ){
		return E_OUTOFMEMORY;
	}
	
	return S_OK;
}



VOID CMyApplication::ReleaseStaticTerrainStuff(){

	SAFE_DELETE( terr.typeNodes );
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
		if( dataIndex > 509 ){
			DWORD blah=0;
		}
		triRecur_bits[dataIndex] = 1;
		triRecur_splitCoords[pos] = TRUE;
		return TRUE;
	}

	return FALSE;
}



HRESULT CMyApplication::InitTerrainTriGroups(){

	DWORD scale,depth,i;

	if( NULL == ( triRecur_indexAtRenderPos = new WORD[ MAX_RENDERED_COORDS ] ) ){
		return E_OUTOFMEMORY;
	}	

	if( NULL == ( triRecur_typeAtRenderPos = new BYTE[ MAX_RENDERED_COORDS ] ) ){
		return E_OUTOFMEMORY;
	}

	ZeroMemory( triRecur_bits, TRIGROUP_NUM_BITS * sizeof(BYTE) );

	for(depth=0, scale=8; depth < TRIGROUP_NUM_DEPTHS; depth++){ 

		if( ( depth % 2 ) == 1 ){
			scale /= 2;
		}

		for(i=0; i < 8; i++){
			triRecur_incCoordPos[depth][i] = ( triRecur_incZ[i] * terr.width + triRecur_incX[i] ) * scale;

			triRecur_incWorldX[depth][i] = triRecur_incX[i] * TERR_SCALE * scale;
			triRecur_incWorldZ[depth][i] = triRecur_incZ[i] * TERR_SCALE * scale;

			triRecur_incTextureU[depth][i] = (FLOAT)triRecur_incX[i] * TERR_TEXTURE_SCALE * scale;
			triRecur_incTextureV[depth][i] = (FLOAT)triRecur_incZ[i] * TERR_TEXTURE_SCALE * scale;

			triRecur_incShadowTextureU[depth][i] = (FLOAT)triRecur_incX[i] * TERR_SHADOWTEXTURE_SCALE * scale;
			triRecur_incShadowTextureV[depth][i] = (FLOAT)triRecur_incZ[i] * TERR_SHADOWTEXTURE_SCALE * scale;
		}
	}

	DWORD tgz, tgx, cz, cx;
	
	if( NULL == ( terr.triGroups = new TRIGROUP[ terr.triGroupsArea ] ) ){
		return E_OUTOFMEMORY;
	}

	ZeroMemory( terr.triGroups, terr.triGroupsArea * sizeof(TRIGROUP) );

	if( NULL == ( triRecur_splitCoords = new BYTE[ terr.area ] ) ){
		return E_OUTOFMEMORY;
	}
	
	ZeroMemory( triRecur_splitCoords, terr.area * sizeof(BYTE) );

// set all tris on type borders to be split
	TERRTYPENODE* pTopNode;
	TERRTYPENODE* pBottomNode;
	BYTE oldTopType, newTopType;
	BYTE oldBottomType, newBottomType;

	for(cz=0; cz < terr.depth - 1; cz++){

		pTopNode = &terr.typeRuns[cz + 1];
		pBottomNode = &terr.typeRuns[cz];

		oldTopType = newTopType = pTopNode->type;
		oldBottomType = newBottomType = pBottomNode->type;
		cx = 0;

		if( newTopType == newBottomType ){
			if( pTopNode->nextStartX < pBottomNode->nextStartX ){
				cx = pTopNode->nextStartX;
				Advance( pTopNode, terr.typeNodes );
				newTopType = pTopNode->type;
			}
			else if( pBottomNode->nextStartX < pTopNode->nextStartX ){
				cx = pBottomNode->nextStartX;
				Advance( pBottomNode, terr.typeNodes );
				newBottomType = pBottomNode->type;
			}
			else{ // runs are changing at same x value
				cx = pTopNode->nextStartX;
				Advance( pTopNode, terr.typeNodes );
				Advance( pBottomNode, terr.typeNodes );
				newTopType = pTopNode->type;
				newBottomType = pBottomNode->type;
			}
		}
		else{
			if( cx == pTopNode->nextStartX ){
				oldTopType = newTopType;
				Advance( pTopNode, terr.typeNodes );
				newTopType = pTopNode->type;
			}
			if( cx == pBottomNode->nextStartX ){
				oldBottomType = newBottomType;
				Advance( pBottomNode, terr.typeNodes );
				newBottomType = pBottomNode->type;
			}
		}

		while( cx < terr.width ){

			if( oldBottomType != newBottomType ){
				triRecur_splitCoords[cz * terr.width + cx - 1] = 1;
				triRecur_splitCoords[cz * terr.width + cx] = 1;
			}

			if( oldBottomType != oldTopType ){
				triRecur_splitCoords[cz * terr.width + cx - 1] = 1;
				triRecur_splitCoords[(cz + 1) * terr.width + cx - 1] = 1;
			}

			if( newBottomType != newTopType ){
				triRecur_splitCoords[cz * terr.width + cx] = 1;
				triRecur_splitCoords[(cz + 1) * terr.width + cx] = 1;
			}

			if( oldTopType != newTopType ){
				triRecur_splitCoords[(cz + 1) * terr.width + cx - 1] = 1;
				triRecur_splitCoords[(cz + 1) * terr.width + cx] = 1;
			}
			

			oldTopType = newTopType;
			oldBottomType = newBottomType;
			cx++;

			if( newTopType == newBottomType ){
				if( pTopNode->nextStartX < pBottomNode->nextStartX ){
					oldTopType = newTopType;
					cx = pTopNode->nextStartX;
					Advance( pTopNode, terr.typeNodes );
					newTopType = pTopNode->type;
				}
				else if( pBottomNode->nextStartX < pTopNode->nextStartX ){
					oldBottomType = newBottomType;
					cx = pBottomNode->nextStartX;
					Advance( pBottomNode, terr.typeNodes );
					newBottomType = pBottomNode->type;
				}
				else{ // runs are changing at same x value
					oldTopType = newTopType;
					oldBottomType = newBottomType;
					cx = pTopNode->nextStartX;
					Advance( pTopNode, terr.typeNodes );
					Advance( pBottomNode, terr.typeNodes );
					newTopType = pTopNode->type;
					newBottomType = pBottomNode->type;
				}
			}
			else{
				if( cx == pTopNode->nextStartX ){
					oldTopType = newTopType;
					Advance( pTopNode, terr.typeNodes );
					newTopType = pTopNode->type;
				}
				if( cx == pBottomNode->nextStartX ){
					oldBottomType = newBottomType;
					Advance( pBottomNode, terr.typeNodes );
					newBottomType = pBottomNode->type;
				}
			}
		}
	}

	
	triRecur_terrCoords = terr.coords;
	triRecur_triGroupMaxError = terr.triGroupMaxError;

	for(tgz=1, cz=TRIGROUP_WIDTH; tgz < terr.triGroupsDepth - 1;
		tgz++, cz += TRIGROUP_WIDTH){ // skip border groups

		for(tgx=1, cx=TRIGROUP_WIDTH; tgx < terr.triGroupsWidth - 1;
			tgx++, cx += TRIGROUP_WIDTH){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				DWORD mask = 0x00000001;
				for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
				}
			}

			SplitTri( 0, 0, 0, 5,
				terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
				terr.coords[ ( cz ) * terr.width + cx ].height,
				terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
				( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

			SplitTri( 0, 1, 1, 1,
				terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
				terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx + TRIGROUP_WIDTH ].height,
				terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
				( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				DWORD mask = 0x00000001;
				for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
					if( triRecur_bits[i * 32 + bit] ){
						terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] |= mask;
					}
				}
			}

		}
	}


	for(DWORD pass=0; pass < 4; pass++){// 3 more passes to check for forced splits

		for(tgz=0, cz=0; tgz < terr.triGroupsDepth; tgz++, cz += TRIGROUP_WIDTH){ // include border groups

			for(tgx=0, cx=0; tgx < terr.triGroupsWidth; tgx++, cx += TRIGROUP_WIDTH){
		
				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						triRecur_bits[i * 32 + bit] = ( terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
					}
				}

				SplitTriIfForced( 0, 0, 0, 5,
					terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					terr.coords[ ( cz ) * terr.width + cx ].height,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
					( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

				SplitTriIfForced( 0, 1, 1, 1,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						if( triRecur_bits[i * 32 + bit] ){
							terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] |= mask;
						}
					}
				}

			}
		}
	}

	delete triRecur_splitCoords;

	return S_OK;
}



/*
	for(DWORD pass=0; pass < 3; pass++){// 3 more passes to check for forced splits

		for(tgz=0, cz=TRIGROUP_WIDTH; tgz < terr.triGroupsDepth;
			tgz++, cz += TRIGROUP_WIDTH){ // include border groups

			for(tgx=0, cx=TRIGROUP_WIDTH; tgx < terr.triGroupsWidth;
				tgx++, cx += TRIGROUP_WIDTH){
		
				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						triRecur_bits[i * 32 + bit] = ( terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
					}
				}

				SplitTriIfForced( 0, 0, 0, 5,
					terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					terr.coords[ ( cz ) * terr.width + cx ].height,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
					( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

				SplitTriIfForced( 0, 1, 1, 1,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						if( triRecur_bits[i * 32 + bit] ){
							terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] |= mask;
						}
					}
				}

			}
		}
	}
*/


/*
HRESULT CMyApplication::UpdateTerrainTriGroups(){

	DWORD scale,depth,i;

	DWORD tgz, tgx, cz, cx;
	
	ZeroMemory( terr.triGroups, terr.triGroupsArea * sizeof(TRIGROUP) );

//	if( NULL == ( triRecur_splitCoords = new BYTE[ terr.area ] ) ){
//		return E_OUTOFMEMORY;
//	}
	
	ZeroMemory( triRecur_splitCoords, terr.area * sizeof(BYTE) );

// set all tris on type borders to be split
	TERRTYPENODE* pTopNode;
	TERRTYPENODE* pBottomNode;
	BYTE oldTopType, newTopType;
	BYTE oldBottomType, newBottomType;

	for(cz=0; cz < terr.depth - 1; cz++){

		pTopNode = &terr.typeRuns[cz + 1];
		pBottomNode = &terr.typeRuns[cz];

		oldTopType = newTopType = pTopNode->type;
		oldBottomType = newBottomType = pBottomNode->type;
		cx = 0;

		if( newTopType == newBottomType ){
			if( pTopNode->nextStartX < pBottomNode->nextStartX ){
				cx = pTopNode->nextStartX;
				Advance( pTopNode, terr.typeNodes );
				newTopType = pTopNode->type;
			}
			else if( pBottomNode->nextStartX < pTopNode->nextStartX ){
				cx = pBottomNode->nextStartX;
				Advance( pBottomNode, terr.typeNodes );
				newBottomType = pBottomNode->type;
			}
			else{ // runs are changing at same x value
				cx = pTopNode->nextStartX;
				Advance( pTopNode, terr.typeNodes );
				Advance( pBottomNode, terr.typeNodes );
				newTopType = pTopNode->type;
				newBottomType = pBottomNode->type;
			}
		}
		else{
			if( cx == pTopNode->nextStartX ){
				oldTopType = newTopType;
				Advance( pTopNode, terr.typeNodes );
				newTopType = pTopNode->type;
			}
			if( cx == pBottomNode->nextStartX ){
				oldBottomType = newBottomType;
				Advance( pBottomNode, terr.typeNodes );
				newBottomType = pBottomNode->type;
			}
		}

		while( cx < terr.width ){

			if( oldBottomType != newBottomType ){
				triRecur_splitCoords[cz * terr.width + cx - 1] = 1;
				triRecur_splitCoords[cz * terr.width + cx] = 1;
			}

			if( oldBottomType != oldTopType ){
				triRecur_splitCoords[cz * terr.width + cx - 1] = 1;
				triRecur_splitCoords[(cz + 1) * terr.width + cx - 1] = 1;
			}

			if( newBottomType != newTopType ){
				triRecur_splitCoords[cz * terr.width + cx] = 1;
				triRecur_splitCoords[(cz + 1) * terr.width + cx] = 1;
			}

			if( oldTopType != newTopType ){
				triRecur_splitCoords[(cz + 1) * terr.width + cx - 1] = 1;
				triRecur_splitCoords[(cz + 1) * terr.width + cx] = 1;
			}
			

			oldTopType = newTopType;
			oldBottomType = newBottomType;
			cx++;

			if( newTopType == newBottomType ){
				if( pTopNode->nextStartX < pBottomNode->nextStartX ){
					oldTopType = newTopType;
					cx = pTopNode->nextStartX;
					Advance( pTopNode, terr.typeNodes );
					newTopType = pTopNode->type;
				}
				else if( pBottomNode->nextStartX < pTopNode->nextStartX ){
					oldBottomType = newBottomType;
					cx = pBottomNode->nextStartX;
					Advance( pBottomNode, terr.typeNodes );
					newBottomType = pBottomNode->type;
				}
				else{ // runs are changing at same x value
					oldTopType = newTopType;
					oldBottomType = newBottomType;
					cx = pTopNode->nextStartX;
					Advance( pTopNode, terr.typeNodes );
					Advance( pBottomNode, terr.typeNodes );
					newTopType = pTopNode->type;
					newBottomType = pBottomNode->type;
				}
			}
			else{
				if( cx == pTopNode->nextStartX ){
					oldTopType = newTopType;
					Advance( pTopNode, terr.typeNodes );
					newTopType = pTopNode->type;
				}
				if( cx == pBottomNode->nextStartX ){
					oldBottomType = newBottomType;
					Advance( pBottomNode, terr.typeNodes );
					newBottomType = pBottomNode->type;
				}
			}
		}
	}

	
	triRecur_terrCoords = terr.coords;
	triRecur_triGroupMaxError = terr.triGroupMaxError;

	for(tgz=1, cz=TRIGROUP_WIDTH; tgz < terr.triGroupsDepth - 1;
		tgz++, cz += TRIGROUP_WIDTH){ // skip border groups

		for(tgx=1, cx=TRIGROUP_WIDTH; tgx < terr.triGroupsWidth - 1;
			tgx++, cx += TRIGROUP_WIDTH){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				DWORD mask = 0x00000001;
				for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
				}
			}

			SplitTri( 0, 0, 0, 5,
				terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
				terr.coords[ ( cz ) * terr.width + cx ].height,
				terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
				( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

			SplitTri( 0, 1, 1, 1,
				terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
				terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx + TRIGROUP_WIDTH ].height,
				terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
				( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				DWORD mask = 0x00000001;
				for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
					if( triRecur_bits[i * 32 + bit] ){
						terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] |= mask;
					}
				}
			}

		}
	}


	for(DWORD pass=0; pass < 4; pass++){// 3 more passes to check for forced splits

		for(tgz=0, cz=0; tgz < terr.triGroupsDepth; tgz++, cz += TRIGROUP_WIDTH){ // include border groups

			for(tgx=0, cx=0; tgx < terr.triGroupsWidth; tgx++, cx += TRIGROUP_WIDTH){
		
				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						triRecur_bits[i * 32 + bit] = ( terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] & mask )? TRUE : FALSE;
					}
				}

				SplitTriIfForced( 0, 0, 0, 5,
					terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					terr.coords[ ( cz ) * terr.width + cx ].height,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
					( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

				SplitTriIfForced( 0, 1, 1, 1,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx ].height,
					terr.coords[ ( cz + TRIGROUP_WIDTH ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					terr.coords[ ( cz ) * terr.width + cx + TRIGROUP_WIDTH ].height,
					( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX );

				for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
					DWORD mask = 0x00000001;
					for(DWORD bit=0; bit < 32; bit++, mask <<= 1){
						if( triRecur_bits[i * 32 + bit] ){
							terr.triGroups[ tgz * terr.triGroupsWidth + tgx ].data[i] |= mask;
						}
					}
				}

			}
		}
	}

	return S_OK;
}
*/



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

	terr.typeRuns = new TERRTYPENODE[ terr.depth ];

	for(WORD z=0; z < terr.depth; z++){
		pCurrNode = &terr.typeRuns[ z ];
		pCurrNode->type = pBitmapData[ z * bm.bmWidthBytes ];
		pCurrNode->next = 0;
		for(WORD x=0; x < terr.width; x++){
			if( pBitmapData[ z * bm.bmWidthBytes + x ] != pCurrNode->type ){
				pCurrNode->nextStartX = x;
				pCurrNode->next = nextAvailNode;
				nextAvailNode++;
				if( nextAvailNode > MAX_TERRAIN_TYPE_NODES ){
					return E_FAIL;
				}
				pCurrNode = &terr.typeNodes[ pCurrNode->next ];
				pCurrNode->type = pBitmapData[ z * bm.bmWidthBytes + x ];
				pCurrNode->next = 0;
			}
		}
		pCurrNode->nextStartX = terr.width;
	}


	for(DWORD i=0; i < terr.numTypes; i++){

		terr.typeIndices[i] = new WORD[ MAX_RENDERED_COORDS * 6 ];

		terr.typeBlendIndices[ i ] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 8 ) * 6];

		terr.typeBlendOnIndices[ i ] = new WORD[ (DWORD)( MAX_RENDERED_COORDS / 16 ) * 6 ];
	}


	return S_OK;
}



VOID CMyApplication::UpdateTerrainLighting( CRectRegion region ){

	DWORD z,x;
	FLOAT xRise, zRise;
	const FLOAT run = 2 * TERR_SCALE;

	D3DVECTOR vSunlight = D3DVECTOR( // this is actually the opposite of it's true direction
		-cosf( sunlightPitch ) * cosf( sunlightDirection ),
		sinf( sunlightPitch ),
		-cosf( sunlightPitch ) * sinf( sunlightDirection ) );
	D3DVECTOR vGroundNormal;

	BOOL doMinZ = FALSE;
	BOOL doMaxZ = FALSE;
	BOOL doMinX = FALSE;
	BOOL doMaxX = FALSE;

	if( region.startZ == 0 ){
		doMinZ = TRUE;
		region.startZ = 1;
	}
	if( region.endZ == terr.depth - 1 ){
		doMaxZ = TRUE;
		region.endZ = terr.depth - 2;
	}
	if( region.startX == 0 ){
		doMinX = TRUE;
		region.startX = 1;
	}
	if( region.endX == terr.width - 1 ){
		doMaxX = TRUE;
		region.endX = terr.width - 2;
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
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.startX = 1;
		}

		if( doMaxX ){
			x = terr.width - 1;
			xRise = 0;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.endX = terr.width - 2;
		}

		for(x=region.startX; x <= region.endX; x++){

			xRise = ( terr.coords[ z * terr.width + ( x - 1 ) ].height - 
				terr.coords[ z * terr.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );
		}

		region.startZ = 1;
	}

	if( doMaxZ ){
		z = terr.depth - 1;
		zRise = 0;

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.startX = 1;
		}

		if( doMaxX ){
			x = terr.width - 1;
			xRise = 0;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.endX = terr.width - 2;
		}

		for(x=region.startX; x <= region.endX; x++){

			xRise = ( terr.coords[ z * terr.width + ( x - 1 ) ].height - 
				terr.coords[ z * terr.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );
		}

		region.endZ = terr.depth - 2;
	}

	for(z=region.startZ; z <= region.endZ; z++){

		if( doMinX ){
			x = 0;
			xRise = 0;
			
			zRise = ( terr.coords[ ( z - 1 ) * terr.width + x ].height - 
				terr.coords[ ( z + 1 ) * terr.width + x ].height ) * TERR_HEIGHT_SCALE;

			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.startX = 1;
		}

		if( doMaxX ){
			x = terr.width - 1;
			xRise = 0;
			
			zRise = ( terr.coords[ ( z - 1 ) * terr.width + x ].height - 
				terr.coords[ ( z + 1 ) * terr.width + x ].height ) * TERR_HEIGHT_SCALE;

			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );

			region.endX = terr.width - 2;
		}

		for(x=region.startX; x <= region.endX; x++){

			zRise = ( terr.coords[ ( z - 1 ) * terr.width + x ].height - 
				terr.coords[ ( z + 1 ) * terr.width + x ].height ) * TERR_HEIGHT_SCALE;
			
			xRise = ( terr.coords[ z * terr.width + ( x - 1 ) ].height - 
				terr.coords[ z * terr.width + ( x + 1 ) ].height ) * TERR_HEIGHT_SCALE;
			
			// vGroundNormal = Normalize( D3DVECTOR( xRise, run, zRise ) );

			FLOAT dotProduct = DotProduct( vSunlight, Normalize( D3DVECTOR( xRise, run, zRise ) ) );
			if( dotProduct < 0.0f ){
				dotProduct = 0.0f;
			}
			terr.coords[ z * terr.width + x ].lightIntensity = 
				( (FLOAT)TERR_LIGHT_AMBIENT + dotProduct * (FLOAT)TERR_LIGHT_VARIANCE );
		}
	}	 
}


/*
VOID CMyApplication::UpdateTriIndexOffsets(){

	DWORD i, x;
	INT32 triIndexXOffsets[32], triIndexZOffsets[32];

	INT32 renderWidth = terr.border.endX - terr.border.startX + 1;
// types 0 & 1 are off by one w/ respect to size
	for( i=0; i < 2; i++ ){ // for sizes 0 & 1
		triIndexXOffsets[ i * 4 ] = 0;
		triIndexZOffsets[ i * 4 ] = 0;
		triIndexXOffsets[ 16 + i * 4 ] = 0;
		triIndexZOffsets[ 16 + i * 4 ] = 0;
		for( x=1; x < 4; x++ ){
			triIndexXOffsets[ i * 4 + x ] = triIndexBaseXOffsets[ i ] << ( x - 1 );
			triIndexZOffsets[ i * 4 + x ] = triIndexBaseZOffsets[ i ] << ( x - 1 );
			triIndexXOffsets[ 16 + i * 4 + x ] = triIndexBaseXOffsets[ 4 + i ] << ( x - 1 );
			triIndexZOffsets[ 16 + i * 4 + x ] = triIndexBaseZOffsets[ 4 + i ] << ( x - 1 );
		}
	}

	for( i=2; i < 4; i++ ){ // for sizes 2 & 3
		for( x=0; x < 4; x++ ){
			triIndexXOffsets[ i * 4 + x ] = triIndexBaseXOffsets[ i ] << x;
			triIndexZOffsets[ i * 4 + x ] = triIndexBaseZOffsets[ i ] << x;
			triIndexXOffsets[ 16 + i * 4 + x ] = triIndexBaseXOffsets[ 4 + i ] << x;
			triIndexZOffsets[ 16 + i * 4 + x ] = triIndexBaseZOffsets[ 4 + i ] << x;
		}
	}

	for( i=0; i < 32; i++ ){
		triIndexOffsets[i] = triIndexZOffsets[i] * renderWidth + triIndexXOffsets[i];
	}
}
*/


HRESULT CMyApplication::LoadTerrainHeights(){

	int i;
	WORD* heights;
	HANDLE newFile;
	DWORD NumberOfBytesRead;

	if( NULL == ( heights = new WORD[ terr.area ] ) ){
		MyAlert( NULL, "LoadTerrainHeights() > heights memory allocation failed" );
		return E_OUTOFMEMORY;
	}

	CHAR filePath[256];
	strcpy( filePath, currTrackFilePath );
	strcat( filePath, "heights.dat" );

	if( ( newFile = CreateFile( filePath,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL ) ) == INVALID_HANDLE_VALUE ){
		delete heights;
		return E_FAIL;
	}

	if( !ReadFile( newFile,
		heights,
		terr.area * sizeof(WORD),
		&NumberOfBytesRead,
		NULL ) ){
		delete heights;
		return E_FAIL;
	}

	CloseHandle( newFile );

	for(i=0; i < terr.area; i++){
		terr.coords[i].height = heights[i];
	}

	delete heights;

	return S_OK;
}	



HRESULT CMyApplication::InitTerrainTextures(){
	
  for( DWORD i = 0; i < terr.numTypes; i++ ){
		if( FAILED( AddTexture( terr.textureFileNames[ i ] ) ) ){
			return E_FAIL;
		}
	}

	return S_OK;
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

			triRecur_pVertex->color = ( pTerrCoord->lightIntensity << 16 ) | ( pTerrCoord->lightIntensity << 8 ) | ( pTerrCoord->lightIntensity );

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
				triRecur_pterrTypeBlendIndex[leftType][0] = topIndex;
				triRecur_pterrTypeBlendIndex[leftType][1] = leftIndex; 
				triRecur_pterrTypeBlendIndex[leftType][2] = rightIndex; 
				triRecur_pterrTypeBlendIndex[leftType] += 3;

				if( leftType == rightType ){ // turn on alpha of both left & right if same type
					triRecur_pterrTypeBlendOnIndex[leftType][0] = leftIndex; 
					triRecur_pterrTypeBlendOnIndex[leftType][1] = rightIndex; 
					triRecur_pterrTypeBlendOnIndex[leftType] += 2;
				}
				else{
					triRecur_pterrTypeBlendOnIndex[leftType][0] = leftIndex; 
					triRecur_pterrTypeBlendOnIndex[leftType]++;
				}
			}
			if( rightType != topType && rightType != leftType ){
				// add tri w/ type of right vertex
				triRecur_pterrTypeBlendIndex[rightType][0] = topIndex;
				triRecur_pterrTypeBlendIndex[rightType][1] = leftIndex; 
				triRecur_pterrTypeBlendIndex[rightType][2] = rightIndex; 
				triRecur_pterrTypeBlendIndex[rightType] += 3;

				triRecur_pterrTypeBlendOnIndex[rightType][0] = rightIndex; 
				triRecur_pterrTypeBlendOnIndex[rightType]++;
			}
		}

		ppIndex = &triRecur_pTypeIndex[triRecur_typeAtRenderPos[renderPos]];

		//ppIndex = &triRecur_pTypeIndex[depth % 6];

		(*ppIndex)[0] = topIndex;
		(*ppIndex)[1] = leftIndex;
		(*ppIndex)[2] = rightIndex;

		(*ppIndex) += 3;

		// texture blends stuff here
	}
}



HRESULT CMyApplication::RenderTerrain(){

	DWORD tgcz, tgcx, tgrz, tgrx, triGroupNum; // coord-based and render-based trigroup positions
	DWORD cz, cx, rz, rx; 
	TRIGROUP* pTriGroup = terr.triGroups;
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

	terr.renderedTriGroups.Set( terr.visibleCoords.startX >> TRIGROUP_WIDTHSHIFT,
		terr.visibleCoords.startZ >> TRIGROUP_WIDTHSHIFT,
		( terr.visibleCoords.endX - 1 ) >> TRIGROUP_WIDTHSHIFT,
		( terr.visibleCoords.endZ - 1 ) >> TRIGROUP_WIDTHSHIFT );

	terr.renderedCoords.Set( terr.renderedTriGroups.startX << TRIGROUP_WIDTHSHIFT,
		terr.renderedTriGroups.startZ << TRIGROUP_WIDTHSHIFT,
		( ( terr.renderedTriGroups.endX + 1 ) << TRIGROUP_WIDTHSHIFT ),
		( ( terr.renderedTriGroups.endZ + 1 ) << TRIGROUP_WIDTHSHIFT ));

	groupRenderWidth = terr.renderedTriGroups.endX - terr.renderedTriGroups.startX + 1;
	coordRenderWidth = terr.renderedCoords.endX - terr.renderedCoords.startX + 1;

	for(depth=0, scale=8; depth < TRIGROUP_NUM_DEPTHS; depth++){ 

		if( ( depth % 2 ) == 1 ){
			scale /= 2;
		}

		for(i=0; i < 8; i++){
			triRecur_incRenderPos[depth][i] = ( triRecur_incZ[i] * coordRenderWidth + triRecur_incX[i] ) * scale;
		}
	}

//// local type map
	for(cz=terr.renderedCoords.startZ, rz=0; cz <= terr.renderedCoords.endZ; cz++, rz++){

		pNode = &terr.typeRuns[cz];

		while( pNode->nextStartX <= terr.renderedCoords.startX ){ Advance( pNode, terr.typeNodes ); }

		currStartX = terr.renderedCoords.startX;

		while( pNode->nextStartX <= terr.renderedCoords.endX ){
			for(i=currStartX; i < pNode->nextStartX; i++){
				triRecur_typeAtRenderPos[rz * coordRenderWidth - terr.renderedCoords.startX + i] =
					pNode->type;
			}
			currStartX = pNode->nextStartX;
			Advance( pNode, terr.typeNodes );
		}

		FillMemory( &triRecur_typeAtRenderPos[ rz * coordRenderWidth + currStartX - terr.renderedCoords.startX ],
			terr.renderedCoords.endX - currStartX + 1,
			pNode->type );
	}
//////////

	if( FAILED( terr.verticesVB->Lock( DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS | DDLOCK_WAIT,	
		(VOID**)&terrVertices, NULL ) ) ){
		return E_FAIL;
	}

	for(i=0; i < terr.numTypes; i++){
		triRecur_pTypeIndex[i] = terr.typeIndices[i];
		triRecur_pterrTypeBlendIndex[i] = terr.typeBlendIndices[i];
		triRecur_pterrTypeBlendOnIndex[i] = terr.typeBlendOnIndices[i];
	}

	triRecur_numVertices = 0;
	triRecur_pVertex = terrVertices;

	ZeroMemory( triRecur_indexAtRenderPos, MAX_RENDERED_COORDS * sizeof(WORD) );

	
	rz = 0;
	wz = terr.renderedCoords.startZ * TERR_SCALE;
	pTerrCoord = &terr.coords[ terr.renderedCoords.startZ * terr.width + terr.renderedCoords.startX ];
	tv = (FLOAT)terr.renderedCoords.startZ * TERR_TEXTURE_SCALE;
	for(cz=terr.renderedCoords.startZ; cz <= terr.renderedCoords.endZ;
		cz += TRIGROUP_WIDTH,	rz += TRIGROUP_WIDTH, wz += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
		tv += ( TERR_TEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

		rx = 0;
		wx = terr.renderedCoords.startX * TERR_SCALE;
		tu = (FLOAT)terr.renderedCoords.startX * TERR_TEXTURE_SCALE;
		for(cx=terr.renderedCoords.startX; cx <= terr.renderedCoords.endX;
			cx += TRIGROUP_WIDTH, rx += TRIGROUP_WIDTH,	wx += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
			tu += ( TERR_TEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

			pTerrCoord = &terr.coords[ cz * terr.width + cx ];

			triRecur_pVertex->x = wx;
			triRecur_pVertex->y = pTerrCoord->height * TERR_HEIGHT_SCALE;
			triRecur_pVertex->z = wz;

			triRecur_pVertex->color = ( pTerrCoord->lightIntensity << 16 ) | ( pTerrCoord->lightIntensity << 8 ) | ( pTerrCoord->lightIntensity );

			triRecur_pVertex->tu = tu;
			triRecur_pVertex->tv = tv;

			triRecur_indexAtRenderPos[ rz * coordRenderWidth + rx ] = triRecur_numVertices;
			triRecur_numVertices++;
			triRecur_pVertex++;
		}
	}

	// fill groupCornerIndices // width & height are 1 excess!!
	triGroupNum = terr.renderedTriGroups.startZ * terr.triGroupsWidth + terr.renderedTriGroups.startX;
	cz = terr.renderedCoords.startZ;
	rz = 0;
	for(tgcz=terr.renderedTriGroups.startZ, tgrz=0; tgcz <= terr.renderedTriGroups.endZ;
		tgcz++, tgrz++,	cz += TRIGROUP_WIDTH, rz+= TRIGROUP_WIDTH, triGroupNum += terr.triGroupsWidth - groupRenderWidth){

		cx = terr.renderedCoords.startX;
		rx = 0;
		for(tgcx=terr.renderedTriGroups.startX, tgrx=0; tgcx <= terr.renderedTriGroups.endX;
			tgcx++, tgrx++, cx += TRIGROUP_WIDTH, rx+= TRIGROUP_WIDTH, triGroupNum++){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				mask = 0x00000001;
				for(bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( terr.triGroups[ triGroupNum ].data[i] & mask )? TRUE : FALSE;
				}
			}
			
			ProcessTri( 0, 0, 0, 5,
				( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
				( tgrz ) * ( groupRenderWidth + 1 ) + tgrx, 
				( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
				( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
				&terr.coords[ ( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX ],
				( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
				( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
				(FLOAT)( cx + TRIGROUP_CENTERZ ) * TERR_TEXTURE_SCALE,
				(FLOAT)( cz + TRIGROUP_CENTERX ) * TERR_TEXTURE_SCALE );

			ProcessTri( 0, 1, 1, 1,
				( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
				( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
				( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
				( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
				&terr.coords[ ( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX ],
				( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
				( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
				(FLOAT)( cx + TRIGROUP_CENTERZ ) * TERR_TEXTURE_SCALE,
				(FLOAT)( cz + TRIGROUP_CENTERX ) * TERR_TEXTURE_SCALE );
		}
	}

	
	if( FAILED( terr.verticesVB->Unlock() ) ){
		return E_FAIL;
	}

	if( FAILED( hr = terr.transformedVerticesVB->ProcessVertices( D3DVOP_TRANSFORM | D3DVOP_CLIP,
		0,
		triRecur_numVertices,
		terr.verticesVB,
		0,
		m_pd3dDevice,
		0 ) ) ){
		MyAlert(&hr, "RenderTerrain() > ProcessVertices() failed" );
		return E_FAIL;
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME );

	for(i=0; i < terr.numTypes; i++){
		if( FAILED( hr = m_pd3dDevice->SetTexture( 0, (GetTexture( terr.textureFileNames[i] ))->m_pddsSurface ) ) ){
		//if( FAILED( hr = m_pd3dDevice->SetTexture( 0, shadowTexture.pddsSurface ) ) )
			if( hr != DDERR_SURFACELOST ){
				MyAlert(&hr, "RenderTerrain() > SetTexture() failed" );
				return E_FAIL;
			}
			if( FAILED( hr = (GetTexture( terr.textureFileNames[i] ))->Restore( m_pd3dDevice ) ) ){
				MyAlert(&hr, "RenderTerrain() > SetTexture() failed (surface lost), CTextureHolder::Restore() failed" );
				return E_FAIL;
			}
		}

		if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
			terr.transformedVerticesVB,
			0,
			triRecur_numVertices,
			terr.typeIndices[ i ],
			(triRecur_pTypeIndex[ i ] - terr.typeIndices[ i ]), 0 ) ) ){
			MyAlert(&hr, "RenderTerrain() > DrawIndexedPrimitiveVB() failed" );
			return E_FAIL;
		}
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID );


	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );

	for(i=0; i < terr.numTypes; i++){
		if( FAILED( hr = m_pd3dDevice->SetTexture( 0, (GetTexture( terr.textureFileNames[i] ))->m_pddsSurface ) ) ){
			if( hr != DDERR_SURFACELOST ){
				MyAlert(&hr, "RenderTerrain() > SetTexture() failed" );
				return E_FAIL;
			}
			if( FAILED( hr = (GetTexture( terr.textureFileNames[i] ))->Restore( m_pd3dDevice ) ) ){
				MyAlert(&hr, "RenderTerrain() > SetTexture() failed (surface lost), CTextureHolder::Restore() failed" );
				return E_FAIL;
			}
		}

		if( FAILED( terr.transformedVerticesVB->Lock( DDLOCK_WRITEONLY | DDLOCK_WAIT,	
			(VOID**)&terrTransformedVertices, NULL ) ) ){
			return E_FAIL;
		}

		for(pIndex = terr.typeBlendIndices[i]; pIndex < triRecur_pterrTypeBlendIndex[i]; pIndex++){
			terrTransformedVertices[ *pIndex ].color &= 0x00FFFFFF;
		}
		for(pIndex = terr.typeBlendOnIndices[i]; pIndex < triRecur_pterrTypeBlendOnIndex[i]; pIndex++){
			terrTransformedVertices[ *pIndex ].color |= 0xFF000000;
		}

		if( FAILED( terr.transformedVerticesVB->Unlock() ) ){
			return E_FAIL;
		}

		HRESULT hr;
		if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
			terr.transformedVerticesVB,
			0,
			triRecur_numVertices,
			terr.typeBlendIndices[ i ],
			(triRecur_pterrTypeBlendIndex[ i ] - terr.typeBlendIndices[ i ]), 0 ) ) ){
			MyAlert(&hr, "RenderTerrain() > Alpha blends > DrawIndexedPrimitiveVB() failed" );
			return E_FAIL;
		}
	}

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );


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
	TRIGROUP* pTriGroup = terr.triGroups;
	DWORD numVertices;
	DWORD depth, scale, bit, i, j;
	DWORD coordRenderWidth, groupRenderWidth, groupShadedWidth;
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

	groupRenderWidth = terr.renderedTriGroups.endX - terr.renderedTriGroups.startX + 1;
	coordRenderWidth = terr.renderedCoords.endX - terr.renderedCoords.startX + 1;

	shadedTriGroups.Set(
		(INT32)( centerX - radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerZ - radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerX + radius ) >> TRIGROUP_WIDTHSHIFT,
		(INT32)( centerZ + radius ) >> TRIGROUP_WIDTHSHIFT );

	shadedTriGroups.Clip( terr.renderedTriGroups.startX,
		terr.renderedTriGroups.startZ,
		terr.renderedTriGroups.endX,
		terr.renderedTriGroups.endZ );

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


	if( FAILED( terr.transformedVerticesVB->Lock( DDLOCK_WRITEONLY | DDLOCK_WAIT,	
		(VOID**)&triRecur_transformedVertices, NULL ) ) ){
		return E_FAIL;
	}

	//triRecur_numVertices = 0;
	triRecur_pTypeIndex[0] = terr.typeIndices[0]; // borrowing terrain type 0's index array

	
	rz = shadedCoords.startZ - terr.renderedCoords.startZ;
	wz = shadedCoords.startZ * TERR_SCALE;
	pTerrCoord = &terr.coords[ shadedCoords.startZ * terr.width + shadedCoords.startX ];
	tv = ( (FLOAT)shadedCoords.startZ - centerZ / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f;
	for(cz=shadedCoords.startZ; cz <= shadedCoords.endZ;
		cz += TRIGROUP_WIDTH,	rz += TRIGROUP_WIDTH, wz += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
		tv += ( TERR_SHADOWTEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

		rx = shadedCoords.startX - terr.renderedCoords.startX;
		wx = shadedCoords.startX * TERR_SCALE;
		tu = ( (FLOAT)shadedCoords.startX - centerX / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f;
		for(cx=shadedCoords.startX; cx <= shadedCoords.endX;
			cx += TRIGROUP_WIDTH, rx += TRIGROUP_WIDTH,	wx += ( TERR_SCALE << TRIGROUP_WIDTHSHIFT ),
			tu += ( TERR_SHADOWTEXTURE_SCALE * (FLOAT)TRIGROUP_WIDTH )){

			//pTerrCoord = &terr.coords[cz * terr.width + cx];

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

	triGroupNum = shadedTriGroups.startZ * terr.triGroupsWidth + shadedTriGroups.startX;
	cz = shadedCoords.startZ;
	rz = shadedCoords.startZ - terr.renderedCoords.startZ;
	tgrz = shadedTriGroups.startZ - terr.renderedTriGroups.startZ;
	for(tgcz=shadedTriGroups.startZ; tgcz <= shadedTriGroups.endZ;
		tgcz++, tgrz++,	cz += TRIGROUP_WIDTH, rz+= TRIGROUP_WIDTH, triGroupNum += terr.triGroupsWidth - groupShadedWidth){

		cx = shadedCoords.startX;
		rx = shadedCoords.startX - terr.renderedCoords.startX;
		tgrx = shadedTriGroups.startX - terr.renderedTriGroups.startX;
		for(tgcx=shadedTriGroups.startX; tgcx <= shadedTriGroups.endX;
			tgcx++, tgrx++, cx += TRIGROUP_WIDTH, rx+= TRIGROUP_WIDTH, triGroupNum++){

			for(i=0; i < TRIGROUP_NUM_DWORDS; i++){
				mask = 0x00000001;
				for(bit=0; bit < 32; bit++, mask <<= 1){
					triRecur_bits[i * 32 + bit] = ( terr.triGroups[ triGroupNum ].data[i] & mask )? TRUE : FALSE;
				}
			}
			
			if( ( (INT32)( centerZ - radius + 0.5f ) - cz ) + ( (INT32)( centerX - radius + 0.5f ) - cx ) <
				TRIGROUP_CENTERX + TRIGROUP_CENTERZ ){
				ShadowProcessTri( 0, 0, 0, 5,
					( tgrz ) * ( groupRenderWidth + 1 ) + tgrx + 1, 
					( tgrz ) * ( groupRenderWidth + 1 ) + tgrx, 
					( tgrz + 1 ) * ( groupRenderWidth + 1 ) + tgrx, 
					( rz + TRIGROUP_CENTERZ ) * coordRenderWidth + rx + TRIGROUP_CENTERX,
					&terr.coords[ ( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX ],
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
					&terr.coords[ ( cz + TRIGROUP_CENTERZ ) * terr.width + cx + TRIGROUP_CENTERX ],
					( cx + TRIGROUP_CENTERZ ) * TERR_SCALE,
					( cz + TRIGROUP_CENTERX ) * TERR_SCALE,
					( (FLOAT)( cx + TRIGROUP_CENTERX ) - centerX / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f,
					( (FLOAT)( cz + TRIGROUP_CENTERZ ) - centerZ / TERR_SCALE ) * TERR_SHADOWTEXTURE_SCALE + 0.5f);
			}
		}
	}


	if( FAILED( terr.transformedVerticesVB->Unlock() ) ){
		return E_FAIL;
	}

	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO );
	//m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR );

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP );

	//m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );

	if( FAILED( hr = m_pd3dDevice->SetTexture( 0, shadowTexture.pddsSurface ) ) ){
		MyAlert(&hr, "ApplyShadowTexture() > SetTexture() failed" );
		return E_FAIL;
	}

	if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitiveVB( D3DPT_TRIANGLELIST,
		terr.transformedVerticesVB,
		0,
		triRecur_numVertices,
		terr.typeIndices[0],
		(triRecur_pTypeIndex[0] - terr.typeIndices[0]), 0 ) ) ){
		MyAlert(&hr, "ApplyShadowTexture() > DrawIndexedPrimitiveVB() failed" );
		return E_FAIL;
	}

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );




	return S_OK;

}

















