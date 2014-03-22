

#include "common.h"





struct TEXTURESEARCHINFO
{
    DWORD dwDesiredBPP;   // Input for texture format search
    BOOL  bUseAlpha;
    BOOL  bUsePalette;
    BOOL  bFoundGoodFormat;

    DDPIXELFORMAT* pddpf; // Output of texture format search
};



DWORD numTextures=0;
CTextureHolder textures[ MAX_TEXTURES ];

extern CMyApplication* g_pMyApp;





//-----------------------------------------------------------------------------
// Name: TextureSearchCallback()
// Desc: Enumeration callback routine to find a best-matching texture format.
//       The param data is the DDPIXELFORMAT of the best-so-far matching
//       texture. Note: the desired BPP is passed in the dwSize field, and the
//       default BPP is passed in the dwFlags field.
//-----------------------------------------------------------------------------
static HRESULT CALLBACK TextureSearchCallback( DDPIXELFORMAT* pddpf,
                                               VOID* param )
{
    if( NULL==pddpf || NULL==param )
        return DDENUMRET_OK;

    TEXTURESEARCHINFO* ptsi = (TEXTURESEARCHINFO*)param;

    // Skip any funky modes
    if( pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV) )
        return DDENUMRET_OK;

    // Check for palettized formats
    if( ptsi->bUsePalette )
    {
        if( !( pddpf->dwFlags & DDPF_PALETTEINDEXED8 ) )
            return DDENUMRET_OK;

        // Accept the first 8-bit palettized format we get
        memcpy( ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT) );
        ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    // Else, skip any paletized formats (all modes under 16bpp)
    if( pddpf->dwRGBBitCount < 16 )
        return DDENUMRET_OK;

    // Skip any FourCC formats
    if( pddpf->dwFourCC != 0 )
        return DDENUMRET_OK;

    // Skip any ARGB 4444 formats (which are best used for pre-authored
    // content designed speciafically for an ARGB 4444 format).
    if( pddpf->dwRGBAlphaBitMask == 0x0000f000 )
        return DDENUMRET_OK;

    // Make sure current alpha format agrees with requested format type
    if( (ptsi->bUseAlpha==TRUE) && !(pddpf->dwFlags&DDPF_ALPHAPIXELS) )
        return DDENUMRET_OK;
    if( (ptsi->bUseAlpha==FALSE) && (pddpf->dwFlags&DDPF_ALPHAPIXELS) )
        return DDENUMRET_OK;

    // Check if we found a good match
    if( pddpf->dwRGBBitCount == ptsi->dwDesiredBPP )
    {
        memcpy( ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT) );
        ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}






CTextureHolder::CTextureHolder()
{
    //lstrcpy( m_strName, strName );
    m_dwWidth     = 0;
    m_dwHeight    = 0;
    m_dwBPP       = 0;
    m_dwFlags     = 0;
		m_bHasMyAlpha	= FALSE;

    m_pddsSurface = NULL;
    m_hbmBitmap   = NULL;
}



//-----------------------------------------------------------------------------
// Name: ~TextureContainer()
// Desc: Destructs the contents of the texture container
//-----------------------------------------------------------------------------
CTextureHolder::~CTextureHolder()
{
    SAFE_RELEASE( m_pddsSurface );
    DeleteObject( m_hbmBitmap );
}




//-----------------------------------------------------------------------------
// Name: LoadBitmapFile()
// Desc: Loads data from a .bmp file, and stores it in a bitmap structure.
//-----------------------------------------------------------------------------
HRESULT CTextureHolder::LoadBitmapFile( TCHAR* strPathname )
{
    // Try to load the bitmap as a file
    m_hbmBitmap = (HBITMAP)LoadImage( NULL, strPathname, IMAGE_BITMAP, 0, 0,
                                      LR_LOADFROMFILE|LR_CREATEDIBSECTION );
    if( m_hbmBitmap )
        return S_OK;
    
    return DDERR_NOTFOUND;
}




HRESULT CTextureHolder::Restore( LPDIRECT3DDEVICE7 pd3dDevice )
{
    // Release any previously created objects
    SAFE_RELEASE( m_pddsSurface );

    // Check params
    if( NULL == pd3dDevice )
        return DDERR_INVALIDPARAMS;

    // Get the device caps
    D3DDEVICEDESC7 ddDesc;
    if( FAILED( pd3dDevice->GetCaps( &ddDesc) ) )
        return E_FAIL;

    // Setup the new surface desc
    DDSURFACEDESC2 ddsd;
    D3DUtil_InitSurfaceDesc( ddsd );
    ddsd.dwFlags         = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|
                           DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps  = DDSCAPS_TEXTURE;
    ddsd.dwTextureStage  = 0; //m_dwStage;
    ddsd.dwWidth         = m_dwWidth;
    ddsd.dwHeight        = m_dwHeight;

    // Turn on texture management for hardware devices
    if( ddDesc.deviceGUID == IID_IDirect3DHALDevice )
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    else if( ddDesc.deviceGUID == IID_IDirect3DTnLHalDevice )
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    else
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

    // Adjust width and height to be powers of 2, if the device requires it
    if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2 )
    {
        for( ddsd.dwWidth=1;  m_dwWidth>ddsd.dwWidth;   ddsd.dwWidth<<=1 );
        for( ddsd.dwHeight=1; m_dwHeight>ddsd.dwHeight; ddsd.dwHeight<<=1 );
    }

    // Limit max texture sizes, if the driver can't handle large textures
    DWORD dwMaxWidth  = ddDesc.dwMaxTextureWidth;
    DWORD dwMaxHeight = ddDesc.dwMaxTextureHeight;
    ddsd.dwWidth  = min( ddsd.dwWidth,  ( dwMaxWidth  ? dwMaxWidth  : 256 ) );
    ddsd.dwHeight = min( ddsd.dwHeight, ( dwMaxHeight ? dwMaxHeight : 256 ) );

    // Make the texture square, if the driver requires it
    if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY )
    {
        if( ddsd.dwWidth > ddsd.dwHeight ) ddsd.dwHeight = ddsd.dwWidth;
        else                               ddsd.dwWidth  = ddsd.dwHeight;
    }

    // Setup the structure to be used for texture enumration.
    TEXTURESEARCHINFO tsi;
    tsi.bFoundGoodFormat = FALSE;
    tsi.pddpf            = &ddsd.ddpfPixelFormat;
    tsi.dwDesiredBPP     = m_dwBPP;
    tsi.bUsePalette      = ( m_dwBPP <= 8 );
    tsi.bUseAlpha        = m_bHasMyAlpha;
    if( m_dwFlags & D3DTEXTR_16BITSPERPIXEL )
        tsi.dwDesiredBPP = 16;
    else if( m_dwFlags & D3DTEXTR_32BITSPERPIXEL )
        tsi.dwDesiredBPP = 32;

    if( m_dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
    {
        if( tsi.bUsePalette )
        {
            if( ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE )
            {
                tsi.bUseAlpha   = TRUE;
                tsi.bUsePalette = TRUE;
            }
            else
            {
                tsi.bUseAlpha   = TRUE;
                tsi.bUsePalette = FALSE;
            }
        }
    }

    // Enumerate the texture formats, and find the closest device-supported
    // texture pixel format
    pd3dDevice->EnumTextureFormats( TextureSearchCallback, &tsi );

    // If we couldn't find a format, let's try a default format
    if( FALSE == tsi.bFoundGoodFormat )
    {
        tsi.bUsePalette  = FALSE;
        tsi.dwDesiredBPP = 16;
        pd3dDevice->EnumTextureFormats( TextureSearchCallback, &tsi );

        // If we still fail, we cannot create this texture
        if( FALSE == tsi.bFoundGoodFormat )
            return E_FAIL;
    }

    // Get the DirectDraw interface for creating surfaces
    LPDIRECTDRAW7        pDD;
    LPDIRECTDRAWSURFACE7 pddsRender;
    pd3dDevice->GetRenderTarget( &pddsRender );
    pddsRender->GetDDInterface( (VOID**)&pDD );
    pddsRender->Release();

    // Create a new surface for the texture
    HRESULT hr = pDD->CreateSurface( &ddsd, &m_pddsSurface, NULL );

    // Done with DDraw
    pDD->Release();

    if( FAILED(hr) )
        return hr;

    // For bitmap-based textures, copy the bitmap image.
    if( m_hbmBitmap )
        return CopyBitmapToSurface();

    // At this point, code can be added to handle other file formats (such as
    // .dds files, .jpg files, etc.).
    return S_OK;
}




DWORD GetShift( DWORD mask ){

	DWORD shift = 0;

	while( !( mask & 0x00000001 ) ){
		shift++;
		mask >>= 1;
	}

	return shift;
}




HRESULT CTextureHolder::CopyBitmapToSurface(){

    // Get a DDraw object to create a temporary surface
    LPDIRECTDRAW7 pDD;
    m_pddsSurface->GetDDInterface( (VOID**)&pDD );

    // Get the bitmap structure (to extract width, height, and bpp)
    BITMAP bm;
    GetObject( m_hbmBitmap, sizeof(BITMAP), &bm );

    // Setup the new surface desc
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = sizeof(ddsd);
    m_pddsSurface->GetSurfaceDesc( &ddsd );
    ddsd.dwFlags          = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|
                            DDSD_TEXTURESTAGE;
    ddsd.ddsCaps.dwCaps   = DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    ddsd.ddsCaps.dwCaps2  = 0L;
    ddsd.dwWidth          = bm.bmWidth;
    ddsd.dwHeight         = bm.bmHeight;

    // Create a new surface for the texture
    LPDIRECTDRAWSURFACE7 pddsTempSurface;
    HRESULT hr;
    if( FAILED( hr = pDD->CreateSurface( &ddsd, &pddsTempSurface, NULL ) ) )
    {
        pDD->Release();
        return hr;
    }

    // Get a DC for the bitmap
    HDC hdcBitmap = CreateCompatibleDC( NULL );
    if( NULL == hdcBitmap )
    {
        pddsTempSurface->Release();
        pDD->Release();
        return hr; // bug? return E_FAIL?
    }
    SelectObject( hdcBitmap, m_hbmBitmap );

    // Handle palettized textures. Need to attach a palette
    if( ddsd.ddpfPixelFormat.dwRGBBitCount == 8 )
    {
        LPDIRECTDRAWPALETTE  pPalette;
        DWORD dwPaletteFlags = DDPCAPS_8BIT|DDPCAPS_ALLOW256;
        DWORD pe[256];
        WORD  wNumColors     = GetDIBColorTable( hdcBitmap, 0, 256, (RGBQUAD*)pe );

        // Create the color table
        for( WORD i=0; i<wNumColors; i++ )
        {
            pe[i] = RGB( GetBValue(pe[i]), GetGValue(pe[i]), GetRValue(pe[i]) );

            // Handle textures with transparent pixels
            if( m_dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
            {
                // Set alpha for opaque pixels
                if( m_dwFlags & D3DTEXTR_TRANSPARENTBLACK )
                {
                    if( pe[i] != 0x00000000 )
                        pe[i] |= 0xff000000;
                }
                else if( m_dwFlags & D3DTEXTR_TRANSPARENTWHITE )
                {
                    if( pe[i] != 0x00ffffff )
                        pe[i] |= 0xff000000;
                }
            }
        }
        // Add DDPCAPS_ALPHA flag for textures with transparent pixels
        if( m_dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
            dwPaletteFlags |= DDPCAPS_ALPHA;

        // Create & attach a palette
        pDD->CreatePalette( dwPaletteFlags, (PALETTEENTRY*)pe, &pPalette, NULL );
        pddsTempSurface->SetPalette( pPalette );
        m_pddsSurface->SetPalette( pPalette );
        SAFE_RELEASE( pPalette );
    }

    // Copy the bitmap image to the surface.
    HDC hdcSurface;
    if( SUCCEEDED( pddsTempSurface->GetDC( &hdcSurface ) ) )
    {
        BitBlt( hdcSurface, 0, 0, bm.bmWidth, bm.bmHeight, hdcBitmap, 0, 0,
                SRCCOPY );
        pddsTempSurface->ReleaseDC( hdcSurface );
    }
    DeleteDC( hdcBitmap );

    // Copy the temp surface to the real texture surface
    m_pddsSurface->Blt( NULL, pddsTempSurface, NULL, DDBLT_WAIT, NULL );

    // Done with the temp surface
    pddsTempSurface->Release();

    // For textures with real alpha (not palettized), set transparent bits
    if( ddsd.ddpfPixelFormat.dwRGBAlphaBitMask )
    {
        if( m_dwFlags & (D3DTEXTR_TRANSPARENTWHITE|D3DTEXTR_TRANSPARENTBLACK) )
        {
            // Lock the texture surface
            DDSURFACEDESC2 ddsd;
            ddsd.dwSize = sizeof(ddsd);
            while( m_pddsSurface->Lock( NULL, &ddsd, 0, NULL ) ==
                   DDERR_WASSTILLDRAWING );

            DWORD dwAlphaMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
            DWORD dwRGBMask   = ( ddsd.ddpfPixelFormat.dwRBitMask |
                                  ddsd.ddpfPixelFormat.dwGBitMask |
                                  ddsd.ddpfPixelFormat.dwBBitMask );
            DWORD dwColorkey  = 0x00000000; // Colorkey on black
            if( m_dwFlags & D3DTEXTR_TRANSPARENTWHITE )
                dwColorkey = dwRGBMask;     // Colorkey on white

            // Add an opaque alpha value to each non-colorkeyed pixel
            for( DWORD y=0; y<ddsd.dwHeight; y++ )
            {
                WORD*  p16 =  (WORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);
                DWORD* p32 = (DWORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);

                for( DWORD x=0; x<ddsd.dwWidth; x++ )
                {
                    if( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 )
                    {
                        if( ( *p16 &= dwRGBMask ) != dwColorkey )
                            *p16 |= dwAlphaMask;
                        p16++;
                    }
                    if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 )
                    {
                        if( ( *p32 &= dwRGBMask ) != dwColorkey )
                            *p32 |= dwAlphaMask;
                        p32++;
                    }
                }
            }
            m_pddsSurface->Unlock( NULL );
        }
				else if( m_bHasMyAlpha ){
					
					DDSURFACEDESC2 ddsd;
					ddsd.dwSize = sizeof(ddsd);
					while( m_pddsSurface->Lock( NULL, &ddsd, 0, NULL ) ==
						DDERR_WASSTILLDRAWING );
					
					DWORD dwRGBMask   = ( ddsd.ddpfPixelFormat.dwRBitMask |
						ddsd.ddpfPixelFormat.dwGBitMask |
						ddsd.ddpfPixelFormat.dwBBitMask );

					DWORD rMask = ddsd.ddpfPixelFormat.dwRBitMask;
					DWORD gMask = ddsd.ddpfPixelFormat.dwGBitMask;
					DWORD bMask = ddsd.ddpfPixelFormat.dwBBitMask;
					DWORD aMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;

					DWORD rShift = GetShift( rMask );
					DWORD gShift = GetShift( gMask );
					DWORD bShift = GetShift( bMask );
					DWORD aShift = GetShift( aMask );

					DWORD maxRVal = rMask >> rShift;
					DWORD maxGVal = gMask >> gShift;
					DWORD maxBVal = bMask >> bShift;
					DWORD maxAVal = aMask >> aShift;

					DWORD rVal, gVal, bVal, aVal;
					FLOAT min, max;
					
					// Add an opaque alpha value to each non-colorkeyed pixel
					for( DWORD y=0; y<ddsd.dwHeight; y++ ){

						WORD*  p16 =  (WORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);
						DWORD* p32 = (DWORD*)((BYTE*)ddsd.lpSurface + y*ddsd.lPitch);
						
						for( DWORD x=0; x<ddsd.dwWidth; x++ ){

							if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 ){
								
								*p32 &= dwRGBMask; // set alpha to zero

								if( *p32 == 0 ){} // black is 100% transparent, so leave alpha at 0%

								else if( *p32 == dwRGBMask ){ // white is opaque, so set alpha to 100%
									*p32 |= aMask;
								}

								else{ // set alpha to equal intensity of brightest hue

									rVal = ( *p32 & rMask ) >> rShift;
									gVal = ( *p32 & gMask ) >> gShift;
									bVal = ( *p32 & bMask ) >> bShift;

									max = max( (FLOAT)rVal / maxRVal, max( (FLOAT)gVal / maxGVal, (FLOAT)bVal / maxBVal ) );
//									min = min( (FLOAT)rVal / maxRVal, min( (FLOAT)gVal / maxGVal, (FLOAT)bVal / maxBVal ) );
									
									aVal = max * 255;

									//if( rVal == gVal && gVal == bVal ){ // white fading to black
									//	*p32 = dwRGBMask; // set color to white
									//}

									// maximize luminosity and saturation
									rVal /= max;
									gVal /= max;
									bVal /= max;

									*p32 = ( aVal << aShift ) | ( rVal << rShift ) | ( gVal << gShift ) | ( bVal << bShift );
								}
								p32++;
							}
							else if( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 ){
								
								*p16 &= dwRGBMask; // set alpha to zero

								if( *p16 == 0 ){} // black is 100% transparent, so leave alpha at 0%

								else if( *p16 == dwRGBMask ){ // white is opaque, so set alpha to 100%
									*p16 |= aMask;
								}

								else{ // set alpha to equal intensity of brightest hue

									rVal = ( *p16 & rMask ) >> rShift;
									gVal = ( *p16 & gMask ) >> gShift;
									bVal = ( *p16 & bMask ) >> bShift;

									aVal = STATSTEXTURE_ALPHA * max( (FLOAT)rVal / maxRVal, max( (FLOAT)gVal / maxGVal, (FLOAT)bVal / maxBVal ) );

									if( aVal < STATSTEXTURE_ALPHA ){ // semi-tranparent white
										*p16 = dwRGBMask;
									}

									*p16 |= aVal << aShift;
								}
								p16++;
							}

						}
					}
					m_pddsSurface->Unlock( NULL );
				}

				DWORD blah=0;
    }

    pDD->Release();

    return S_OK;
}



HRESULT AddTexture( CHAR* fileName ){

		if( NULL == GetTexture( fileName ) ){

		if( numTextures >= MAX_TEXTURES ){
			MyAlert( NULL, "Too many textures" );
			return E_FAIL;
		}

		CHAR filePath[80];
		CTextureHolder* pth = &textures[numTextures];

		strcpy( filePath, TEXTURE_DIRECTORY );
		strcat( filePath, fileName );

		strcpy( pth->name, fileName );

		pth->m_dwFlags |= D3DTEXTR_16BITSPERPIXEL;

		// Create a bitmap and load the texture file into it,
		if( FAILED( pth->LoadBitmapFile( filePath ) ) ){
			CHAR errorMsg[512];
			strcpy( errorMsg, "Could not load file: " );
			strcat( errorMsg, filePath );
			MyAlert( NULL, errorMsg );
			return E_FAIL;
		}
	
		// Save the image's dimensions
		if( pth->m_hbmBitmap ){
			BITMAP bm;
			GetObject( pth->m_hbmBitmap, sizeof(BITMAP), &bm );
			pth->m_dwWidth  = (DWORD)bm.bmWidth;
			pth->m_dwHeight = (DWORD)bm.bmHeight;
			pth->m_dwBPP    = (DWORD)bm.bmBitsPixel;
		}

		numTextures++;
	}

	return S_OK;
}





HRESULT AddStatsTexture(){

		if( NULL == GetTexture( STATSTEXTURE_FILENAME ) ){

		if( numTextures >= MAX_TEXTURES ){
			MyAlert( NULL, "Too many textures" );
			return E_FAIL;
		}

		CHAR filePath[80];
		CTextureHolder* pth = &textures[numTextures];

		strcpy( filePath, TEXTURE_DIRECTORY );
		strcat( filePath, STATSTEXTURE_FILENAME );

		strcpy( pth->name, STATSTEXTURE_FILENAME );

		pth->m_dwFlags |= D3DTEXTR_32BITSPERPIXEL; // if no 32bpp found, the default 16bpp will do

		pth->m_bHasMyAlpha = TRUE;

		// Create a bitmap and load the texture file into it,
		if( FAILED( pth->LoadBitmapFile( filePath ) ) ){
			CHAR errorMsg[512];
			strcpy( errorMsg, "Could not load file: " );
			strcat( errorMsg, filePath );
			MyAlert( NULL, errorMsg );
			return E_FAIL;
		}
	
		// Save the image's dimensions
		if( pth->m_hbmBitmap ){
			BITMAP bm;
			GetObject( pth->m_hbmBitmap, sizeof(BITMAP), &bm );
			pth->m_dwWidth  = (DWORD)bm.bmWidth;
			pth->m_dwHeight = (DWORD)bm.bmHeight;
			pth->m_dwBPP    = (DWORD)bm.bmBitsPixel;
		}

		numTextures++;
	}

	return S_OK;
}





CTextureHolder* GetTexture( CHAR* fileName ){

	for(DWORD i=0; i < numTextures; i++){
		if( 0 == strcmp( fileName, textures[i].name ) ){
			return &textures[i];
		}
	}

	return NULL;
}



HRESULT RestoreTextures(){
	
	HRESULT hr;

  for( WORD i = 0; i < numTextures; i++ ){
		if( FAILED( hr = textures[i].Restore( g_pMyApp->GetD3DDevice() ) ) ){
			MyAlert(&hr, "Unable to restore textures" );
			return hr;
		}
	}

	return S_OK;
}



VOID ReleaseTextures(){
	
  for( WORD i = 0; i < numTextures; i++ ){
		SAFE_RELEASE( textures[i].m_pddsSurface );
	}
}



VOID DestroyTextures(){

  for( WORD i = 0; i < numTextures; i++ ){
    SAFE_RELEASE( textures[i].m_pddsSurface );
    DeleteObject( textures[i].m_hbmBitmap );
	}
	numTextures = 0;
}





D3DMATRIX shadowTextureMatProj, shadowTextureMatView;

D3DMATERIAL7 shadowTextureMtrl = {
	0.0f, 0.0f, 0.0f, 1.0,
	SHADOW_AMBIENT, SHADOW_AMBIENT, SHADOW_AMBIENT, 1.0,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f };




HRESULT CMyApplication::InitRenderShadowTexture(){

	HRESULT hr;

	shadowTexture.vp.dwX = 0;
	shadowTexture.vp.dwY = 0;
	shadowTexture.vp.dwWidth = SHADOWTEXTURE_SIZE;
	shadowTexture.vp.dwHeight = SHADOWTEXTURE_SIZE;
	shadowTexture.vp.dvMinZ = 0.0f;
	shadowTexture.vp.dvMaxZ = 1.0f;
	
	DDSURFACEDESC2 ddsd;
  D3DUtil_InitSurfaceDesc( ddsd );
	ddsd.dwFlags					= DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_TEXTURESTAGE |	DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.dwWidth					= shadowTexture.vp.dwWidth;
	ddsd.dwHeight					= shadowTexture.vp.dwHeight;
	ddsd.dwTextureStage		= 0;

	ddsd.ddsCaps.dwCaps 	= DDSCAPS_3DDEVICE | DDSCAPS_TEXTURE | 
		DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

	// Setup the structure to be used for texture enumration.
	TEXTURESEARCHINFO tsi;
	tsi.bFoundGoodFormat = FALSE;
	tsi.pddpf            = &ddsd.ddpfPixelFormat;
	tsi.dwDesiredBPP     = 32;
	tsi.bUsePalette      = FALSE;
	tsi.bUseAlpha        = TRUE;
	
	// Enumerate the texture formats, and find the closest device-supported
	// texture pixel format
	m_pd3dDevice->EnumTextureFormats( TextureSearchCallback, &tsi );
	
	if( FALSE == tsi.bFoundGoodFormat ){ // if no 32bpp, try 16bpp
		tsi.dwDesiredBPP = 16;	
		m_pd3dDevice->EnumTextureFormats( TextureSearchCallback, &tsi );
		if( FALSE == tsi.bFoundGoodFormat ){
			return E_FAIL;
		}
	}

	if( FAILED( hr = m_pDD->CreateSurface( &ddsd, &shadowTexture.pddsSurface, NULL ) ) ){
		return E_FAIL;
	}

	SetViewMatrix(
		shadowTextureMatView,
		D3DVECTOR( 0.0f,   5.0f, 0.0f ), // directly above origin
		D3DVECTOR( 0.0f,   0.0f, 0.0f ),
		D3DVECTOR( 0.0f,   0.0f, 1.0f ) ); // up vector

	D3DUtil_SetIdentityMatrix( shadowTextureMatProj );
	shadowTextureMatProj._22 = -1;
	shadowTextureMatProj._43 = 1;
	shadowTextureMatProj._44 = MAX_MODEL_SIZE / 2;

	return S_OK;
}



BOOL CMyApplication::TestRenderShadowTexture(){

	DDSURFACEDESC2 ddsd;

	// render to newly-created shadow texture
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 FALSE );
	if( FAILED( m_pd3dDevice->SetRenderTarget( shadowTexture.pddsSurface, 0L ) ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->SetViewport( &shadowTexture.vp ) ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->BeginScene() ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET,
		0x00000000, 0L, 0L ) ) ){
		return FALSE;
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, FALSE );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_COLORVERTEX, TRUE ); 
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, FALSE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE );

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );

	// render triange in top-left corner
	D3DTLVERTEX triVertices[ 4 ];
	D3DCOLOR c = D3DRGBA( 0.0, 1.0, 0.0, 1.0 );

	for(DWORD vert=0; vert < 4; vert++){
		triVertices[vert].color = c;
		triVertices[vert].rhw = 1.0f;
		triVertices[vert].sz = 0.0f;
	}

	triVertices[0].sx = 0.0f;
	triVertices[0].sy = 0.0f;
	triVertices[1].sx = 0.0f;
	triVertices[1].sy = SHADOWTEXTURE_SIZE;
	triVertices[2].sx = SHADOWTEXTURE_SIZE;
	triVertices[2].sy = 0.0f;
	triVertices[3].sx = SHADOWTEXTURE_SIZE;
	triVertices[3].sy = SHADOWTEXTURE_SIZE;
	
	if( FAILED( m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, 
		triVertices, 4, 0 ) ) ){
		return FALSE;
	}

	m_pd3dDevice->EndScene();

	// now render shadow texture to normal render target
	if( FAILED( m_pd3dDevice->SetRenderTarget(m_pFramework->GetBackBuffer(), 0L ) ) ){
		return FALSE;
	}
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, TRUE );
	if( FAILED( m_pd3dDevice->SetViewport( &m_vp ) ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->BeginScene() ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
		0x00000000, 1.0f, 0L ) ) ){
		return FALSE;
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTFG_POINT ); 
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTFG_POINT ); 

	triVertices[0].sx = 0.0f;
	triVertices[0].sy = 0.0f;
	triVertices[0].tu = 0.0f;
	triVertices[0].tv = 0.0f;

	triVertices[1].sx = 0.0f;
	triVertices[1].sy = 2.0f;
	triVertices[1].tu = 0.0f;
	triVertices[1].tv = 1.0f;

	triVertices[2].sx = 2.0f;
	triVertices[2].sy = 0.0f;
	triVertices[2].tu = 1.0f;
	triVertices[2].tv = 0.0f;

	triVertices[3].sx = 2.0f;
	triVertices[3].sy = 2.0f;
	triVertices[3].tu = 1.0f;
	triVertices[3].tv = 1.0f;

	if( FAILED( m_pd3dDevice->SetTexture( 0, shadowTexture.pddsSurface ) ) ){
		return FALSE;
	}

	if( FAILED( m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, 
		triVertices, 4, 0 ) ) ){
		return FALSE;
	}

	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );

	m_pd3dDevice->EndScene();


//	m_pFramework->ShowFrame( m_bVSync );

//	MyAlert( NULL, "bkah" );

	
	// see if little green square showed up in top-left corner
	ZeroMemory( &ddsd, sizeof(ddsd) );
	ddsd.dwSize = sizeof(ddsd);

	if( FAILED( m_pFramework->GetBackBuffer()->Lock( NULL, &ddsd, 0, NULL ) ) ){
		return FALSE;
	}

	DWORD data;
	
	if( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 ){
		data = *(WORD *)ddsd.lpSurface;
	}
	else if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 ){
		data = *(DWORD *)ddsd.lpSurface;
	}
	else{
		data == 0x00000000; // 24-bit mode, probably an old card anyway, so fake failure
	}

	m_pFramework->GetBackBuffer()->Unlock( NULL );

	// return false if anything but solid green displayed
	if( ( data & ddsd.ddpfPixelFormat.dwGBitMask ) == 0 ||
		  ( data & ddsd.ddpfPixelFormat.dwRBitMask ) != 0 ||
			( data & ddsd.ddpfPixelFormat.dwBBitMask ) != 0 ){
		return FALSE;
	}


	// now set shadow texture as a render target and try clearing it
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 FALSE );

	if( FAILED( m_pd3dDevice->SetRenderTarget( shadowTexture.pddsSurface, 0L ) ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->SetViewport( &shadowTexture.vp ) ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->BeginScene() ) ){
		return FALSE;
	}

	if( FAILED( m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET,
		0x00000000, 0L, 0L ) ) ){
		return FALSE;
	}

	m_pd3dDevice->EndScene();


	// now render shadow texture *again* to normal render target
	if( FAILED( m_pd3dDevice->SetRenderTarget(m_pFramework->GetBackBuffer(), 0L ) ) ){
		return FALSE;
	}
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 TRUE );
	if( FAILED( m_pd3dDevice->SetViewport( &m_vp ) ) ){
		return FALSE;
	}
	if( FAILED( m_pd3dDevice->BeginScene() ) ){
		return FALSE;
	}
	// don't clear normal target, just z-buff
	if( FAILED( m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER,
		0x00000000, 1.0f, 0L ) ) ){
		return FALSE;
	}

	if( FAILED( m_pd3dDevice->SetTexture( 0, shadowTexture.pddsSurface ) ) ){
		return FALSE;
	}

	if( FAILED( m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, D3DFVF_TLVERTEX, 
		triVertices, 4, 0 ) ) ){
		return FALSE;
	}

	m_pd3dDevice->EndScene();



	// check to see if green square was overwritten by black square
	ZeroMemory( &ddsd, sizeof(ddsd) );
	ddsd.dwSize = sizeof(ddsd);

	if( FAILED( m_pFramework->GetBackBuffer()->Lock( NULL, &ddsd, 0, NULL ) ) ){
		return FALSE;
	}

	if( ddsd.ddpfPixelFormat.dwRGBBitCount == 16 ){
		data = *(WORD *)ddsd.lpSurface;
	}
	else if( ddsd.ddpfPixelFormat.dwRGBBitCount == 32 ){
		data = *(DWORD *)ddsd.lpSurface;
	}
	else{
		data = 0xFFFFFFFF; // 24-bit mode, probably an old card anyway, so fake failure
	}

	m_pFramework->GetBackBuffer()->Unlock( NULL );

	// return false if any color present (but don't worry about alpha)
	if( ( data & ( ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask |
		ddsd.ddpfPixelFormat.dwBBitMask ) ) != 0 ){
		return FALSE;
	}


	return TRUE;

}





HRESULT CMyApplication::InitShadowTexture(){

	HRESULT hr;

	shadowTexture.pddsSurface = NULL;

	if( bTestShadow ){

		bRenderShadowTexture = TRUE;

		if( FAILED( InitRenderShadowTexture() ) ){
			return E_FAIL;
		}

		if( TestRenderShadowTexture() ){
			return S_OK;
		}
		else{
			// bTestShadow = FALSE;
			// bRenderShadowTexture = FALSE;
			return E_FAIL;
		}
	}
	else{ // else user wants to override test

		if( bRenderShadowTexture ){
			if( FAILED( InitRenderShadowTexture() ) ){
				return E_FAIL;
			}
		}
		else{
			if( FAILED( AddTexture( "staticshadow.bmp" ) ) ){
				return E_FAIL;
			}
		}
	}

	return S_OK;
}




VOID CMyApplication::ReleaseShadowTexture(){
	
	if( bRenderShadowTexture ){
		SAFE_RELEASE( shadowTexture.pddsSurface );
	}
	// else static shadow texture is released under normal texture management
}




HRESULT CMyApplication::RenderToShadowTexture( CModel* pModel ){

	HRESULT hr=0;
	MODELMESH* pMesh;
	D3DMATRIX matView, matProj;
	DWORD x, y;

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 FALSE );


	if( FAILED( hr = m_pd3dDevice->SetRenderTarget( shadowTexture.pddsSurface, 0L ) ) ){
		MyAlert(&hr, "Unable to set shadow texture as render target" );
		return E_FAIL;
	}

	if( FAILED( hr = m_pd3dDevice->SetViewport( &shadowTexture.vp ) ) ){
		MyAlert(&hr, "Unable to set viewport for shadow rendering" );
		return hr;
	}

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, FALSE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE );

	if( FAILED( hr = m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET,
		0x00000000, 0L, 0L ) ) ){
		MyAlert(&hr, "Unable to clear shadow texture" );
		return E_FAIL;
	}

	if( FAILED( m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &shadowTextureMatProj ) ) ){
		MyAlert( NULL, "unable to set projection matrix for shadow rendering");
		return E_FAIL;
	}

	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &shadowTextureMatView );

	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO );
	m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );

//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_LIGHTING, TRUE );

	if( FAILED( hr = m_pd3dDevice->SetMaterial( &shadowTextureMtrl ) ) ){
		MyAlert(&hr, "Unable to set material for shadow rendering" );
		return E_FAIL;
	}

	

	pMesh = pModel->meshes;
	for(DWORD i=0; i < pModel->numMeshes; i++, pMesh++){

		if( FAILED( hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, D3DFVF_VERTEX, 
			pMesh->vertices, pMesh->numVertices, 
			pMesh->indices, pMesh->numIndices, 0 ) ) ){
			MyAlert(&hr, "Unable to render to shadow texture" );
			return E_FAIL;
		}
	}


	if( FAILED( hr = m_pd3dDevice->SetMaterial( &m_defaultMtrl ) ) ){
		MyAlert(&hr, "Unable to reset material after shadow rendering" );
		return E_FAIL;
	}

	if( FAILED( m_pd3dDevice->SetRenderTarget(m_pFramework->GetBackBuffer(), 0L ) ) ){
		MyAlert(&hr, "Unable to reset render target after shadow rendering" );
	}

	if( FAILED( hr = m_pd3dDevice->SetViewport( &m_vp ) ) ){
		MyAlert(&hr, "Unable to reset viewport after shadow rendering" );
		return E_FAIL;
	}

	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_PROJECTION, &m_matProj );

	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &m_matView );
	
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ZENABLE, 			 TRUE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA );
//	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD );
	m_pd3dDevice->SetRenderState( D3DRENDERSTATE_FOGENABLE, 	 TRUE );

	return S_OK;
}








