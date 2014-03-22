
#define MAX_SOUNDS			6

#define MAX_SOUNDINSTANCES			MAX_RACERS

// #define SOUND_MINDISTANCE				25.0f
#define SOUND_MAXDISTANCE				150.0f
#define SOUND_ROLLOFF						4.0f

#define SOUND_NOADJUST					-2.0f

#define SOUND_UNIVERSALCAPS			DSBCAPS_STATIC | DSBCAPS_MUTE3DATMAXDISTANCE

struct SOUNDINSTANCE{

	IDirectSoundBuffer* pDSBuff;
	IDirectSound3DBuffer* pDS3DBuff;
};



class CSoundHolder{
public:

	CHAR m_name[32];
	BYTE *m_pbWaveData;               // pointer into wave resource (for restore)
	DWORD m_cbWaveSize;               // size of wave data (for restore)
	BOOL m_bLoop;
	DWORD m_baseFrequency;

	DWORD m_numInstances;
	SOUNDINSTANCE m_instances[ MAX_SOUNDINSTANCES ];
	DWORD m_lastInstance;

	CSoundHolder();
	~CSoundHolder();

	HRESULT Create( CHAR* name, DWORD numInstances, BOOL bLoop, FLOAT minDist, DWORD caps );
};



HRESULT InitDirectSound( HWND hWnd );

CSoundHolder* GetSound( CHAR* name );

HRESULT AddSound( CHAR* name, DWORD numInstances, BOOL bLoop, FLOAT minDist, DWORD caps );

BOOL IsSoundInstancePlaying( CHAR* name, DWORD instance );

VOID AdjustSoundInstance2D( CHAR* name, DWORD instance,
													 FLOAT frequencyScale, FLOAT pan, FLOAT volume );

VOID AdjustSoundInstance3D( CHAR* name, DWORD instance,
													 D3DVECTOR position, D3DVECTOR velocity );

HRESULT PlaySoundInstance( CHAR* name, DWORD instance );

HRESULT StopSoundInstance( CHAR* name, DWORD instance );

VOID StopAllSoundInstances();

HRESULT PlayFreeSoundInstance( CHAR* name,
															FLOAT frequencyScale, FLOAT pan, FLOAT volume,
															D3DVECTOR position, D3DVECTOR velocity );

VOID UpdateListener( D3DVECTOR dir, D3DVECTOR position, D3DVECTOR velocity );

VOID FreeDirectSound();

//// streaming buffer stuff
HRESULT	LoadWaveFile( TCHAR* strFileName );
HRESULT	PlayBuffer( BOOL bLooped, DWORD offset );
BOOL		IsStreamingBufferPlaying();
VOID		AdjustStreamingSoundBuffer( FLOAT volume );
HRESULT	StopBuffer();
// BOOL     IsBufferSignaled();
HRESULT	HandleNotification( BOOL bLooped );
