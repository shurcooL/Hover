
#include "hover.h"


const INT32 triIndexBaseXOffsets[8] =
{ -1, -2, -1, -1,
  -1, -1,  0, -1 };
	 
const INT32 triIndexBaseZOffsets[8] =
{ -1, 0, 0, 0,
   1, 1, 1, 1 };

INT32 triIndexWXOffsets[32];
INT32 triIndexWZOffsets[32];
INT32 triIndexOffsets[32];

const BYTE triVertMaxDist[16] =
{ 0, 1, 2, 4,
  0, 1, 2, 4,
	1, 2, 4, 8,
	1, 2, 4, 8 };

const BYTE triAltVertMaxDist[16] =
{ 0, 1, 2, 4,
  0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0 };

const BYTE triHorzMaxDist[16] =
{ 0, 1, 2, 4,
  0, 2, 4, 8,
	1, 2, 4, 8,
	1, 2, 4, 8 };


TCHAR terrTextureFileNames[][80] =
{ "seafloor.bmp", "stripes.bmp", "grass.bmp" };
 
WORD numTerrTextures = 3;


HRESULT CMyD3DApplication::InitTerrain(){

	if( ( terrIndices = new WORD[ MAX_RENDER_AREA * 6 ] ) == NULL ){
    return E_OUTOFMEMORY;
	}
	
	if( ( terrVertices = new TERRTLVERTEX[ MAX_RENDER_AREA ] ) == NULL ){
    return E_OUTOFMEMORY;
	}

	if( ( terrTextures = new TextureHolder[ MAX_TERRAIN_TEXTURES ] ) == NULL ){
    return E_OUTOFMEMORY;
	}

	return S_OK;
}



HRESULT CMyD3DApplication::RaceInitTerrain(){

	int i,x,z;
	HRESULT hr;

	terrWidth = 6;
	terrDepth = 6;
	terrArea = terrWidth * terrDepth;

	if( ( terrCoords = new TERRCOORD[ terrArea ] ) == NULL ){
    return E_OUTOFMEMORY;
	}

	// initialize tri index offsets; types 0 & 1 are off by one w/ respect to size

	for( i=0; i < 2; i++ ){ // for sizes 0 & 1
		triIndexWXOffsets[ i * 4 ] = 0;
		triIndexWZOffsets[ i * 4 ] = 0;
		triIndexWXOffsets[ 16 + i * 4 ] = 0;
		triIndexWZOffsets[ 16 + i * 4 ] = 0;
		for( x=1; x < 4; x++ ){
			triIndexWXOffsets[ i * 4 + x ] = triIndexBaseXOffsets[ i ] << ( x - 1 );
			triIndexWZOffsets[ i * 4 + x ] = triIndexBaseZOffsets[ i ] << ( x - 1 );
			triIndexWXOffsets[ 16 + i * 4 + x ] = triIndexBaseXOffsets[ 4 + i ] << ( x - 1 );
			triIndexWZOffsets[ 16 + i * 4 + x ] = triIndexBaseZOffsets[ 4 + i ] << ( x - 1 );
		}
	}

	for( i=2; i < 4; i++ ){ // for sizes 2 & 3
		for( x=0; x < 4; x++ ){
			triIndexWXOffsets[ i * 4 + x ] = triIndexBaseXOffsets[ i ] << x;
			triIndexWZOffsets[ i * 4 + x ] = triIndexBaseZOffsets[ i ] << x;
			triIndexWXOffsets[ 16 + i * 4 + x ] = triIndexBaseXOffsets[ 4 + i ] << x;
			triIndexWZOffsets[ 16 + i * 4 + x ] = triIndexBaseZOffsets[ 4 + i ] << x;
		}
	}

	for( i=0; i < 32; i++ ){
		triIndexOffsets[i] = triIndexWZOffsets[i] * terrWidth + triIndexWXOffsets[i];
		triIndexWXOffsets[i] *= TERR_SCALE;
		triIndexWZOffsets[i] *= TERR_SCALE;
	}
	
	// initialize random terrain and lighting

	srand(timeGetTime());

	for( z=0; z<terrDepth; z++ ){
		for( x=0; x<terrWidth; x++ ){
			terrCoords[ z * terrWidth + x ].height = (rand()/(FLOAT)RAND_MAX)*5;//(sin(x) + cos(z))*1.5 + 3;
			terrCoords[ z * terrWidth + x ].lightIndex	= (float)(0.25 + 0.75*((float)terrCoords[ z * terrWidth + x ].height / 5.0f)) * 255;
			terrCoords[ z * terrWidth + x ].twoTriCodes = 0;
		}
	}

	terrCoords[ 0 * terrWidth + 1 ].twoTriCodes = 0x80;
	terrCoords[ 0 * terrWidth + 2 ].twoTriCodes = 0xC0;
	terrCoords[ 0 * terrWidth + 3 ].twoTriCodes = 0x80;
	terrCoords[ 0 * terrWidth + 4 ].twoTriCodes = 0xC0;
	terrCoords[ 0 * terrWidth + 5 ].twoTriCodes = 0x80;

	terrCoords[ 1 * terrWidth + 1 ].twoTriCodes = 0x1C;
	terrCoords[ 1 * terrWidth + 2 ].twoTriCodes = 0x88;
	terrCoords[ 1 * terrWidth + 3 ].twoTriCodes = 0xC5;
	terrCoords[ 1 * terrWidth + 5 ].twoTriCodes = 0x90;

	terrCoords[ 2 * terrWidth + 0 ].twoTriCodes = 0x0C;
	terrCoords[ 2 * terrWidth + 1 ].twoTriCodes = 0x88;
	terrCoords[ 2 * terrWidth + 2 ].twoTriCodes = 0xC1;

	terrCoords[ 3 * terrWidth + 1 ].twoTriCodes = 0x1C;
	terrCoords[ 3 * terrWidth + 2 ].twoTriCodes = 0x88;
	terrCoords[ 3 * terrWidth + 3 ].twoTriCodes = 0xC9;
	terrCoords[ 3 * terrWidth + 5 ].twoTriCodes = 0xD0;

	terrCoords[ 4 * terrWidth + 0 ].twoTriCodes = 0x0C;
	terrCoords[ 4 * terrWidth + 1 ].twoTriCodes = 0x88;
	terrCoords[ 4 * terrWidth + 2 ].twoTriCodes = 0xC1;

	terrCoords[ 5 * terrWidth + 0 ].twoTriCodes = 0x08;
	terrCoords[ 5 * terrWidth + 1 ].twoTriCodes = 0x0C;
	terrCoords[ 5 * terrWidth + 2 ].twoTriCodes = 0x08;
	terrCoords[ 5 * terrWidth + 3 ].twoTriCodes = 0x0D;
}




HRESULT CMyD3DApplication::LoadTerrainTextures(){
	
	HRESULT hr;

  for( WORD i = 0; i < numTerrTextures; i++ ){

		TextureHolder* pth = &terrTextures[ i ];

		// Create a bitmap and load the texture file into it,
		if( FAILED( pth->LoadBitmapFile( terrTextureFileNames[ i ] ) ) ){
			return E_FAIL;
		}
	
		// Save the image's dimensions
		if( pth->m_hbmBitmap ){
			BITMAP bm;
			GetObject( pth->m_hbmBitmap, sizeof(BITMAP), &bm );
			pth->m_dwWidth  = (DWORD)bm.bmWidth;
			pth->m_dwHeight = (DWORD)bm.bmHeight;
			pth->m_dwBPP    = (DWORD)bm.bmBitsPixel;

			DWORD tempWidth = pth->m_dwWidth;
			DWORD tempHeight = pth->m_dwHeight;
			DWORD tempBPP = pth->m_dwBPP;
		
		}

		if( FAILED( hr = pth->Restore( m_pd3dDevice ) ) ){
			return hr;
		}
	}

	return S_OK;
}	





HRESULT CMyD3DApplication::ReleaseTerrainTextures(){
	
  for( WORD i = 0; i < numTerrTextures; i++ ){

		TextureHolder* pth = &terrTextures[ i ];

		SAFE_RELEASE( pth->m_pddsSurface );
	}

	return S_OK;
}




/*
inline TERRLVERTEX ProjectCoord( WORD x, WORD y, WORD z, BYTE lightIndex ){
	TERRLVERTEX	vertex;

	vertex.x = x;
	vertex.y = y;
	vertex.z = z;

	// change to look-up table: 
	// float lightTable[lightIndex] == lightIndex / 255
	FLOAT lightIntensity = (float)lightIndex / 255.0f;
//	tVertex.dcColor = D3DRGBA( 0.8f * lightIntensity, 0.3f * lightIntensity, 0.0f, 1.0f );
	FLOAT alpha = ((FLOAT)(x) / (FLOAT)(TERR_SCALE)) - 1.0f;
	if( alpha < 0 ){ alpha = 0; }
	else if( alpha > 1 ){ alpha = 1; }
	vertex.color = D3DRGBA( lightIntensity, lightIntensity, lightIntensity, alpha );

	vertex.tu0 = vertex.tu1 = vertex.tu2 = (FLOAT)x / ((FLOAT) TERR_SCALE * 6 );
	vertex.tv0 = vertex.tv1 = vertex.tv2 = (FLOAT)z / ((FLOAT) TERR_SCALE * 6 );

	vertex.tu1 += 0.5;
	vertex.tv1 += 0.5;

	return vertex;
}



inline void ProjectLeftAltCoords( WORD wx, WORD wz, BYTE triCode, INT32 *pTriIndexOffsets,
														  TERRCOORD *pTerrCoord, TERRLVERTEX *pTerrVertex ){
	WORD altWx,altWz;	
	TERRCOORD *pAltTerrCoord;
	TERRLVERTEX *pAltTerrVertex;

	altWx = wx + triIndexWXOffsets[ triCode ];
	altWz = wz + triIndexWZOffsets[ triCode ];
	pAltTerrCoord = pTerrCoord + *( pTriIndexOffsets );
	pAltTerrVertex = pTerrVertex + *( pTriIndexOffsets );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
		pAltTerrCoord->lightIndex );

	altWx = wx + triIndexWXOffsets[ triCode + 16 ];
	altWz = wz + triIndexWZOffsets[ triCode + 16 ];
	pAltTerrCoord = pTerrCoord + *( pTriIndexOffsets + 16 );
	pAltTerrVertex = pTerrVertex + *( pTriIndexOffsets + 16 );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
		pAltTerrCoord->lightIndex );
}




inline void ProjectRightAltCoords( WORD wx, WORD wz, BYTE triCode, INT32 *pTriIndexOffsets,
														  TERRCOORD *pTerrCoord, TERRLVERTEX *pTerrVertex ){
	WORD altWx,altWz;	
	TERRCOORD *pAltTerrCoord;
	TERRLVERTEX *pAltTerrVertex;

	altWx = wx - triIndexWXOffsets[ triCode ];
	altWz = wz - triIndexWZOffsets[ triCode ];
	pAltTerrCoord = pTerrCoord - *( pTriIndexOffsets );
	pAltTerrVertex = pTerrVertex - *( pTriIndexOffsets );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
		pAltTerrCoord->lightIndex );

	altWx = wx - triIndexWXOffsets[ triCode + 16 ];
	altWz = wz - triIndexWZOffsets[ triCode + 16 ];
	pAltTerrCoord = pTerrCoord - *( pTriIndexOffsets + 16 );
	pAltTerrVertex = pTerrVertex - *( pTriIndexOffsets + 16 );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
		pAltTerrCoord->lightIndex );
}
*/

inline TERRTLVERTEX ProjectCoord( WORD x, WORD y, WORD z, BYTE lightIndex,
	D3DMATRIX matSet, DWORD dwClipWidth, DWORD dwClipHeight)
{
	TERRTLVERTEX	tVertex;

	FLOAT xp = matSet._11*x + matSet._21*y + matSet._31*z + matSet._41;
	FLOAT yp = matSet._12*x + matSet._22*y + matSet._32*z + matSet._42;
	FLOAT zp = matSet._13*x + matSet._23*y + matSet._33*z + matSet._43;
	FLOAT wp = matSet._14*x + matSet._24*y + matSet._34*z + matSet._44;
			
	tVertex.dvSX = ( 1.0f + (xp/wp) ) * dwClipWidth;;
	tVertex.dvSY = ( 1.0f - (yp/wp) ) * dwClipHeight;
	tVertex.dvSZ = zp / wp;
	tVertex.dvRHW = wp;

	// change to look-up table: 
	// D3DRGBA terrVertRGBA[lightIndex] == D3DRGBA( lightIndex / 255, lightIndex / 255, etc.
	FLOAT lightIntensity = (float)lightIndex / 255.0f;
//	tVertex.dcColor = D3DRGBA( 0.8f * lightIntensity, 0.3f * lightIntensity, 0.0f, 1.0f );

/*
	FLOAT alpha = ((FLOAT)(x) / (FLOAT)(TERR_SCALE)) - 1.0f;
	if( alpha < 0 ){ alpha = 0; }
	else if( alpha > 1 ){ alpha = 1; }
*/
	tVertex.dcColor = D3DRGBA( lightIntensity, lightIntensity, lightIntensity, 1.0f );

	tVertex.dvTU = (FLOAT)x / ((FLOAT) TERR_SCALE * 6 );
	tVertex.dvTV = (FLOAT)z / ((FLOAT) TERR_SCALE * 6 );

	return tVertex;
}




inline void ProjectLeftAltCoords( WORD wx, WORD wz, BYTE triCode, INT32 *pTriIndexOffsets,
														  TERRCOORD *pTerrCoord, TERRTLVERTEX *pTerrVertex,
															D3DMATRIX matSet, DWORD dwClipWidth, DWORD dwClipHeight ){
	WORD altWx,altWz;	
	TERRCOORD *pAltTerrCoord;
	TERRTLVERTEX *pAltTerrVertex;

	altWx = wx + triIndexWXOffsets[ triCode ];
	altWz = wz + triIndexWZOffsets[ triCode ];
	pAltTerrCoord = pTerrCoord + *( pTriIndexOffsets );
	pAltTerrVertex = pTerrVertex + *( pTriIndexOffsets );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
	pAltTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

	altWx = wx + triIndexWXOffsets[ triCode + 16 ];
	altWz = wz + triIndexWZOffsets[ triCode + 16 ];
	pAltTerrCoord = pTerrCoord + *( pTriIndexOffsets + 16 );
	pAltTerrVertex = pTerrVertex + *( pTriIndexOffsets + 16 );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
	pAltTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );
}




inline void ProjectRightAltCoords( WORD wx, WORD wz, BYTE triCode, INT32 *pTriIndexOffsets,
														  TERRCOORD *pTerrCoord, TERRTLVERTEX *pTerrVertex,
															D3DMATRIX matSet, DWORD dwClipWidth, DWORD dwClipHeight ){
	WORD altWx,altWz;	
	TERRCOORD *pAltTerrCoord;
	TERRTLVERTEX *pAltTerrVertex;

	altWx = wx - triIndexWXOffsets[ triCode ];
	altWz = wz - triIndexWZOffsets[ triCode ];
	pAltTerrCoord = pTerrCoord - *( pTriIndexOffsets );
	pAltTerrVertex = pTerrVertex - *( pTriIndexOffsets );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
	pAltTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

	altWx = wx - triIndexWXOffsets[ triCode + 16 ];
	altWz = wz - triIndexWZOffsets[ triCode + 16 ];
	pAltTerrCoord = pTerrCoord - *( pTriIndexOffsets + 16 );
	pAltTerrVertex = pTerrVertex - *( pTriIndexOffsets + 16 );

	*pAltTerrVertex = ProjectCoord( altWx, pAltTerrCoord->height, altWz,
	pAltTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );
}




HRESULT CMyD3DApplication::RenderTerrain(){

	HRESULT hr;

	LPDIRECTDRAWSURFACE7 pddsTempSurface;

	DWORD numIndices=0;
	D3DMATRIX 	matWorld, matView, matProj, matSet;
	FLOAT 		matVertex[4],matTVertex[4];
	camPhase = timeGetTime() / 2000.0f;

	TERRTLVERTEX *pTerrVertex, *pAltTerrVertex;
	TERRCOORD *pTerrCoord;
	WORD *pTerrIndex;
	WORD iz,ix,wx,wz;
	WORD renderOffset;
	INT32 offset;
	BYTE twoTriCodes, triCode, triSize;
	INT32 *pTriIndexOffsets;

	//matWorld._11 = matWorld._22 = matWorld._33 = matWorld._44 = 1.0f;
	//matWorld._12 = matWorld._13 = matWorld._14 = matWorld._41 = 0.0f;
	//matWorld._21 = matWorld._23 = matWorld._24 = matWorld._42 = 0.0f;
	//matWorld._31 = matWorld._32 = matWorld._34 = matWorld._43 = 0.0f;
    
	//m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_WORLD, &matWorld );
	
	///// viewpoint ////
	
	D3DVECTOR vEyePt		= D3DVECTOR( (cos(camPhase)*(float)terrWidth*0.75 + (float)terrWidth/2) * TERR_SCALE, 45.0f + sin(camPhase/1.8)*(float)30, (sin(camPhase)*(float)terrDepth*0.75 + (float)terrDepth/2) * TERR_SCALE );
//	D3DVECTOR vEyePt		= D3DVECTOR( terrWidth * TERR_SCALE, 40.0f, terrDepth/2 * TERR_SCALE );
//	D3DVECTOR vEyePt		= D3DVECTOR( terrWidth/2 * TERR_SCALE, 40.0f, terrDepth * TERR_SCALE );
	D3DVECTOR vLookatPt = D3DVECTOR( terrWidth/2 * TERR_SCALE, 0.0f, terrDepth/2 * TERR_SCALE );
	D3DVECTOR vUpVec		= D3DVECTOR( 0.0f, 1.0f,	 0.0f );

	SetViewMatrix(
		matView,
		vEyePt,
		vLookatPt,
		vUpVec );

	//m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &matView );

	D3DVIEWPORT7 vp;
	m_pd3dDevice->GetViewport(&vp);
	FLOAT fAspect = ((FLOAT)vp.dwHeight) / vp.dwWidth;

	DWORD dwClipWidth  = vp.dwWidth/2;
	DWORD dwClipHeight = vp.dwHeight/2;
	
	D3DUtil_SetProjectionMatrix( matProj, g_PI/3, fAspect, 1.0f, 1000.0f );

	D3DMath_MatrixMultiply( matSet, matView, matProj );

	//m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &matProj );

	
	INT32 visibleStartX = 0;
	INT32 visibleStartZ = 0;
	INT32 visibleEndX = terrWidth - 1;
	INT32 visibleEndZ = terrDepth - 1;

	INT32 borderStartX = visibleStartX - 8;
	INT32 borderStartZ = visibleStartZ - 8;
	INT32 borderEndX = visibleEndX + 8;
	INT32 borderEndZ = visibleEndZ + 8;

	if( borderStartX < 0 ){
		if( visibleStartX < 0 ){
			visibleStartX = 0;
		}
		borderStartX = 0;
	}

	if( borderStartZ < 0 ){
		if( visibleStartZ < 0 ){
			visibleStartZ = 0;
		}
		borderStartZ = 0;
	}

	if( borderEndX >= terrWidth ){
		if( visibleEndX >= terrWidth ){
			visibleEndX = terrWidth - 1;;
		}
		borderEndX = terrWidth - 1;
	}

	if( borderEndZ >= terrDepth ){
		if( visibleEndZ >= terrDepth ){
			visibleEndZ = terrDepth - 1;;
		}
		borderEndZ = terrDepth - 1;
	}

	WORD borderLeft = visibleStartX - borderStartX;
	WORD borderTop = visibleStartZ - borderStartZ;
	WORD borderRight = borderEndX - visibleEndX;
	WORD borderBottom = borderEndZ - visibleEndZ;

	WORD visibleWidth = visibleEndX - visibleStartX + 1;
	WORD visibleDepth = visibleEndZ - visibleStartZ + 1;

	WORD renderWidth = borderLeft + visibleWidth + borderRight;
	WORD renderDepth = borderTop + visibleDepth + borderBottom;

	pTerrIndex = terrIndices;
	
	// render border

	// top-left corner

	renderOffset = 0;
	pTerrCoord = terrCoords + ( borderStartZ ) * terrWidth + borderStartX;
	pTerrVertex = terrVertices + renderOffset;
	wz = borderStartZ * TERR_SCALE;

	// the two freak vertices: top-left & bottom-right of terrain
	if( borderStartX == 0 && borderStartZ == 0 ){
		*pTerrVertex = ProjectCoord( 0, pTerrCoord->height, 0,
		pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );
	}

	if( borderEndX == terrWidth - 1 && borderEndZ == terrDepth - 1 ){
		terrVertices[ ( renderDepth ) * renderWidth - 1 ] =
		ProjectCoord( borderEndX * TERR_SCALE,
		terrCoords[ borderEndZ * terrWidth + borderEndX ].height,
		borderEndZ * TERR_SCALE,
		terrCoords[ borderEndZ * terrWidth + borderEndX ].lightIndex,
		matSet, dwClipWidth, dwClipHeight);
	}

	for(iz = borderTop; iz > 0; 
	iz--, wz += TERR_SCALE, pTerrCoord += ( terrWidth - borderLeft ),
	pTerrVertex += ( renderWidth - borderLeft ), renderOffset += ( renderWidth - borderLeft ) ){
		for(ix = borderLeft, wx = borderStartX * TERR_SCALE; ix > 0;
		ix--, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( triCode = pTerrCoord->twoTriCodes & TRI_RIGHT ) ){
				if( iz <= triAltVertMaxDist[ triCode ] &&
					ix <= triHorzMaxDist[ triCode ] ){

					*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
						pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset - *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset - *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// project this tri's other two vertices
					ProjectRightAltCoords( wx, wz, triCode, pTriIndexOffsets,
						pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
				}
			}
		}
	}

	// top-right corner

	renderOffset = borderLeft + visibleWidth;
	pTerrCoord = terrCoords + ( borderStartZ ) * terrWidth + ( visibleEndX + 1 );
	pTerrVertex = terrVertices + renderOffset;
	wz = borderStartZ * TERR_SCALE;

	for(iz = borderTop; iz > 0; 
	iz--, wz += TERR_SCALE, pTerrCoord += ( terrWidth - borderRight ),
	pTerrVertex += ( renderWidth - borderRight ), renderOffset += ( renderWidth - borderRight ) ){
		for(ix = 1, wx = ( visibleEndX + 1 ) * TERR_SCALE; ix <= borderRight;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( triCode = ( pTerrCoord->twoTriCodes & TRI_LEFT ) >> 4 ) ){
				if( iz <= triVertMaxDist[ triCode ] &&
					ix <= triHorzMaxDist[ triCode ] ){

					*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
						pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset + *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset + *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// project this tri's other two vertices
					ProjectLeftAltCoords( wx, wz, triCode, pTriIndexOffsets,
						pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
				}
			}
		}
	}

	// bottom-left corner
	
	renderOffset = ( borderTop + visibleDepth ) * renderWidth;
	pTerrCoord = terrCoords + ( visibleEndZ + 1 ) * terrWidth + borderStartX;
	pTerrVertex = terrVertices + renderOffset;
	wz = ( visibleEndZ + 1 ) * TERR_SCALE;

	for(iz = 1; iz <= borderBottom; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - borderLeft ),
	pTerrVertex += ( renderWidth - borderLeft ), renderOffset += ( renderWidth - borderLeft ) ){
		for(ix = borderLeft, wx = borderStartX * TERR_SCALE; ix > 0;
		ix--, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( triCode = pTerrCoord->twoTriCodes & TRI_RIGHT ) ){
				if( iz <= triVertMaxDist[ triCode ] &&
					ix <= triHorzMaxDist[ triCode ] ){

					*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
						pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset - *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset - *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// project this tri's other two vertices
					ProjectRightAltCoords( wx, wz, triCode, pTriIndexOffsets,
						pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
				}
			}
		}
	}

	// bottom-right corner
	
	renderOffset = ( borderTop + visibleDepth ) * renderWidth + ( borderLeft + visibleWidth );
	pTerrCoord = terrCoords + ( visibleEndZ + 1 ) * terrWidth + ( visibleEndX + 1 );
	pTerrVertex = terrVertices + renderOffset;
	wz = ( visibleEndZ + 1 ) * TERR_SCALE;

	for(iz = 1; iz <= borderBottom; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - borderRight ),
	pTerrVertex += ( renderWidth - borderRight ), renderOffset += ( renderWidth - borderRight ) ){
		for(ix = 1, wx = ( visibleEndX + 1 ) * TERR_SCALE; ix <= borderRight;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( triCode = ( pTerrCoord->twoTriCodes & TRI_LEFT ) >> 4 ) ){
				if( iz <= triAltVertMaxDist[ triCode ] &&
					ix <= triHorzMaxDist[ triCode ] ){

					*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
						pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset + *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset + *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// project this tri's other two vertices
					ProjectLeftAltCoords( wx, wz, triCode, pTriIndexOffsets,
						pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
				}
			}
		}
	}

	// top row

	renderOffset = borderLeft;
	pTerrCoord = terrCoords + ( borderStartZ ) * terrWidth + visibleStartX;
	pTerrVertex = terrVertices + renderOffset;
	wz = borderStartZ * TERR_SCALE;

	for(iz = borderTop; iz > 0; 
	iz--, wz += TERR_SCALE, pTerrCoord += ( terrWidth - visibleWidth ),
	pTerrVertex += ( renderWidth - visibleWidth ), renderOffset += ( renderWidth - visibleWidth ) ){
		for(ix = 0, wx = visibleStartX * TERR_SCALE; ix < visibleWidth;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( twoTriCodes = pTerrCoord->twoTriCodes ) ){
				if( ( triCode = ( twoTriCodes & TRI_LEFT ) >> 4 ) ){
					if( iz <= triVertMaxDist[ triCode ] ){			

						*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
							pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

						pTriIndexOffsets = triIndexOffsets + triCode;
						*pTerrIndex = renderOffset;
						*( pTerrIndex + 1 ) = renderOffset + *( pTriIndexOffsets ); 
						*( pTerrIndex + 2 ) = renderOffset + *( pTriIndexOffsets + 16 ); 
						pTerrIndex += 3;

						// project this tri's other two vertices
						ProjectLeftAltCoords( wx, wz, triCode, pTriIndexOffsets,
							pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
					}
				}
				if( ( triCode = twoTriCodes & TRI_RIGHT ) ){
					if( iz <= triAltVertMaxDist[ triCode ] ){

						*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
							pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

						pTriIndexOffsets = triIndexOffsets + triCode;
						*pTerrIndex = renderOffset;
						*( pTerrIndex + 1 ) = renderOffset - *( pTriIndexOffsets ); 
						*( pTerrIndex + 2 ) = renderOffset - *( pTriIndexOffsets + 16 ); 
						pTerrIndex += 3;

					// project this tri's other two vertices
						ProjectRightAltCoords( wx, wz, triCode, pTriIndexOffsets,
							pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
					}
				}
			}
		}
	}

	// bottom row

	renderOffset = ( borderTop + visibleDepth ) * renderWidth + borderLeft;
	pTerrCoord = terrCoords + ( visibleEndZ + 1 ) * terrWidth + visibleStartX;
	pTerrVertex = terrVertices + renderOffset;
	wz = ( visibleEndZ + 1 ) * TERR_SCALE;

	for(iz = 1; iz <= borderBottom; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - visibleWidth ),
	pTerrVertex += ( renderWidth - visibleWidth ), renderOffset += ( renderWidth - visibleWidth ) ){
		for(ix = 0, wx = visibleStartX * TERR_SCALE; ix < visibleWidth;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( twoTriCodes = pTerrCoord->twoTriCodes ) ){
				if( ( triCode = ( twoTriCodes & TRI_LEFT ) >> 4 ) ){
					if( iz <= triAltVertMaxDist[ triCode ] ){			

						*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
							pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

						pTriIndexOffsets = triIndexOffsets + triCode;
						*pTerrIndex = renderOffset;
						*( pTerrIndex + 1 ) = renderOffset + *( pTriIndexOffsets ); 
						*( pTerrIndex + 2 ) = renderOffset + *( pTriIndexOffsets + 16 ); 
						pTerrIndex += 3;

						// project this tri's other two vertices
						ProjectLeftAltCoords( wx, wz, triCode, pTriIndexOffsets,
							pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
					}
				}
				if( ( triCode = twoTriCodes & TRI_RIGHT ) ){
					if( iz <= triVertMaxDist[ triCode ] ){

						*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
							pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

						pTriIndexOffsets = triIndexOffsets + triCode;
						*pTerrIndex = renderOffset;
						*( pTerrIndex + 1 ) = renderOffset - *( pTriIndexOffsets ); 
						*( pTerrIndex + 2 ) = renderOffset - *( pTriIndexOffsets + 16 ); 
						pTerrIndex += 3;

					// project this tri's other two vertices
						ProjectRightAltCoords( wx, wz, triCode, pTriIndexOffsets,
							pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
					}
				}
			}
		}
	}


	// left column

	renderOffset = ( borderTop ) * renderWidth;
	pTerrCoord = terrCoords + ( visibleStartZ ) * terrWidth + borderStartX;
	pTerrVertex = terrVertices + renderOffset;
	wz = visibleStartZ * TERR_SCALE;

	for(iz = 0; iz < visibleDepth; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - borderLeft ),
	pTerrVertex += ( renderWidth - borderLeft ), renderOffset += ( renderWidth - borderLeft ) ){
		for(ix = borderLeft, wx = borderStartX * TERR_SCALE; ix > 0;
		ix--, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( triCode = pTerrCoord->twoTriCodes & TRI_RIGHT ) ){
				if( ix <= triHorzMaxDist[ triCode ] ){

					*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
						pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset - *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset - *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// project this tri's other two vertices
					ProjectRightAltCoords( wx, wz, triCode, pTriIndexOffsets,
						pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
				}
			}
		}
	}

	
	// right column

	renderOffset = ( borderTop ) * renderWidth + ( borderLeft + visibleWidth );;
	pTerrCoord = terrCoords + ( visibleStartZ ) * terrWidth + ( visibleEndX + 1 );
	pTerrVertex = terrVertices + renderOffset;
	wz = visibleStartZ * TERR_SCALE;

	for(iz = 0; iz < visibleDepth; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - borderRight ),
	pTerrVertex += ( renderWidth - borderRight ), renderOffset += ( renderWidth - borderRight ) ){
		for(ix = 1, wx = ( visibleEndX + 1 ) * TERR_SCALE; ix <= borderRight;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( triCode = ( pTerrCoord->twoTriCodes & TRI_LEFT ) >> 4 ) ){
				if( ix <= triHorzMaxDist[ triCode ] ){

					*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
						pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset + *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset + *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// project this tri's other two vertices
					ProjectLeftAltCoords( wx, wz, triCode, pTriIndexOffsets,
						pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
				}
			}
		}
	}
	
  // visible area

	renderOffset = ( borderTop ) * renderWidth + ( borderLeft );
	pTerrCoord = terrCoords + ( visibleStartZ ) * terrWidth + visibleStartX;
	pTerrVertex = terrVertices + renderOffset;
	wz = visibleStartZ * TERR_SCALE;

	for(iz = 0; iz < visibleDepth; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - visibleWidth ),
	pTerrVertex += ( renderWidth - visibleWidth ), renderOffset += ( renderWidth - visibleWidth ) ){
		for(ix = 0, wx = visibleStartX * TERR_SCALE; ix < visibleWidth;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, renderOffset++ ){
			if( ( twoTriCodes = pTerrCoord->twoTriCodes ) ){

				*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
				pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

				if( ( triCode = ( twoTriCodes & TRI_LEFT ) >> 4 ) ){
					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset + *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset + *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					// all tri max dist tests are off by one here, so use < instead of <=
					if( ix < triHorzMaxDist[ triCode ] ||
						iz < triAltVertMaxDist[ triCode ] ||
						( visibleDepth - iz - 1 ) < triVertMaxDist[ triCode ] ){
						ProjectLeftAltCoords( wx, wz, triCode, pTriIndexOffsets,
							pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
					}
				}
				if( ( triCode = twoTriCodes & TRI_RIGHT ) ){ 
					pTriIndexOffsets = triIndexOffsets + triCode;
					*pTerrIndex = renderOffset;
					*( pTerrIndex + 1 ) = renderOffset - *( pTriIndexOffsets ); 
					*( pTerrIndex + 2 ) = renderOffset - *( pTriIndexOffsets + 16 ); 
					pTerrIndex += 3;

					if( iz < triVertMaxDist[ triCode ] ||
						( visibleDepth - iz - 1 ) < triAltVertMaxDist[ triCode ] ||
						( visibleWidth - ix - 1 ) < triHorzMaxDist[ triCode ] ){
						ProjectRightAltCoords( wx, wz, triCode, pTriIndexOffsets,
							pTerrCoord, pTerrVertex, matSet, dwClipWidth, dwClipHeight );
					}
				}
			}
		}
	}


	m_pd3dDevice->SetTexture( 0, terrTextures[ 1 ].m_pddsSurface );
//	m_pd3dDevice->SetTexture( 1, terrTextures[ 1 ].m_pddsSurface );
//	m_pd3dDevice->SetTexture( 2, terrTextures[ 2 ].m_pddsSurface );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );

/*
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );

	m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
	m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_BLENDDIFFUSEALPHA );
*/
//	m_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
//	m_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLORARG1, D3DTA_CURRENT );
//	m_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_MODULATE );

//	hr = m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
//	hr = m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );

//	hr = m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_CURRENT );
//	hr = m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
//	hr = m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	
/*
	for(iz=0; iz < renderDepth; iz++){
		for(ix=0; ix < renderWidth; ix++){
			if( ( ix % 4 < 2 ) ){
				terrVertices[ iz * renderWidth + ix ].color = D3DRGBA( 1.0f, 1.0f, 1.0f, 0.0f );
			}
		}
	}
*/
	if( (WORD)(pTerrIndex - terrIndices) <= MAX_RENDER_AREA * 6 ){
		m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, TERRTLVERTEXDESC,
			terrVertices, terrArea, terrIndices, (pTerrIndex - terrIndices), D3DDP_WAIT );				
	}


	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );

	m_pd3dDevice->SetTexture( 0, terrTextures[ 0 ].m_pddsSurface );

	//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHATESTENABLE, TRUE );    
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
	
//	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
//	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );

	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	hr = m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );

	if( (WORD)(pTerrIndex - terrIndices) <= MAX_RENDER_AREA * 6 ){
		m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, TERRTLVERTEXDESC,
			terrVertices, terrArea, terrIndices, (pTerrIndex - terrIndices), D3DDP_WAIT );				
	}

	return S_OK;
}
