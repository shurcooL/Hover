

#define ED_DEFAULT_SUNLIGHT_DIRECTION		( g_PI / 4 )
#define ED_DEFAULT_SUNLIGHT_PITCH				( g_PI / 4 )



#define MAX_TERRAIN_HEIGHT							0xFFFF // max WORD value

#define ED_DEFAULT_TERRAIN_HEIGHT				MAX_TERRAIN_HEIGHT / 3

#define ED_PASS_ABOVE		1
#define ED_PASS_BELOW		2

#define ED_TEXTUREFILENAME_CURSOR				"editor/cursor.bmp"
#define ED_TEXTUREFILENAME_PASS_ABOVE		"editor/pass_above.bmp"
#define ED_TEXTUREFILENAME_PASS_BELOW		"editor/pass_below.bmp"


#define ED_MAX_MOD_SEGMENTS							100

#define ED_DEFAULT_MOD_RADIUS						10
#define ED_DEFAULT_MOD_PLATEAU_RADIUS		10

#define ED_NUM_UNDO_STEPS								100


#define ED_MIN_PITCH			( 1.0f / 16 * g_PI )
#define ED_MAX_PITCH			( 7.0f / 16 * g_PI )

#define ED_PITCH_INC			( g_PI / 64 )
#define ED_DIRECTION_INC	( g_PI / 32 )

#define NAVDATA_NORMALTERRAIN						0
#define NAVDATA_STARTCOORD							1
#define NAVDATA_FINISHCOORD							2
#define NAVDATA_NORMALCOORD							3
#define NAVDATA_BRANCHCOORD							4
#define NAVDATA_BRANCHENDCOORD					5
#define NAVDATA_FORCEDCONNECTIONCOORD		6
#define NAVDATA_SLOPE										7
#define NAVDATA_SLOPEBASE								8
#define NAVDATA_RACERSTARTPOSITION			9
#define NAVDATA_ALTSTARTCOORD						10

#define MIN_PASSABLE_SLOPE_NORMAL_Y			0.75
#define MAX_FORCEDCONNECTIONDIST				100.0f

#define MAX_NAVCOORDS							5000 // NAVCOORD_NOCOORD is dependent on this
#define MAX_NAVCOORDLOOKUP_NODES	150000


struct MODCOORD {
	INT32		x, z;
	WORD		height;
};


struct UNDOSTEP {
	CRectRegion region;
	DWORD bufferSize;
	WORD* buffer;
};





class CEdApplication : public CMyApplication{

	BOOL						isEditing;

	INT32						cursorX; // adjust with
	INT32						cursorZ; //		arrow keys

	INT32						oldCursorX;
	INT32						oldCursorZ;
	BYTE						typeUnderCursor;

	FLOAT						camPitch;
	FLOAT						camDirection;
	FLOAT						camDist;

	BOOL						modInProgress;
	WORD						currHeight; // adjust with up, down arrow keys
	INT32						currRadius; // adjust with left, right arrow keys
	INT32						currPlateauRadius; // adjust [,]
	DWORD						currSegment; // adjust with <, >
	CRectRegion			modRegion;

	MODCOORD				modCoords[ ED_MAX_MOD_SEGMENTS ];

	UNDOSTEP				undoSteps[ ED_NUM_UNDO_STEPS ];
	DWORD						modCount;
	DWORD						undoBuffersMemUsage;

	BYTE*						terrBaseTypes;
	BYTE*						terrDisplayTypes;
	BYTE						cursorType;
	DWORD						numTerrTypeNodes;
	DWORD						numNavCoordLookupNodes;

	HRESULT Init();
	HRESULT InitDeviceObjects();
	HRESULT	EdInitTrack();

	HRESULT	Move();
	VOID		EdUpdateRacers();

	BOOL		HandleKeyInputEvent( WPARAM );

	VOID		UpdateModInProgress();
	VOID		DisplayMod();
	BOOL		UndoMod();
	VOID		CreateUndoStep();
	VOID		DisplayPoint( MODCOORD );
	VOID		DisplayLine( MODCOORD, MODCOORD );

	VOID		EdSetCamera();

	VOID		UpdateTerrainLightingAndTypes( CRectRegion region );
	WORD		UpdateTerrainTypeRuns( CRectRegion, BOOL );

	HRESULT	Render();

	VOID		Display2DStuff();

	HRESULT	CreateNavStuff();
	VOID		ConnectNavCoord( WORD prev, WORD curr, BYTE* navCoordProperties, BYTE* navData );
	BOOL		IsNavCoordAccessible( INT32 originX, INT32 originZ,
																WORD destCoord, BOOL goDownSlopes , BYTE* navData );
	FLOAT		DistToNavCoord( INT32 originX, INT32 originZ, WORD destCoord );
	FLOAT		DistSqrdToNavCoord( INT32 originX, INT32 originZ, WORD destCoord );
	WORD		DistToStartCoord( WORD currCoord );

	HRESULT	SaveTrack();
	HRESULT	CreateNavMap();
	HRESULT	CreateTerrainBitmap( BYTE* data, CHAR* fileDesc, CHAR* templateFileDesc );

//	HRESULT	CreateGameTerrainTypesBitmap();

	VOID		EdReleaseTrack();
	VOID		DeleteDeviceObjects();
	VOID		FinalCleanup();

public:
	CEdApplication();

};
