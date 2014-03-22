
#define TERR_SCALE									1
#define TERR_HEIGHT_SCALE						( 1.0f / 32 )
#define TERR_TEXTURE_SCALE					( 1.0f / 20 )
#define TERR_SHADOWTEXTURE_SCALE		( 1.0f / 24 ) // from max_model_size


#define TERR_LIGHT_AMBIENT					96
#define TERR_LIGHT_VARIANCE					( 255.0f - TERR_LIGHT_AMBIENT )


#define MAX_REAL_TERRAIN_TYPES		8
#define NUM_ED_TYPES							4
#define MAX_TERRAIN_TYPES					( MAX_REAL_TERRAIN_TYPES + NUM_ED_TYPES )


#define MAX_TERRAINTYPE_NODES			40000 // make bigger


#define TERRLVERTEXDESC ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 )
#define TERRTLVERTEXDESC ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 )

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

#define MAX_RENDERED_GROUPS_WIDTH		33 // limited by maxvertices
#define MAX_RENDERED_GROUPS					( MAX_RENDERED_GROUPS_WIDTH * MAX_RENDERED_GROUPS_WIDTH )
// MAX_RENDERED_COORDS must be less than D3D's MAXVERTICES (64K)
#define MAX_RENDERED_COORDS					( ( MAX_RENDERED_GROUPS_WIDTH * TRIGROUP_WIDTH + 1 ) * ( MAX_RENDERED_GROUPS_WIDTH * TRIGROUP_WIDTH + 1 ) )



struct TRIGROUP {
	DWORD data[ TRIGROUP_NUM_DWORDS ];
};


#pragma pack(1) 

struct TERRCOORD { 
	WORD height;
	BYTE lightIntensity;
};

#pragma pack()

 
struct TERRTYPENODE {
	BYTE type;
	WORD nextStartX;
	WORD next;
};

struct TERRTLVERTEX { 
	D3DVALUE sx, sy, sz; 
	D3DVALUE rhw; 
	D3DCOLOR color; 
	D3DCOLOR specular;
	D3DVALUE tu, tv; 
};

struct TERRLVERTEX {
	D3DVALUE x, y, z;
	D3DCOLOR color;
	D3DCOLOR specular;
	D3DVALUE tu, tv;
};


struct NAVCOORD {
	WORD x, z;
	WORD distToStartCoord; // decider at forks, and determines racers' rank/place
	WORD next;
	WORD alt;
};

struct NAVCOORDLOOKUPNODE {
	WORD navCoord;
	WORD nextStartX;
	WORD next;
};



struct TRACK {

	DWORD						trackNum;

	DWORD						width;
	DWORD						depth;
	DWORD 					numTerrCoords;
	TERRCOORD*			terrCoords;
	CRectRegion			entireRegion;

	DWORD						triGroupsWidth;
	DWORD						triGroupsDepth;
	DWORD						numTriGroups;
	TRIGROUP*				triGroups;

	D3DCOLOR				bgColor;

	FLOAT						sunlightDirection;
	FLOAT						sunlightPitch;

	D3DVECTOR				racerStartPositions[ MAX_RACERS ];

	DWORD						numNavCoords;
	NAVCOORD*				navCoords;
	NAVCOORDLOOKUPNODE* navCoordLookupRuns;
	NAVCOORDLOOKUPNODE* navCoordLookupNodes;
	WORD						trackLength;

	DWORD						numTerrTypes;
	WORD* 					terrTypeIndices[ MAX_TERRAIN_TYPES ];
	WORD*						terrTypeBlendIndices[ MAX_TERRAIN_TYPES ];
	WORD*						terrTypeBlendOnIndices[ MAX_TERRAIN_TYPES ];
	CHAR						terrTypeTextureFilenames[ MAX_TERRAIN_TYPES ][ 32 ];
	TERRTYPENODE*		terrTypeRuns;
	TERRTYPENODE*		terrTypeNodes;

	CRectRegion			visibleTerrCoords;
	CRectRegion			renderedTriGroups;
	CRectRegion			renderedTerrCoords;

	LPDIRECT3DVERTEXBUFFER7		terrVB;
	LPDIRECT3DVERTEXBUFFER7		terrTransformedVB;

};




struct TRACKFILEHEADER {
	FLOAT			sunlightDirection, sunlightPitch;
	D3DVECTOR	racerStartPositions[ 8 ]; // MAX_RACERS ];
	WORD			numTerrTypes;
	WORD			numTerrTypeNodes;
	WORD			numNavCoords;
	WORD			numNavCoordLookupNodes;
	WORD			width, depth;
};
