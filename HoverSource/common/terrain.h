
#define TERR_SCALE									1
#define TERR_HEIGHT_SCALE						( 1.0f / 32 )
#define TERR_TEXTURE_SCALE					( 1.0f / 12 )
#define TERR_SHADOWTEXTURE_SCALE		( 1.0f / 24 ) // from max_model_size

#define TRI_LEFT		0xF0 // 1111 0000
#define TRI_RIGHT 	0x0F // 0000 1111

#define TRI_TYPE		0xC // 1100
#define TRI_SIZE		0x3 // 0011

#define TRI_LEFT_DEFAULT		0x80 // 1000 0000 type 2, size 0 
#define TRI_RIGHT_DEFAULT 	0x08 // 0000 1000 type 2, size 0


#define MAX_TERRAIN_HEIGHT			0xFFFF // max WORD value


#define MAX_REAL_TERRAIN_TYPES		8
#define NUM_ED_TYPES							4
#define MAX_TERRAIN_TYPES					( MAX_REAL_TERRAIN_TYPES + NUM_ED_TYPES )


#define TERR_LIGHT_AMBIENT					96
#define TERR_LIGHT_VARIANCE					( 255.0f - TERR_LIGHT_AMBIENT )

//#define MAX_TERRAIN_TYPE_NODES	sizeof(WORD)
#define MAX_TERRAIN_TYPE_NODES			10000

#define TERRLVERTEXDESC ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
#define TERRTLVERTEXDESC ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1 )

#define TERRSHADOWVERTEXDESC ( D3DFVF_XYZ | D3DFVF_TEX1 )

#define TRIGROUP_WIDTH							16
#define TRIGROUP_CENTERX						( TRIGROUP_WIDTH / 2 )
#define TRIGROUP_CENTERZ						( TRIGROUP_WIDTH / 2 )
#define TRIGROUP_WIDTHSHIFT					4
#define TRIGROUP_AREA								( TRIGROUP_WIDTH * TRIGROUP_WIDTH )
#define TRIGROUP_NUM_DEPTHS					9
#define TRIGROUP_MAX_DEPTH					( TRIGROUP_NUM_DEPTHS - 1 )
#define TRIGROUP_NUM_BITS_USED			510
#define TRIGROUP_NUM_DWORDS					( ( TRIGROUP_NUM_BITS_USED + 2 ) / 32 )
#define TRIGROUP_NUM_BITS						1022

#define MAX_RENDERED_GROUPS_HEIGHT	7
#define MAX_RENDERED_GROUPS_WIDTH		7
#define MAX_RENDERED_GROUPS					( MAX_RENDERED_GROUPS_HEIGHT * MAX_RENDERED_GROUPS_WIDTH )
#define MAX_RENDERED_COORDS					( MAX_RENDERED_GROUPS * TRIGROUP_AREA )



struct TRIGROUP {
	DWORD data[ TRIGROUP_NUM_DWORDS ];
};

struct TERRCOORD { 
	WORD height;
	BYTE lightIntensity;
};

struct TERRTYPENODE {
	BYTE type;
	WORD nextStartX;
	WORD next;
};

struct TERRTLVERTEX { 
	D3DVALUE sx, sy, sz; 
	D3DVALUE rhw; 
	D3DCOLOR color; 
	D3DVALUE tu, tv; 
};

struct TERRLVERTEX {
	D3DVALUE x, y, z;
	D3DCOLOR color;
	D3DVALUE tu, tv;
};


struct TERRAIN {

	DWORD						width;
	DWORD						depth;
	DWORD 					area;
	TERRCOORD*			coords;

	FLOAT						triGroupMaxError;
	DWORD						triGroupsWidth;
	DWORD						triGroupsDepth;
	DWORD						triGroupsArea;
	TRIGROUP*				triGroups;

	WORD* 					typeIndices[ MAX_TERRAIN_TYPES ];
	WORD*						typeBlendIndices[ MAX_TERRAIN_TYPES ];
	WORD*						typeBlendOnIndices[ MAX_TERRAIN_TYPES ];

	LPDIRECT3DVERTEXBUFFER7		verticesVB;
	LPDIRECT3DVERTEXBUFFER7		transformedVerticesVB;

	DWORD						numTypes;
	CHAR						textureFileNames[ MAX_TERRAIN_TYPES ][ 32 ];
	//CTextureHolder	terrTextures[ MAX_TERRAIN_TYPES ];
	TERRTYPENODE*		typeRuns;
	TERRTYPENODE*		typeNodes;
	CRectRegion			visibleCoords;
	CRectRegion			renderedTriGroups;
	CRectRegion			renderedCoords;

//	CRectRegion			renderedTriGroups;
//	CRectRegion			renderedCoords;
};




struct TRACK {

	DWORD						width;
	DWORD						depth;
	DWORD 					area;
	TERRCOORD*			coords;

	FLOAT						triGroupMaxError;
	DWORD						triGroupsWidth;
	DWORD						triGroupsDepth;
	DWORD						triGroupsArea;
	TRIGROUP*				triGroups;

	WORD* 					typeIndices[ MAX_TERRAIN_TYPES ];
	WORD*						typeBlendIndices[ MAX_TERRAIN_TYPES ];
	WORD*						typeBlendOnIndices[ MAX_TERRAIN_TYPES ];

	LPDIRECT3DVERTEXBUFFER7		verticesVB;
	LPDIRECT3DVERTEXBUFFER7		transformedVerticesVB;

	DWORD						numTypes;
	CHAR						textureFileNames[ MAX_TERRAIN_TYPES ][ 32 ];
	//CTextureHolder	terrTextures[ MAX_TERRAIN_TYPES ];
	TERRTYPENODE*		typeRuns;
	TERRTYPENODE*		typeNodes;
	CRectRegion			visibleCoords;
	CRectRegion			renderedTriGroups;
	CRectRegion			renderedCoords;


struct NAVCOORD {
	WORD x, z;
	WORD distToFinish; // decider at forks, and determines racers' rank/place
	WORD next;
	WORD alt;
};




struct TRACKFILEHEADER {
	FLOAT			sunlightDirection, sunlightPitch;
	//D3DVECTOR	racerStartPositions[ MAX_RACERS ];

	WORD			numTypes;
	WORD			numTypeNodes;
	WORD			numNavCoords;
	WORD			numNavCoordLookupNodes;
	WORD			width, depth;
};
