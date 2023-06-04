/*=============================================================================
	UWaveMeshBuilder.cpp: UnrealEd wavy mesh builder.
	Copyright 2000 Legend Entertainment Company All Rights Reserved.

Revision history:
	* Created by Aaron Leiby.
=============================================================================*/

#include "EditorPrivate.h"

/*-----------------------------------------------------------------------------
	UWaveMeshBuilder.
-----------------------------------------------------------------------------*/

void UWaveMeshBuilder::execCreateWavyMesh( FFrame& Stack, RESULT_DECL )
{
	guard(UWaveMeshBuilder::execCreateWavyMesh);

	P_GET_BYTE(MeshType);
	P_GET_FLOAT(Width);
	P_GET_FLOAT(Height);
	P_GET_FLOAT(Magnitude);
	P_GET_FLOAT(MinRate);
	P_GET_FLOAT(MaxRate);
	P_GET_INT(USubDiv);
	P_GET_INT(VSubDiv);
	P_GET_INT_OPTX(UTexTile,0);
	P_GET_INT_OPTX(VTexTile,0);
	P_GET_FLOAT_OPTX(Noise,0.0);
	P_GET_UBOOL_OPTX(bFixedEdges,0);
	P_GET_UBOOL_OPTX(bFlushEdges,0);
	P_GET_UBOOL_OPTX(bShiny,0);
	P_GET_UBOOL_OPTX(bMultiTexture,0);
	P_FINISH;

	UWaveMesh* Mesh = 
		MeshType==MT_Quad ? new( GEditor->Level, NAME_None, RF_Public|RF_Standalone )UWaveMesh ( Width, Height, Magnitude, MinRate, MaxRate, USubDiv, VSubDiv, UTexTile, VTexTile, Noise, bFixedEdges==1, bShiny==1, bMultiTexture==1 ) :
		MeshType==MT_Tri  ? new( GEditor->Level, NAME_None, RF_Public|RF_Standalone )UWaveMesh2( Width, Height, Magnitude, MinRate, MaxRate, USubDiv, VSubDiv, UTexTile, VTexTile, Noise, bFixedEdges==1, bFlushEdges==1, bShiny==1, bMultiTexture==1 ) :
		NULL;

	UBOOL GIsScriptableSaved = 0;
	Exchange(GIsScriptable,GIsScriptableSaved);

	AWavyMesh* Actor = CastChecked<AWavyMesh>(GEditor->Level->SpawnActor( AWavyMesh::StaticClass(), NAME_None, NULL, NULL, FVector(0,0,0), FRotator(0,0,0), NULL, 1, 0 ));

	Exchange(GIsScriptable,GIsScriptableSaved);

	if( Mesh && Actor )
	{
		Actor->Mesh = Mesh;
		Mesh->SetOwner(Actor);
	}

	GEditor->RedrawLevel( GEditor->Level );

	*(DWORD*)Result = (Mesh!=NULL && Actor!=NULL);

	unguard;
}

IMPLEMENT_CLASS(UWaveMeshBuilder)

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
