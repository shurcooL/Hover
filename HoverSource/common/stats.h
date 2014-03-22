

#include "common.h"



// these FVFs is only used in stats.cpp
#define STATTLVERTEXDESC		( D3DFVF_XYZRHW | D3DFVF_TEX1 )
struct STATTLVERTEX { 
	D3DVALUE sx, sy, sz; 
	D3DVALUE rhw; 
	D3DVALUE tu, tv; 
};


#define STATCOLORTLVERTEXDESC		( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
struct STATCOLORTLVERTEX { 
	D3DVALUE sx, sy, sz; 
	D3DVALUE rhw; 
	D3DCOLOR color;
	D3DVALUE tu, tv; 
};


#define NUM_STATS						7

#define STAT_RANK_INDEX										0
const INT32 STAT_TIME_DIGIT_INDEX[4] =		{ 4, 12, 16, 24 };
#define STAT_TIME_DEC_INDEX								8
#define STAT_TIME_COLON_INDEX							20

#define STAT_RANK_SX											5
#define STAT_RANK_SY											5
#define STAT_RANK_WIDTH										50
#define STAT_RANK_HEIGHT									50

const INT32 STAT_TIME_DIGIT_SX[4] =				{ -30, -68, -93, -131 };
#define STAT_TIME_DIGIT_SY								5 
#define STAT_TIME_DIGIT_WIDTH							25
#define STAT_TIME_DIGIT_HEIGHT						25

#define STAT_TIME_DEC_SX									-44
#define STAT_TIME_DEC_SY									23
#define STAT_TIME_DEC_WIDTH								8
#define STAT_TIME_DEC_HEIGHT							3

#define STAT_TIME_COLON_SX								-107
#define STAT_TIME_COLON_SY								14
#define STAT_TIME_COLON_WIDTH							11
#define STAT_TIME_COLON_HEIGHT						12

INT32 statPositions[ NUM_STATS * 4 ] = {
 STAT_RANK_SX,					STAT_RANK_SY,					STAT_RANK_WIDTH,				STAT_RANK_HEIGHT,
 STAT_TIME_DIGIT_SX[0],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,	STAT_TIME_DIGIT_HEIGHT,
 STAT_TIME_DEC_SX,			STAT_TIME_DEC_SY,			STAT_TIME_DEC_WIDTH,		STAT_TIME_DEC_HEIGHT,
 STAT_TIME_DIGIT_SX[1],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,	STAT_TIME_DIGIT_HEIGHT,
 STAT_TIME_DIGIT_SX[2],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,	STAT_TIME_DIGIT_HEIGHT,
 STAT_TIME_COLON_SX,		STAT_TIME_COLON_SY,		STAT_TIME_COLON_WIDTH,	STAT_TIME_COLON_HEIGHT,
 STAT_TIME_DIGIT_SX[3],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,	STAT_TIME_DIGIT_HEIGHT };

STATTLVERTEX statVertices[ NUM_STATS * 4 ];
WORD statIndices[ NUM_STATS * 6 ];


#define MAX_DISPLAYED_NAVCOORDS						50

STATCOLORTLVERTEX navCoordVertices[ MAX_DISPLAYED_NAVCOORDS * 4 ];






HRESULT CMyApplication::InitStats(){

	INT32 sx, sy, width, height;
	DWORD stat, coord;

	STATTLVERTEX emptyVertex = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, -1.0f };

	for(stat=0; stat < NUM_STATS; stat++){

		sx = statPositions[stat*4];
		if( sx < 0 ){
			sx = m_vp.dwWidth + sx;
		}

		sy = statPositions[stat*4+1];
		if( sy < 0 ){
			sy = m_vp.dwHeight + sy;
		}

		width = statPositions[stat*4+2];
		height = statPositions[stat*4+3];

		statVertices[stat*4] = emptyVertex;
		statVertices[stat*4].sx = sx;
		statVertices[stat*4].sy = sy;

		statVertices[stat*4+1] = emptyVertex;
		statVertices[stat*4+1].sx = sx + width;
		statVertices[stat*4+1].sy = sy;

		statVertices[stat*4+2] = emptyVertex;
		statVertices[stat*4+2].sx = sx + width;
		statVertices[stat*4+2].sy = sy + height;

		statVertices[stat*4+3] = emptyVertex;
		statVertices[stat*4+3].sx = sx;
		statVertices[stat*4+3].sy = sy + height;

		statIndices[stat*6] = stat*4;
		statIndices[stat*6+1] = stat*4+3;
		statIndices[stat*6+2] = stat*4+2;

		statIndices[stat*6+3] = stat*4;
		statIndices[stat*6+4] = stat*4+2;
		statIndices[stat*6+5] = stat*4+1;
	}

	STATCOLORTLVERTEX emptyColorVertex = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, -1.0f };

	for(coord=0; coord < MAX_STAT_NAVCOORDS * 6; coord++){

		statNavCoordVertices[coord*6] = emptyColorVertex;
		statNavCoordVertices[coord*6].tu = 50.0f / 255.0f;
		statNavCoordVertices[coord*6].tv = 126.0f / 255.0f;

		statNavCoordVertices[coord*6+1] = emptyColorVertex;
		statNavCoordVertices[coord*6+1].tu = ( 50.0f + STAT_NAVCOORD_WIDTH ) / 255.0f;
		statNavCoordVertices[coord*6+1].tv = ( 126.0f + STAT_NAVCOORD_HEIGHT ) / 255.0f;

		statNavCoordVertices[coord*6+2] = emptyColorVertex;
		statNavCoordVertices[coord*6+2].tu = ( 50.0f + STAT_NAVCOORD_WIDTH ) / 255.0f;
		statNavCoordVertices[coord*6+2].tv = 126.0f / 255.0f;

		statNavCoordVertices[coord*6+3] = emptyColorVertex;
		statNavCoordVertices[coord*6+3].tu = 50.0f / 255.0f;
		statNavCoordVertices[coord*6+3].tv = 126.0f / 255.0f;

		statNavCoordVertices[coord*6+4] = emptyColorVertex;
		statNavCoordVertices[coord*6+4].tu = 50.0f / 255.0f;
		statNavCoordVertices[coord*6+4].tv = ( 126.0f + STAT_NAVCOORD_HEIGHT ) / 255.0f;

		statNavCoordVertices[coord*6+5] = emptyColorVertex;
		statNavCoordVertices[coord*6+5].tu = ( 50.0f + STAT_NAVCOORD_WIDTH ) / 255.0f;
		statNavCoordVertices[coord*6+5].tv = ( 126.0f + STAT_NAVCOORD_HEIGHT ) / 255.0f;
	}

	return S_OK;
}





HRESULT CMyApplication::DisplayStats(){

	HRESULT hr;
	FLOAT topLeftTu, topLeftTv, width, height;
	DWORD i, stat;

	static FLOAT fFPS      = 0.0f;
	static FLOAT fLastTime = 0.0f;
	static DWORD dwFrames  = 0L;
	DWORD fpsTenths, elapsedTimeTenths = ( elapsedTime * 10 + 0.5 );

	// Keep track of the time lapse and frame count
	FLOAT fTime = timeGetTime() * 0.001f; // Get current time in seconds
	++dwFrames;
	
	// Update the frame rate once per second
	if( fTime - fLastTime > 1.0f )
	{
		fFPS      = dwFrames / (fTime - fLastTime);
		fLastTime = fTime;
		dwFrames  = 0L;
	}
	fpsTenths = (DWORD)( fFPS * 10 + 0.5 );

	statVertices[STAT_RANK_INDEX].tu = 0.0f;
	statVertices[STAT_RANK_INDEX].tv = 0.0f;


	for(i=0; i < 4; i++){
		statVertices[STAT_TIME_DIGIT_INDEX[i]].tu = (FLOAT)( elapsedTimeTenths % 10 ) * 25.0f / 255.0f;
		statVertices[STAT_TIME_DIGIT_INDEX[i]].tv = 101.0f / 255.0f;
		elapsedTimeTenths /= 10;
	}

	statVertices[STAT_TIME_DEC_INDEX].tu = 7.0f / 255.0f;
	statVertices[STAT_TIME_DEC_INDEX].tv = 143.0f / 255.0f;

	statVertices[STAT_TIME_COLON_INDEX].tu = 31.0f / 255.0f;
	statVertices[STAT_TIME_COLON_INDEX].tv = 134.0f / 255.0f;


	// calc texture coords for other 3 vertices of each stat
	for(stat=0; stat < NUM_STATS; stat++){

		topLeftTu = statVertices[stat*4].tu;
		topLeftTv = statVertices[stat*4].tv;

		width = (FLOAT)statPositions[stat*4+2] / 255.0f;
		height = (FLOAT)statPositions[stat*4+3] / 255.0f;

		statVertices[stat*4+1].tu = topLeftTu + width;
		statVertices[stat*4+1].tv = topLeftTv;

		statVertices[stat*4+2].tu = topLeftTu + width;
		statVertices[stat*4+2].tv = topLeftTv + height;

		statVertices[stat*4+3].tu = topLeftTu;
		statVertices[stat*4+3].tv = topLeftTv + height;
	}


	if( FAILED( hr = m_pd3dDevice->SetTexture( 0, (GetTexture( STATSTEXTURE_FILENAME ))->m_pddsSurface ) ) ){
		MyAlert(&hr, "DisplayStats() > SetTexture() failed" );
		return E_FAIL;
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ); 
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_POINT ); 

	if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, STATTLVERTEXDESC, 
			statVertices, NUM_STATS * 4, 
			statIndices, NUM_STATS * 6, NULL ) ) ){
		MyAlert( &hr, "DisplayStats() > DrawIndexedPrimitive() failed" );
		return hr;
	}

/*
	STATTLVERTEX vRect[4];
	STATTLVERTEX emptyVertex = { 0, 0, 0, 1, 0, 0 };

	vRect[0] = emptyVertex;
	vRect[0].sx = 0; 
	vRect[0].sy = 0;
	vRect[0].tu = 0;
	vRect[0].tv = 0;

	vRect[1] = emptyVertex;
	vRect[1].sx = 0; 
	vRect[1].sy = 256;
	vRect[1].tu = 0;
	vRect[1].tv = 1;

	vRect[2] = emptyVertex;
	vRect[2].sx = 256;
	vRect[2].sy = 0;
	vRect[2].tu = 1;
	vRect[2].tv = 0;

	vRect[3] = emptyVertex;
	vRect[3].sx = 256;
	vRect[3].sy = 256;
	vRect[3].tu = 1;
	vRect[3].tv = 1;

	
	if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, STATTLVERTEXDESC,
		vRect, 4, statIndices, 6, NULL ) ) ){
		MyAlert( &hr, "DisplayStats() > DrawPrimitive() failed" );
		return hr;
	} 
*/

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_ANISOTROPIC  ); 
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_ANISOTROPIC ); 

	return S_OK;
}




HRESULT CMyApplication::DisplayNavMap(){

	DWORD coord;
	STATCOLORTLVERTEX* pVertex;
	NAVCOORD* pCoord;
	INT32 centerx, centerz;
	FLOAT mapx, mapy;

	pCoord = track.navCoords;

	pVertex = statNavCoordVertices;
	
	centerx = racers[camera.racerNum].pos.x;
	centerz = racers[camera.racerNum].pos.z;

	for(coord=0; coord < track.numNavCoords; coord++, pCoord++){
	
		mapx = (FLOAT)( pCoord->x - centerx ) * navMapScale;
		mapy = (FLOAT)( pCoord->z - centerz ) * navMapScale;

		if( mapx >= 0.0f && mapz >= 0.0f && mapx <= NAVMAP_WIDTH && mapz <= NAVMAP_HEIGHT ){

			sx = navMapSx + mapx;
			sy = navMapSy + mapy;

			pVertex[0].sx = sx;
			pVertex[0].sy = sy;
			pVertex[1].sx = sx + STAT_NAVCOORD_WIDTH;
			pVertex[1].sy = sy + STAT_NAVCOORD_HEIGHT;
			pVertex[2].sx = sx + STAT_NAVCOORD_WIDTH;
			pVertex[2].sy = sy;

			pVertex[3].sx = sx;
			pVertex[3].sy = sy;
			pVertex[4].sx = sx;
			pVertex[4].sy = sy + STAT_NAVCOORD_HEIGHT;
			pVertex[5].sx = sx + STAT_NAVCOORD_WIDTH;
			pVertex[5].sy = sy + STAT_NAVCOORD_HEIGHT;

			pVertex[0].color = pVertex[1].color = pVertex[2].color =
				pVertex[3].color = pVertex[4].color = pVertex[5].color = 0xFFFFFFFF;

			pVertex += 6;
		}
	}

	if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, STATCOLORTLVERTEXDESC, 
			navCoordVertices, ( pVertex - statNavCoordVertices ), NULL ) ) ){
		MyAlert( &hr, "DisplayStats() > DrawIndexedPrimitive() failed" );
		return hr;
	}

}


