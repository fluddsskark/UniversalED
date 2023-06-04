/*=============================================================================
	UnEdAct.cpp: Unreal editor actor-related functions
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EditorPrivate.h"

#pragma DISABLE_OPTIMIZATION /* Not performance-critical */

/*-----------------------------------------------------------------------------
   Actor adding/deleting functions.
-----------------------------------------------------------------------------*/

//
// Copy selected actors to the clipboard.
//
void UEditorEngine::edactCopySelected( ULevel* Level )
{
	guard(UEditorEngine::edactCopySelected);

	// Export the actors.
	FStringOutputDevice Ar;
	UExporter::ExportToOutputDevice( Level, NULL, Ar, TEXT("copy"), 0 );
	appClipboardCopy( *Ar );

	unguard;
}

//
// Paste selected actors from the clipboard.
//
void UEditorEngine::edactPasteSelected( ULevel* Level )
{
	guard(UEditorEngine::edactPasteSelected);

	// Get pasted text.
	FString PasteString = appClipboardPaste();
	const TCHAR* Paste = *PasteString;

	// Import the actors.
	Level->RememberActors();
	ULevelFactory* Factory = new ULevelFactory;
	Factory->FactoryCreateText( ULevel::StaticClass(), Level->GetOuter(), Level->GetName(), 0, NULL, TEXT("paste"), Paste, Paste+appStrlen(Paste), GWarn );
	delete Factory;
	GCache.Flush();
	Level->ReconcileActors();
	ResetSound();

	// Offset them.
	for( INT i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) && Level->Actors(i)->bSelected )
			Level->Actors(i)->Location += FVector(32,32,32);

	// Note change.
	EdCallback( EDC_MapChange, 0 );
	NoteSelectionChange( Level );

	unguard;
}

//
// Delete all selected actors.
//
void UEditorEngine::edactDeleteSelected( ULevel* Level )
{
	guard(UEditorEngine::edactDeleteSelected);
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if
		(	(Actor)
		&&	(Actor->bSelected)
		&&	(Level->Actors.Num()<1 || Actor!=Level->Actors(0))
		&&	(Level->Actors.Num()<2 || Actor!=Level->Actors(1))
		&&  (Actor->GetFlags() & RF_Transactional) )
		{
			Level->DestroyActor( Actor );
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

//
// Duplicate all selected actors and select just the duplicated set.
//
#if 1 //NEW: ParticleSystems
struct FuncParms{ class AActor* A; };
struct FuncParms2{ class AActor* A; UBOOL B; };
#endif
#if 0 //NEW: DuplicateUpstreamConnections
void UEditorEngine::edactDuplicateSelected( ULevel* Level, UBOOL DuplicateUpstreamConnections )
#else
void UEditorEngine::edactDuplicateSelected( ULevel* Level )
#endif
{
	guard(UEditorEngine::edactDuplicateSelected);
	FVector Delta(32.0, 32.0, 0.0);

	// Untag all actors.
	for( int i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) )
			Level->Actors(i)->bTempEditor = 0;

	// Duplicate and deselect all actors.
	for( i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if
		(	Actor
		&&	Actor->bSelected
		&&  !Actor->bTempEditor
		&&  Actor!=Level->Brush() 
		&&  (Actor->GetFlags() & RF_Transactional) )
		{
			FVector NewLocation = Actor->Location + Delta;
#if 0 //NEW: DuplicateUpstreamConnections
			if( DuplicateUpstreamConnections )
				GObjDuplicateUpstreamConnections = 1;
			debugf( TEXT("GObjDuplicateUpstreamConnections %d"), GObjDuplicateUpstreamConnections );
#endif
#if 1 //NEW: ParticleSystems
			AActor* NewActor = NULL;
			if( Actor->IsA( TEXT("ParticleGenerator") ) )
			{
				UBOOL GIsSavedScriptableSaved = 1;
				Exchange(GIsScriptable,GIsSavedScriptableSaved);
				FuncParms Parms;
				Parms.A = NULL;
				Actor->UObject::ProcessEvent(Actor->FindFunctionChecked(FName(TEXT("Duplicate"),FNAME_Intrinsic)),&Parms);
				NewActor = Parms.A;
				Exchange(GIsScriptable,GIsSavedScriptableSaved);
			}
			else NewActor = Level->SpawnActor
#else
			AActor* NewActor = Level->SpawnActor
#endif
			(
				Actor->GetClass(),
				NAME_None, 
				NULL,
				NULL,
				NewLocation,
				Actor->Rotation,
				Actor,
				0,
				0
			);
			if( NewActor )
			{
				NewActor->Modify();
				if( Actor->IsBrush() )
				{
					csgCopyBrush( (ABrush*)NewActor, ((ABrush*)Actor), ((ABrush*)Actor)->PolyFlags, Actor->GetFlags(), Actor->IsMovingBrush() );
					if( !Actor->IsMovingBrush() )
						NewActor->SetFlags( RF_NotForClient | RF_NotForServer );
				}
				NewActor->PostEditMove();
				NewActor->bTempEditor = 1;
				NewActor->Location = Actor->Location + FVector(Constraints.GridSize.X,Constraints.GridSize.Y,0);
			}
			Actor->Modify();
			Actor->bSelected = 0;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

#if 1
//
// Replace all selected brushes with the default brush
//
void UEditorEngine::edactReplaceSelectedBrush( ULevel* Level )
{
	guard(UEditorEngine::edactReplaceSelectedBrush);

	// Untag all actors.
	for( int i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) )
			Level->Actors(i)->bTempEditor = 0;

	// Replace all selected brushes
	ABrush* DefaultBrush = Level->Brush();
	for( i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if
		(	Actor
		&&	Actor->bSelected
		&&  !Actor->bTempEditor
		&&  Actor->IsBrush()
		&&  Actor!=DefaultBrush
		&&  (Actor->GetFlags() & RF_Transactional) )
		{
			ABrush* Brush = (ABrush*)Actor;
			ABrush* NewBrush = csgAddOperation( DefaultBrush, Level, Brush->PolyFlags, (ECsgOper)Brush->CsgOper );
			if( NewBrush )
			{
				NewBrush->Modify();
				NewBrush->Group = Brush->Group;
				NewBrush->CopyPosRotScaleFrom( Brush );
				NewBrush->PostEditMove();
				NewBrush->bTempEditor = 1;
				NewBrush->bSelected = 1;
				Level->DestroyActor( Actor );
			}
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

#if 1 //LEGEND
static void CopyActorProperties( AActor* Dest, const AActor *Src )
{
	// Events
	Dest->Event	= Src->Event;
	Dest->Tag	= Src->Tag;

	// Object
	Dest->Group	= Src->Group;
}
#endif

//
// Replace all selected non-brush actors with the specified class
//
void UEditorEngine::edactReplaceSelectedWithClass( ULevel* Level,UClass* Class )
{
	guard(UEditorEngine::edactReplaceSelectedWithClass);

	// Untag all actors.
	for( int i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) )
			Level->Actors(i)->bTempEditor = 0;

	// Replace all selected brushes
	for( i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if
		(	Actor
		&&	Actor->bSelected
		&&  !Actor->bTempEditor
		&&  !Actor->IsBrush()
		&&  (Actor->GetFlags() & RF_Transactional) )
		{
			AActor* NewActor = Level->SpawnActor
			(
				Class,
				NAME_None, 
				NULL,
				NULL,
				Actor->Location,
				Actor->Rotation,
				NULL,
				0,
				1
			);
			if( NewActor )
			{
				NewActor->Modify();
#if 1 //LEGEND
				CopyActorProperties( NewActor, Actor );
#else
				NewActor->Group = Actor->Group;
#endif
				NewActor->bTempEditor = 1;
				NewActor->bSelected = 1;
				Level->DestroyActor( Actor );
			}
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

#if 1 //LEGEND
void UEditorEngine::edactReplaceClassWithClass( ULevel* Level,UClass* Class,UClass* WithClass )
{
	guard(UEditorEngine::edactReplaceClassWithClass);

	// Untag all actors.
	for( int i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) )
			Level->Actors(i)->bTempEditor = 0;

	// Replace all matching actors
	for( i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if
		(	Actor
		&&	Actor->IsA( Class )
		&&  !Actor->bTempEditor
		&&  (Actor->GetFlags() & RF_Transactional) )
		{
			AActor* NewActor = Level->SpawnActor
			(
				WithClass,
				NAME_None, 
				NULL,
				NULL,
				Actor->Location,
				Actor->Rotation,
				NULL,
				0,
				1
			);
			if( NewActor )
			{
				NewActor->Modify();
				NewActor->bTempEditor = 1;
				NewActor->bSelected = 1;
				CopyActorProperties( NewActor, Actor );
				Level->DestroyActor( Actor );
			}
		}
	}
	NoteSelectionChange( Level );
	unguard;
}
#endif

/*-----------------------------------------------------------------------------
   Actor hiding functions.
-----------------------------------------------------------------------------*/

//
// Hide selected actors (set their bHiddenEd=true)
//
void UEditorEngine::edactHideSelected( ULevel* Level )
{
	guard(UEditorEngine::edactHideSelected);

	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor!=Level->Brush() && Actor->bSelected )
		{
			Actor->Modify();
			Actor->bHiddenEd = 1;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

//
// Hide unselected actors (set their bHiddenEd=true)
//
void UEditorEngine::edactHideUnselected( ULevel* Level )
{
	guard(UEditorEngine::edactHideUnselected);

	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Cast<ACamera>(Actor) && Actor!=Level->Brush() && !Actor->bSelected )
		{
			Actor->Modify();
			Actor->bHiddenEd = 1;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

//
// UnHide selected actors (set their bHiddenEd=true)
//
void UEditorEngine::edactUnHideAll( ULevel* Level )
{
	guard(UEditorEngine::edactUnHideAll);
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if
		(	Actor
		&&	!Cast<ACamera>(Actor)
		&&	Actor!=Level->Brush()
		&&	Actor->GetClass()->GetDefaultActor()->bHiddenEd==0 )
		{
			Actor->Modify();
			Actor->bHiddenEd = 0;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}
#endif

/*-----------------------------------------------------------------------------
   Actor selection functions.
-----------------------------------------------------------------------------*/

//
// Select all actors except cameras and hidden actors.
//
void UEditorEngine::edactSelectAll( ULevel* Level )
{
	guard(UEditorEngine::edactSelectAll);
#if 1
	// Add all selected actors' group name to the GroupArray
	TArray<FName> GroupArray;
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Cast<ACamera>(Actor) && !Actor->bHiddenEd )
		{
			if( Actor->bSelected && Actor->Group!=NAME_None )
			{
				GroupArray.AddUniqueItem( Actor->Group );
			}
		}
	}

	// if the default brush is the only brush selected, select objects inside the default brush
	if( GroupArray.Num() == 0 && Level->Brush()->bSelected ) {
		edactSelectInside( Level );
		return;

	// if GroupArray is empty, select all unselected actors (v156 default "Select All" behavior)
	} else if( GroupArray.Num() == 0 ) {
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && !Cast<ACamera>(Actor) && !Actor->bSelected && !Actor->bHiddenEd )
			{
				Actor->Modify();
				Actor->bSelected=1;
			}
		}

	// otherwise, select all actors that match one of the groups,
	} else {
		// use appStrfind() to allow selection based on hierarchically organized group names
		for( i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && !Cast<ACamera>(Actor) && !Actor->bSelected && !Actor->bHiddenEd )
			{
				for( INT j=0; j<GroupArray.Num(); j++ ) {
					if( appStrfind( *Actor->Group, *GroupArray(j) ) != NULL ) {
						Actor->Modify();
						Actor->bSelected=1;
						break;
					}
				}
			}
		}
	}
#else
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Cast<ACamera>(Actor) && !Actor->bSelected && !Actor->bHiddenEd )
		{
			Actor->Modify();
			Actor->bSelected=1;
		}
	}
#endif
	NoteSelectionChange( Level );
	unguard;
}

#if 1 //NEW: U2Ed

extern TArray<FVertexHit> VertexHitList;
extern void vertexedit_AddPosition( ABrush* pBrush, INT PolyIndex, INT VertexIndex );

void UEditorEngine::edactBoxSelect( UViewport* Viewport, ULevel* Level, FVector Start, FVector End )
{
	guard(UEditorEngine::edactBoxSelect);

	// Create box brush based on the start/end points (normalizes the points).
	FBox SelBBox(0);	SelBBox += Start;	SelBBox += End;

	// If we are not doing a cumulative selection (holding down SHIFT), unselect everything first.
	if( !Viewport->Input->KeyDown(IK_Shift) )
	{
		if( GEditor->Constraints.ShowVertices )
		{
			// Vertices
			VertexHitList.Empty();
		}
		else
		{
			// Actors
			for( INT i=0; i<Level->Actors.Num(); i++ )
				if( Level->Actors(i) )
					Level->Actors(i)->bSelected = 0;
		}
	}

	// Compare the locations of all actors to the selection planes.  If a location is in front of every
	// selection plane, select the actor.
	for( INT i = 0 ; i < Level->Actors.Num() ; i++ )
	{
		AActor* pActor = Level->Actors(i);

		if( pActor )
		{
			if( GEditor->Constraints.ShowVertices )
			{
				// Select vertices
				if( pActor->bSelected && pActor->IsA(ABrush::StaticClass()) )
					for( int poly = 0 ; poly < ((ABrush*)pActor)->Brush->Polys->Element.Num() ; poly++ )
					{
						FPoly pPoly = ((ABrush*)pActor)->Brush->Polys->Element(poly);
						for( int vertex = 0 ; vertex < pPoly.NumVertices ; vertex++ )
						{
							FCoords BrushC(pActor->ToWorld());
							FVector Location = pPoly.Vertex[vertex].TransformPointBy(BrushC);

							switch( Viewport->Actor->RendMap )
							{
								case REN_OrthXY:
									if( FPointPlaneDist( Location, SelBBox.Min, FVector(1,0,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Max, FVector(-1,0,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Min, FVector(0,1,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Max, FVector(0,-1,0) )  >= 0 )
										vertexedit_AddPosition( (ABrush*)pActor, poly, vertex );
									break;

								case REN_OrthXZ:
									if( FPointPlaneDist( Location, SelBBox.Min, FVector(1,0,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Max, FVector(-1,0,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Min, FVector(0,0,1) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Max, FVector(0,0,-1) )  >= 0 )
										vertexedit_AddPosition( (ABrush*)pActor, poly, vertex );
									break;

								case REN_OrthYZ:
									if( FPointPlaneDist( Location, SelBBox.Min, FVector(0,1,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Max, FVector(0,-1,0) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Min, FVector(0,0,1) ) >= 0
											&& FPointPlaneDist( Location, SelBBox.Max, FVector(0,0,-1) )  >= 0 )
										vertexedit_AddPosition( (ABrush*)pActor, poly, vertex );
									break;
							}
						}
					}
			}
			else
			{
				// Select actors
				switch( Viewport->Actor->RendMap )
				{
					case REN_OrthXY:
						if( FPointPlaneDist( pActor->Location, SelBBox.Min, FVector(1,0,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Max, FVector(-1,0,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Min, FVector(0,1,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Max, FVector(0,-1,0) )  >= 0 )
							pActor->bSelected = 1;
						break;

					case REN_OrthXZ:
						if( FPointPlaneDist( pActor->Location, SelBBox.Min, FVector(1,0,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Max, FVector(-1,0,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Min, FVector(0,0,1) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Max, FVector(0,0,-1) )  >= 0 )
							pActor->bSelected = 1;
						break;

					case REN_OrthYZ:
						if( FPointPlaneDist( pActor->Location, SelBBox.Min, FVector(0,1,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Max, FVector(0,-1,0) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Min, FVector(0,0,1) ) >= 0
								&& FPointPlaneDist( pActor->Location, SelBBox.Max, FVector(0,0,-1) )  >= 0 )
							pActor->bSelected = 1;
						break;
				}
			}
		}
	}

	NoteSelectionChange( Level );
	unguard;
}
#endif

#if 1
//
// Select all actors inside the volume of the Default Brush
//
void UEditorEngine::edactSelectInside( ULevel* Level )
{
	guard(UEditorEngine::edactSelectInside);

	// Untag all actors.
	for( INT i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) )
			Level->Actors(i)->bTempEditor = 0;

	// tag all candidate actors
	for( i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Cast<ACamera>(Actor) && Actor!=Level->Brush() && !Actor->bHiddenEd )
		{
			Actor->bTempEditor = 1;
		}
	}

	// deselect all actors that are outside the default brush
	UModel* DefaultBrush = Level->Brush()->Brush;
	FCoords DefaultBrushC(Level->Brush()->ToWorld());
	for( i=0; i<DefaultBrush->Polys->Element.Num(); i++ )
	{
		// get the plane for each polygon in the default brush
		FPoly* Poly = &DefaultBrush->Polys->Element( i );
		FPlane Plane( Poly->Base.TransformPointBy(DefaultBrushC), Poly->Normal.TransformVectorBy(DefaultBrushC) );
		for( INT j=0; j<Level->Actors.Num(); j++ )
		{
			// deselect all actors that are in front of the plane (outside the brush)
			AActor* Actor = Level->Actors(j);
			if( Actor && Actor->bTempEditor ) {
				// treat non-brush actors as point objects
				if( !Cast<ABrush>(Actor) ) {
					FLOAT Dist = Plane.PlaneDot( Actor->Location );
					if( Dist >= 0.0 ) {
						// actor is in front of the plane (outside the default brush)
						Actor->bTempEditor = 0;
					}

				} else {
#if 1 //LEGEND
					// if the brush data is corrupt, abort this actor -- see mpoesch email to Tim sent 9/8/98
					if( Actor->Brush == 0 )
						continue;
#endif
					// examine all the points
					UPolys* Polys = Actor->Brush->Polys;
					for( INT k=0; k<Polys->Element.Num(); k++ ) 
					{
						FCoords BrushC(Actor->ToWorld());
						for( INT m=0; m<Polys->Element(k).NumVertices; m++ ) 
						{
							FLOAT Dist = Plane.PlaneDot( Polys->Element(k).Vertex[m].TransformPointBy(BrushC) );
							if( Dist >= 0.0 )
							{
								// actor is in front of the plane (outside the default brush)
								Actor->bTempEditor = 0;
							}
						}
					}
				}
			}
		}
	}

	// update the selection state with the result from above
	for( i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor->bSelected != Actor->bTempEditor )
		{
			Actor->Modify();
			Actor->bSelected = Actor->bTempEditor;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

//
// Invert the selection of all actors
//
void UEditorEngine::edactSelectInvert( ULevel* Level )
{
	guard(UEditorEngine::edactSelectInvert);
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Cast<ACamera>(Actor) && Actor!=Level->Brush() && !Actor->bHiddenEd )
		{
			Actor->Modify();
			Actor->bSelected ^= 1;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}
#endif

//
// Select all actors in a particular class.
//
void UEditorEngine::edactSelectOfClass( ULevel* Level, UClass* Class )
{
	guard(UEditorEngine::edactSelectOfClass);
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
#if 1
		if( Actor && Actor->GetClass()==Class && !Actor->bSelected && !Actor->bHiddenEd )
#else
		if( Actor && Actor->GetClass()==Class && !Actor->bSelected )
#endif
		{
			Actor->Modify();
			Actor->bSelected=1;
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

#if 1 //NEW: U2Ed
//
// Select all actors in a particular class and its subclasses.
//
void UEditorEngine::edactSelectSubclassOf( ULevel* Level, UClass* Class )
{
	guard(UEditorEngine::edactSelectSubclassOf);
	FName ClassName = Class ? Class->GetFName() : NAME_None;
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Actor->bSelected && !Actor->bHiddenEd )
		{
			for( UClass *TempClass=Actor->GetClass(); TempClass; TempClass=TempClass->GetSuperClass() )
			{
				if( TempClass->GetFName() == ClassName )
				{
					Actor->Modify();
					Actor->bSelected=1;
					break;
				}
			}
		}
	}
	NoteSelectionChange( Level );
	unguard;
}

//
// Select all actors in a level that are marked for deletion.
//
void UEditorEngine::edactSelectDeleted( ULevel* Level )
{
	guard(UEditorEngine::edactSelectDeleted);
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && !Actor->bSelected && !Actor->bHiddenEd )
		{
			if( Actor->bDeleteMe )
			{
				Actor->Modify();
				Actor->bSelected=1;
			}
		}
	}
	NoteSelectionChange( Level );
	unguard;
}
#endif

//
// Recompute and adjust all vertices (based on the current transformations), 
// then reset the transformations
//
void RecomputePoly( FPoly* Poly, INT i );
void UEditorEngine::edactApplyTransform( ULevel* Level )
{
	guard(UEditorEngine::edactApplyTransform);

	// apply transformations to all selected brushes
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor->bSelected && Actor->IsBrush() )
		{
			UModel* Brush = ((ABrush*)Actor)->Brush;

			FModelCoords Coords;
			FLOAT Orientation = ((ABrush*)Actor)->BuildCoords( &Coords, NULL );
			Brush->Modify();

			// recompute new locations for all vertices based on the current transformations
			UPolys* Polys = Brush->Polys;
			Polys->Element.ModifyAllItems();
			for( INT j=0; j<Polys->Element.Num(); j++ )
			{
				Polys->Element(j).Transform( Coords, FVector(0,0,0), FVector(0,0,0), Orientation );

				// the following function is a bit of a hack.  But, for some reason, 
				// the normal/textureU/V recomputation in FPoly::Transform() isn't working.  LEGEND
				RecomputePoly( &Polys->Element(j), j );
			}

			// reset the transformations
			((ABrush*)Actor)->PrePivot = ((ABrush*)Actor)->PrePivot.TransformVectorBy( Coords.PointXform );
			((ABrush*)Actor)->MainScale = GMath.UnitScale;
			((ABrush*)Actor)->PostScale = GMath.UnitScale;
			((ABrush*)Actor)->Rotation  = FRotator(0,0,0);

			((ABrush*)Actor)->Brush->BuildBound();

			((ABrush*)Actor)->PostEditChange();
		}
	}

	unguard;
}

#if 1 //LEGEND
//
// Align all vertices with the current grid
//
// NOTE:	the center handle (origin of the brush) must be selected as the current
//			pivot point, and the brush must be transformed permanently before this 
//			function will cause its vertices to be properly aligned with the grid.
//
// WARNING:	this routine does not verify that 4+ vertex polys remain coplanar.
//			the user must be careful to apply this transformation only to brushes
//			that are aligned in such a way that adjusting their vertices independently
//			will not result in a non-coplanar polygon.  (3 vertex polygons will always
//			be transformed properly.)
//
void UEditorEngine::edactAlignVertices( ULevel* Level )
{
	guard(UEditorEngine::edactAlignVertices);

	// apply transformations to all selected brushes
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor->bSelected && Actor->IsBrush() )
		{
			// snap each vertex in the brush to an integer grid
			UPolys* Polys = ((ABrush*)Actor)->Brush->Polys;
			Polys->Element.ModifyAllItems();
			for( INT j=0; j<Polys->Element.Num(); j++ )
			{
				FPoly* Poly = &Polys->Element(j);
				for( INT k=0; k<Poly->NumVertices; k++ )
				{
					// snap each vertex to the nearest grid
					Poly->Vertex[k].X = appRound( ( Poly->Vertex[k].X + Actor->Location.X )  / Constraints.GridSize.X ) * Constraints.GridSize.X - Actor->Location.X;
					Poly->Vertex[k].Y = appRound( ( Poly->Vertex[k].Y + Actor->Location.Y )  / Constraints.GridSize.Y ) * Constraints.GridSize.Y - Actor->Location.Y;
					Poly->Vertex[k].Z = appRound( ( Poly->Vertex[k].Z + Actor->Location.Z )  / Constraints.GridSize.Z ) * Constraints.GridSize.Z - Actor->Location.Z;
				}

				RecomputePoly( &Polys->Element(j), j );
			}

			((ABrush*)Actor)->Brush->BuildBound();

			((ABrush*)Actor)->PostEditChange();
		}
	}

	unguard;
}
#endif

#if 1 //NEW: ParticleSystems
//
// Export all selected ParticleGenerators to the specified package.
//
void UEditorEngine::edactExportParticleSystems( ULevel* Level, FName PackageName )
{
	guard(UEditorEngine::edactExportParticleSystems);

	// Fix ARL: Find existing package.
	UPackage* Pkg = CreatePackage(NULL,*PackageName);

	{for( FObjectIterator It; It; ++It )
	{
		AActor* Actor = Cast<AActor>(*It);
		if( Actor && Actor->bSelected && Actor->IsA( TEXT("ParticleGenerator") ) )
		{
			debugf( TEXT("Exporting %s"), Actor->GetFullName() );

			AActor* NewActor = (AActor*)UObject::StaticConstructObject( Actor->GetClass(), Pkg, NAME_None, 0, Actor );	
			if( NewActor )
			{
//				NewActor->InitExecution();
//				NewActor->Spawned();

				// Make sure we have a level so we don't crash.
				NewActor->XLevel = GEditor->Level;

				UBOOL GIsSavedScriptableSaved = 1;
				Exchange(GIsScriptable,GIsSavedScriptableSaved);
								
				FuncParms2 Parms;
				Parms.A = Actor;
				Parms.B = 0;
				NewActor->UObject::ProcessEvent(NewActor->FindFunctionChecked(FName(TEXT("Conform"),FNAME_Intrinsic)),&Parms);

				NewActor->UObject::ProcessEvent(NewActor->FindFunctionChecked(FName(TEXT("Clean"),FNAME_Intrinsic)),NULL);
				
				Exchange(GIsScriptable,GIsSavedScriptableSaved);

				NewActor->Region		= NULL;
				NewActor->Level			= NULL;
				NewActor->XLevel		= NULL;
				NewActor->Owner			= NULL;
				NewActor->Instigator	= NULL;
				NewActor->Base			= NULL;
				NewActor->Location		= FVector(0,0,0);
				NewActor->OldLocation	= FVector(0,0,0);
				NewActor->Rotation		= FRotator(0,0,0);
			}
		}
	}}

	SavePackage( Pkg, NULL, 0xffffffff, *(FString(*PackageName)+FString(TEXT(".ups"))), GError, NULL );

	// Clean up
	{for( FObjectIterator It; It; ++It )
	{
		if( It->IsIn(Pkg) && It->IsA( TEXT("ParticleGenerator") ) )
		{
			UObject* Obj = *It;
			Obj->ConditionalDestroy();
			delete Obj;
		}
	}}

	Pkg->ConditionalDestroy();
	delete Pkg;

	unguard;
}

//
// Create a new ParticleGenerator like the given Image.
//
void UEditorEngine::edactCreateNewParticleSystem( ULevel* Level, AActor* Image )
{
	guard(UEditorEngine::edactCreateNewParticleSystem);

	if( !Image )
	{
		debugf( NAME_Warning, TEXT("CreateNewParticleSystem failed : Image == NULL") );
		return;
	}

	debugf(TEXT("Creating new ParticleGenerator (Image=%s)"), Image ? Image->GetFullName() : TEXT("None") );

	Trans->Begin( TEXT("Creating new ParticleGenerator") );
	SelectNone( Level, 0 );
	Level->Modify();

	FVector Collision = FVector(Image->CollisionRadius,Image->CollisionRadius,Image->CollisionHeight);
	FVector Location  = ClickLocation + ClickPlane * (FBoxPushOut(ClickPlane,Collision) + 0.1);

	AActor* Actor = Level->SpawnActor( Image->GetClass(), NAME_None, NULL, NULL, Location, Image->Rotation, Image );
	if( Actor )
	{
		Actor->bDynamicLight = 1;
		Actor->bSelected     = 1;
		if( !Level->FarMoveActor( Actor, Location ) )//necessary??!!
		{
			GWarn->Logf( TEXT("Actor doesn't fit there") );
			Level->DestroyActor( Actor );
		}
		else debugf( NAME_Log, TEXT("Added %s successfully"), Actor->GetClass()->GetName() );
		Actor->PostEditMove();

		UBOOL GIsSavedScriptableSaved = 1;
		Exchange(GIsScriptable,GIsSavedScriptableSaved);
						
		FuncParms2 Parms;
		Parms.A = Image;
		Parms.B = 0;
		Actor->UObject::ProcessEvent(Actor->FindFunctionChecked(FName(TEXT("Conform"),FNAME_Intrinsic)),&Parms);

		Exchange(GIsScriptable,GIsSavedScriptableSaved);
	}
	else GWarn->Logf( TEXT("Actor doesn't fit there") );
	Trans->End();

	NoteSelectionChange( Level );
	unguard;
}

//
// Conform all selected ParticleGenerators to the given Image.
//
void UEditorEngine::edactConformParticleSystems( ULevel* Level, AActor* Image )
{
	guard(UEditorEngine::edactConformParticleSystems);

	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor->bSelected && Actor->IsA( TEXT("ParticleGenerator") ) )
		{
			debugf( TEXT("Conforming %s to %s"), Actor->GetFullName(), Image->GetFullName() );

			UBOOL GIsSavedScriptableSaved = 1;
			Exchange(GIsScriptable,GIsSavedScriptableSaved);
							
			FuncParms2 Parms;
			Parms.A = Image;
			Parms.B = 1;
			Actor->UObject::ProcessEvent(Actor->FindFunctionChecked(FName(TEXT("Conform"),FNAME_Intrinsic)),&Parms);

//			Actor->UObject::ProcessEvent(Actor->FindFunctionChecked(FName(TEXT("Clean"),FNAME_Intrinsic)),NULL);
			
			Exchange(GIsScriptable,GIsSavedScriptableSaved);
		}
	}

	unguard;
}

//
// Delete all particles from level.
//
void UEditorEngine::edactCleanParticleSystems( ULevel* Level )
{
	guard(UEditorEngine::edactCleanParticleSystems);

	UBOOL GIsSavedScriptableSaved = 1;
	Exchange(GIsScriptable,GIsSavedScriptableSaved);

	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor->IsA( TEXT("ParticleGenerator") ) )
		{
			debugf( TEXT("Cleaning %s."), Actor->GetFullName() );
			Actor->UObject::ProcessEvent(Actor->FindFunctionChecked(FName(TEXT("Clean"),FNAME_Intrinsic)),NULL);
		}
	}

	Exchange(GIsScriptable,GIsSavedScriptableSaved);

	Level->ParticleMap.Empty();
	Level->ForceVarsMap.Empty();

	CollectGarbage( RF_Native | RF_Standalone );

	unguard;
}
#endif

/*-----------------------------------------------------------------------------
   The End.
-----------------------------------------------------------------------------*/
