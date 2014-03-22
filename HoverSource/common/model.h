

#define MAX_MODELS						32

#define MAX_MODEL_SIZE				24

#define MAX_MODEL_MESHES			6 // # of meshes PER model

#define MODELVERTEXDESC ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 )

#define COLORVERTEXDESC ( D3DFVF_XYZ | D3DFVF_DIFFUSE )



struct MODELMESH {
	DWORD						numVertices;
	D3DVERTEX*			vertices;
	DWORD						numIndices;
	WORD*						indices;
	CHAR*						textureName;
	D3DCOLORVALUE		specularColor;
	FLOAT						specularPower;
};



class CModel {
public:
	CHAR						name[32];     // filename
	DWORD						numMeshes;
	MODELMESH				meshes[ MAX_MODEL_MESHES ];

	HRESULT					Load( CHAR*, FLOAT scale, BOOL isSymmetrical );
	VOID						Release();

	CModel();
};




HRESULT AddModel( CHAR*, FLOAT scale, BOOL isSymmetrical );
CModel* GetModel( CHAR* );
VOID		DestroyModels();
