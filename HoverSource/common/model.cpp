
#include "common.h"


DWORD						numModels=0;
CModel					models[ MAX_MODELS ];




CModel::CModel(){

	MODELMESH* pMesh = meshes;

	for(DWORD i=0; i < MAX_MODEL_MESHES; i++){
		pMesh->indices = NULL;
		pMesh->vertices = NULL;
		pMesh->textureName = NULL;
	}
}




HRESULT AddModel( CHAR* fileName, FLOAT scale, BOOL isSymmetrical ){

	if( NULL == GetModel( fileName ) ){

		if( numModels >= MAX_MODELS ){
			MyAlert( NULL, "Too many models" );
			return E_FAIL;
		}

		if( FAILED( models[numModels].Load( fileName, scale, isSymmetrical ) ) ){
			return E_FAIL;
		}
		
		numModels++;
	}

	return S_OK;
}



CModel* GetModel( CHAR* fileName ){

	for(DWORD i=0; i < numModels; i++){
		if( 0 == strcmp( fileName, models[i].name ) ){
			return &models[i];
		}
	}

	return NULL;
}



VOID ReleaseModels(){

	CModel* pModel = models;

	for(DWORD i=0; i < numModels; i++, pModel++){
		pModel->Release();
	}
}



inline BOOL CompareVectors( D3DVERTEX* v1, D3DVERTEX* v2 ){
	return v1->x == v2->x && v1->y == v2->y && v1->z == v2->z;
}




HRESULT CModel::Load( CHAR* fileName, FLOAT scale, BOOL isSymmetrical ){
										 
//	FLOAT scale = VEHICLE_MODEL_SCALE;
//	BOOL isSymmetrical = TRUE; 
	
	CHAR filePath[256];
	CHAR meshStr[32];
	CHAR str[10];
	CD3DFile* pFile;
	MODELMESH* pMesh;
	DWORD num;
	WORD* pIndex;
	WORD* pMirroredIndex;
	DWORD vert, ind, ind_r, edge, face, face_r, mesh, mesh_r;
	D3DVERTEX* pVertex;
	D3DVERTEX* pMirroredVertex;
	D3DVECTOR* pFaceNormal;

	
	strcpy( filePath, MODEL_DIRECTORY );
	strcat( filePath, fileName );
		
	pFile = new CD3DFile();
	if( FAILED( pFile->Load( filePath ) ) ){
		CHAR errorMsg[512];
		strcpy( errorMsg, "Could not load file: " );
		strcat( errorMsg, filePath );
		MyAlert( NULL, errorMsg );
		return E_FAIL;
	}
		
	strcpy( name, fileName );

	FLOAT centerZ = 8.5 * scale;

	pFile->Scale( scale );
		
	numMeshes=0;
		
	while( TRUE ){

		pMesh = &meshes[numMeshes];

		strcpy( meshStr, "Mesh" );
		_itoa( numMeshes, str, 10 );
		strcat( meshStr, str );
			
		if( FAILED( pFile->GetMeshVertices( meshStr, NULL, &num ) ) ){
			break; // no more hull sections for this model
		}
			
		if( isSymmetrical ){
			pMesh->numVertices = num * 2;
		}
		else{
			pMesh->numVertices = num;
		}

		pMesh->vertices = new D3DVERTEX[ pMesh->numVertices ];

		if( NULL == pMesh->vertices ){
			MyAlert( NULL, "Out of memory" );
			return E_FAIL;
		}
		pFile->GetMeshVertices( meshStr, &pVertex, NULL );
		CopyMemory( pMesh->vertices, pVertex, sizeof(D3DVERTEX) * num );

		if( isSymmetrical ){
			pVertex = pMesh->vertices;
			pMirroredVertex = pVertex + num;
			for(vert=0; vert < num; vert++, pVertex++, pMirroredVertex++){
				pMirroredVertex->x = -pVertex->x;
				pMirroredVertex->y = pVertex->y;
				pMirroredVertex->z = pVertex->z;
					
				pMirroredVertex->nx = -pVertex->nx;
				pMirroredVertex->ny = pVertex->ny;
				pMirroredVertex->nz = pVertex->nz;
					
				pMirroredVertex->tu = pVertex->tu;
				pMirroredVertex->tv = pVertex->tv;
			}
		}
			
		pVertex = pMesh->vertices;
			
		pVertex = pMesh->vertices;
		D3DVERTEX tempVertex;
		for(vert=0; vert < pMesh->numVertices; vert++, pVertex++){ 
			tempVertex = *pVertex;
			tempVertex.z -= centerZ;
				
			pVertex->x = tempVertex.z; // rotate 90 degrees ( x & z )
			pVertex->y = tempVertex.y;
			pVertex->z = -tempVertex.x;
				
			pVertex->nx = -tempVertex.nz; // D3DFileObject thinks polys are cw, not ccw
			pVertex->ny = -tempVertex.ny;
			pVertex->nz = tempVertex.nx;
				
			pVertex->tv = -tempVertex.tv; // flip texture coords vertically
		}
			
			
		pFile->GetMeshIndices( meshStr, NULL, &num );
		if( isSymmetrical ){
			pMesh->numIndices = num * 2;
		}
		else{
			pMesh->numIndices = num;
		}

		pMesh->indices = new WORD[ pMesh->numIndices ];
		if( NULL == pMesh->indices ){
			MyAlert( NULL, "Out of memory" );
			return E_FAIL;
		}
		pFile->GetMeshIndices( meshStr, &pIndex, NULL );
		CopyMemory( pMesh->indices, pIndex, sizeof(WORD) * num );
				
		if( isSymmetrical ){
			pIndex = pMesh->indices;
			pMirroredIndex = pIndex + num;
			WORD halfNumVertices = pMesh->numVertices / 2;
			for(ind=0; ind < num; ind+=3, pIndex+=3, pMirroredIndex+=3){
				*( pMirroredIndex + 0 ) = *( pIndex + 0 ) + halfNumVertices;
				*( pMirroredIndex + 1 ) = *( pIndex + 2 ) + halfNumVertices;
				*( pMirroredIndex + 2 ) = *( pIndex + 1 ) + halfNumVertices;
			}

			// straighten normals for vertices at middle
			pVertex = pMesh->vertices;
			for(vert=0; vert < pMesh->numVertices; vert++, pVertex++){ 
				if( pVertex->z == 0.0f ){
					pVertex->nz = 0.0f;
					*(D3DVECTOR *)(&pVertex->nx) = Normalize( *(D3DVECTOR *)(&pVertex->nx) );
				}
			}
		}
		
		D3DMATERIAL7 mtrl;
		pFile->GetMeshMaterial( meshStr, &mtrl );
		pMesh->specularColor = mtrl.specular;
		pMesh->specularPower = mtrl.power;

		CHAR* str;
		pFile->GetMeshTextureName( meshStr, &str );
		if( *str != NULL ){
			pMesh->textureName = strdup( str );
			if( FAILED( AddTexture( pMesh->textureName ) ) ){
				return E_FAIL;
			}
		}
		numMeshes++;
	}
		
	if( !numMeshes ){
		CHAR errorMsg[512];
		strcpy( errorMsg, "Invalid data in file: " );
		strcat( errorMsg, filePath );
		MyAlert( NULL, errorMsg );
		return E_FAIL;
	}


	delete pFile;
		
	return S_OK;
}





VOID CModel::Release(){

	MODELMESH* pMesh = meshes;

	for(DWORD i=0; i < numMeshes; i++){
		// SAFE_DELETE( pMesh->vertices );
		// SAFE_DELETE( pMesh->indices );
		SAFE_ARRAYDELETE( pMesh->vertices );
		SAFE_ARRAYDELETE( pMesh->indices );
		//if( pMesh->vertices ) delete [] pMesh->vertices
	}
}





VOID DestroyModels(){

	for(DWORD i=0; i > numModels; i++){
		models[i].Release();
	}
	numModels = 0;
}