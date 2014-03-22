
#include "common.h"



static const char c_szWAV[] = "WAVE";

DWORD g_numSounds = 0;
CSoundHolder g_sounds[ MAX_SOUNDS ];

LPDIRECTSOUND           g_pDS;
LPDIRECTSOUND3DLISTENER g_p3DListener;
DSCAPS									g_dsCaps;



#include "WavRead.h"

HRESULT CreateStreamingBuffer();
HRESULT UpdateProgress();
HRESULT FillBuffer( BOOL bLooped, DWORD offset );
HRESULT WriteToBuffer( BOOL bLooped, VOID* pbBuffer, DWORD dwBufferLength );
HRESULT RestoreBuffers( BOOL bLooped, DWORD offset );

#define NUM_PLAY_NOTIFICATIONS  16

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

LPDIRECTSOUNDBUFFER g_pDSBuffer      = NULL;
LPDIRECTSOUNDNOTIFY g_pDSNotify      = NULL;
CWaveSoundRead*     g_pWaveSoundRead = NULL;

DSBPOSITIONNOTIFY   g_aPosNotify[ NUM_PLAY_NOTIFICATIONS ];  
HANDLE              g_SoundNotificationEvents[1];

DWORD               g_dwBufferSize;
DWORD               g_dwNotifySize;
DWORD               g_dwNextWriteOffset;
DWORD               g_dwProgress;
DWORD               g_dwLastPos;
BOOL                g_bFoundEnd;





//-----------------------------------------------------------------------------
// Name: InitDirectSound()
// Desc: Creates the DirectSound object, primary buffer, and 3D listener
//-----------------------------------------------------------------------------
HRESULT InitDirectSound( HWND hWnd ){

	DSBUFFERDESC        dsbd;
	LPDIRECTSOUNDBUFFER lpdsbPrimary;
	WAVEFORMATEX        wfm;
	HRESULT hr;

	if( FAILED( hr = CoInitialize( NULL ) ) )
		return hr;

	if( FAILED( DirectSoundCreate( NULL, &g_pDS, NULL ) ) )
		return E_FAIL;
	
	// Set cooperative level.
	if( FAILED( g_pDS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) ) )
		return E_FAIL;

	ZeroMemory( &g_dsCaps, sizeof(DSCAPS) );
	g_dsCaps.dwSize = sizeof(DSCAPS);
	g_pDS->GetCaps( &g_dsCaps );
	
	// Create primary buffer.
	ZeroMemory( &dsbd, sizeof( DSBUFFERDESC ) );
	dsbd.dwSize  = sizeof( DSBUFFERDESC );
	dsbd.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
	
	if( FAILED( g_pDS->CreateSoundBuffer( &dsbd, &lpdsbPrimary, NULL ) ) )
		return E_FAIL;
	
	// Set primary buffer format.
	ZeroMemory( &wfm, sizeof( WAVEFORMATEX ) ); 
	wfm.wFormatTag      = WAVE_FORMAT_PCM; 
	wfm.nChannels       = 2; 
	wfm.nSamplesPerSec  = 22050; 
	wfm.wBitsPerSample  = 16; 
	wfm.nBlockAlign     = wfm.wBitsPerSample / 8 * wfm.nChannels;
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
	
	lpdsbPrimary->SetFormat( &wfm ); 
	
	// Get listener interface.
	if( FAILED( lpdsbPrimary->QueryInterface( IID_IDirectSound3DListener,
		(VOID**)&g_p3DListener ) ) )
	{
		lpdsbPrimary->Release();
		return E_FAIL;
	}
	lpdsbPrimary->Release(); lpdsbPrimary = NULL;

	g_p3DListener->SetRolloffFactor( SOUND_ROLLOFF, DS3D_DEFERRED );

	g_SoundNotificationEvents[0] = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	return S_OK;
}





CSoundHolder::CSoundHolder(){

	m_numInstances = 0;

	for(DWORD i=0; i < MAX_SOUNDINSTANCES; i++){
		m_instances[i].pDSBuff = NULL;
		m_instances[i].pDS3DBuff = NULL;
	}

	m_lastInstance = 0;
}




CSoundHolder::~CSoundHolder(){

	for(DWORD i=0; i < m_numInstances; i++){
		SAFE_RELEASE( m_instances[i].pDSBuff );
		SAFE_RELEASE( m_instances[i].pDS3DBuff );
	}

	m_numInstances = 0;
}



BOOL DSParseWaveResource(void *pvRes, WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData,DWORD *pcbWaveSize)
{
    DWORD *pdw;
    DWORD *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;

    if (ppWaveHeader)
        *ppWaveHeader = NULL;

    if (ppbWaveData)
        *ppbWaveData = NULL;

    if (pcbWaveSize)
        *pcbWaveSize = 0;

    pdw = (DWORD *)pvRes;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        goto exit;      // not even RIFF

    if (dwType != mmioFOURCC('W', 'A', 'V', 'E'))
        goto exit;      // not a WAV

    pdwEnd = (DWORD *)((BYTE *)pdw + dwLength-4);

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
        case mmioFOURCC('f', 'm', 't', ' '):
            if (ppWaveHeader && !*ppWaveHeader)
            {
                if (dwLength < sizeof(WAVEFORMAT))
                    goto exit;      // not a WAV

                *ppWaveHeader = (WAVEFORMATEX *)pdw;

                if ((!ppbWaveData || *ppbWaveData) &&
                    (!pcbWaveSize || *pcbWaveSize))
                {
                    return TRUE;
                }
            }
            break;

        case mmioFOURCC('d', 'a', 't', 'a'):
            if ((ppbWaveData && !*ppbWaveData) ||
                (pcbWaveSize && !*pcbWaveSize))
            {
                if (ppbWaveData)
                    *ppbWaveData = (LPBYTE)pdw;

                if (pcbWaveSize)
                    *pcbWaveSize = dwLength;

                if (!ppWaveHeader || *ppWaveHeader)
                    return TRUE;
            }
            break;
        }

        pdw = (DWORD *)((BYTE *)pdw + ((dwLength+1)&~1));
    }

exit:
    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
//
// DSGetWaveResource
//
///////////////////////////////////////////////////////////////////////////////
BOOL DSGetWaveResource(HMODULE hModule, LPCTSTR lpName,
    WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize)
{
    HRSRC hResInfo;
    HGLOBAL hResData;
    void * pvRes;

    if (((hResInfo = FindResource(hModule, lpName, c_szWAV)) != NULL) &&
        ((hResData = LoadResource(hModule, hResInfo)) != NULL) &&
        ((pvRes = LockResource(hResData)) != NULL) &&
        DSParseWaveResource(pvRes, ppWaveHeader, ppbWaveData, pcbWaveSize))
    {
        return TRUE;
    }

    return FALSE;

}


///////////////////////////////////////////////////////////////////////////////
//
// DSGetWaveFile
//
///////////////////////////////////////////////////////////////////////////////
BOOL DSGetWaveFile(HMODULE hModule, LPCTSTR lpName,
    WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize,
	void **ppvBase)
{
    void *pvRes;
    HANDLE hFile, hMapping;

    *ppvBase = NULL;

    hFile = CreateFile (lpName, GENERIC_READ, FILE_SHARE_READ, 
	                    NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    hMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFile); 
        return FALSE;
    }

    CloseHandle (hFile);

    pvRes = MapViewOfFile (hMapping, FILE_MAP_READ, 0, 0, 0);
    if (pvRes == NULL)
    {
        CloseHandle (hMapping);
        return FALSE;
    }

        CloseHandle (hMapping);

    if (DSParseWaveResource(pvRes, ppWaveHeader, ppbWaveData, pcbWaveSize) == FALSE)
    {
        UnmapViewOfFile (pvRes);
        return FALSE;
    }

    *ppvBase = pvRes;
    return TRUE;
}





BOOL DSFillSoundBuffer(IDirectSoundBuffer *pDSB, BYTE *pbWaveData, DWORD cbWaveSize)
{
    if (pDSB && pbWaveData && cbWaveSize)
    {
        LPVOID pMem1, pMem2;
        DWORD dwSize1, dwSize2;

        if (SUCCEEDED(IDirectSoundBuffer_Lock(pDSB, 0, cbWaveSize,
            &pMem1, &dwSize1, &pMem2, &dwSize2, 0)))
        {
            CopyMemory(pMem1, pbWaveData, dwSize1);

            if ( 0 != dwSize2 )
                CopyMemory(pMem2, pbWaveData+dwSize1, dwSize2);

            IDirectSoundBuffer_Unlock(pDSB, pMem1, dwSize1, pMem2, dwSize2);
            return TRUE;
        }
    }

    return FALSE;
}



///////////////////////////////////////////////////////////////////////////////
//
// DSLoad3DSoundBuffer - Loads a .WAV into a 3D sound buffer and returns the sound buffer
//
///////////////////////////////////////////////////////////////////////////////

IDirectSoundBuffer *DSLoad3DSoundBuffer( IDirectSound *pDS, LPCTSTR lpName, DWORD flags )
{
    IDirectSoundBuffer *pDSB = NULL;
    DSBUFFERDESC dsBD;
    BYTE *pbWaveData;
    void *pvBase;

		ZeroMemory( &dsBD, sizeof(DSBUFFERDESC) );
    dsBD.dwSize = sizeof(dsBD);
    dsBD.dwFlags = flags;

		//dsBD.guid3DAlgorithm = DS3DALG_HRTF_FULL; //DS3DALG_NO_VIRTUALIZATION;

		//_GUID temp2 = DS3DALG_NO_VIRTUALIZATION;
		//_GUID temp3 = DS3DALG_HRTF_FULL;
		//_GUID temp4 = DS3DALG_HRTF_LIGHT;

			

    if (DSGetWaveResource(NULL, lpName, &dsBD.lpwfxFormat, &pbWaveData, &dsBD.dwBufferBytes))
    {
        if (SUCCEEDED(IDirectSound_CreateSoundBuffer(pDS, &dsBD, &pDSB, NULL)))
        {
            if (!DSFillSoundBuffer(pDSB, pbWaveData, dsBD.dwBufferBytes))
            {
                IDirectSoundBuffer_Release(pDSB);
                pDSB = NULL;
            }
        }
        else
        {
            pDSB = NULL;
        }
    } else if (DSGetWaveFile(NULL, lpName, &dsBD.lpwfxFormat, &pbWaveData,
                        &dsBD.dwBufferBytes, &pvBase))
    {
        if (SUCCEEDED(IDirectSound_CreateSoundBuffer(pDS, &dsBD, &pDSB, NULL)))
        {
            if (!DSFillSoundBuffer(pDSB, pbWaveData, dsBD.dwBufferBytes))
            {
                IDirectSoundBuffer_Release(pDSB);
                pDSB = NULL;
            }
        }
        else
        {
            pDSB = NULL;
        }
	UnmapViewOfFile (pvBase);
    }

    return pDSB;
}





///////////////////////////////////////////////////////////////////////////////
//
// DSReloadSoundBuffer
//
///////////////////////////////////////////////////////////////////////////////

BOOL DSReloadSoundBuffer(IDirectSoundBuffer *pDSB, CHAR* name )
{
	BOOL result=FALSE;
	BYTE *pbWaveData;
	DWORD cbWaveSize;
	void *pvBase;
	
	CHAR filePath[80];
	strcpy( filePath, SOUND_DIRECTORY );
	strcat( filePath, name );

	if (DSGetWaveResource(NULL, filePath, NULL, &pbWaveData, &cbWaveSize))
	{
		if (SUCCEEDED(IDirectSoundBuffer_Restore(pDSB)) &&
			DSFillSoundBuffer(pDSB, pbWaveData, cbWaveSize))
		{
			result = TRUE;
		}
	} else if( DSGetWaveFile(NULL, filePath, NULL, &pbWaveData, &cbWaveSize, &pvBase))
	{
		if (SUCCEEDED(IDirectSoundBuffer_Restore(pDSB)) &&
			DSFillSoundBuffer(pDSB, pbWaveData, cbWaveSize))
		{
			result = TRUE;
		}
		UnmapViewOfFile (pvBase);
	}
	
	return result;
}


///////////////////////////////////////////////////////////////////////////////
// SndObj fns
///////////////////////////////////////////////////////////////////////////////

HRESULT CSoundHolder::Create( CHAR* name, DWORD numInstances, BOOL bLoop, FLOAT minDist, 
														 DWORD caps ){
		
	LPWAVEFORMATEX pWaveHeader;
	BOOL fFound = FALSE;
	BYTE* pbData;
	DWORD cbData;
	void* pvBase;

	CHAR filePath[80];
	strcpy( filePath, SOUND_DIRECTORY );
	strcat( filePath, name );

	if (DSGetWaveResource(NULL, filePath, &pWaveHeader, &pbData, &cbData))
		fFound = TRUE;
	if (DSGetWaveFile(NULL, filePath, &pWaveHeader, &pbData, &cbData, &pvBase)){
		fFound = TRUE;
		UnmapViewOfFile( pvBase );
	}
	
	if( !fFound ){
		return E_FAIL;
	}
	
	// static, mute3D
	caps |= SOUND_UNIVERSALCAPS;

	strcpy( m_name, name );
	m_numInstances = numInstances;
	m_bLoop = bLoop;
	m_pbWaveData = pbData;
	m_cbWaveSize = cbData;

	m_instances[0].pDSBuff = DSLoad3DSoundBuffer( g_pDS, filePath, caps );
	if( !m_instances[0].pDSBuff ){
		return E_FAIL;
	}
	m_instances[0].pDSBuff->GetFrequency( &m_baseFrequency );

	for (DWORD i=1; i < m_numInstances; i++){

		if( FAILED( g_pDS->DuplicateSoundBuffer( m_instances[0].pDSBuff,
			&m_instances[i].pDSBuff ) ) ){
			m_instances[i].pDSBuff = DSLoad3DSoundBuffer( g_pDS, filePath, caps );
			if( !m_instances[i].pDSBuff ){
				return E_FAIL;
			}
		}
	}

	if( caps & DSBCAPS_CTRL3D ){
		for (DWORD i=0; i < m_numInstances; i++){

			m_instances[i].pDSBuff->QueryInterface( IID_IDirectSound3DBuffer,
				(LPVOID*)&m_instances[i].pDS3DBuff );
			if( !m_instances[0].pDS3DBuff ){
				return E_FAIL;
			}
			m_instances[i].pDS3DBuff->SetMinDistance( minDist, DS3D_DEFERRED );

			// leave maxdist at default (near infinity)
			//m_instances[i].pDS3DBuff->SetMaxDistance( SOUND_MAXDISTANCE + , DS3D_DEFERRED );
		}
	}

	return S_OK;
}





CSoundHolder* GetSound( CHAR* name ){

	for(DWORD i=0; i < g_numSounds; i++){
		if( 0 == strcmp( name, g_sounds[i].m_name ) ){
			return &g_sounds[i];
		}
	}

	return NULL;
}




HRESULT AddSound( CHAR* name, DWORD numInstances, BOOL bLoop, FLOAT minDist, DWORD caps ){

	// fail if sound already added, or already at max sounds
	if( GetSound( name ) != NULL || g_numSounds >= MAX_SOUNDS ){
		return E_FAIL;
	}

	if( FAILED( g_sounds[g_numSounds].Create( name, numInstances, bLoop, minDist, caps ) ) ){
		CHAR msg[256];
		sprintf( msg, "Unable to load sound file: %s%s", SOUND_DIRECTORY, name );
		MyAlert( NULL, msg );
		return E_FAIL;
	}
	g_numSounds++;

	return S_OK;
}




BOOL IsSoundInstancePlaying( CHAR* name, DWORD instance ){

	CSoundHolder* psh;
	IDirectSoundBuffer* pDSBuff;

	if( ( psh = GetSound( name ) ) == NULL ){
		return FALSE;
	}
	if( ( pDSBuff = psh->m_instances[instance].pDSBuff ) ){

		DWORD dwStatus;
		if( FAILED( pDSBuff->GetStatus( &dwStatus ) ) )
			return FALSE;
		
		return ( dwStatus & DSBSTATUS_PLAYING );
	}
	return FALSE;
}


VOID AdjustSoundInstance2D( CHAR* name, DWORD instance,
														FLOAT frequencyScale, FLOAT pan, FLOAT volume ){

	HRESULT hr;
	CSoundHolder* psh;
	IDirectSoundBuffer* pDSBuff;
	
	if( ( psh = GetSound( name ) ) == NULL ){
		return;
	}
	if( ( pDSBuff = psh->m_instances[instance].pDSBuff ) ){
		
		DWORD dwStatus;
		if( FAILED( hr = pDSBuff->GetStatus( &dwStatus ) ) )
			return;
		
		if( frequencyScale != SOUND_NOADJUST ){
			DWORD frequencyParam = psh->m_baseFrequency * frequencyScale;
			DWORD min = ( dwStatus & DSBSTATUS_LOCHARDWARE ) ? 
				g_dsCaps.dwMinSecondarySampleRate : DSBFREQUENCY_MIN;
			DWORD max = ( dwStatus & DSBSTATUS_LOCHARDWARE ) ? 
				g_dsCaps.dwMaxSecondarySampleRate : DSBFREQUENCY_MAX;
			if( frequencyParam < min ){
				frequencyParam = min;
			}
			else if( frequencyParam > max ){
				frequencyParam = max;
			}
			pDSBuff->SetFrequency( frequencyParam );
		}
		
		if( pan != SOUND_NOADJUST ){
			LONG panParam = DSBPAN_LEFT + ( DSBPAN_RIGHT - DSBPAN_LEFT ) * ( ( pan + 1.0f ) * 0.5f );
			pDSBuff->SetPan( panParam );
		}
		
		if( volume != SOUND_NOADJUST ){
			LONG volumeParam = DSBVOLUME_MIN + ( DSBVOLUME_MAX - DSBVOLUME_MIN ) * volume;
			pDSBuff->SetVolume( volumeParam );
		}
	}
}





VOID AdjustSoundInstance3D( CHAR* name, DWORD instance,
															D3DVECTOR position, D3DVECTOR velocity ){

	HRESULT hr;
	CSoundHolder* psh;
	IDirectSoundBuffer* pDSBuff;
	IDirectSound3DBuffer* pDS3DBuff;

	if( ( psh = GetSound( name ) ) == NULL ){
		return;
	}
	if( ( pDS3DBuff = psh->m_instances[instance].pDS3DBuff ) ){

		pDSBuff = psh->m_instances[instance].pDSBuff;
		DWORD dwStatus;
		if( FAILED( hr = pDSBuff->GetStatus( &dwStatus ) ) )
			return;

		pDS3DBuff->SetPosition( position.x, position.y, position.z, DS3D_DEFERRED );
		pDS3DBuff->SetVelocity( velocity.x, velocity.y, velocity.z, DS3D_DEFERRED );
	}
}





VOID UpdateListener( D3DVECTOR dir, D3DVECTOR position, D3DVECTOR velocity ){
/* hack
	g_p3DListener->SetOrientation( dir.x, dir.y, dir.z, 0.0f, 1.0f, 0.0f, DS3D_DEFERRED );
	g_p3DListener->SetPosition( position.x, position.y, position.z, DS3D_DEFERRED );
	g_p3DListener->SetVelocity( velocity.x, velocity.y, velocity.z, DS3D_DEFERRED );

	g_p3DListener->CommitDeferredSettings();
    */
}




HRESULT PlaySoundInstance( CHAR* name, DWORD instance ){

	HRESULT hr;
	CSoundHolder* psh;
	IDirectSoundBuffer* pDSBuff;

	if( ( psh = GetSound( name ) ) == NULL ){
		return E_FAIL;
	}
	pDSBuff = psh->m_instances[instance].pDSBuff;

	DWORD dwStatus;
	if( FAILED( hr = pDSBuff->GetStatus( &dwStatus ) ) )
		return hr;

	if( dwStatus & DSBSTATUS_BUFFERLOST ){
		if( FAILED( hr = DSReloadSoundBuffer( pDSBuff, psh->m_name ) ) ){
			return hr;
		}
	}

	if( dwStatus & DSBSTATUS_PLAYING ){
		// if already playing, just restart if necessary
		if( !psh->m_bLoop ){
			pDSBuff->SetCurrentPosition( 0 );
		}
	}
	else{
		if( FAILED( hr = pDSBuff->Play( 0, 0, ( psh->m_bLoop ? DSBPLAY_LOOPING : 0L ) ) ) ){
			return hr;
    }
	}

	psh->m_lastInstance = instance;

	return S_OK;
}




HRESULT PlayFreeSoundInstance( CHAR* name,
															FLOAT frequencyScale, FLOAT pan, FLOAT volume,
															D3DVECTOR position, D3DVECTOR velocity ){
/* hack
	CSoundHolder* psh;

	if( ( psh = GetSound( name ) ) == NULL ){
		return E_FAIL;
	}

	DWORD instance = ( psh->m_lastInstance + 1 ) % psh->m_numInstances;

	AdjustSoundInstance2D( name, instance, frequencyScale, pan, volume );
	AdjustSoundInstance3D( name, instance, position, velocity );
	g_p3DListener->CommitDeferredSettings(); // commit new position, etc. before restarting sound

	if( FAILED( PlaySoundInstance( name, instance ) ) ){
		return E_FAIL;
	}
*/
	return S_OK;
    
}

		


HRESULT StopSoundInstance( CHAR* name, DWORD instance ){

	CSoundHolder* psh;
	IDirectSoundBuffer* pDSBuff;

	if( ( psh = GetSound( name ) ) == NULL ){
		return E_FAIL;
	}
	pDSBuff = psh->m_instances[instance].pDSBuff;

	DWORD dwStatus;
	if( FAILED( pDSBuff->GetStatus( &dwStatus ) ) )
		return E_FAIL;

	if( ( dwStatus & DSBSTATUS_PLAYING ) ){
		pDSBuff->Stop();
		pDSBuff->SetCurrentPosition( 0 );
	}

 	return S_OK;
}




VOID StopAllSoundInstances(){

	CSoundHolder* psh = g_sounds;
	IDirectSoundBuffer* pDSBuff;

	for(DWORD i=0; i < g_numSounds; i++, psh++){
		for(DWORD instance = 0; instance < psh->m_numInstances; instance++){

			pDSBuff = psh->m_instances[instance].pDSBuff;

			DWORD dwStatus;
			if( FAILED( pDSBuff->GetStatus( &dwStatus ) ) )
				continue;

			if( ( dwStatus & DSBSTATUS_PLAYING ) ){
				pDSBuff->Stop();
				pDSBuff->SetCurrentPosition( 0 );
			}
		}
	}
}




//////// STREAMING STUFF ///////////////////////
////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Name: CreateStreamingBuffer()
// Desc: Creates a streaming buffer, and the notification events to handle
//       filling it as sound is played
//-----------------------------------------------------------------------------
HRESULT CreateStreamingBuffer()
{
    HRESULT hr; 

    // This samples works by dividing a 3 second streaming buffer into 
    // NUM_PLAY_NOTIFICATIONS (or 16) pieces.  it creates a notification for each
    // piece and when a notification arrives then it fills the circular streaming 
    // buffer with new wav data over the sound data which was just played

    // The size of wave data is in pWaveFileSound->m_ckIn
    DWORD nBlockAlign = (DWORD)g_pWaveSoundRead->m_pwfx->nBlockAlign;
    INT nSamplesPerSec = g_pWaveSoundRead->m_pwfx->nSamplesPerSec;

    // The g_dwNotifySize should be an integer multiple of nBlockAlign
    g_dwNotifySize = nSamplesPerSec * 3 * nBlockAlign / NUM_PLAY_NOTIFICATIONS;
    g_dwNotifySize -= g_dwNotifySize % nBlockAlign;   

    // The buffersize should approximately 3 seconds of wav data
    g_dwBufferSize  = g_dwNotifySize * NUM_PLAY_NOTIFICATIONS;
    g_bFoundEnd     = FALSE;
    g_dwProgress    = 0;
    g_dwLastPos     = 0;

    // Set up the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can 
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 |
			DSBCAPS_CTRLVOLUME;
    dsbd.dwBufferBytes = g_dwBufferSize;
    dsbd.lpwfxFormat   = g_pWaveSoundRead->m_pwfx;

    // Create a DirectSound buffer
    if( FAILED( hr = g_pDS->CreateSoundBuffer( &dsbd, &g_pDSBuffer, NULL ) ) )
        return hr;

    // Create a notification event, for when the sound stops playing
    if( FAILED( hr = g_pDSBuffer->QueryInterface( IID_IDirectSoundNotify, 
                                                  (VOID**)&g_pDSNotify ) ) )
        return hr;

    for( INT i = 0; i < NUM_PLAY_NOTIFICATIONS; i++ )
    {
        g_aPosNotify[i].dwOffset = (g_dwNotifySize * i) + g_dwNotifySize - 1;
        g_aPosNotify[i].hEventNotify = g_SoundNotificationEvents[0];             
    }
    
    // g_aPosNotify[i].dwOffset     = DSBPN_OFFSETSTOP;
    // g_aPosNotify[i].hEventNotify = g_SoundNotificationEvents[1];

    // Tell DirectSound when to notify us. the notification will come in the from 
    // of signaled events that are handled in WinMain()
    if( FAILED( hr = g_pDSNotify->SetNotificationPositions( NUM_PLAY_NOTIFICATIONS, 
			g_aPosNotify ) ) ){
			MyAlert( &hr, "Could not set DirectSound notifications" );
      return hr;
		}

    return S_OK;
}





//-----------------------------------------------------------------------------
// Name: LoadWaveFile()
// Desc: Loads a wave file into a secondary streaming DirectSound buffer
//-----------------------------------------------------------------------------
HRESULT LoadWaveFile( TCHAR* name )
{
    // Close and delete any existing open wave file
    if( NULL != g_pWaveSoundRead )
    {
        g_pWaveSoundRead->Close();
        SAFE_DELETE( g_pWaveSoundRead );
    }

    g_pWaveSoundRead = new CWaveSoundRead();

		CHAR filePath[80];
		strcpy( filePath, SOUND_DIRECTORY );
		strcat( filePath, name );

    // Load the wave file
    if( FAILED( g_pWaveSoundRead->Open( filePath ) ) )
    {
			CHAR msg[80];
			sprintf( msg, "Could not load sound file: %s", filePath );
			MyAlert( NULL, msg ); 
      return E_FAIL;
    }
    else // The load call succeeded
    {
        // Start with a new sound buffer object
        SAFE_RELEASE( g_pDSNotify );
        SAFE_RELEASE( g_pDSBuffer );
        ZeroMemory( &g_aPosNotify, sizeof(DSBPOSITIONNOTIFY) );

        // Create the sound buffer object from the wave file data
        if( FAILED( CreateStreamingBuffer() ) )
        {
					MyAlert( NULL, "Could not create streaming sound buffer" ); 
					return E_FAIL;
        }
    }

		return S_OK;
}





VOID AdjustStreamingSoundBuffer( FLOAT volume ){

	if( volume != SOUND_NOADJUST ){
		LONG volumeParam = DSBVOLUME_MIN + ( DSBVOLUME_MAX - DSBVOLUME_MIN ) * volume;
		g_pDSBuffer->SetVolume( volumeParam );
	}
}



//-----------------------------------------------------------------------------
// Name: PlayBuffer()
// Desc: Play the DirectSound buffer
//-----------------------------------------------------------------------------
HRESULT PlayBuffer( BOOL bLooped, DWORD offset )
{
    HRESULT hr;
    VOID*   pbBuffer = NULL;

    if( NULL == g_pDSBuffer )
        return E_FAIL;

    // Restore the buffers if they are lost
    if( FAILED( hr = RestoreBuffers( bLooped, offset ) ) )
        return hr;

    // Fill the entire buffer with wave data
    if( FAILED( hr = FillBuffer( bLooped, offset ) ) )
        return hr;

    // Always play with the LOOPING flag since the streaming buffer
    // wraps around before the entire WAV is played
    if( FAILED( hr = g_pDSBuffer->Play( 0, 0, DSBPLAY_LOOPING ) ) )
        return hr;

    return S_OK;
}





BOOL IsStreamingBufferPlaying(){

	DWORD dwStatus;
	if( FAILED( g_pDSBuffer->GetStatus( &dwStatus ) ) )
		return E_FAIL;

	return ( dwStatus & DSBSTATUS_PLAYING );
}






//-----------------------------------------------------------------------------
// Name: FillBuffer()
// Desc: Fills the DirectSound buffer with wave data
//-----------------------------------------------------------------------------
HRESULT FillBuffer( BOOL bLooped, DWORD offset )
{
    HRESULT hr;
    VOID*   pbBuffer = NULL;
    DWORD   dwBufferLength;

    if( NULL == g_pDSBuffer )
        return E_FAIL;

    g_bFoundEnd = FALSE;
    g_dwNextWriteOffset = 0; 
    g_dwLastPos  = 0;
    g_dwProgress = offset;

    // Reset the wav file to the beginning
    g_pWaveSoundRead->Reset();
		g_pWaveSoundRead->Advance( offset );
    g_pDSBuffer->SetCurrentPosition( 0 );

    // Lock the buffer down, 
    if( FAILED( hr = g_pDSBuffer->Lock( 0, g_dwBufferSize, 
                                        &pbBuffer, &dwBufferLength, 
                                        NULL, NULL, 0L ) ) )
        return hr;

    // Fill the buffer with wav data 
    if( FAILED( hr = WriteToBuffer( bLooped, pbBuffer, dwBufferLength ) ) )
        return hr;

    // Now unlock the buffer
    g_pDSBuffer->Unlock( pbBuffer, dwBufferLength, NULL, 0 );

    g_dwNextWriteOffset = dwBufferLength; 
    g_dwNextWriteOffset %= g_dwBufferSize; // Circular buffer

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WriteToBuffer()
// Desc: Writes wave data to the streaming DirectSound buffer 
//-----------------------------------------------------------------------------
HRESULT WriteToBuffer( BOOL bLooped, VOID* pbBuffer, DWORD dwBufferLength )
{
    HRESULT hr;
    UINT nActualBytesWritten;

    if( !g_bFoundEnd )
    {
        // Fill the DirectSound buffer with WAV data
        if( FAILED( hr = g_pWaveSoundRead->Read( dwBufferLength, 
                                                 (BYTE*) pbBuffer, 
                                                 &nActualBytesWritten ) ) )           
            return hr;
    }
    else
    {
        // Fill the DirectSound buffer with silence
        FillMemory( pbBuffer, dwBufferLength, 
                    (BYTE)( g_pWaveSoundRead->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ) );
        nActualBytesWritten = dwBufferLength;
    }

    // If the number of bytes written is less than the 
    // amount we requested, we have a short file.
    if( nActualBytesWritten < dwBufferLength )
    {
        if( !bLooped ) 
        {
            g_bFoundEnd = TRUE;

            // Fill in silence for the rest of the buffer.
            FillMemory( (BYTE*) pbBuffer + nActualBytesWritten, 
                        dwBufferLength - nActualBytesWritten, 
                        (BYTE)(g_pWaveSoundRead->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ) );
        }
        else
        {
            // We are looping.
            UINT nWritten = nActualBytesWritten;    // From previous call above.
            while( nWritten < dwBufferLength )
            {  
                // This will keep reading in until the buffer is full. For very short files.
                if( FAILED( hr = g_pWaveSoundRead->Reset() ) )
                    return hr;

                if( FAILED( hr = g_pWaveSoundRead->Read( (UINT)dwBufferLength - nWritten,
                                                          (BYTE*)pbBuffer + nWritten,
                                                          &nActualBytesWritten ) ) )
                    return hr;

                nWritten += nActualBytesWritten;
            } 
        } 
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: StopBuffer()
// Desc: Stop the DirectSound buffer
//-----------------------------------------------------------------------------
HRESULT StopBuffer() 
{
    if( NULL != g_pDSBuffer )
    {
        g_pDSBuffer->Stop();    
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: HandleNotification()
// Desc: Handle the notification that tell us to put more wav data in the 
//       circular buffer
//-----------------------------------------------------------------------------
HRESULT HandleNotification( BOOL bLooped ) 
{
    HRESULT hr;
    VOID* pbBuffer  = NULL;
    DWORD dwBufferLength;

    UpdateProgress();

    // Lock the buffer down
    if( FAILED( hr = g_pDSBuffer->Lock( g_dwNextWriteOffset, g_dwNotifySize, 
                                        &pbBuffer, &dwBufferLength, NULL, NULL, 0L ) ) )
        return hr;

    // Fill the buffer with wav data 
    if( FAILED( hr = WriteToBuffer( bLooped, pbBuffer, dwBufferLength ) ) )
        return hr;

    // Now unlock the buffer
    g_pDSBuffer->Unlock( pbBuffer, dwBufferLength, NULL, 0 );
    pbBuffer = NULL;

    // If the end was found, detrimine if there's more data to play 
    // and if not, stop playing
    if( g_bFoundEnd )
    {
        // We don't want to cut off the sound before it's done playing.
        // if doneplaying is set, the next notification event will post a stop message.
        if( g_pWaveSoundRead->m_ckInRiff.cksize > g_dwNotifySize )
        {
            if( g_dwProgress >= g_pWaveSoundRead->m_ckInRiff.cksize - g_dwNotifySize )
            {
                g_pDSBuffer->Stop();
            }
        }
        else // For short files.
        {
            if( g_dwProgress >= g_pWaveSoundRead->m_ckInRiff.cksize )
            {
                g_pDSBuffer->Stop();
            }
        }
    }

    g_dwNextWriteOffset += dwBufferLength; 
    g_dwNextWriteOffset %= g_dwBufferSize; // Circular buffer

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: UpdateProgress()
// Desc: Update the progress variable to know when the entire buffer was played
//-----------------------------------------------------------------------------
HRESULT UpdateProgress()
{
    HRESULT hr;
    DWORD   dwPlayPos;
    DWORD   dwWritePos;
    DWORD   dwPlayed;

    if( FAILED( hr = g_pDSBuffer->GetCurrentPosition( &dwPlayPos, &dwWritePos ) ) )
        return hr;

    if( dwPlayPos < g_dwLastPos )
        dwPlayed = g_dwBufferSize - g_dwLastPos + dwPlayPos;
    else
        dwPlayed = dwPlayPos - g_dwLastPos;

    g_dwProgress += dwPlayed;
		// g_dwProgress %= g_pWaveSoundRead->m_ckInRiff.cksize;
    g_dwLastPos = dwPlayPos;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreBuffers()
// Desc: Restore lost buffers and fill them up with sound if possible
//-----------------------------------------------------------------------------
HRESULT RestoreBuffers( BOOL bLooped, DWORD offset )
{
    HRESULT hr;

    if( NULL == g_pDSBuffer )
        return S_OK;

    DWORD dwStatus;
    if( FAILED( hr = g_pDSBuffer->GetStatus( &dwStatus ) ) )
        return hr;

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so 
        // the restoring the buffer may fail.  
        // If it does, sleep until DirectSound gives us control.
        do 
        {
            hr = g_pDSBuffer->Restore();
            if( hr == DSERR_BUFFERLOST )
                Sleep( 10 );
        }
        while( hr = g_pDSBuffer->Restore() );

        // if( FAILED( hr = FillBuffer( bLooped, offset ) ) )
        //     return hr;
    }

    return S_OK;
}



// this applies to static and streaming buffers

VOID FreeDirectSound(){
	
	for(DWORD i=0; i < g_numSounds; i++){
		for(DWORD instance=0; instance < g_sounds[i].m_numInstances; instance++){
			SAFE_RELEASE( g_sounds[i].m_instances[instance].pDS3DBuff );
			SAFE_RELEASE( g_sounds[i].m_instances[instance].pDSBuff );
		}
	}
	
	g_numSounds = 0;
	
	// streaming stuff
	if( NULL != g_pWaveSoundRead ){
		g_pWaveSoundRead->Close();
		SAFE_DELETE( g_pWaveSoundRead );
	}
	SAFE_RELEASE( g_pDSNotify );
	SAFE_RELEASE( g_pDSBuffer );
	
	
	SAFE_RELEASE( g_p3DListener );
	SAFE_RELEASE( g_pDS );
	
	// Release COM
	CoUninitialize();
}
