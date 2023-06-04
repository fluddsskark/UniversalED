/*=============================================================================
	UnMeshEd.cpp: Unreal editor mesh code
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EditorPrivate.h"
#include "UnRender.h"

/*-----------------------------------------------------------------------------
	Data types for importing James' creature meshes.
-----------------------------------------------------------------------------*/

/* debug logging */
#undef  NLOG                
#define NLOG(func) {}
//#define NLOG(func) func


// James mesh info.
struct FJSDataHeader
{
	_WORD	NumPolys;
	_WORD	NumVertices;
	_WORD	BogusRot;
	_WORD	BogusFrame;
	DWORD	BogusNormX,BogusNormY,BogusNormZ;
	DWORD	FixScale;
	DWORD	Unused1,Unused2,Unused3;
};

// James animation info.
struct FJSAnivHeader
{
	_WORD	NumFrames;		// Number of animation frames.
	_WORD	FrameSize;		// Size of one frame of animation.
};

// Mesh triangle.
struct FJSMeshTri
{
	_WORD		iVertex[3];		// Vertex indices.
	BYTE		Type;			// James' mesh type.
	BYTE		Color;			// Color for flat and Gouraud shaded.
	FMeshUV		Tex[3];			// Texture UV coordinates.
	BYTE		TextureNum;		// Source texture offset.
	BYTE		Flags;			// Unreal mesh flags (currently unused).
};



/*-----------------------------------------------------------------------------
	Import functions.
-----------------------------------------------------------------------------*/

// Mesh sorting function.
static QSORT_RETURN CDECL CompareTris( const FMeshTri* A, const FMeshTri* B )
{
	if     ( (A->PolyFlags&PF_Translucent) > (B->PolyFlags&PF_Translucent) ) return  1;
	else if( (A->PolyFlags&PF_Translucent) < (B->PolyFlags&PF_Translucent) ) return -1;
	else if( A->TextureIndex               > B->TextureIndex               ) return  1;
	else if( A->TextureIndex               < B->TextureIndex               ) return -1;
	else if( A->PolyFlags                  > B->PolyFlags                  ) return  1;
	else if( A->PolyFlags                  < B->PolyFlags                  ) return -1;
	else                                                                     return  0;
}



//
// Import a mesh from James' editor.  Uses file commands instead of object
// manager.  Slow but works fine.
//
void UEditorEngine::meshImport
(
	const TCHAR*		MeshName,
	UObject*			InParent,
	const TCHAR*		AnivFname, 
	const TCHAR*		DataFname,
	UBOOL				Unmirror,
	UBOOL				ZeroTex,
	INT					UnMirrorTex,
	ULODProcessInfo*	LODInfo
)
{
	guard(UEditorEngine::meshImport);

	UMesh*			Mesh;
	FArchive*		AnivFile;
	FArchive*		DataFile;
	FJSDataHeader	JSDataHdr;
	FJSAnivHeader	JSAnivHdr;
	INT				i;
	INT				Ok = 0;
	INT				MaxTextureIndex = 0;

	debugf( NAME_Log, TEXT("Importing %s"), MeshName );
	GWarn->BeginSlowTask( TEXT("Importing mesh"), 1, 0 );
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Reading files") );

	// Open James' animation vertex file and read header.
	AnivFile = GFileManager->CreateFileReader( AnivFname, 0, GLog );
	if( !AnivFile )
	{
		debugf( NAME_Log, TEXT("Error opening file %s"), AnivFname );
		goto Out1;
	}
	AnivFile->Serialize( &JSAnivHdr, sizeof(FJSAnivHeader) );
	if( AnivFile->IsError() )
	{
		debugf( NAME_Log, TEXT("Error reading %s"), AnivFname );
		goto Out2;
	}

	// Open James' mesh data file and read header.
	DataFile = GFileManager->CreateFileReader( DataFname, 0, GLog );
	if( !DataFile )
	{
		debugf( NAME_Log, TEXT("Error opening file %s"), DataFname );
		goto Out2;
	}
	DataFile->Serialize( &JSDataHdr, sizeof(FJSDataHeader) );
	if( DataFile->IsError() )
	{
		debugf( NAME_Log, TEXT("Error reading %s"), DataFname );
		goto Out3;
	}

	// Allocate mesh or lodmesh object.
#if 1 //NEW
	if( LODInfo->LevelOfDetail == -1 )
	{
		LODInfo->LevelOfDetail = 0;
		Mesh = new( InParent, MeshName, RF_Public|RF_Standalone )UWaveMesh( JSDataHdr.NumPolys, JSDataHdr.NumVertices, JSAnivHdr.NumFrames );
	}
	else
#endif
	if( !LODInfo->LevelOfDetail )
		Mesh = new( InParent, MeshName, RF_Public|RF_Standalone )UMesh( JSDataHdr.NumPolys, JSDataHdr.NumVertices, JSAnivHdr.NumFrames );
	else 
		Mesh = new( InParent, MeshName, RF_Public|RF_Standalone )ULodMesh( JSDataHdr.NumPolys, JSDataHdr.NumVertices, JSAnivHdr.NumFrames );

	// Display summary info.
	debugf(NAME_Log,TEXT(" * Triangles  %i"),Mesh->Tris.Num());
	debugf(NAME_Log,TEXT(" * Vertices   %i"),Mesh->FrameVerts);
	debugf(NAME_Log,TEXT(" * AnimFrames %i"),Mesh->AnimFrames);
	debugf(NAME_Log,TEXT(" * FrameSize  %i"),JSAnivHdr.FrameSize);
	debugf(NAME_Log,TEXT(" * AnimSeqs   %i"),Mesh->AnimSeqs.Num());

	// Import mesh triangles.
	debugf( NAME_Log, TEXT("Importing triangles") );
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Importing Triangles") );
	DataFile->Seek( DataFile->Tell() + 12 );
	for( i=0; i<Mesh->Tris.Num(); i++ )
	{
		guard(Importing triangles);

		// Load triangle.
		FJSMeshTri Tri;
		DataFile->Serialize( &Tri, sizeof(Tri) );
		if( DataFile->IsError() )
		{
			debugf( NAME_Log, TEXT("Error processing %s"), DataFname );
			goto Out4;
		}
		if( Unmirror )
		{
			Exchange( Tri.iVertex[1], Tri.iVertex[2] );
			Exchange( Tri.Tex    [1], Tri.Tex    [2] );
			if( Tri.TextureNum == UnMirrorTex )
			{
				Tri.Tex[0].U = 255 - Tri.Tex[0].U;
				Tri.Tex[1].U = 255 - Tri.Tex[1].U;
				Tri.Tex[2].U = 255 - Tri.Tex[2].U;
			}
		}
		if( ZeroTex )
		{
			Tri.TextureNum = 0;
		}

		// Copy to Unreal structures.
		Mesh->Tris(i).iVertex[0]	= Tri.iVertex[0];
		Mesh->Tris(i).iVertex[1]	= Tri.iVertex[1];
		Mesh->Tris(i).iVertex[2]	= Tri.iVertex[2];
		Mesh->Tris(i).Tex[0]		= Tri.Tex[0];
		Mesh->Tris(i).Tex[1]		= Tri.Tex[1];
		Mesh->Tris(i).Tex[2]		= Tri.Tex[2];
		Mesh->Tris(i).TextureIndex	= Tri.TextureNum;
		MaxTextureIndex = Max<INT>(MaxTextureIndex,Tri.TextureNum);		

		// Set style based on triangle type.
		DWORD PolyFlags=0;
#if 1 //NEW
		if     ( (Tri.Type&15)==0  ) PolyFlags = 0;
		else if( (Tri.Type&15)==1  ) PolyFlags = PF_TwoSided;
		else if( (Tri.Type&15)==2  ) PolyFlags = PF_Translucent | PF_TwoSided;
		else if( (Tri.Type&15)==3  ) PolyFlags = PF_Masked      | PF_TwoSided;
		else if( (Tri.Type&15)==4  ) PolyFlags = PF_Modulated   | PF_TwoSided;
		else if( (Tri.Type&15)==5  ) PolyFlags = PF_NoSmooth;
		else if( (Tri.Type&15)==6  ) PolyFlags = PF_Environment;
		else if( (Tri.Type&15)==7  ) PolyFlags = 0;
		else if( (Tri.Type&15)==8  ) PolyFlags = PF_Invisible   | PF_TwoSided;
		else if( (Tri.Type&15)==9  ) PolyFlags = PF_NoSmooth    | PF_TwoSided;
		else if( (Tri.Type&15)==10 ) PolyFlags = PF_Environment | PF_TwoSided;
		else if( (Tri.Type&15)==11 ) PolyFlags = PF_Environment | PF_Translucent | PF_TwoSided;
		else if( (Tri.Type&15)==12 ) PolyFlags = PF_NoSmooth    | PF_Translucent | PF_TwoSided;
		else if( (Tri.Type&15)==13 ) PolyFlags = PF_Environment | PF_Modulated   | PF_TwoSided;
		else if( (Tri.Type&15)==14 ) PolyFlags = PF_NoSmooth    | PF_Modulated   | PF_TwoSided;
		else if( (Tri.Type&15)==15 ) PolyFlags = 0;

		// Handle effects.
		if     ( Tri.Type&16       ) PolyFlags |= PF_Unlit;
		if     ( Tri.Type&32       ) PolyFlags |= PF_Flat;
		if     ( Tri.Type&64       ) PolyFlags |= PF_FakeSpecular;
		if     ( Tri.Type&128      ) PolyFlags |= PF_MultiTexture;
#else
		if     ( (Tri.Type&15)==MTT_Normal         ) PolyFlags |= 0;
		else if( (Tri.Type&15)==MTT_NormalTwoSided ) PolyFlags |= PF_TwoSided;
		else if( (Tri.Type&15)==MTT_Modulate       ) PolyFlags |= PF_TwoSided | PF_Modulated;
		else if( (Tri.Type&15)==MTT_Translucent    ) PolyFlags |= PF_TwoSided | PF_Translucent;
		else if( (Tri.Type&15)==MTT_Masked         ) PolyFlags |= PF_TwoSided | PF_Masked;
		else if( (Tri.Type&15)==MTT_Placeholder    ) PolyFlags |= PF_TwoSided | PF_Invisible;

		// Handle effects.
		if     ( Tri.Type&MTT_Unlit             ) PolyFlags |= PF_Unlit;
		if     ( Tri.Type&MTT_Flat              ) PolyFlags |= PF_Flat;
		if     ( Tri.Type&MTT_Environment       ) PolyFlags |= PF_Environment;
		if     ( Tri.Type&MTT_NoSmooth          ) PolyFlags |= PF_NoSmooth;

		// per-pixel Alpha flag ( Reuses Flatness triangle tag and Wavy engine tag...)
		if     ( Tri.Type&MTT_Flat				) PolyFlags |= PF_BigWavy; 
#endif

		// Set flags.
		Mesh->Tris(i).PolyFlags = PolyFlags;

		unguard;
	}

	// Sort triangles by texture and flags.
	appQsort( &Mesh->Tris(0), Mesh->Tris.Num(), sizeof(Mesh->Tris(0)), (QSORT_COMPARE)CompareTris );

	// Texture LOD.
	for( i=0; i<MaxTextureIndex+1; i++ )
	{
		Mesh->TextureLOD.AddItem( 1.0f );
	}
	while( MaxTextureIndex >= Mesh->Textures.Num() )
		Mesh->Textures.AddItem( NULL );

	debugf(TEXT(" Mesh Textures: %i LodItems: %i"),Mesh->Textures.Num(),Mesh->TextureLOD.Num());

	// Import mesh vertices.
	debugf( NAME_Log, TEXT("Importing vertices") );
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Importing Vertices") );
	for( i=0; i<Mesh->AnimFrames; i++ )
	{
		guard(Importing animation frames);
		AnivFile->Serialize( &Mesh->Verts(i * Mesh->FrameVerts), sizeof(FMeshVert) * Mesh->FrameVerts );
		if( AnivFile->IsError() )
		{
			debugf( NAME_Log, TEXT("Vertex error in mesh %s, frame %i: expecting %i verts"), AnivFname, i, Mesh->FrameVerts );
			break;
		}
		if( Unmirror )
			for( INT j=0; j<Mesh->FrameVerts; j++ )
				Mesh->Verts(i * Mesh->FrameVerts + j).X *= -1;
		AnivFile->Seek( AnivFile->Tell() + JSAnivHdr.FrameSize - Mesh->FrameVerts * sizeof(FMeshVert) );
		unguard;
	}

	// Build list of triangles per vertex.
	if( !LODInfo->LevelOfDetail )
	{
		GWarn->StatusUpdatef( i, Mesh->FrameVerts, TEXT("%s"), TEXT("Linking mesh") );
		for( i=0; i<Mesh->FrameVerts; i++ )
		{
			guard(ImportingVertices);
			Mesh->Connects(i).NumVertTriangles = 0;
			Mesh->Connects(i).TriangleListOffset = Mesh->VertLinks.Num();
			for( INT j=0; j<Mesh->Tris.Num(); j++ )
			{
				for( INT k=0; k<3; k++ )
				{
					if( Mesh->Tris(j).iVertex[k] == i )
					{
						Mesh->VertLinks.AddItem(j);
						Mesh->Connects(i).NumVertTriangles++;
					}
				}
			}
			unguard;
		}
		debugf( NAME_Log, TEXT("Made %i links"), Mesh->VertLinks.Num() );
	}

	// Compute per-frame bounding volumes plus overall bounding volume.
	meshBuildBounds(Mesh);

	// Process for LOD. Called last; needs the mesh bounds from above.
	if( LODInfo->LevelOfDetail )
		meshLODProcess( (ULodMesh*)Mesh, LODInfo );

	// Exit labels.
	Ok = 1;
	Out4: if (!Ok) {delete Mesh;}
	Out3: delete DataFile;
	Out2: delete AnivFile;
	Out1: GWarn->EndSlowTask();
	unguard;
}

void UEditorEngine::meshDropFrames
(
	UMesh*			Mesh,
	INT				StartFrame,
	INT				NumFrames
)
{
	guard(UEditorEngine::meshDropFrames);
	Mesh->Verts.Remove( StartFrame*Mesh->FrameVerts, NumFrames*Mesh->FrameVerts );
	Mesh->AnimFrames -= NumFrames;
	unguard;
}

/*-----------------------------------------------------------------------------
	Bounds.
-----------------------------------------------------------------------------*/

//
// Build bounding boxes for each animation frame of the mesh,
// and one bounding box enclosing all animation frames.
//
void UEditorEngine::meshBuildBounds( UMesh* Mesh )
{
	guard(UEditorEngine::meshBuildBounds);
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Bounding mesh") );

	// Bound all frames.
	TArray<FVector> AllFrames;
	for( INT i=0; i<Mesh->AnimFrames; i++ )
	{
		TArray<FVector> OneFrame;
		for( INT j=0; j<Mesh->FrameVerts; j++ )
		{
			FVector Vertex = Mesh->Verts( i * Mesh->FrameVerts + j ).Vector();
			OneFrame .AddItem( Vertex );
			AllFrames.AddItem( Vertex );
		}
		Mesh->BoundingBoxes  (i) = FBox   ( &OneFrame(0), OneFrame.Num() );
		Mesh->BoundingSpheres(i) = FSphere( &OneFrame(0), OneFrame.Num() );
	}
	Mesh->BoundingBox    = FBox   ( &AllFrames(0), AllFrames.Num() );
	Mesh->BoundingSphere = FSphere( &AllFrames(0), AllFrames.Num() );

	// Display bounds.
	debugf
	(
		NAME_Log,
		TEXT("BoundingBox (%f,%f,%f)-(%f,%f,%f) BoundingSphere (%f,%f,%f) %f"),
		Mesh->BoundingBox.Min.X,
		Mesh->BoundingBox.Min.Y,
		Mesh->BoundingBox.Min.Z,
		Mesh->BoundingBox.Max.X,
		Mesh->BoundingBox.Max.Y,
		Mesh->BoundingBox.Max.Z,
		Mesh->BoundingSphere.X,
		Mesh->BoundingSphere.Y,
		Mesh->BoundingSphere.Z,
		Mesh->BoundingSphere.W
	);
	unguard;
}


// Warning: unlike the old vertex animation, these bounds only reflect
// the size of the reference pose. Actual animations are linked in at runtime
// so we can never be exactly sure of the bounding box for those. The best thing
// would be to override these sizes with a safety margin, in a script #exec command ?

void UEditorEngine::modelBuildBounds( USkeletalMesh* Mesh )
{
	guard(UEditorEngine::modelBuildBounds);
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Bounding skeletal mesh") );

	Mesh->BoundingSpheres.Add();
	Mesh->BoundingBoxes.Add();

	// Bound using the reference skin stretched to the reference skeleton pose
	TArray <FCoords> SpaceBases;
	SpaceBases.Add( Mesh->RefSkeleton.Num() );

	// Weapon bone 
	Mesh->WeaponBoneIndex = -1;
	
	// Assume the default pose.
	debugf(TEXT("Rendering default pose."));
	for( INT t=0; t<Mesh->RefSkeleton.Num(); t++)
	{
		FQuat ThisQuat   = Mesh->RefSkeleton(t).BonePos.Orientation;
		FVector ThisPos  = Mesh->RefSkeleton(t).BonePos.Position;
		FMatrix Base;
		Base = ThisQuat.FQuatToFMatrix();
		Base.XPlane.W = ThisPos.X;
		Base.YPlane.W = ThisPos.Y;
		Base.ZPlane.W = ThisPos.Z;
		SpaceBases(t) = FCoordsFromFMatrix(Base);			
	}

	// debugf(TEXT("Starting weighed bones"));
	TArray <FVector> OutVerts;
    OutVerts.AddZeroed(Mesh->FrameVerts);
	
	for( INT w=0; w< Mesh->BoneWeightIdx.Num(); w++)
	{
		INT Index  = Mesh->BoneWeightIdx(w).WeightIndex;
		INT Number = Mesh->BoneWeightIdx(w).Number;
		if ( Number > 0)
		{
			for( INT b=Index; b<(Index+Number); b++ )
			{
				INT VertIndex = Mesh->BoneWeights(b).PointIndex;
				if (VertIndex < Mesh->FrameVerts )   // LOD check...
				{					
					FLOAT Weight = (FLOAT)Mesh->BoneWeights(b).BoneWeight * ( 1.f/65535.f );					
					OutVerts(VertIndex) +=  Weight * ( Mesh->LocalPoints(b).PivotTransform(SpaceBases(w)));
					// .TransformPointBy(Coords)
				}				
			}
		}
	}


	Mesh->BoundingBox    = FBox   ( &OutVerts(0), OutVerts.Num() );
	Mesh->BoundingSphere = FSphere( &OutVerts(0), OutVerts.Num() );

	FBox Temp = Mesh->BoundingBox;
	// Skeletal meshes - don't have frames beyond 0.

#if 1 //NEW: skeletal fix from erik (sent to UnProg 07/20/00) -- replaces 6-line test
	// Extend by 2 from center to compensate for the fact that the skeletal bounds reflect the reference pose only.
	Mesh->BoundingBox.Min  = 2.0f*Temp.Min -(Temp.Min+Temp.Max)*0.5f;
	Mesh->BoundingBox.Max = 2.0f*Temp.Max-(Temp.Min+Temp.Max)*0.5f;
#elif 0 //NEW: RLO changes from erik 05/02/00
	//The cause of the erratic bounding extents for some skeletal characters
	//(which caused blinking out of actors in the engine, and often complete
	//invisibility in the editor)

	// Extend by 2 to compensate for the fact that the skeletal bounds reflect the reference pose only...
	Mesh->BoundingBox.Min.X = Temp.Min.X + Temp.Min.X - (Temp.Min.X + Temp.Max.X)*0.5f;
	Mesh->BoundingBox.Min.Y = Temp.Min.Y + Temp.Min.Y - (Temp.Min.Y + Temp.Max.Y)*0.5f;
	Mesh->BoundingBox.Min.Z = Temp.Min.Z + Temp.Min.Z - (Temp.Min.Z + Temp.Max.Z)*0.5f;
	
	Mesh->BoundingBox.Max.X = Temp.Max.X + Temp.Max.X - (Temp.Max.X + Temp.Max.X)*0.5f;
	Mesh->BoundingBox.Max.Y = Temp.Max.Y + Temp.Max.Y - (Temp.Max.Y + Temp.Max.Y)*0.5f;
	Mesh->BoundingBox.Max.Z = Temp.Max.Z + Temp.Max.Z - (Temp.Max.Z + Temp.Max.Z)*0.5f;
#else
	// Extend by 2 to compensate for the fact that the skeletal bounds reflect the reference pose only...
	Mesh->BoundingBox.Min.X = Temp.Min.X + Temp.Min.X - (Temp.Min.X + Temp.Max.X)*0.5f;
	Mesh->BoundingBox.Min.Y = Temp.Min.X + Temp.Min.X - (Temp.Min.Y + Temp.Max.Y)*0.5f;
	Mesh->BoundingBox.Min.Z = Temp.Min.X + Temp.Min.X - (Temp.Min.Z + Temp.Max.Z)*0.5f;

	Mesh->BoundingBox.Max.X = Temp.Max.X + Temp.Max.X - (Temp.Max.X + Temp.Max.X)*0.5f;
	Mesh->BoundingBox.Max.Y = Temp.Max.X + Temp.Max.X - (Temp.Max.Y + Temp.Max.Y)*0.5f;
	Mesh->BoundingBox.Max.Z = Temp.Max.X + Temp.Max.X - (Temp.Max.Z + Temp.Max.Z)*0.5f;
#endif

	Mesh->BoundingBoxes  (0) = Mesh->BoundingBox; 
	Mesh->BoundingSpheres(0) = Mesh->BoundingSphere;
	
	// Display bounds.
	debugf
	(
		NAME_Log,
		TEXT("BoundingBox (skeletal) (%f,%f,%f)-(%f,%f,%f) BoundingSphere (%f,%f,%f) %f"),
		Mesh->BoundingBox.Min.X,
		Mesh->BoundingBox.Min.Y,
		Mesh->BoundingBox.Min.Z,
		Mesh->BoundingBox.Max.X,
		Mesh->BoundingBox.Max.Y,
		Mesh->BoundingBox.Max.Z,
		Mesh->BoundingSphere.X,
		Mesh->BoundingSphere.Y,
		Mesh->BoundingSphere.Z,
		Mesh->BoundingSphere.W
	);
	unguard;
}



/*-----------------------------------------------------------------------------
	Special importers for skeletal data.
-----------------------------------------------------------------------------*/

//
// To do: Need much better error and version handling on all skeletal file reading - Erik
//

void UEditorEngine::modelImport//modelImport
(
	const TCHAR*		MeshName,
	UObject*			InParent,
	const TCHAR*		SkinFname, 
	UBOOL				Unmirror,
	UBOOL				ZeroTex,
	INT					UnMirrorTex,
	ULODProcessInfo*	LODInfo
)
{
	guard(UEditorEngine::modelImport);

	/*	 
	// File header structure. 
	struct VChunkHeader
	{
		ANSICHAR	ChunkID[20];  // string ID of up to 19 chars (usually zero-terminated..)
		INT			TypeFlag;     // Flags/reserved
		INT         DataSize;     // size per struct following;
		INT         DataCount;    // number of structs/
	};
	*/
	
	// Temp structs for importing
	USkelImport RawData; // local struct

	//
	// Alternate loading and construction of skeletal mesh object 
	//
	// 2nd draft: loads animation data with a SEPARATE script #exec, as a separate
	// object type, and all animations and bones link up by name.
	//

	USkeletalMesh*			Mesh;
	FArchive*		SkinFile;
	VChunkHeader	ChunkHeader;

	debugf( NAME_Log, TEXT("Importing skin %s"), MeshName );
	GWarn->BeginSlowTask( TEXT("Importing skeletal mesh"), 1, 0 );
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Reading files") );

	// Allocate skeletal mesh object.
	Mesh = new( InParent, MeshName, RF_Public|RF_Standalone )USkeletalMesh();

	// Open skeletal skin file and read header.
	SkinFile = GFileManager->CreateFileReader( SkinFname, 0, GLog );
	if( !SkinFile )
	{
		appErrorf( NAME_Log, TEXT("Error opening skin file %s"), SkinFname );
		//goto Out1; &&
	}

	SkinFile->Serialize( &ChunkHeader, sizeof(VChunkHeader) );
	if( SkinFile->IsError() )
	{
		appErrorf( NAME_Log, TEXT("Error reading skin file %s"), SkinFname );
		//goto Out2; &&
	}

	guard(ReadPointData);
	// Read the temp skin structures..
	// 3d points "vpoints" datasize*datacount....
	SkinFile->Serialize( &ChunkHeader, sizeof(VChunkHeader) );
	RawData.Points.Add(ChunkHeader.DataCount);
	SkinFile->Serialize( &RawData.Points(0), sizeof(VPoint) * ChunkHeader.DataCount);	
	unguard;

	guard(ReadWedgeData);
	//  Wedges (VVertex)
	SkinFile->Serialize(&ChunkHeader, sizeof(VChunkHeader) );
	RawData.Wedges.Add(ChunkHeader.DataCount);
	SkinFile->Serialize( &RawData.Wedges(0), sizeof(VVertex) * ChunkHeader.DataCount);
	unguard;

	guard(ReadFaceData);
	// Faces (VTriangle)
	SkinFile->Serialize(&ChunkHeader, sizeof(VChunkHeader) );
	RawData.Faces.Add(ChunkHeader.DataCount);
	SkinFile->Serialize( &RawData.Faces(0), sizeof(VTriangle) * ChunkHeader.DataCount);
	unguard

	guard(ReadMaterialData);
	// Materials (VMaterial)
	SkinFile->Serialize(&ChunkHeader, sizeof(VChunkHeader) );
	RawData.Materials.Add(ChunkHeader.DataCount);
	SkinFile->Serialize( &RawData.Materials(0), sizeof(VMaterial) * ChunkHeader.DataCount);
	unguard;

	guard(ReadRefSkeleton);
	// Reference skeleton (VBones)
	SkinFile->Serialize(&ChunkHeader, sizeof(VChunkHeader) );
	RawData.RefBonesBinary.Add(ChunkHeader.DataCount);
	SkinFile->Serialize( &RawData.RefBonesBinary(0), sizeof(VBone) * ChunkHeader.DataCount);
	unguard;
	
	guard(ReadBoneInfluences);
	// Raw bone influences (VRawBoneInfluence)
	SkinFile->Serialize(&ChunkHeader, sizeof(VChunkHeader) );
	RawData.Influences.Add(ChunkHeader.DataCount);
	SkinFile->Serialize( &RawData.Influences(0), sizeof(VRawBoneInfluence) * ChunkHeader.DataCount);
	unguard;

	delete SkinFile;

	// Allocate textures pointers  - Todo: Materials may share textures 
	// so this should probably really only add unique textures...
	while( RawData.Materials.Num() >= Mesh->Textures.Num() )
			Mesh->Textures.AddItem( NULL );

	// display summary info
	debugf(NAME_Log,TEXT(" * Skeletal skin VPoints: %i "),RawData.Points.Num());
	debugf(NAME_Log,TEXT(" * Skeletal skin VVertices: %i "),RawData.Wedges.Num());
	debugf(NAME_Log,TEXT(" * Skeletal skin VTriangles: %i "),RawData.Faces.Num());
	debugf(NAME_Log,TEXT(" * Skeletal skin VMaterials: %i "),RawData.Materials.Num());
	debugf(NAME_Log,TEXT(" * Skeletal skin VBones: %i "),RawData.RefBonesBinary.Num());
	debugf(NAME_Log,TEXT(" * Skeletal skin VRawBoneInfluences: %i "),RawData.Influences.Num());

	modelLODProcess( Mesh, LODInfo, &RawData );

	// Compute per-frame bounding volumes plus overall bounding volume.
	modelBuildBounds( Mesh ); 
		
	debugf(NAME_Log,TEXT(" * Total materials: %i "),((USkeletalMesh*)Mesh)->Materials.Num());

	// Set LOD defaults.
	Mesh->LODMinVerts	  = 10;		// Minimum number of vertices with which to draw a model. (Minimum for a cube = 8...)
	Mesh->LODStrength	  = 1.00f;	// Scales the (not necessarily linear) falloff of vertices with distance.
	Mesh->LODMorph        = 0.30f;	// Morphing range. 0.0 = no morphing.
	Mesh->LODZDisplace    = 0.00f;  // Z-displacement (in world units) for falloff function tweaking.
	Mesh->LODHysteresis	  = 0.00f;	// Controls LOD-level change delay/morphing. (unused)

	// display summary info
	debugf(NAME_Log,TEXT(" * Skeletal skin Points: %i size %i "),Mesh->Points.Num(), sizeof(FVector) );
	debugf(NAME_Log,TEXT(" * Skeletal skin Wedges: %i size %i "),Mesh->Wedges.Num(), sizeof(FMeshWedge) );
	debugf(NAME_Log,TEXT(" * Skeletal skin Triangles: %i size %i "),Mesh->Faces.Num(), sizeof(FMeshFace) );
	debugf(NAME_Log,TEXT(" * Skeletal skin Skeleton: %i size %i "),Mesh->RefSkeleton.Num(), sizeof(FMeshBone) );
	debugf(NAME_Log,TEXT(" * Skeletal skin Materials: %i size %i "),Mesh->Materials.Num(), sizeof(FMeshMaterial));
	debugf(NAME_Log,TEXT(" * Skeletal skin BoneWeights: %i size %i "),Mesh->BoneWeights.Num(), sizeof(VBoneInfluence) );
	debugf(NAME_Log,TEXT(" * Skeletal skin BoneIndices: %i size %i "),Mesh->BoneWeightIdx.Num(), sizeof(VBoneInfIndex) );

	unguard;
}

void UEditorEngine::modelAssignWeaponBone
(
    USkeletalMesh* Mesh,
    FName TempFname 
)
{
	guard(UEditorEngine::modelAssignWeaponBone);
	for( INT b=0; b< Mesh->RefSkeleton.Num(); b++)
	{
		if ( Mesh->RefSkeleton(b).Name == TempFname )
		{
			Mesh->WeaponBoneIndex = b;
			debugf(TEXT("Classic weapon bone link assigned to bone: %s"),*Mesh->RefSkeleton(b).Name);
			break;
		}
	}
	unguard;
}



INT UEditorEngine::animGetBoneIndex
( 
	UAnimation* Anim,
	FName TempFname 
)
{
	guard(UEditorEngine::animGetBoneIndex);
	for( INT b=0; b< Anim->RefBones.Num(); b++)
	{
		if ( Anim->RefBones(b).Name == TempFname )
		{
			return b;
		}
	}
	return 0;
	unguard;
}


void UEditorEngine::modelSetWeaponPosition
( 
	USkeletalMesh* Mesh, 
	FCoords WeaponCoords 
)
{
	guard(UEditorEngine::modelSetWeaponPosition);
	// Set the private weapon coordinate system (constructed from a vector and a rotation in UnEdSrv.cpp, guaranteed not
	// to change scale.
	Mesh->WeaponAdjust = WeaponCoords;
	unguard;
}

void UEditorEngine::animationImport
(
	const TCHAR*		AnimName,
	UObject*			InParent,
	const TCHAR*		DataFname,
	UBOOL				Unmirror,
	UBOOL               ImportSeqs,
	FLOAT				CompDefault
)
{
	guard(UEditorEngine::animationImport);

	UAnimation*	    NewAnimation;
	FArchive*		AnimationFile;
	VChunkHeader	ChunkHeader;

	debugf( NAME_Log, TEXT("Importing animation %s"), AnimName );
	GWarn->BeginSlowTask( TEXT("Importing skeletal mesh"), 1, 0 );
	GWarn->StatusUpdatef( 0, 0, TEXT("%s"), TEXT("Reading files") );

	// Allocate skeletal mesh object.
	NewAnimation = new( InParent, AnimName, RF_Public|RF_Standalone )UAnimation();
	
	// Open skeletal animation key file and read header.
	AnimationFile = GFileManager->CreateFileReader( DataFname, 0, GLog );
	if( !AnimationFile )
	{
		appErrorf( NAME_Log, TEXT("Error opening animation file %s"), DataFname );
	}

	// Read main header
	AnimationFile->Serialize( &ChunkHeader, sizeof(VChunkHeader) );
	if( AnimationFile->IsError() )
	{
		appErrorf( NAME_Log, TEXT("Error reading animation file %s"), DataFname );
	}

	
	// Read the header and bone names.
	AnimationFile->Serialize( &ChunkHeader, sizeof(VChunkHeader) );

	
	TArray<FNamedBoneBinary> RawBoneNames;
	guard( BoneNames );
	RawBoneNames.Add( ChunkHeader.DataCount );	
	NewAnimation->RefBones.Add( ChunkHeader.DataCount );
	AnimationFile->Serialize( &RawBoneNames(0), sizeof( FNamedBoneBinary ) * ChunkHeader.DataCount );
	unguard;

	guard(RawBoneNames);
	// Translate the raw data from the bones to Animation->RefBones FNames
	for( INT n=0; n<RawBoneNames.Num(); n++ )
	{
		appTrimSpaces(&RawBoneNames(n).Name[0]);
		NewAnimation->RefBones(n).Name  = FName( appFromAnsi(&RawBoneNames(n).Name[0]) );
		NewAnimation->RefBones(n).Flags = RawBoneNames(n).Flags;
		NewAnimation->RefBones(n).ParentIndex = RawBoneNames(n).ParentIndex;
	}
	unguard;

	guard(SeqInfoRaw);
	// Read the header and the animation sequence info if present...
	AnimationFile->Serialize( &ChunkHeader, sizeof(VChunkHeader) );
	NewAnimation->RawAnimSeqInfo.Add(ChunkHeader.DataCount);
	AnimationFile->Serialize( &NewAnimation->RawAnimSeqInfo(0), sizeof(AnimInfoBinary) * ChunkHeader.DataCount);
	// Remember to change  the raw animation name and group to FNames... =  FName(appFromAnsi( &name ));
	unguard;
	
	guard(animheader);
	// Read the header and beta keys.
	AnimationFile->Serialize( &ChunkHeader, sizeof(VChunkHeader) );
	NewAnimation->RawAnimKeys.Add(ChunkHeader.DataCount);
	AnimationFile->Serialize( &NewAnimation->RawAnimKeys(0), sizeof(VQuatAnimKey) * ChunkHeader.DataCount);	
	NewAnimation->RawNumFrames = ChunkHeader.DataCount / NewAnimation->RefBones.Num();
	NewAnimation->CompFactor = CompDefault;
	delete AnimationFile;
	unguard;

	guard(ImportSeqs);
	// Add binary sequence info to queue.
	if( ImportSeqs )
	{
		debugf(TEXT("New animation %s has %i imported sequences."), NewAnimation->GetName(), NewAnimation->RawAnimSeqInfo.Num());
			
		// Convert any animation info as defined in the input file RawAnimSeqInfo into MovesInfo...
		for( INT i=0; i<NewAnimation->RawAnimSeqInfo.Num(); i++)
		{
			MotionChunkDigestInfo NewMoveInfo;
			// Explicit copying - we need to convert names to FNames...
			NewMoveInfo.Name = FName(appFromAnsi( NewAnimation->RawAnimSeqInfo(i).Name));
			NewMoveInfo.Group = FName(appFromAnsi( NewAnimation->RawAnimSeqInfo(i).Group));
#if 1 //NEW: substituted animations support (mdf-tbd)
			// mdf-tbd: move into plugin output
			//NewMoveInfo.BaseName = FName(appFromAnsi( NewAnimation->RawAnimSeqInfo(i).BaseName));

			// intialize
			NewMoveInfo.BaseName = NAME_None;
#endif
			NewMoveInfo.FirstRawFrame		= NewAnimation->RawAnimSeqInfo(i).FirstRawFrame;
			NewMoveInfo.KeyCompressionStyle = NewAnimation->RawAnimSeqInfo(i).KeyCompressionStyle;
			NewMoveInfo.KeyQuotum			= NewAnimation->RawAnimSeqInfo(i).KeyQuotum;
			NewMoveInfo.KeyReduction		= NewAnimation->RawAnimSeqInfo(i).KeyReduction;
			NewMoveInfo.NumRawFrames		= NewAnimation->RawAnimSeqInfo(i).NumRawFrames;
			NewMoveInfo.RootInclude			= NewAnimation->RawAnimSeqInfo(i).RootInclude;
			NewMoveInfo.StartBone			= NewAnimation->RawAnimSeqInfo(i).StartBone;
			NewMoveInfo.TrackTime			= NewAnimation->RawAnimSeqInfo(i).TrackTime;
			NewMoveInfo.AnimRate			= NewAnimation->RawAnimSeqInfo(i).AnimRate;

			// Force Rate/Time.Framerate match
			if( NewMoveInfo.AnimRate > 0.0f )
				NewMoveInfo.TrackTime = NewMoveInfo.NumRawFrames / NewMoveInfo.AnimRate;

			// Addunique - based on name ? 
			// Check whether legal, then add...
			if( (NewMoveInfo.NumRawFrames != 0) && ( (NewMoveInfo.KeyQuotum != 0) || (NewMoveInfo.KeyReduction !=0.0f ) ) )
				NewAnimation->MovesInfo.AddItem(NewMoveInfo);
		}
	}
	NewAnimation->RawAnimSeqInfo.Empty();		
	unguard;

	unguard;
}


// Quick inter-key error evaluation. Assumes Key+1 and Key-1 indices are valid.
void GetInterKeyError( AnalogTrack& Track, const INT Key, TrackDiffs& Dev)
{
	FLOAT IntervalSize = Track.KeyTime(Key+1) - Track.KeyTime(Key-1);
	FLOAT Alpha = IntervalSize > 0.000001f ? (Track.KeyTime(Key)- Track.KeyTime(Key-1))/ IntervalSize : 0.0f;  
				
	FQuat LerpedQuat = SlerpQuat( Track.KeyQuat(Key-1),Track.KeyQuat(Key+1), Alpha);
	FLOAT QuatError = FQuatError( LerpedQuat, Track.KeyQuat(Key));
	Dev.QuatErr(Key) = QuatError;

	FVector LerpedPos = ( Track.KeyPos(Key-1)*Alpha + Track.KeyPos(Key+1)*(1.f-Alpha) );
	FLOAT PosError = (LerpedPos - Track.KeyPos(Key)).Size();				
	Dev.PosErr(Key) = PosError;
}



//
// Compress single animation from the raw data ( bones * frames ) 
// as found in RawAnimKeys()
// Anim->Moves(MoveIndex)
// Anim->MovesInfo(MoveIndex)
// Anim->AnimSeqs(MoveIndex)
//
void UEditorEngine::movementDigest(	UAnimation* Anim, INT MoveIndex )
{
	#define MAXANGLEDIFF 0.0001f   // angle difference below which positions are considered identical   0.0001 
	#define MAXPOSDIFF   0.0003f   // spatial difference below which positions are considered identical 0.0001  
	#define MAXERRX      0.0004f    // max 'key error' (posdiff+anglediff) below which keys are considered identical 
	#define MAXLERPERRX  0.1000f    // max angle difference below which an interpolation is considered feasible 
	
	guard(movementDigest);

	MotionChunk* ThisMove = &Anim->Moves(MoveIndex);
	MotionChunkDigestInfo* ThisMoveInfo = &Anim->MovesInfo(MoveIndex);

	// Builds a FMeshAnimSeq and associated animation data for each new Move.
	Anim->AnimSeqs.AddZeroed();
	//FMeshAnimSeq* Seq = &Anim->AnimSeqs(MoveIndex);
	FMeshAnimSeq* Seq = &Anim->AnimSeqs(Anim->AnimSeqs.Num()-1);
	
	debugf(TEXT("Digesting movement number %i  name %s "),MoveIndex,*ThisMoveInfo->Name); 

	//  ! Nonzero startbone not supported by GetFrame yet.

	// Refill the remap array according to Startbone
	
	// Fill array with reduced hierarchy bone indices.
	TArray <INT> Hierarchy;
	TArray <INT> MarkParent;

	MarkParent.AddZeroed(Anim->RefBones.Num());
	
	ThisMove->StartBone = ThisMoveInfo->StartBone;
	// Rip out the sub-hierarchy.
	if( ThisMove->StartBone )
		for(INT i=ThisMove->StartBone; i<Anim->RefBones.Num(); i++)
		{		
			if( i==ThisMove->StartBone || MarkParent( Anim->RefBones(i).ParentIndex ) ) MarkParent(i)=1;
			if (MarkParent(i)) Hierarchy.AddItem(i);
		}
	else
		for(INT i=0; i<Anim->RefBones.Num(); i++)
		{
			Hierarchy.AddItem(i);
		}

	debugf(TEXT(" movement Digestion: Sub-hierarchy startbone: %i Total nodes: %i "),ThisMove->StartBone, Anim->RefBones.Num());

	// AnimTracks workspace.
	ThisMove->AnimTracks.Empty();
	ThisMove->AnimTracks.AddZeroed(Hierarchy.Num()); // AddZeroed needed - these contain dynamic arrays.

	////////////////////////////////////////////////////////////////////////////////

	// To do: Actually the KeyReduction factor should apply AFTER we threw out all trivial keys.
	
	INT KeyMaximum = (INT)Abs( (ThisMove->AnimTracks.Num() * ThisMoveInfo->NumRawFrames)*ThisMoveInfo->KeyReduction); 
	
	if (ThisMoveInfo->KeyQuotum > 0)
		KeyMaximum = Min(ThisMoveInfo->KeyQuotum,KeyMaximum);

	ThisMove->AnimTracks.Empty();
	ThisMove->AnimTracks.AddZeroed(Hierarchy.Num()); 

	debugf(TEXT("Processing uAnimation: %s - number of Bones: %i  KeyReduction %f NumRawFrames %i"),Anim->GetName(),Anim->RefBones.Num(), ThisMoveInfo->KeyReduction, ThisMoveInfo->NumRawFrames );

	ThisMove->TrackTime = ThisMoveInfo->TrackTime;

	// Fill in the backward-compatible sequence data.
	// Adding notifys: only done AFTER the digestion.

#if 1 //NEW: substituted animations support (mdf-tbd)
	Seq->BaseName = ThisMoveInfo->BaseName;
#endif
	Seq->Group = ThisMoveInfo->Group;
	Seq->Name =  ThisMoveInfo->Name;
	Seq->NumFrames = ThisMoveInfo->NumRawFrames;
	Seq->Rate = ThisMoveInfo->AnimRate; // Should still be 'frames per second'.
	Seq->StartFrame = 0; // Always the start of a compressed skeletal move.

	// Does the range overrun the actual amount of keys ?
	if( (ThisMoveInfo->FirstRawFrame + ThisMoveInfo->NumRawFrames) * Anim->RefBones.Num() > Anim->RawAnimKeys.Num() )
		debugf(TEXT("Skeletal frame number overrun warning for sequence %s : Total %i Requested: %i to %i"),Anim->GetName(),Anim->RawAnimKeys.Num()/Anim->RefBones.Num(),  ThisMoveInfo->FirstRawFrame, ThisMoveInfo->FirstRawFrame+ThisMoveInfo->NumRawFrames ) ;

	// Reorder raw data into the appropriate tracks - Full bones.
	for(INT i=0; i<ThisMove->AnimTracks.Num(); i++)
	{		
		INT b= Hierarchy(i);		
		NLOG( debugf(TEXT(" Bone B:%i for hierarchy I: %i"),b,i);)
#if 1 //NEW: Fix -- for movementDigest crash RLO 05/09/00
		AnalogTrack& CurrentAnalogTrack = ThisMove->AnimTracks(i);
		CurrentAnalogTrack.Flags = 0;
#if 1 //NEW: Fix -- from eric 05/18/00
		CurrentAnalogTrack.KeyPos.Empty( ThisMoveInfo->NumRawFrames );
		CurrentAnalogTrack.KeyQuat.Empty( ThisMoveInfo->NumRawFrames );
		CurrentAnalogTrack.KeyTime.Empty( ThisMoveInfo->NumRawFrames );
#else
		CurrentAnalogTrack.KeyPos.Empty();
		CurrentAnalogTrack.KeyQuat.Empty();
		CurrentAnalogTrack.KeyTime.Empty();
#endif

		for( INT f=0; f < ThisMoveInfo->NumRawFrames; f++ )
		{			
			// Min() makes sure no illegal key index is used.
			INT KeyIdx = Min(Anim->RawAnimKeys.Num()-1,( ThisMoveInfo->FirstRawFrame + f ) * Anim->RefBones.Num() + b);
			//debugf(TEXT("Raw frame %i KeyIndex %i rawanimkeystotal %i "), f, KeyIdx, Anim->RawAnimKeys.Num());

			if( Anim->RawAnimKeys.IsValidIndex( KeyIdx ) )
			{
				VQuatAnimKey& CurrentRawAnimKey = Anim->RawAnimKeys( KeyIdx );
				CurrentAnalogTrack.KeyPos.AddItem( CurrentRawAnimKey.Position );
				CurrentAnalogTrack.KeyQuat.AddItem( CurrentRawAnimKey.Orientation );
				CurrentAnalogTrack.KeyTime.AddItem( (FLOAT)f/(FLOAT) ThisMoveInfo->NumRawFrames * ThisMove->TrackTime );
			}
		}
#else

		ThisMove->AnimTracks(i).Flags = 0;
#if 0 //NEW: Fix -- from eric 05/18/00
		ThisMove->AnimTracks(i).KeyPos.Empty( ThisMoveInfo->NumRawFrames );
		ThisMove->AnimTracks(i).KeyQuat.Empty( ThisMoveInfo->NumRawFrames );
		ThisMove->AnimTracks(i).KeyTime.Empty( ThisMoveInfo->NumRawFrames );
#else
		ThisMove->AnimTracks(i).KeyPos.Empty();
		ThisMove->AnimTracks(i).KeyQuat.Empty();
		ThisMove->AnimTracks(i).KeyTime.Empty();
#endif

		for( INT f=0; f< ThisMoveInfo->NumRawFrames; f++ )
		{			
			// Min() makes sure no illegal key index is used.
			INT KeyIdx = Min(Anim->RawAnimKeys.Num()-1,( ThisMoveInfo->FirstRawFrame + f ) * Anim->RefBones.Num() + b);
			//debugf(TEXT("Raw frame %i KeyIndex %i rawanimkeystotal %i "), f, KeyIdx, Anim->RawAnimKeys.Num());
			ThisMove->AnimTracks(i).KeyPos.AddItem( Anim->RawAnimKeys(KeyIdx).Position );
			ThisMove->AnimTracks(i).KeyQuat.AddItem( Anim->RawAnimKeys(KeyIdx).Orientation );
			ThisMove->AnimTracks(i).KeyTime.AddItem( (FLOAT)f/(FLOAT) ThisMoveInfo->NumRawFrames * ThisMove->TrackTime );  
		}
#endif
	}

	// Nothing to eliminate if there are < 3 keys in all tracks. => To do:  except for static position tracks.
	if( ThisMoveInfo->NumRawFrames < 3 ) return; 
	
	// Find largest bone size (seems to range from 10 to 40..) -> to factor into the error.
	FLOAT BoneMax = 0.0f;

	guard(FindMax);
	for( i=0; i<ThisMove->AnimTracks.Num(); i++ ) 
	{
		INT b=Hierarchy(i);
		// Bone size(pivot offset) may vary over time.
		if( b != 0 ) // Ignore root track offset which does not represent a bone.
		{
			for( INT p=0; p< ThisMove->AnimTracks(i).KeyPos.Num(); p++ )
			{
				FLOAT BoneSize = ThisMove->AnimTracks(i).KeyPos(p).Size();
				if( BoneSize > BoneMax ) 
				BoneMax = BoneSize;
			}
		}
	}
	NLOG( debugf(TEXT("Max bone size for this animation: %f"),BoneMax) );
	unguard;
	
	// Create a hierarchy-depth number for each bone to scale errors properly by keeping in mind cumulative inaccuracies.
	TArray <INT> BoneDepth;
	BoneDepth.Add(Anim->RefBones.Num());

	TArray <FLOAT> ErrorBias;
	ErrorBias.Add(ThisMove->AnimTracks.Num());
	
	INT MaxDepth = 0;
	
	for( INT b=0; b<Anim->RefBones.Num(); b++ )
	{
		INT Parent = Anim->RefBones(b).ParentIndex;
		BoneDepth(b) = 1.0f;
		if( Parent != b )
		{
			BoneDepth(b) += BoneDepth(Parent);
		}
		//NLOG( debugf(TEXT("Parent of %i  is %i - hierarchy depth: %i"),b,Parent,BoneDepth(b)) );

		if( MaxDepth < BoneDepth(b))
		{
			MaxDepth = BoneDepth(b);
		}
	}
	for( i=0; i<ThisMove->AnimTracks.Num(); i++ )
	{
		
		INT b = Hierarchy(i);
		// conservative error scaling: root is about 2x as important as end bones ?
		ErrorBias(i) =  1.0f + (MaxDepth+1-BoneDepth(b))/MaxDepth; 
		//if (b < 5) ErrorBias(b) = 3.0f;
		
	}
	BoneDepth.Empty();

	

	// Scale bone errors according depending on max bone size.
	// Arbitrary scaler. 20/BoneMax seems reasonable; in compression, a higher PosFactor will result in
	// more accurate bone _offsets_, lower will conserve more accurate _angles_.
	FLOAT PosFactor = 10.0f / BoneMax;
	NLOG( debugf(TEXT("Keys before culling: %i  Target: %i"), ThisMove->AnimTracks.Num() * ThisMoveInfo->NumRawFrames,KeyMaximum) );

	// Stats keeping
	INT TotalKeys = ThisMove->AnimTracks.Num() * ThisMoveInfo->NumRawFrames;
	INT RemovedMatched = 0;
	INT RemovedLerped = 0;	

	// First culling step.
	for( b=0; b<ThisMove->AnimTracks.Num(); b++)
	{
		AnalogTrack& Track = ThisMove->AnimTracks(b);
	
		INT QuatNum = Track.KeyQuat.Num();

		// From tail on down. Immediately throw away all keys that are lerp-able with negligible error.
#if 0 //NEW: Fix -- from eric 05/18/00
		for( INT i=QuatNum-2; i>0; i-- )
#else
		for( INT i=QuatNum-1; i>0; i-- )  
#endif
		{
			FLOAT KeyErr;
			KeyErr  = FQuatError( Track.KeyQuat(i), Track.KeyQuat(i-1) );
			KeyErr += FQuatError( Track.KeyQuat(i), Track.KeyQuat(i+1) );
			if( (KeyErr*0.5f) < MAXANGLEDIFF )
			{
				// Throw away only if positions match also
				FLOAT PosErr;
				PosErr  = ( Track.KeyPos(i-1) - Track.KeyPos(i) ).Size();
				PosErr += ( Track.KeyPos(i+1) - Track.KeyPos(i) ).Size();
				if( (PosFactor*PosErr*0.5f) < MAXPOSDIFF ) //###
				{
					if (1) //(b >= 0 ) 
					{
						Track.KeyQuat.Remove(i);
						Track.KeyPos.Remove(i);
						Track.KeyTime.Remove(i);
						RemovedMatched++;
						TotalKeys--;
					}
				}
			}
		}
	} 

	
	
	/*
	  ### Note:
	  Hierarchy.Num() instead of Anim->RefBones.Num() usually;
	  Error arrays etc all size Hierarchy.Num(), or Thismove->animTracks.Num()....
	  Unless you need actual full skeleton bone info keep to this size/index otherwise b=Hierarchy(i)

	*/

	// Allocate error arrays
	TArray <FLOAT> MinTrackError;
	TArray <FLOAT> MinTrackErrIdx;
	MinTrackError.AddZeroed( ThisMove->AnimTracks.Num());
	MinTrackErrIdx.AddZeroed( ThisMove->AnimTracks.Num());

	TArray <TrackDiffs> DevTrack;
	DevTrack.AddZeroed(ThisMove->AnimTracks.Num());

	// Main interpolation/compression loop

	// Precaculate per-track smallest lerp error.
	for( b=0; b<ThisMove->AnimTracks.Num(); b++)
	{
		AnalogTrack& Track = ThisMove->AnimTracks(b);

		DevTrack(b).QuatErr.AddZeroed( Track.KeyQuat.Num() );
		DevTrack(b).PosErr.AddZeroed( Track.KeyQuat.Num() );

		if ( Track.KeyQuat.Num() > 2)
		{
			// Fill error arrays, and min-error index & error;
			MinTrackErrIdx(b) = -1;
			MinTrackError(b) = 1000000.0f;

			for( INT i=1; i<Track.KeyQuat.Num()-1; i++)
			{
				GetInterKeyError(Track, i, DevTrack(b) );
				FLOAT ThisError = DevTrack(b).QuatErr(i)+ PosFactor*DevTrack(b).PosErr(i);
				// Why posFactor: QuatErr and PosErr need to be scaled to equivalent units -> Position relative to mesh-size.
				if (ThisError < MinTrackError(b))
				{
					MinTrackError(b) = ThisError;
					MinTrackErrIdx(b) = i;
				}
			}			
		}
	}


	debugf(TEXT("Start: Keymax %i Totalkeys %i "),KeyMaximum, TotalKeys );


	guard(While);
	while ( KeyMaximum < TotalKeys ) // TotalKeys must keep track of all keys
	{
		// Find the smallest overall error
		INT SmallestIdx = -1;
		FLOAT SmallestError = 100000000.f;

		//debugf(TEXT("Keymax %i Totalkeys %i "),KeyMaximum, TotalKeys );

		guard(Errortest);
		for( i=0; i<ThisMove->AnimTracks.Num(); i++)
		{
			if ( (MinTrackError(i) < SmallestError) && ThisMove->AnimTracks(i).KeyQuat.Num() > 2 )
			{
				SmallestIdx = i;
				SmallestError = ErrorBias(i) * MinTrackError(i);
			}
		}
		unguard;

		guard(Deletelerp);
		if( SmallestIdx == -1) break; // Less than 3 keys left in all tracks, so exit.
		// Check whether we need to exit because the smallest overall error is too big to lerp
		// if( SmallestError > MAXLERPERRX ) break;

		// Delete the most lerp-able key.
		INT i = MinTrackErrIdx(SmallestIdx);
		AnalogTrack& Track = ThisMove->AnimTracks(SmallestIdx);

		//debugf(TEXT("Removing %i from keys totals %i %i %i "),i,Track.KeyQuat.Num(),Track.KeyPos.Num(),Track.KeyTime.Num());
		//debugf(TEXT("Removing %i from  err totals %i %i smallestidx %i "), i,DevTrack(SmallestIdx).QuatErr.Num(),DevTrack(SmallestIdx).PosErr.Num(),SmallestIdx);
		Track.KeyQuat.Remove(i);
		Track.KeyPos.Remove(i);
		Track.KeyTime.Remove(i);
		DevTrack(SmallestIdx).QuatErr.Remove(i);
		DevTrack(SmallestIdx).PosErr.Remove(i);

		RemovedLerped++;
		TotalKeys--;		

		// Update the error for all (bordering) keys in this track,
		// And the MinTrackError for this track.
		if( Track.KeyQuat.Num() > 2)
		{			
			if (i < Track.KeyQuat.Num() - 1)
			{
				GetInterKeyError( Track, i, DevTrack(SmallestIdx) );
			}
			if (i > 1) 
			{
				GetInterKeyError( Track, i-1, DevTrack(SmallestIdx) );
			}

			MinTrackError(SmallestIdx)  = 1000000.f;
			MinTrackErrIdx(SmallestIdx) = -1;

			// update MinTrackError and MinTrackErrIdx for this track.
			for( INT i=1; i<Track.KeyQuat.Num()-1; i++)
			{				
				FLOAT ThisError = DevTrack(SmallestIdx).QuatErr(i) + PosFactor*DevTrack(SmallestIdx).PosErr(i);
				// QuatErr and PosErr need to be scaled appropriately -> Position is mesh-size dependent !
				if (ThisError < MinTrackError(SmallestIdx))
				{
					MinTrackError(SmallestIdx) = ThisError;
					MinTrackErrIdx(SmallestIdx) = i;
				}
			}			
		}		
		unguard;
	}
	unguard;

	DevTrack.Empty();

	NLOG( debugf(TEXT(" QuatKeys after lossy culling 1: %i Duplicates: %i LerpMatches: %i "),TotalKeys,RemovedMatched,RemovedLerped) );

	// Turn any 2-key tracks into 1 key tracks if possible.
	for( b=0; b<ThisMove->AnimTracks.Num(); b++)
	{
		AnalogTrack& Track = ThisMove->AnimTracks(b);
		
		if( Track.KeyQuat.Num() == 2 )
		{
			// Collapse 2nd key only if both Quat and Pos difference fall within the max delta limits.
			FLOAT QuatDiff = FQuatError( Track.KeyQuat(0),Track.KeyQuat(1) );
			FLOAT PosDiff  = ( Track.KeyPos(0) - Track.KeyPos(1) ).Size();

			if ( (QuatDiff + PosFactor*PosDiff) < MAXERRX )
			{
				Track.KeyQuat.Remove(1);
				Track.KeyPos.Remove(1);
				Track.KeyTime.Remove(1);
				RemovedMatched++;
				TotalKeys--;
			}		
		}
	
		if( ThisMove->AnimTracks(b).KeyQuat.Num() > 1 )
			NLOG( debugf(TEXT("#-> [%5i] QuatKeys in track %20s [%5i]"),ThisMove->AnimTracks(b).KeyQuat.Num(),*Anim->RefBones(b).Name, b ) );
	}
		
	// Compress positions: Now that tracks have been compressed, decide for each track
	// if it only needs to hold one static position or remain a full position track ( usually 
	// the root bone Pos track remains )

	INT RemovedPos = 0;
	
	for( b=0; b<ThisMove->AnimTracks.Num(); b++ )
	{
		FLOAT MaxDelta = 0.0f;
		for(INT i=0; i<ThisMove->AnimTracks(b).KeyPos.Num(); i++)
		{
			FVector DiffPos = ThisMove->AnimTracks(b).KeyPos(i) - ThisMove->AnimTracks(b).KeyPos(0);
			FLOAT LocalDiff;
			LocalDiff = DiffPos.Size();
			if( LocalDiff>MaxDelta ) MaxDelta = LocalDiff;
		}
		if( MaxDelta < MAXPOSDIFF) // No significant deviation in all local positions => turn it into a static track.
		{
#if 1 //NEW: Fix -- for movementDigest crash RLO 05/09/00
			if( ThisMove->AnimTracks(b).KeyPos.IsValidIndex( 0 ) )
			{
				FVector SinglePos( ThisMove->AnimTracks(b).KeyPos( 0 ) );
				ThisMove->AnimTracks(b).KeyPos.Empty();
				ThisMove->AnimTracks(b).KeyPos.AddItem(SinglePos);
			}
			else
			{
				ThisMove->AnimTracks(b).KeyPos.AddItem( FVector(0,0,0) );
			}
#else			
			FVector SinglePos = ThisMove->AnimTracks(b).KeyPos(0);
			ThisMove->AnimTracks(b).KeyPos.Empty();
			ThisMove->AnimTracks(b).KeyPos.AddItem(SinglePos);
#endif
			RemovedPos++;
		}
		if( ThisMove->AnimTracks(b).KeyPos.Num() > 1 )
			NLOG( debugf(TEXT("#-> [%5i] PosKeys in track %20s [%5i]"),ThisMove->AnimTracks(b).KeyPos.Num(),*Anim->RefBones(b).Name, b ) );
	}
	
	debugf(TEXT("QuatKeys after lossy culling 2:%i  Duplicates: %i LerpMatches: %i Removed Pos tracks: %i"),TotalKeys,RemovedMatched,RemovedLerped,RemovedPos);
	
	// Align quats. The first and last quats in looping animations can only be aligned at runtime...
	for( b=0; b<ThisMove->AnimTracks.Num(); b++)
	{
		for(INT i=1; i< ThisMove->AnimTracks(b).KeyQuat.Num(); i++)
		{
			AlignFQuatWith( ThisMove->AnimTracks(b).KeyQuat(i), ThisMove->AnimTracks(b).KeyQuat(i-1));		
		}
	}

	// When limited to a bone subset, copy the hierarchy bonemapping array into our ThisMove
	ThisMove->BoneIndices.Empty();
	for( INT t=0; t<Anim->RefBones.Num(); t++)
	{
		INT NodeIdx =  Hierarchy.FindItemIndex(t);
		if( NodeIdx!=INDEX_NONE )
			ThisMove->BoneIndices.AddItem(NodeIdx);
		else
			ThisMove->BoneIndices.AddItem(-1);
	}

	unguard;
}

//
// Digest the raw frame data into a movement repertoire.
//
void UEditorEngine::digestMovementRepertoire( UAnimation* Anim)
{	
	debugf(TEXT("## Digesting %i movements for animation %s "),Anim->MovesInfo.Num(),Anim->GetName());

	// Discard raw sequence info.
	Anim->RawAnimSeqInfo.Empty();

	// Allocate moves.
	Anim->Moves.Empty();
	Anim->Moves.AddZeroed(Anim->MovesInfo.Num());

	{for(INT i=0; i<Anim->MovesInfo.Num(); i++)
	{		
		debugf(TEXT("Digesting motion [%s] number %i  raw keys: %i reduction: %f "), (*Anim->MovesInfo(i).Name), i, Anim->MovesInfo(i).NumRawFrames * Anim->RefBones.Num(),Anim->MovesInfo(i).KeyReduction);

		if (0)
		{
#if 1 //NEW: substituted animations support (mdf-tbd)
			debugf(TEXT("Group: %s  BaseName: %s  Rate %f  Time %f  StartBone %i RootInclude %i NumRawFrames %i KeyReduction %f KeyQuotum %i KeyCompStyle %i FirstRawFrame %i"),
#else
			debugf(TEXT("Group: %s  Rate %f  Time %f  StartBone %i RootInclude %i NumRawFrames %i KeyReduction %f KeyQuotum %i KeyCompStyle %i FirstRawFrame %i"),
#endif
			*Anim->MovesInfo(i).Group,
#if 1 //NEW: substituted animations support (mdf-tbd)
			*Anim->MovesInfo(i).BaseName,
#endif
			Anim->MovesInfo(i).AnimRate, 
			Anim->MovesInfo(i).TrackTime,
			Anim->MovesInfo(i).StartBone,
			Anim->MovesInfo(i).RootInclude,
			Anim->MovesInfo(i).NumRawFrames,
			Anim->MovesInfo(i).KeyReduction,
			Anim->MovesInfo(i).KeyQuotum,
			Anim->MovesInfo(i).KeyCompressionStyle,
			Anim->MovesInfo(i).FirstRawFrame );
		}
		
		movementDigest( Anim, i ); 
		// debugf(TEXT("  digested motion : %i keytracks:  %i rate: %f "),Anim->Moves.Num(), Anim->Moves(i).AnimTracks.Num(),Anim->MovesInfo(i).AnimRate );
	}}

	Anim->AnimSeqs.Shrink(); 
	Anim->Moves.Shrink();
	Anim->RefBones.Shrink();
}


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
