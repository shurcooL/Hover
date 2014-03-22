

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


#define COLORTLVERTEXDESC		( D3DFVF_XYZRHW | D3DFVF_DIFFUSE )
struct COLORTLVERTEX { 
	D3DVALUE sx, sy, sz; 
	D3DVALUE rhw; 
	D3DCOLOR color;
};



#define MAX_STATS													14

/*
#define STAT_RANK_INDEX						0
const INT32 STAT_TIME_DIGIT_INDEX[4] =		{ 4, 12, 16, 24 };
#define STAT_TIME_DEC_INDEX								8
#define STAT_TIME_COLON_INDEX							20
#define STAT_RANKBAR_INDEX								28
#define STAT_TIMEBAR_INDEX								32
#define STAT_TITLE_INDEX									36
#define STAT_COUNTDOWN_INDEX							40
*/

#define STAT_RANK_SX							5
#define STAT_RANK_SY							15
#define STAT_RANK_WIDTH						51
#define STAT_RANK_HEIGHT					48

const INT32 STAT_TIME_DIGIT_SX[4] =				{ -30, -68, -93, -131 };
#define STAT_TIME_DIGIT_SY								17 
#define STAT_TIME_DIGIT_WIDTH							25
#define STAT_TIME_DIGIT_HEIGHT						22

#define STAT_TIME_DEC_SX									-44
#define STAT_TIME_DEC_SY									29
#define STAT_TIME_DEC_WIDTH								12
#define STAT_TIME_DEC_HEIGHT							8

#define STAT_TIME_COLON_SX								-107
#define STAT_TIME_COLON_SY								22
#define STAT_TIME_COLON_WIDTH							14
#define STAT_TIME_COLON_HEIGHT						15

#define STAT_RANKBAR_SX										0
#define STAT_RANKBAR_SY										0
#define STAT_RANKBAR_WIDTH								63
#define STAT_RANKBAR_HEIGHT								16

#define STAT_TIMEBAR_SX										-126
#define STAT_TIMEBAR_SY										0
#define STAT_TIMEBAR_WIDTH								126
#define STAT_TIMEBAR_HEIGHT								16

#define STAT_TITLE_SX											0
#define STAT_TITLE_SY											-59
#define STAT_TITLE_WIDTH									171
#define STAT_TITLE_HEIGHT									59

#define STAT_RACE_WIDTH										201
#define STAT_RACE_HEIGHT									46

const INT32 STAT_LAP_DOT_SX[3] =					{ -35, -25, -15 };
#define STAT_LAP_DOT_SY										2
#define STAT_LAP_DOT_WIDTH								7
#define STAT_LAP_DOT_HEIGHT								7

/*
INT32 statPositions[ NUM_STATS * 4 ] = {
 STAT_RANK_SX,					STAT_RANK_SY,					STAT_RANK_WIDTH,					STAT_RANK_HEIGHT,
 STAT_TIME_DIGIT_SX[0],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,		STAT_TIME_DIGIT_HEIGHT,
 STAT_TIME_DEC_SX,			STAT_TIME_DEC_SY,			STAT_TIME_DEC_WIDTH,			STAT_TIME_DEC_HEIGHT,
 STAT_TIME_DIGIT_SX[1],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,		STAT_TIME_DIGIT_HEIGHT,
 STAT_TIME_DIGIT_SX[2],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,		STAT_TIME_DIGIT_HEIGHT,
 STAT_TIME_COLON_SX,		STAT_TIME_COLON_SY,		STAT_TIME_COLON_WIDTH,		STAT_TIME_COLON_HEIGHT,
 STAT_TIME_DIGIT_SX[3],	STAT_TIME_DIGIT_SY,		STAT_TIME_DIGIT_WIDTH,		STAT_TIME_DIGIT_HEIGHT,
 STAT_RANKBAR_SX,				STAT_RANKBAR_SY,			STAT_RANKBAR_WIDTH,				STAT_RANKBAR_HEIGHT,
 STAT_TIMEBAR_SX,				STAT_TIMEBAR_SY,			STAT_TIMEBAR_WIDTH,				STAT_TIMEBAR_HEIGHT,
 STAT_TITLE_SX,					STAT_TITLE_SY,        STAT_TITLE_WIDTH,					STAT_TITLE_HEIGHT,
 0,											0,										0,												0 // for countdown
};
*/

STATTLVERTEX statVertices[ MAX_STATS * 6 ];


#define MAX_STAT_NAVCOORDS								1500

#define STAT_NAVCOORD_SOURCEWIDTH					9
#define STAT_NAVCOORD_SOURCEHEIGHT				9
#define STAT_NAVCOORD_WIDTH								5
#define STAT_NAVCOORD_HEIGHT							5

#define STAT_RACERCOORD_SOURCEWIDTH				9
#define STAT_RACERCOORD_SOURCEHEIGHT			9
#define STAT_RACERCOORD_WIDTH							7
#define STAT_RACERCOORD_HEIGHT						7

#define STAT_CAMRACERCOORD_SOURCEWIDTH		13		
#define STAT_CAMRACERCOORD_SOURCEHEIGHT		13
#define STAT_CAMRACERCOORD_WIDTH					13
#define STAT_CAMRACERCOORD_HEIGHT					13

#define NAVMAP_SCALE											0.0007f // 0.0015f

#define NAVMAP_SHORTRANGE									105
#define NAVMAP_LONGRANGE									420
#define NAVMAP_CLOSEBEHINDCOLOR						D3DXCOLOR(1.0, 0.0, 0.0, 1.0)
#define NAVMAP_FARBEHINDCOLOR							D3DXCOLOR(1.0, 0.0, 0.0, 0.0)
#define NAVMAP_CLOSEAHEADCOLOR						D3DXCOLOR(1.0, 0.8, 0.0, 1.0)
#define NAVMAP_FARAHEADCOLOR							D3DXCOLOR(1.0, 0.8, 0.0, 0.0)


FLOAT	navMapXScale, navMapYScale;
FLOAT navMapWidth, navMapHeight;
FLOAT statNavCoordWidth, statNavCoordHeight;
FLOAT statRacerCoordWidth, statRacerCoordHeight;
FLOAT statCamRacerCoordWidth, statCamRacerCoordHeight;
FLOAT navMapSx, navMapSy;

STATCOLORTLVERTEX* statNavCoordVertices = NULL; //[ MAX_STAT_NAVCOORDS * 6 ];
STATCOLORTLVERTEX statRacerCoordVertices[ MAX_RACERS * 6 ];
BYTE* statNavCoordsAlreadyDisplayed = NULL;

COLORTLVERTEX alphaRectVertices[ 4 ];


//FLOAT oldYScale;

HRESULT CMyApplication::InitStats(){

	INT32 sx, sy, width, height;
	DWORD stat, coord, vert;


	SAFE_ARRAYDELETE( statNavCoordVertices );

	if( FAILED( statNavCoordVertices = new STATCOLORTLVERTEX[ MAX_STAT_NAVCOORDS * 6 ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}

	SAFE_ARRAYDELETE( statNavCoordsAlreadyDisplayed );

	if( FAILED( statNavCoordsAlreadyDisplayed = new BYTE[ track.numNavCoords ] ) ){
		MyAlert( NULL, "Out of memory" );
		return E_FAIL;
	}

	navMapXScale = (FLOAT)m_vp.dwWidth * NAVMAP_SCALE;
	navMapYScale = (FLOAT)m_vp.dwHeight * NAVMAP_SCALE * 1.333f; // 4/3 aspect ratio

	navMapWidth = 0.25f * m_vp.dwWidth;
	navMapHeight = ( 0.333f ) * m_vp.dwHeight;

	statNavCoordWidth = STAT_NAVCOORD_WIDTH * m_vp.dwWidth / 640;
	statNavCoordHeight = STAT_NAVCOORD_HEIGHT * m_vp.dwHeight / 480;

	statRacerCoordWidth = STAT_RACERCOORD_WIDTH * m_vp.dwWidth / 640;
	statRacerCoordHeight = STAT_RACERCOORD_WIDTH * m_vp.dwHeight / 480;

	statCamRacerCoordWidth = STAT_CAMRACERCOORD_WIDTH * m_vp.dwWidth / 640;
	statCamRacerCoordHeight = STAT_CAMRACERCOORD_WIDTH * m_vp.dwHeight / 480;

	navMapSx = m_vp.dwWidth - ( navMapWidth * 1.1 );
	navMapSy = m_vp.dwHeight - ( navMapHeight * 1.1 );

	STATCOLORTLVERTEX emptyStatColorVertex = { -1.0f, -1.0f, 0.0f, 1.0f, 0, -1.0f, -1.0f };

	for(coord=0; coord < MAX_STAT_NAVCOORDS; coord++){

		statNavCoordVertices[coord*6] = emptyStatColorVertex;
		statNavCoordVertices[coord*6].tu = 26.0f / 255.0f;
		statNavCoordVertices[coord*6].tv = 118.0f / 255.0f;

		statNavCoordVertices[coord*6+1] = emptyStatColorVertex;
		statNavCoordVertices[coord*6+1].tu = ( 26.0f + STAT_NAVCOORD_SOURCEWIDTH ) / 255.0f;
		statNavCoordVertices[coord*6+1].tv = ( 118.0f + STAT_NAVCOORD_SOURCEHEIGHT ) / 255.0f;

		statNavCoordVertices[coord*6+2] = emptyStatColorVertex;
		statNavCoordVertices[coord*6+2].tu = ( 26.0f + STAT_NAVCOORD_SOURCEWIDTH ) / 255.0f;
		statNavCoordVertices[coord*6+2].tv = 118.0f / 255.0f;

		statNavCoordVertices[coord*6+3] = emptyStatColorVertex;
		statNavCoordVertices[coord*6+3].tu = 26.0f / 255.0f;
		statNavCoordVertices[coord*6+3].tv = 118.0f / 255.0f;

		statNavCoordVertices[coord*6+4] = emptyStatColorVertex;
		statNavCoordVertices[coord*6+4].tu = 26.0f / 255.0f;
		statNavCoordVertices[coord*6+4].tv = ( 118.0f + STAT_NAVCOORD_SOURCEHEIGHT ) / 255.0f;

		statNavCoordVertices[coord*6+5] = emptyStatColorVertex;
		statNavCoordVertices[coord*6+5].tu = ( 26.0f + STAT_NAVCOORD_SOURCEWIDTH ) / 255.0f;
		statNavCoordVertices[coord*6+5].tv = ( 118.0f + STAT_NAVCOORD_SOURCEHEIGHT ) / 255.0f;
	}

	for(vert=0; vert < MAX_RACERS * 6; vert++){
		statRacerCoordVertices[vert] = emptyStatColorVertex;
	}

	COLORTLVERTEX emptyColorVertex = { -1.0f, -1.0f, 0.0f, 1.0f, 0 };

	alphaRectVertices[0] = emptyColorVertex;
	alphaRectVertices[0].sx = 0.0f;
	alphaRectVertices[0].sy = 0.0f;

	alphaRectVertices[1] = emptyColorVertex;
	alphaRectVertices[1].sx = 0.0f;
	alphaRectVertices[1].sy = m_vp.dwHeight;

	alphaRectVertices[2] = emptyColorVertex;
	alphaRectVertices[2].sx = m_vp.dwWidth;
	alphaRectVertices[2].sy = 0.0;//m_vp.dwHeight;

	alphaRectVertices[3] = emptyColorVertex;
	alphaRectVertices[3].sx = m_vp.dwWidth;
	alphaRectVertices[3].sy = m_vp.dwHeight;//0.0f;

	return S_OK;
}




VOID AddStat( STATTLVERTEX* statVertices, FLOAT sx, FLOAT sy, 
						 FLOAT tx, FLOAT ty, FLOAT width, FLOAT height, D3DVIEWPORT7 vp ){
	
	for(DWORD i=0; i < 6; i++){
		statVertices[i].sz = 0.0f;
		statVertices[i].rhw = 1.0f;
	}

	if( sx < 0 ){
		sx = vp.dwWidth + sx;
	}
	
	if( sy < 0 ){
		sy = vp.dwHeight + sy;
	}

	statVertices[0].sx = sx;
	statVertices[0].sy = sy;
	statVertices[0].tu = tx / 255;
	statVertices[0].tv = ty / 255;

	statVertices[1].sx = sx + width;
	statVertices[1].sy = sy + height;
	statVertices[1].tu = ( tx + width ) / 255;
	statVertices[1].tv = ( ty + height ) / 255;
	
	statVertices[2].sx = sx + width;
	statVertices[2].sy = sy;
	statVertices[2].tu = ( tx + width ) / 255;
	statVertices[2].tv = ty / 255;
		
	statVertices[3].sx = sx;
	statVertices[3].sy = sy;
	statVertices[3].tu = tx / 255;
	statVertices[3].tv = ty / 255;
		
	statVertices[4].sx = sx;
	statVertices[4].sy = sy + height;
	statVertices[4].tu = tx / 255;
	statVertices[4].tv = ( ty + height ) / 255;

	statVertices[5].sx = sx + width;
	statVertices[5].sy = sy + height;
	statVertices[5].tu = ( tx + width ) / 255;
	statVertices[5].tv = ( ty + height ) / 255;
}




// no position interpretation
VOID AddStatExact( STATTLVERTEX* statVertices, FLOAT sx, FLOAT sy, 
						 FLOAT tx, FLOAT ty, FLOAT width, FLOAT height, D3DVIEWPORT7 vp ){
	
	for(DWORD i=0; i < 6; i++){
		statVertices[i].sz = 0.0f;
		statVertices[i].rhw = 1.0f;
	}

	statVertices[0].sx = sx;
	statVertices[0].sy = sy;
	statVertices[0].tu = tx / 255;
	statVertices[0].tv = ty / 255;

	statVertices[1].sx = sx + width;
	statVertices[1].sy = sy + height;
	statVertices[1].tu = ( tx + width ) / 255;
	statVertices[1].tv = ( ty + height ) / 255;
	
	statVertices[2].sx = sx + width;
	statVertices[2].sy = sy;
	statVertices[2].tu = ( tx + width ) / 255;
	statVertices[2].tv = ty / 255;
		
	statVertices[3].sx = sx;
	statVertices[3].sy = sy;
	statVertices[3].tu = tx / 255;
	statVertices[3].tv = ty / 255;
		
	statVertices[4].sx = sx;
	statVertices[4].sy = sy + height;
	statVertices[4].tu = tx / 255;
	statVertices[4].tv = ( ty + height ) / 255;

	statVertices[5].sx = sx + width;
	statVertices[5].sy = sy + height;
	statVertices[5].tu = ( tx + width ) / 255;
	statVertices[5].tv = ( ty + height ) / 255;
}




HRESULT CMyApplication::DisplayStats(){
	
	HRESULT hr;
	FLOAT topLeftTu, topLeftTv, width, height;
	DWORD i, stat;
	CRacer* pRacer = &racers[cam.racerNum];
	DWORD numStatsToDisplay = 0;
	INT32 displayPosition;
	static bFirstTransition = TRUE;
	
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, FALSE );
	
	if( cam.mode == CAMERAMODE_NORMAL || gameState == GAMESTATE_POSTRACE ){
		
		if( gameState != GAMESTATE_POSTRACE ){
			displayPosition = pRacer->racePosition;
		}
		else{
			displayPosition = finishRacePosition;
		}

		if( displayPosition + 1 <= 4 ){ 
			AddStat( &statVertices[numStatsToDisplay * 6], STAT_RANK_SX, STAT_RANK_SY,
				( displayPosition + 1 ) * 51.0f, 0.0f, STAT_RANK_WIDTH, STAT_RANK_HEIGHT, m_vp );
		}
		else{
			AddStat( &statVertices[numStatsToDisplay * 6], STAT_RANK_SX, STAT_RANK_SY,
				( displayPosition - 4 ) * 51.0f, 48.0f, STAT_RANK_WIDTH, STAT_RANK_HEIGHT, m_vp );
		}
		numStatsToDisplay++;
		
		// cap to 9:59.9
		INT32 timeTenths;
		if( gameState == GAMESTATE_POSTRACE ){
			timeTenths = finishTime * 10;
		}
		else{	
			timeTenths = elapsedTime * 10;
		}
		if( timeTenths > 5999 ){
			timeTenths = 5999;
		}
		else if( timeTenths < 0 ){
			timeTenths = 0;
		}
		DWORD base;
		for(i=0; i < 4; i++){
			
			if( i == 2 ){
				base = 6;
			}
			else{
				base = 10;
			}

			AddStat( &statVertices[numStatsToDisplay * 6], STAT_TIME_DIGIT_SX[i], STAT_RANK_SY,
				(FLOAT)( timeTenths % base ) * 25.0f, 96.0f, STAT_TIME_DIGIT_WIDTH, STAT_TIME_DIGIT_HEIGHT, m_vp );
						
			timeTenths /= base;
			numStatsToDisplay++;
		}

		AddStat( &statVertices[numStatsToDisplay * 6], STAT_TIME_DEC_SX, STAT_TIME_DEC_SY,
			0.0f, 125.0f, STAT_TIME_DEC_WIDTH, STAT_TIME_DEC_HEIGHT, m_vp );
		numStatsToDisplay++;

		AddStat( &statVertices[numStatsToDisplay * 6], STAT_TIME_COLON_SX, STAT_TIME_COLON_SY,
			12.0f, 118.0f, STAT_TIME_COLON_WIDTH, STAT_TIME_COLON_HEIGHT, m_vp );
		numStatsToDisplay++;
		
		AddStat( &statVertices[numStatsToDisplay * 6], STAT_RANKBAR_SX, STAT_RANKBAR_SY,
			170.0f, 118.0f, STAT_RANKBAR_WIDTH, STAT_RANKBAR_HEIGHT, m_vp );
		numStatsToDisplay++;
		
		AddStat( &statVertices[numStatsToDisplay * 6], STAT_TIMEBAR_SX, STAT_TIMEBAR_SY,
			48.0f, 118.0f, STAT_TIMEBAR_WIDTH, STAT_TIMEBAR_HEIGHT, m_vp );
		numStatsToDisplay++;

		for(INT32 lap=0; lap < 3; lap++){
			AddStat( &statVertices[numStatsToDisplay * 6], STAT_LAP_DOT_SX[lap], STAT_LAP_DOT_SY,
				( pRacer->currLap < lap ? 233.0f : 240.0f ), 118.0f, STAT_LAP_DOT_WIDTH, STAT_LAP_DOT_HEIGHT, m_vp );
			numStatsToDisplay++;
		}
	}

	// render title *before* fade if not first transition
	if( gameState == GAMESTATE_DEMORACE && !bFirstTransition ){
		AddStat( &statVertices[numStatsToDisplay * 6], STAT_TITLE_SX, STAT_TITLE_SY,
			0.0f, 134.0f, STAT_TITLE_WIDTH, STAT_TITLE_HEIGHT, m_vp );
		numStatsToDisplay++;
	}
	
	if( gameState == GAMESTATE_PLAYERRACE ){

		FLOAT raceIntroTime = -elapsedTime; // + RACE_INTRO_DURATION;

		if( raceIntroTime >= 1.5f ){}
		else if( raceIntroTime >= 0.0f ){
			// "3","2","1"
			FLOAT sx = m_vp.dwWidth * 0.5f - STAT_RANK_WIDTH * 0.5f; // using "rank" digits
			FLOAT sy = m_vp.dwHeight * 0.5 - STAT_RANK_HEIGHT * 0.5f;
			DWORD num = raceIntroTime * 2 + 1;

			AddStat( &statVertices[numStatsToDisplay * 6], sx, sy,
				num * 51.0f, 0.0f, STAT_RANK_WIDTH, STAT_RANK_HEIGHT, m_vp );
			numStatsToDisplay++;
		}
		else if( raceIntroTime >= -0.5f ){
			// "race!"
			FLOAT sx = m_vp.dwWidth * 0.5f - STAT_RACE_WIDTH * 0.5f;
			FLOAT sy = m_vp.dwHeight * 0.5 - STAT_RACE_HEIGHT * 0.5f - 
				( pow( ( -raceIntroTime ), 4 ) * 16.0f ) * ( m_vp.dwHeight * 0.5 + STAT_RACE_HEIGHT );

			AddStatExact( &statVertices[numStatsToDisplay * 6], sx, sy,
				0.0f, 193.0f, STAT_RACE_WIDTH, STAT_RACE_HEIGHT, m_vp );
			numStatsToDisplay++;
		}
	}

	if( FAILED( hr = m_pd3dDevice->SetTexture( 0, (GetTexture( STATSTEXTURE_FILENAME ))->m_pddsSurface ) ) ){
		MyAlert(&hr, "Unable to set texture for displaying race info" );
		return E_FAIL;
	}
		
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	
	if( numStatsToDisplay ){
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ); 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_POINT ); 
		
		if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, STATTLVERTEXDESC, 
			statVertices, numStatsToDisplay * 6, NULL ) ) ){
			MyAlert( &hr, "Unable to display race info" );
			return hr;
		}
		
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ); 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_LINEAR ); 
	}

	if( cam.mode == CAMERAMODE_NORMAL ){
		if( FAILED( hr = DisplayNavMap() ) ){
			return hr;
		}
	}
		
	// fade to/from black if necessary
	if( transitionState != TRANSITIONSTATE_NORMAL ){
			
		FLOAT alpha;
		if( transitionState == TRANSITIONSTATE_FADEIN ){
			alpha = min( 1.0f, transitionTime / FADEIN_DURATION );
		}
		else if( transitionState == TRANSITIONSTATE_FADEOUT ){
			alpha = 1.0f - transitionTime / FADEOUT_DURATION;
		}
		
		D3DCOLOR c = D3DRGBA( 0.0f, 0.0f, 0.0f, alpha );
		for(DWORD vert=0; vert < 4; vert++){
			alphaRectVertices[vert].color = c;
		}
		
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
			
		if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, COLORTLVERTEXDESC, 
			alphaRectVertices, 4, NULL ) ) ){
			MyAlert( &hr, "Unable to render screen transition" );
			return hr;
		}
		
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	}

	if( gameState == GAMESTATE_DEMORACE && bFirstTransition ){ // display title *over* transition fade
		numStatsToDisplay = 0;

		AddStat( &statVertices[numStatsToDisplay * 6], STAT_TITLE_SX, STAT_TITLE_SY,
			0.0f, 134.0f, STAT_TITLE_WIDTH, STAT_TITLE_HEIGHT, m_vp );
		numStatsToDisplay++;
	
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ); 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_POINT ); 
		
		if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, STATTLVERTEXDESC, 
			statVertices, numStatsToDisplay * 6, NULL ) ) ){
			MyAlert( &hr, "Unable to display title graphic" );
			return hr;
		}
		
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR ); 
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_LINEAR ); 
	}

	if( transitionState != TRANSITIONSTATE_FADEIN ){
		bFirstTransition = FALSE;
	}
	
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, TRUE );
	
	return S_OK;
}





STATCOLORTLVERTEX* recur_pVertex;
FLOAT	recur_cosYaw, recur_sinYaw;
WORD recur_racerDistToStartCoord;
FLOAT recur_centerx, recur_centerz;

VOID CMyApplication::DisplayStatNavCoord( DWORD curr ){
	
	FLOAT cx, cz, mapx, mapy, sx, sy;
	FLOAT relDist;
	D3DXCOLOR lerpColor;
	FLOAT intensity;

	if( statNavCoordsAlreadyDisplayed[curr] ){
		return;
	}
	
	statNavCoordsAlreadyDisplayed[curr] = 1;
	
	NAVCOORD* pCoord = &track.navCoords[curr];
	
	cx = (FLOAT)pCoord->x - recur_centerx;
	cz = -( (FLOAT)pCoord->z - recur_centerz );
		
	mapx = ( cx * recur_cosYaw + cz * recur_sinYaw ) * navMapXScale + navMapWidth/2;
	mapy = ( cz * recur_cosYaw - cx * recur_sinYaw ) * navMapYScale + navMapHeight/2;
		
	if( mapx >= 0.0f && mapy >= 0.0f && mapx <= navMapWidth && mapy <= navMapHeight ){
		
		sx = navMapSx + mapx - statNavCoordWidth/2;
		sy = navMapSy + mapy - statNavCoordHeight/2;
		
		recur_pVertex[0].sx = sx;
		recur_pVertex[0].sy = sy;
		recur_pVertex[1].sx = sx + statNavCoordWidth;
		recur_pVertex[1].sy = sy + statNavCoordHeight;
		recur_pVertex[2].sx = sx + statNavCoordWidth;
		recur_pVertex[2].sy = sy;
		
		recur_pVertex[3].sx = sx;
		recur_pVertex[3].sy = sy;
		recur_pVertex[4].sx = sx;
		recur_pVertex[4].sy = sy + statNavCoordHeight;
		recur_pVertex[5].sx = sx + statNavCoordWidth;
		recur_pVertex[5].sy = sy + statNavCoordHeight;
		
		relDist = pCoord->distToStartCoord - recur_racerDistToStartCoord;
		
		if( relDist < -track.trackLength/2 ){
			relDist += track.trackLength;
		}
		else if( relDist > track.trackLength/2 ){
			relDist -= track.trackLength;
		}
		
		if( relDist < -NAVMAP_SHORTRANGE ){
			intensity = ( -relDist - NAVMAP_SHORTRANGE ) / ( NAVMAP_LONGRANGE - NAVMAP_SHORTRANGE );
			D3DXColorLerp( &lerpColor, &NAVMAP_CLOSEAHEADCOLOR, &NAVMAP_FARAHEADCOLOR, intensity );
		}	
		else if( relDist > NAVMAP_SHORTRANGE ){				
			intensity = ( relDist - NAVMAP_SHORTRANGE ) / ( NAVMAP_LONGRANGE - NAVMAP_SHORTRANGE );
			D3DXColorLerp( &lerpColor, &NAVMAP_CLOSEBEHINDCOLOR, &NAVMAP_FARBEHINDCOLOR, intensity );
		}	
		else{
			intensity = ( relDist + NAVMAP_SHORTRANGE ) / ( 2 * NAVMAP_SHORTRANGE );
			D3DXColorLerp( &lerpColor, &NAVMAP_CLOSEAHEADCOLOR, &NAVMAP_CLOSEBEHINDCOLOR, intensity );
		}
		
		recur_pVertex[0].color = recur_pVertex[1].color = recur_pVertex[2].color =
			recur_pVertex[3].color = recur_pVertex[4].color = recur_pVertex[5].color = (DWORD)lerpColor;
		
		recur_pVertex += 6;
	}

	if( pCoord->next != NAVCOORD_NOCOORD ){
		DisplayStatNavCoord( pCoord->next );
	}

	if( pCoord->alt != NAVCOORD_NOCOORD ){
		DisplayStatNavCoord( pCoord->alt );
	}
}





HRESULT CMyApplication::DisplayNavMap(){

	HRESULT hr;
	DWORD coord, racer;
	FLOAT cx, cz, mapx, mapy, sx, sy;
	FLOAT camYaw;

	recur_pVertex = statNavCoordVertices;

	recur_racerDistToStartCoord = racers[cam.racerNum].distToStartCoord;

	recur_centerx = racers[cam.racerNum].pos.x;
	recur_centerz = racers[cam.racerNum].pos.z;
	
	if( fabs( cam.dir.x ) < g_EPSILON && fabs( cam.dir.z ) < g_EPSILON ){
		camYaw = racers[cam.racerNum].yaw;
	}
	else{
		camYaw = atan2( cam.dir.z, cam.dir.x );
	}

	recur_cosYaw = cosf( -camYaw + g_PI / 2 );
	recur_sinYaw = sinf( -camYaw + g_PI / 2 );

	ZeroMemory( statNavCoordsAlreadyDisplayed, track.numNavCoords * sizeof(BYTE) );

    if( !bAutopilot )
    {
        DisplayStatNavCoord( 0 );
    }
    else
    {
        DisplayStatNavCoord( 0 );

        NAVCOORD* pCoord = &track.navCoords[0];
        for( DWORD i=0; i < track.numNavCoords; i++, pCoord++ )
        {
            if( !statNavCoordsAlreadyDisplayed[i] )
            {
	            cx = (FLOAT)pCoord->x - recur_centerx;
	            cz = -( (FLOAT)pCoord->z - recur_centerz );
		            
	            mapx = ( cx * recur_cosYaw + cz * recur_sinYaw ) * navMapXScale + navMapWidth/2;
	            mapy = ( cz * recur_cosYaw - cx * recur_sinYaw ) * navMapYScale + navMapHeight/2;
		            
	            if( mapx >= 0.0f && mapy >= 0.0f && mapx <= navMapWidth && mapy <= navMapHeight ){
		            
		            sx = navMapSx + mapx - statNavCoordWidth/2;
		            sy = navMapSy + mapy - statNavCoordHeight/2;
		            
		            recur_pVertex[0].sx = sx;
		            recur_pVertex[0].sy = sy;
		            recur_pVertex[1].sx = sx + statNavCoordWidth;
		            recur_pVertex[1].sy = sy + statNavCoordHeight;
		            recur_pVertex[2].sx = sx + statNavCoordWidth;
		            recur_pVertex[2].sy = sy;
		            
		            recur_pVertex[3].sx = sx;
		            recur_pVertex[3].sy = sy;
		            recur_pVertex[4].sx = sx;
		            recur_pVertex[4].sy = sy + statNavCoordHeight;
		            recur_pVertex[5].sx = sx + statNavCoordWidth;
		            recur_pVertex[5].sy = sy + statNavCoordHeight;
		        		            
		            recur_pVertex[0].color = recur_pVertex[1].color = recur_pVertex[2].color =
			            recur_pVertex[3].color = recur_pVertex[4].color = recur_pVertex[5].color = (DWORD)0xFF00FF00;
		            
		            recur_pVertex += 6;
                }
            }
	    }
    }
/*
	for(coord=0; coord < track.numNavCoords; coord++){
		//if( track.navCoords[coord].next == coord ){
			DisplayStatNavCoord( coord );
		//}
	}
*/

    if( recur_pVertex - statNavCoordVertices )
    {
	    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );

	    //m_misc = ( recur_pVertex - statNavCoordVertices );

	    if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, STATCOLORTLVERTEXDESC, 
	    		statNavCoordVertices, ( recur_pVertex - statNavCoordVertices ), NULL ) ) ){
	    	MyAlert( &hr, "Unable to render track map" );
	    	return hr;
	    }
    }

	recur_pVertex = statRacerCoordVertices;

	for(racer=0; racer < numRacers; racer++){

		cx = racers[racer].pos.x - recur_centerx;
		cz = -( racers[racer].pos.z - recur_centerz );

		mapx = ( cx * recur_cosYaw + cz * recur_sinYaw ) * navMapXScale + navMapWidth/2;
		mapy = ( cz * recur_cosYaw - cx * recur_sinYaw ) * navMapYScale + navMapHeight/2;

		if( mapx >= 0.0f && mapy >= 0.0f && mapx <= navMapWidth && mapy <= navMapHeight ){

			sx = navMapSx + mapx;
			sy = navMapSy + mapy;

			if( racer == cam.racerNum ){
				recur_pVertex[0].tu = 35.0f / 255.0f;
				recur_pVertex[0].tv = 118.0f / 255.0f;
				
				recur_pVertex[1].tu = ( 35.0f + STAT_CAMRACERCOORD_SOURCEWIDTH ) / 255.0f;
				recur_pVertex[1].tv = ( 118.0f + STAT_CAMRACERCOORD_SOURCEHEIGHT ) / 255.0f;
				
				recur_pVertex[2].tu = ( 35.0f + STAT_CAMRACERCOORD_SOURCEWIDTH ) / 255.0f;
				recur_pVertex[2].tv = 118.0f / 255.0f;
				
				recur_pVertex[3].tu = 35.0f / 255.0f;
				recur_pVertex[3].tv = 118.0f / 255.0f;
				
				recur_pVertex[4].tu = 35.0f / 255.0f;
				recur_pVertex[4].tv = ( 118.0f + STAT_CAMRACERCOORD_SOURCEHEIGHT ) / 255.0f;
				
				recur_pVertex[5].tu = ( 35.0f + STAT_CAMRACERCOORD_SOURCEWIDTH ) / 255.0f;
				recur_pVertex[5].tv = ( 118.0f + STAT_CAMRACERCOORD_SOURCEHEIGHT ) / 255.0f;
				
				sx -= statCamRacerCoordWidth/2;
				sy -= statCamRacerCoordHeight/2;

				recur_pVertex[0].sx = sx;
				recur_pVertex[0].sy = sy;
				recur_pVertex[1].sx = sx + statCamRacerCoordWidth;
				recur_pVertex[1].sy = sy + statCamRacerCoordHeight;
				recur_pVertex[2].sx = sx + statCamRacerCoordWidth;
				recur_pVertex[2].sy = sy;
				
				recur_pVertex[3].sx = sx;
				recur_pVertex[3].sy = sy;
				recur_pVertex[4].sx = sx;
				recur_pVertex[4].sy = sy + statCamRacerCoordHeight;
				recur_pVertex[5].sx = sx + statCamRacerCoordWidth;
				recur_pVertex[5].sy = sy + statCamRacerCoordHeight;
				
			}
			else{
				recur_pVertex[0].tu = 26.0f / 255.0f;
				recur_pVertex[0].tv = 118.0f / 255.0f;
				
				recur_pVertex[1].tu = ( 26.0f + STAT_RACERCOORD_SOURCEWIDTH ) / 255.0f;
				recur_pVertex[1].tv = ( 118.0f + STAT_RACERCOORD_SOURCEHEIGHT ) / 255.0f;
				
				recur_pVertex[2].tu = ( 26.0f + STAT_RACERCOORD_SOURCEWIDTH ) / 255.0f;
				recur_pVertex[2].tv = 118.0f / 255.0f;
				
				recur_pVertex[3].tu = 26.0f / 255.0f;
				recur_pVertex[3].tv = 118.0f / 255.0f;
				
				recur_pVertex[4].tu = 26.0f / 255.0f;
				recur_pVertex[4].tv = ( 118.0f + STAT_RACERCOORD_SOURCEHEIGHT ) / 255.0f;
				
				recur_pVertex[5].tu = ( 26.0f + STAT_RACERCOORD_SOURCEWIDTH ) / 255.0f;
				recur_pVertex[5].tv = ( 118.0f + STAT_RACERCOORD_SOURCEHEIGHT ) / 255.0f;
				
				sx -= statRacerCoordWidth/2;
				sy -= statRacerCoordHeight/2;

				recur_pVertex[0].sx = sx;
				recur_pVertex[0].sy = sy;
				recur_pVertex[1].sx = sx + statRacerCoordWidth;
				recur_pVertex[1].sy = sy + statRacerCoordHeight;
				recur_pVertex[2].sx = sx + statRacerCoordWidth;
				recur_pVertex[2].sy = sy;
				
				recur_pVertex[3].sx = sx;
				recur_pVertex[3].sy = sy;
				recur_pVertex[4].sx = sx;
				recur_pVertex[4].sy = sy + statRacerCoordHeight;
				recur_pVertex[5].sx = sx + statRacerCoordWidth;
				recur_pVertex[5].sy = sy + statRacerCoordHeight;
			}

			recur_pVertex[0].color = recur_pVertex[1].color = recur_pVertex[2].color =
				recur_pVertex[3].color = recur_pVertex[4].color = recur_pVertex[5].color = 0xFFFFFFFF;

			recur_pVertex += 6;
		}
	}

    if( recur_pVertex - statRacerCoordVertices )
    {
        if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, STATCOLORTLVERTEXDESC, 
		        statRacerCoordVertices, ( recur_pVertex - statRacerCoordVertices ), NULL ) ) ){
	        MyAlert( &hr, "Unable to render track map" );
	        return hr;
        }
    }

/*
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	
	if( FAILED( hr = m_pd3dDevice->DrawPrimitive( D3DPT_LINELIST, COLORTLVERTEXDESC, 
			statLineVertices, ( pLineVertex - statLineVertices ), NULL ) ) ){
		MyAlert( &hr, "DisplayNavMap() > DrawPrimitive(lines) failed" );
		return hr;
	}

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
*/
//	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );

	return S_OK;
}


