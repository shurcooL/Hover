//-----------------------------------------------------------------------------
// File: Dolphin.cpp
//
// Desc: Sample of swimming dolphin
//
//			 Note: This code uses the D3D Framework helper library.
//
// Copyright (c) 1998-1999 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#define D3D_OVERLOADS
#include <stdio.h>
#include "D3DApp.h"
#include "D3DUtil.h"
#include "D3DMath.h"
#include "D3DTextr.h"
#include "D3DFile.h"

#include "terrain.h"

#define WATER_COLOR 				 D3DRGBA(0.6f,0.2f,0.0f,0.0f)



//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Main class to run this application. Most functionality is inherited
//			 from the CD3DApplication base class.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{

	WORD			terrWidth;
	WORD			terrDepth;
	DWORD 		terrArea;

	TERRCOORD*		terrCoords;

	WORD* 		terrIndices;

	DWORD 		terrVertexTypeDesc; 
	TERRTLVERTEX* terrVertices;
//		D3DTLVERTEX*	terrVertices;
	FLOAT camPhase;

public:
		HRESULT OneTimeSceneInit();
		HRESULT InitDeviceObjects();
		HRESULT DeleteDeviceObjects();
		HRESULT Render();
		HRESULT RenderTerrain();
	HRESULT FrameMove( FLOAT );
		HRESULT FinalCleanup();

		CMyD3DApplication();
};




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//			 message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
		CMyD3DApplication d3dApp;

		if( FAILED( d3dApp.Create( hInst, strCmdLine ) ) )
				return 0;

		return d3dApp.Run();
}





//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
									:CD3DApplication()
{
		// Override base class members
		m_strWindowTitle	= TEXT("Dolphin: Blending Meshes in Real Time");
		m_bAppUseZBuffer	= TRUE;
		m_bAppUseStereo 	= TRUE;
		m_bShowStats			= TRUE;
		m_fnConfirmDevice = NULL;

}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//			 permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
	int i,x,z;
	HRESULT hr;

	terrWidth = 6;
	terrDepth = 6;
	terrArea = terrWidth * terrDepth;
	terrCoords = new TERRCOORD[terrArea];
	terrIndices = new WORD[terrArea * 6];
	
	//	terrVertexTypeDesc = ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX0 ); 
	terrVertexTypeDesc = ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE ); 
	terrVertices = new TERRTLVERTEX[terrArea];
	 
	srand(timeGetTime());

	for( i=0; i < 8; i++ ){
		triIndexOffsets[i] = triIndexZOffsets[i] * terrWidth + triIndexXOffsets[i];
	}
	
	// initialize random terrain and lighting
	for( z=0; z<terrDepth; z++ ){
		for( x=0; x<terrWidth; x++ ){
			terrCoords[ z * terrWidth + x ].height = (rand()/(FLOAT)RAND_MAX)*5;//(sin(x) + cos(z))*1.5 + 3;
			terrCoords[ z * terrWidth + x ].lightIndex	= (float)(0.25 + 0.75*((float)terrCoords[ z * terrWidth + x ].height / 5.0f)) * 255;
			terrCoords[ z * terrWidth + x ].twoTriCodes = 0;
//			if( x > 0 && z < terrDepth - 1 ){
//				terrCoords[ z * terrWidth + x ].twoTriCodes |= TRI_LEFT_DEFAULT;
//			}
//			if( z > 0 && x < terrWidth - 1 ){
//				terrCoords[ z * terrWidth + x ].twoTriCodes |= TRI_RIGHT_DEFAULT;
//			}
		}
	} 

	terrCoords[ 0 * terrWidth + 1 ].twoTriCodes = 0x80;
	terrCoords[ 0 * terrWidth + 2 ].twoTriCodes = 0xC0;
	terrCoords[ 0 * terrWidth + 3 ].twoTriCodes = 0x80;
	terrCoords[ 0 * terrWidth + 4 ].twoTriCodes = 0xC0;
	terrCoords[ 0 * terrWidth + 5 ].twoTriCodes = 0x80;

	terrCoords[ 1 * terrWidth + 1 ].twoTriCodes = 0x15;
	terrCoords[ 1 * terrWidth + 3 ].twoTriCodes = 0xD5;
	terrCoords[ 1 * terrWidth + 5 ].twoTriCodes = 0x90;

	terrCoords[ 2 * terrWidth + 0 ].twoTriCodes = 0x01;

	terrCoords[ 3 * terrWidth + 1 ].twoTriCodes = 0x12;
	terrCoords[ 4 * terrWidth + 0 ].twoTriCodes = 0x01;

	terrCoords[ 5 * terrWidth + 0 ].twoTriCodes = 0x08;
	terrCoords[ 5 * terrWidth + 1 ].twoTriCodes = 0x09;
	terrCoords[ 5 * terrWidth + 3 ].twoTriCodes = 0x0D;

//	terrCoords[ 4 * terrWidth + 8 ].twoTriCodes = 0xA0 | TRI_RIGHT_DEFAULT;
//	terrCoords[ 8 * terrWidth + 4 ].twoTriCodes = TRI_LEFT_DEFAULT | 0x0A;
	

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//			 the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove( FLOAT fTimeKey )
{
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//			 rendering. This function sets up render states, clears the
//			 viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
		// Clear the viewport
		m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
												 0xFFFFFFFF, 1.0f, 0L );

		// Begin the scene 
		if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
		{
				//m_pFloorObject->Render( m_pd3dDevice );

				//m_pDolphinObject->Render( m_pd3dDevice );
		RenderTerrain();

		// End the scene.
				m_pd3dDevice->EndScene();
		}

		return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
		// Set default render states
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DITHERENABLE,	 FALSE );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SPECULARENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 TRUE );

/*
		// Turn on fog
		FLOAT fFogStart =  1.0f;
		FLOAT fFogEnd 	= 50.0f;
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	 TRUE );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGCOLOR,		 WATER_COLOR );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGVERTEXMODE,  D3DFOG_LINEAR );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGSTART, *((DWORD *)(&fFogStart)) );
		m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGEND,	 *((DWORD *)(&fFogEnd)) );
*/
		return S_OK;
}


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
	// float lightTable[lightIndex] == lightIndex / 255
	FLOAT lightIntensity = (float)lightIndex / 255.0f;
	tVertex.dcColor = D3DRGBA( 0.8f * lightIntensity, 0.3f * lightIntensity, 0.0f, 1.0f );
	return tVertex;
}



HRESULT CMyD3DApplication::RenderTerrain(){

	DWORD numIndices=0;
	D3DMATRIX 	matView, matProj, matSet;
	FLOAT 		matVertex[4],matTVertex[4];
	camPhase = timeGetTime() / 4000.0f;

	TERRTLVERTEX *pTerrVertex;
	TERRCOORD *pTerrCoord;
	WORD *pTerrIndex;
	WORD iz,ix,wx,wy,wz;
	DWORD terrOffset;
	INT32 offset;
	BYTE twoTriCodes, triCode, triSize;
	INT32 *pTriIndexOffsets;
	///// viewpoint ////
	
	D3DVECTOR vEyePt		= D3DVECTOR( (cos(camPhase)*(float)terrWidth + (float)terrWidth/2) * TERR_SCALE, 40.0f + sin(camPhase/3)*(float)15, (sin(camPhase)*(float)terrDepth + (float)terrDepth/2) * TERR_SCALE );
	D3DVECTOR vLookatPt = D3DVECTOR( terrWidth/2 * TERR_SCALE, 0.0f, terrDepth/2 * TERR_SCALE );
	D3DVECTOR vUpVec		= D3DVECTOR( 0.0f, 1.0f,	 0.0f );

	D3DUtil_SetViewMatrix(
		matView,
		vEyePt,
		vLookatPt,
		vUpVec );

	D3DVIEWPORT7 vp;
	m_pd3dDevice->GetViewport(&vp);
	FLOAT fAspect = ((FLOAT)vp.dwHeight) / vp.dwWidth;

	DWORD dwClipWidth  = vp.dwWidth/2;
	DWORD dwClipHeight = vp.dwHeight/2;
	
	D3DUtil_SetProjectionMatrix( matProj, g_PI/3, fAspect, 1.0f, 1000.0f );
	
	D3DMath_MatrixMultiply( matSet, matView, matProj );
	
///////////////////
	
	WORD visibleStartX = 0;
	WORD visibleStartZ = 0;
	WORD visibleEndX = terrWidth - 1;
	WORD visibleEndZ = terrDepth - 1;

	WORD borderStartX = visibleStartX - 8;
	WORD borderStartZ = visibleStartZ - 8;
	WORD borderEndX = visibleEndX + 8;
	WORD borderEndZ = visibleEndZ + 8;

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
	WORD borderRight = visibleEndX - borderEndX;
	WORD borderBottom = visibleEndZ - borderEndZ;

	WORD visibleWidth = visibleEndX - visibleStartX + 1;
	WORD visibleDepth = visibleEndZ - visibleStartZ + 1;
	// render border
	
	// top-left corner
/*
	terrOffset = ( borderStartZ ) * terrWidth + borderStartX;
	pTerrCoord = terrCoords + terrOffset;
	pTerrVertex = terrVertices + terrOffset;
	pTerrIndex = terrIndices;
	wx = borderStartX * TERR_SCALE;
	wz = borderStartZ * TERR_SCALE;
	for(iz = ( 8 - borderTop ); iz < 8; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - 8 ),
	pTerrVertex += ( terrWidth - 8 ), terrOffset += ( terrWidth - 8 ) ){
		for(ix = ( 8 - borderLeft ); ix < 8;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, terrOffset++ ){
			// only right triangle type 0 might be drawn here
			if( ( triRight = pTerrCoord->triCodes && TRI_RIGHT ) ){
				if( ( triType = ( triRight && TRI_RIGHT_TYPE ) >> 2 ) == TRI_TYPE0 ){ // only type 0 (any size)
					triSize = ( triRight && TRI_RIGHT_SIZE )
					triDimension = 1 << triSize; // height & width
					if( ix + triDimension >= 8 && iz + triDimension >= 8 ){

						*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
							pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

						// project right triangle type 0's top vertex, too
						alt1Wx = wx + triDimension;
						alt1Wz = wz - triDimension;
						pTriIndexOffsets = triIndexOffsets + ( triCode && TRI_RIGHT_TYPE >> 1 );
						pAlt1TerrCoord = pTerrCoord - *( pTriIndexOffsets ); // negative for right triangles
						pAlt1TerrVertex = pTerrVertex - *( pTriIndexOffsets );

						*pAlt1TerrVertex = ProjectCoord( alt1Wx, pAlt1TerrCoord->height, alt1Wz,
							pAlt1TerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );
				
						*pTerrIndex = terrOffset;
						*( pTerrIndex + 1 ) = terrOffset - *( pTriIndexOffsets );
						*( pTerrIndex + 2 ) = terrOffset - *( pTriIndexOffsets + 1 ) 
						pTerrIndex += 3;
					}
				}
			}
		}
	}
*/
	// top-right corner
	// bottom-left corner
	// bottom-right corner
	// top row
	// bottom row
	// left column
	// right column
	
	terrOffset = ( visibleStartZ ) * terrWidth + visibleStartX;
	pTerrCoord = terrCoords + terrOffset;
	pTerrVertex = terrVertices + terrOffset;
	pTerrIndex = terrIndices;
	wz = visibleStartZ * TERR_SCALE;

	// the two freak vertices: top-left and bottom-right
	*pTerrVertex = ProjectCoord( visibleStartX * TERR_SCALE, pTerrCoord->height, wz,
	pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

	terrVertices[ visibleEndZ * terrWidth + visibleEndX ] =
		ProjectCoord( visibleEndX * TERR_SCALE,
		terrCoords[ visibleEndZ * terrWidth + visibleEndX ].height,
		visibleEndZ * TERR_SCALE,
		terrCoords[ visibleEndZ * terrWidth + visibleEndX ].lightIndex,
		matSet, dwClipWidth, dwClipHeight );
	
	for(iz = 0; iz < visibleDepth; 
	iz++, wz += TERR_SCALE, pTerrCoord += ( terrWidth - visibleWidth ),
	pTerrVertex += ( terrWidth - visibleWidth ), terrOffset += ( terrWidth - visibleWidth ) ){
		for(ix = 0, wx = visibleStartX * TERR_SCALE; ix < visibleWidth;
		ix++, wx += TERR_SCALE, pTerrCoord++, pTerrVertex++, terrOffset++ ){
			if( ( twoTriCodes = pTerrCoord->twoTriCodes ) ){

				*pTerrVertex = ProjectCoord( wx, pTerrCoord->height, wz,
				pTerrCoord->lightIndex, matSet, dwClipWidth, dwClipHeight );

//				*pTerrVertex = ProjectCoord( ix * TERR_SCALE, 0, iz * TERR_SCALE,
//				255, matSet, dwClipWidth, dwClipHeight );

				if( ( triCode = ( twoTriCodes & TRI_LEFT ) >> 4 ) ){
					triSize = triCode & TRI_SIZE;
					pTriIndexOffsets = triIndexOffsets + ( ( triCode & TRI_TYPE ) >> 1 );
					*pTerrIndex = iz * terrWidth + ix;
					*( pTerrIndex + 1 ) = iz * terrWidth + ix + ( ( *( pTriIndexOffsets ) ) << triSize ); 
					*( pTerrIndex + 2 ) = iz * terrWidth + ix + ( ( *( pTriIndexOffsets + 1 ) ) << triSize ); 
					pTerrIndex += 3;
				}
				if( ( triCode = twoTriCodes & TRI_RIGHT ) ){ 
					triSize = triCode & TRI_SIZE;
					pTriIndexOffsets = triIndexOffsets + ( ( triCode & TRI_TYPE ) >> 1 );
					*pTerrIndex = iz * terrWidth + ix;
					*( pTerrIndex + 1 ) = iz * terrWidth + ix - ( ( *( pTriIndexOffsets ) ) << triSize );
					*( pTerrIndex + 2 ) = iz * terrWidth + ix - ( ( *( pTriIndexOffsets + 1 ) ) << triSize ); 
					pTerrIndex += 3;
				}
/*		
				if(iz < terrDepth - 1 && ix < terrWidth - 1){
					*pTerrIndex = iz * terrWidth + ix;
					*(pTerrIndex + 1) = ( iz + 1 ) * terrWidth + ix;
					*(pTerrIndex + 2) = ( iz + 1 ) * terrWidth + ( ix + 1 );
					*(pTerrIndex + 3) = iz * terrWidth + ix;
					*(pTerrIndex + 4) = ( iz + 1 ) * terrWidth + ( ix + 1 );
					*(pTerrIndex + 5) = iz * terrWidth + ( ix + 1 );
					pTerrIndex += 6;
				}
*/
			}
		}
	}

	
/*
	pTerrIndex = terrIndices;
	for(iz=0; iz < terrDepth; iz++){
		for(ix=0; ix < terrWidth; ix++){
//				matVertex[0] = x * TERR_SCALE;
//				matVertex[1] = terrCoords[ z * terrWidth + x ].height;
//				matVertex[2] = z * TERR_SCALE;
//				matVertex[3] = 1.0f;
		
			 // Get the untransformed vertex position
			WORD x = ix * TERR_SCALE;
			WORD y = terrCoords[ iz * terrWidth + ix ].height;
			WORD z = iz * TERR_SCALE;

			terrVertices[ iz * terrWidth + ix ] = 
				ProjectCoord( x, y, z, 
				terrCoords[ iz * terrWidth + ix ].lightIndex,
				matSet, dwClipWidth, dwClipHeight );

			if( iz < terrDepth-1 && ix < terrWidth-1 ){
				*pTerrIndex 		= iz * terrWidth + ix;
				*(pTerrIndex + 1) = ( iz + 1 ) * terrWidth + ix;
				*(pTerrIndex + 2) = ( iz + 1 ) * terrWidth + ( ix + 1 );
				*(pTerrIndex + 3) = iz * terrWidth + ix;
				*(pTerrIndex + 4) = ( iz + 1 ) * terrWidth + ( ix + 1 );
				*(pTerrIndex + 5) = iz * terrWidth + ( ix + 1 );
				numIndices += 6;
				pTerrIndex += 6;
			}
		}
	} 

	m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, terrVertexTypeDesc,
		terrVertices, terrArea, terrIndices, numIndices, NULL );				
		return S_OK;
*/	
	m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, terrVertexTypeDesc,
		terrVertices, terrArea, terrIndices, (pTerrIndex - terrIndices), NULL );				
		return S_OK;
}



//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//			 this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
		D3DTextr_InvalidateAllTextures();

		return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//			 to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
	delete terrCoords; // these should be deleted after each race
	delete terrIndices;
	delete terrVertices;

		return S_OK;
}




