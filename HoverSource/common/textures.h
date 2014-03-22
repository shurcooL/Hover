

#define MAX_TEXTURES		32 // # of textures to use for ALL models

#define SHADOW_AMBIENT				0.5f
#define SHADOWTEXTURE_SIZE		256

#define STATSTEXTURE_FILENAME		"stats.bmp"
#define STATSTEXTURE_ALPHA			128 // NOT USED




class CTextureHolder{
public:
	CHAR		name[32];     // image filename

	DWORD   m_dwWidth;
	DWORD   m_dwHeight;
	DWORD   m_dwBPP;
	DWORD   m_dwFlags;
	BOOL		m_bHasMyAlpha;

	HBITMAP m_hbmBitmap;       // Bitmap containing texture image
	LPDIRECTDRAWSURFACE7 m_pddsSurface; // Surface of the texture

	HRESULT LoadBitmapFile( TCHAR* strPathname );
	HRESULT Restore( LPDIRECT3DDEVICE7 pd3dDevice );
	HRESULT CopyBitmapToSurface();

	CTextureHolder();
	~CTextureHolder();
};



HRESULT AddTexture( CHAR* );
HRESULT AddStatsTexture();
CTextureHolder* GetTexture( CHAR* );
HRESULT RestoreTextures();
VOID		ReleaseTextures();
VOID		DestroyTextures();



struct SHADOWTEXTURE {

	D3DVIEWPORT7					vp;
	LPDIRECTDRAWSURFACE7	pddsSurface;
};