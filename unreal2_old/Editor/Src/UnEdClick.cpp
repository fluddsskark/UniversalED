/*=============================================================================
	UnEdClick.cpp: Editor click-detection code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EditorPrivate.h"
#include "UnRender.h"

#if 1 //NEW: U2Ed (Vertex Editing)
extern TArray<FVertexHit> VertexHitList;
#endif

/*-----------------------------------------------------------------------------
	Adding actors.
-----------------------------------------------------------------------------*/

void UEditorEngine::AddActor( ULevel* Level, UClass* Class, FVector V )
{
	guard(AddActor);
	check(Class);
	debugf( NAME_Log, TEXT("addactor") );

	// Validate everything.
	if( Class->ClassFlags & CLASS_Abstract )
	{
		GWarn->Logf( TEXT("Class %s is abstract.  You can't add actors of this class to the world."), Class->GetName() );
		return;
	}
	if( Class->ClassFlags & CLASS_NoUserCreate )
	{
		GWarn->Logf( TEXT("You can't add actors of this class to the world."), Class->GetName() );
		return;
	}
	else if( Class->ClassFlags & CLASS_Transient )
	{
		GWarn->Logf( TEXT("Class %s is transient.  You can't add actors of this class in UnrealEd."), Class->GetName() );
		return;
	}

	// Transactionally add the actor.
	Trans->Begin( TEXT("Add Actor") );
	SelectNone( Level, 0 );
	Level->Modify();
	AActor* Actor = Level->SpawnActor( Class, NAME_None, NULL, NULL, V );
	if( Actor )
	{
		Actor->bDynamicLight = 1;
		Actor->bSelected     = 1;
		if( !Level->FarMoveActor( Actor, V ) )//necessary??!!
		{
			GWarn->Logf( TEXT("Actor doesn't fit there") );
			Level->DestroyActor( Actor );
		}
		else debugf( NAME_Log, TEXT("Added actor successfully") );
		if( Class->GetDefaultActor()->IsBrush() )
			csgCopyBrush( (ABrush*)Actor, (ABrush*)Class->GetDefaultActor(), 0, 0, 1 );
		Actor->PostEditMove();
	}
	else GWarn->Logf( TEXT("Actor doesn't fit there") );
	Trans->End();

	NoteSelectionChange( Level );
	unguard;
}

/*-----------------------------------------------------------------------------
	HTextureView.
-----------------------------------------------------------------------------*/

void HTextureView::Click( const FHitCause& Cause )
{
	guard(HTextureView::Click);
	check(Texture);
	Texture->Click( Cause.Buttons, Cause.MouseX*Texture->USize/ViewX, Cause.MouseY*Texture->VSize/ViewY );
	unguard;
}

/*-----------------------------------------------------------------------------
	HBackdrop.
-----------------------------------------------------------------------------*/

#if 1 //NEW: U2Ed
// Counts the number of ClipMarkers currently in the world.
int NumClipMarkerS(void)
{
	int markers = 0;

	for( int i = 0 ; i < GEditor->Level->Actors.Num() ; i++ )
	{
		AActor* pActor = GEditor->Level->Actors(i);
		if( pActor && pActor->IsA(AClipMarker::StaticClass()) )
			markers++;
	}

	return markers;
}
#endif

void HBackdrop::Click( const FHitCause& Cause )
{
	guard(HBackdrop::Click);
	GEditor->ClickLocation = Location;
	GEditor->ClickPlane    = FPlane(0,0,0,0);
	if( (Cause.Buttons&MOUSE_Left) && Cause.Viewport->Input->KeyDown(IK_A) )
	{
		if( GEditor->CurrentClass )
		{
			TCHAR Cmd[256];
			appSprintf( Cmd, TEXT("ACTOR ADD CLASS=%s"), GEditor->CurrentClass->GetName() );
			GEditor->Exec( Cmd );
		}
	}
#if 1 //NEW: U2Ed
	else if( Cause.Buttons&MOUSE_Right && Cause.Buttons & MOUSE_Ctrl )
	{
		// If there are 3 (or more) clipmarkers, already in the level delete them so the user can start fresh.
		if( NumClipMarkerS() > 2 )
			GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
		else
			GEditor->Exec( TEXT("ACTOR ADD CLASS=CLIPMARKER SNAP=1") );
	}
#endif
	else if( (Cause.Buttons&MOUSE_Left) && Cause.Viewport->Input->KeyDown(IK_L) )
	{
		GEditor->Exec( TEXT("ACTOR ADD CLASS=LIGHT") );
	}
	else if( Cause.Buttons & MOUSE_Right )
	{
		if( Cause.Viewport->IsOrtho() )
		{
			GEditor->EdCallback( EDC_RtClickWindowCanAdd, 0 );
		}
		else GEditor->EdCallback( EDC_RtClickWindow, 0 );
	}
	else if( Cause.Buttons & MOUSE_Left )
	{
		if( !(Cause.Buttons & MOUSE_Ctrl) )
		{
			GEditor->Trans->Begin( TEXT("Select None") );
			GEditor->SelectNone( Cause.Viewport->Actor->GetLevel(), 1 );
			GEditor->Trans->End();
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	FEditorHitObserver implementation.
-----------------------------------------------------------------------------*/

#if 1 //LEGEND
static FBspSurf GSaveSurf;
#endif
void FEditorHitObserver::Click( const FHitCause& Cause, const struct HBspSurf& Hit )
{
	guard(FEditorHitObserver::ClickHBspSurf);
	UModel*   Model = Cause.Viewport->Actor->GetLevel()->Model;
	FBspSurf& Surf  = Model->Surfs(Hit.iSurf);

	// Adding actor.
	check(Hit.Parent);
	check(Hit.Parent->IsA(TEXT("HCoords")));
	HCoords* HitCoords     = (HCoords*)Hit.Parent;
	FPlane	Plane		   = FPlane(Model->Points(Surf.pBase),Model->Vectors(Surf.vNormal));
	GEditor->ClickLocation = FLinePlaneIntersection( HitCoords->Coords.Origin, HitCoords->Coords.Origin + HitCoords->Direction, Plane );
	GEditor->ClickPlane    = Plane;

	// Remember hit location for actor-adding.
	if( (Cause.Buttons&MOUSE_Left) && Cause.Viewport->Input->KeyDown(IK_A) )
	{
		if( GEditor->CurrentClass )
		{
			TCHAR Cmd[256];
			appSprintf( Cmd, TEXT("ACTOR ADD CLASS=%s"), GEditor->CurrentClass->GetName() );
			GEditor->Exec( Cmd );
		}
	}
	else if( (Cause.Buttons&MOUSE_Left) && Cause.Viewport->Input->KeyDown(IK_L) )
	{
		GEditor->Exec( TEXT("ACTOR ADD CLASS=LIGHT") );
	}
	else if( (Cause.Buttons&MOUSE_Alt) && (Cause.Buttons&MOUSE_Right) )
	{
		// Grab the texture.
		GEditor->CurrentTexture = Surf.Texture;
#if 1 //LEGEND
		GSaveSurf = Surf;
#endif
		GEditor->EdCallback( EDC_CurTexChange, 0 );
	}
	else if( (Cause.Buttons & MOUSE_Shift) && (Cause.Buttons&MOUSE_Left) )
	{
		// Apply texture to all selected.
		GEditor->Trans->Begin( TEXT("apply texture to selected surfaces") );
		for( INT i=0; i<Model->Surfs.Num(); i++ )
		{
#if 1 //NEW: PolyFlagsEx
			if( Model->Surfs(i).PolyFlags[0] & PF_Selected )
#else
			if( Model->Surfs(i).PolyFlags & PF_Selected )
#endif
			{
				Model->ModifySurf( i, 1 );
				Model->Surfs(i).Texture = GEditor->CurrentTexture;
				GEditor->polyUpdateMaster( Model, i, 0, 0 );
			}
		}
		GEditor->Trans->End();
	}
	else if( (Cause.Buttons&MOUSE_Alt) && (Cause.Buttons&MOUSE_Left) )
	{
		// Apply texture to the one polygon clicked on.
		GEditor->Trans->Begin( TEXT("apply texture to surface") );
		Model->ModifySurf( Hit.iSurf, 1 );
		Surf.Texture = GEditor->CurrentTexture;
#if 1 //LEGEND
		if( Cause.Buttons & MOUSE_Ctrl )
		{
#if 1 //NEW: Fix -- MERGED
			Surf.vTextureU	= GSaveSurf.vTextureU;
			Surf.vTextureV	= GSaveSurf.vTextureV;
			if( Surf.vNormal == GSaveSurf.vNormal )
			{
				GLog->Logf( TEXT("WARNING: the texture coordinates were not parallel to the surface.") );
			}
#else
			if( Surf.vNormal == GSaveSurf.vNormal )
			{
				Surf.vTextureU	= GSaveSurf.vTextureU;
				Surf.vTextureV	= GSaveSurf.vTextureV;
			}
			else
			{
				GLog->Logf( TEXT("WARNING: Can't map texture coordinates for non-parallel surfaces.") );
			}
#endif
#if 1 //NEW: PolyFlagsEx
			Surf.PolyFlags[0]	= GSaveSurf.PolyFlags[0];
			Surf.PolyFlags[1]	= GSaveSurf.PolyFlags[1];
#else
			Surf.PolyFlags	= GSaveSurf.PolyFlags;
#endif
			Surf.PanU		= GSaveSurf.PanU;
			Surf.PanV		= GSaveSurf.PanV;
			GEditor->polyUpdateMaster( Model, Hit.iSurf, 1, 1 );
		}
		else
		{
			GEditor->polyUpdateMaster( Model, Hit.iSurf, 0, 0 );
		}
#else
		GEditor->polyUpdateMaster( Model, Hit.iSurf, 0, 0 );
#endif
		GEditor->Trans->End();
	}
	else if( Cause.Buttons & MOUSE_Right ) 
	{
		// Edit surface properties.
		GEditor->Trans->Begin( TEXT("select surface for editing") );
		Model->ModifySurf( Hit.iSurf, 0 );
#if 1 //NEW: PolyFlagsEx
		Surf.PolyFlags[0] |= PF_Selected;
#else
		Surf.PolyFlags |= PF_Selected;
#endif
		GEditor->NoteSelectionChange( Cause.Viewport->Actor->GetLevel() );
		GEditor->EdCallback( EDC_RtClickPoly, 0 );
		GEditor->Trans->End();
	}
	else
	{
		// Select or deselect surfaces.
		GEditor->Trans->Begin( TEXT("select surfaces") );
#if 1 //NEW: PolyFlagsEx
		DWORD SelectMask = Surf.PolyFlags[0] & PF_Selected;
#else
		DWORD SelectMask = Surf.PolyFlags & PF_Selected;
#endif
		if( !(Cause.Buttons & MOUSE_Ctrl) )
			GEditor->SelectNone( Cause.Viewport->Actor->GetLevel(), 0 );
		Model->ModifySurf( Hit.iSurf, 0 );
#if 1 //NEW: PolyFlagsEx
		Surf.PolyFlags[0] = (Surf.PolyFlags[0] & ~PF_Selected) | (SelectMask ^ PF_Selected);
#else
		Surf.PolyFlags = (Surf.PolyFlags & ~PF_Selected) | (SelectMask ^ PF_Selected);
#endif
		GEditor->NoteSelectionChange( Cause.Viewport->Actor->GetLevel() );
		GEditor->Trans->End();
	}
	unguard;
}
void FEditorHitObserver::Click( const FHitCause& Cause, const struct HActor& Hit )
{
	guard(FEditorHitObserver::ClickHActor);

#if 1 //NEW: U2Ed
	// Click on a non-vertex clears the current list of vertices.
	VertexHitList.Empty();
#endif

	// Handle selection.
	GEditor->Trans->Begin( TEXT("clicking on actors") );
	if( Cause.Buttons & MOUSE_Right )
	{
		// Bring up properties of this actor and other selected actors.
		Hit.Actor->Modify();
		Hit.Actor->bSelected = 1;
		GEditor->NoteSelectionChange( Cause.Viewport->Actor->GetLevel() );
		GEditor->EdCallback( EDC_RtClickActor, 0 );
	}
	else if( Cause.Buttons & MOUSE_LeftDouble )
	{
		if( !(Cause.Buttons & MOUSE_Ctrl) )
			GEditor->SelectNone( Cause.Viewport->Actor->GetLevel(), 0 );
		Hit.Actor->Modify();
		Hit.Actor->bSelected = 1;
		GEditor->NoteSelectionChange( Cause.Viewport->Actor->GetLevel() );
		GEditor->Exec( TEXT("HOOK ACTORPROPERTIES") );
	}
	else
	{
		// Toggle actor selection.
		Hit.Actor->Modify();
		if( Cause.Buttons & MOUSE_Ctrl )
		{
			Hit.Actor->bSelected ^= 1;
		}
		else
		{
			GEditor->SelectNone( Cause.Viewport->Actor->GetLevel(), 0 );
			Hit.Actor->bSelected = 1;
		}
		GEditor->NoteSelectionChange( Cause.Viewport->Actor->GetLevel() );
	}
	GEditor->Trans->End();
	unguard;
}

#if 1 //NEW: U2Ed (Vertex Editing)
// Attempts to add a vertex position to the list.
void vertexedit_AddPosition( ABrush* pBrush, INT PolyIndex, INT VertexIndex )
{
	guard(vertexedit_AddPosition);

	// If this position is already in the list, leave.
	for( int vertex = 0 ; vertex < VertexHitList.Num() ; vertex++ )
		if( VertexHitList(vertex) == FVertexHit( pBrush, PolyIndex, VertexIndex ) )
			return;

	// Add it to the list.
	new(VertexHitList)FVertexHit( pBrush, PolyIndex, VertexIndex );

	unguard;
}

void vertexedit_Click( const FHitCause& Cause, ABrush* pBrush, FVector _Location )
{
	guard(vertexedit_Click);

	// If user is not doing a cumulative selection, empty out the current list.
	if( !(Cause.Buttons & MOUSE_Ctrl) )
		VertexHitList.Empty();

	for( int poly = 0 ; poly < pBrush->Brush->Polys->Element.Num() ; poly++ )
	{
		FPoly pPoly = pBrush->Brush->Polys->Element(poly);
		for( int vertex = 0 ; vertex < pPoly.NumVertices ; vertex++ )
		{
			FCoords BrushC(pBrush->ToWorld());
			FVector Location = pPoly.Vertex[vertex].TransformPointBy(BrushC);

			if( Location == _Location )
				vertexedit_AddPosition( pBrush, poly, vertex );
		}
	}

	unguard;
}
#endif

void FEditorHitObserver::Click( const FHitCause& Cause, const struct HBrushVertex& Hit )
{
	guard(FEditorHitObserver::ClickHBrushVertex);

#if 1 //NEW: U2Ed (Vertex Editing)

	if( GEditor->Constraints.ShowVertices )
	{
		FVector NormalHit = Hit.Location - Hit.Brush->Location;
		vertexedit_Click( Cause, Hit.Brush, NormalHit );
	}
	else
	{
		// Set new pivot point.
		GEditor->Trans->Begin( TEXT("brush vertex selection") );
		GEditor->SetPivot( Hit.Location, (Cause.Buttons&MOUSE_Right)!=0, 1 );
		GEditor->Trans->End();
	}
#else
	// Set new pivot point.
	GEditor->Trans->Begin( TEXT("brush vertex selection") );
	GEditor->SetPivot( Hit.Location, (Cause.Buttons&MOUSE_Right)!=0, 1 );
	GEditor->Trans->End();
#endif

	unguard;
}
void FEditorHitObserver::Click( const FHitCause& Cause, const struct HGlobalPivot& Hit )
{
	guard(FEditorHitObserver::ClickHGlobalPivot);

	// Set new pivot point.
	GEditor->Trans->Begin( TEXT("brush vertex selection") );
	GEditor->SetPivot( Hit.Location, (Cause.Buttons&MOUSE_Right)!=0, 1 );
	GEditor->Trans->End();

	unguard;
}
void FEditorHitObserver::Click( const FHitCause& Cause, const struct HBrowserTexture& Hit )
{
	guard(FEditorHitObserver::ClickHBrowserTexture);
	if( Cause.Buttons==MOUSE_Left )
	{
		// Select textures.
		TCHAR Temp[256];
		appSprintf( Temp, TEXT("POLY DEFAULT TEXTURE=%s"), Hit.Texture->GetName() );
		GEditor->Exec( Temp );
		appSprintf( Temp, TEXT("POLY SET TEXTURE=%s"), Hit.Texture->GetName() );
		GEditor->Exec( Temp );
		GEditor->EdCallback( EDC_CurTexChange, 0 );
	}
	else if( Cause.Buttons==MOUSE_Right )
	{
		// Bring up texture popup menu.
		GEditor->CurrentTexture = Hit.Texture;
		GEditor->EdCallback( EDC_RtClickTexture, 0 );
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
