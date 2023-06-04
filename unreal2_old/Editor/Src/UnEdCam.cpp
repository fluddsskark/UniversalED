/*=============================================================================
	UnEdCam.cpp: Unreal editor camera movement/selection functions
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EditorPrivate.h"
#include "UnRender.h"

#if 1 //NEW: U2Ed
extern void brushclipDeleteMarkers();
#endif

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Click flags.
enum EViewportClick
{
	CF_MOVE_ACTOR	= 1,	// Set if the actors have been moved since first click
	CF_MOVE_TEXTURE = 2,	// Set if textures have been adjusted since first click
	CF_MOVE_ALL     = (CF_MOVE_ACTOR | CF_MOVE_TEXTURE),
};

// Internal declarations.
void NoteTextureMovement( ULevel* Level );
void MoveActors( ULevel* Level, FVector Delta, FRotator DeltaRot, UBOOL Constrained, APlayerPawn* ViewActor );

// Global variables.
#if 1 //NEW: U2Ed
__declspec(dllexport) INT GLastScroll=0;
#else
INT GLastScroll=0;
#endif
INT GFixPanU=0, GFixPanV=0;
INT GFixScale=0;
INT GForceXSnap=0, GForceYSnap=0, GForceZSnap=0;
#if 1 //NEW: U2Ed
FString GTexNameFilter;
#endif

// Editor state.
UBOOL GPivotShown=0, GSnapping=0;
FVector GPivotLocation, GSnappedLocation, GGridBase;
FRotator GPivotRotation, GSnappedRotation;

// Temporary.
static TArray<INT> OriginalUVectors;
static TArray<INT> OriginalVVectors;
static INT OrigNumVectors=0;

/*-----------------------------------------------------------------------------
   Primitive mappings of input to axis movement and rotation.
-----------------------------------------------------------------------------*/

//
// Axial rotation.
//
void CalcAxialRot
( 
	UViewport*	Viewport, 
	SWORD		MouseX,
	SWORD		MouseY,
	DWORD		Buttons,
	FRotator&	Delta
)
{
	guard(CalcAxialPerspRot);

	// Do single-axis movement.
	if	   ( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left)             ) Delta.Pitch = +MouseX*4;
	else if( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Right)            )	Delta.Yaw   = +MouseX*4;
	else if( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left|MOUSE_Right) ) Delta.Roll  = -MouseY*4;

	unguard;
}

//
// Freeform movement and rotation.
//
void CalcFreeMoveRot
(
	UViewport*	Viewport,
	FLOAT		MouseX,
	FLOAT		MouseY,
	DWORD		Buttons,
	FVector&	Delta,
	FRotator&	DeltaRot
)
{
	guard(CalcFreeMoveRot);
	if( Viewport->IsOrtho() )
	{
		// Figure axes.
		FLOAT *OrthoAxis1, *OrthoAxis2, Axis2Sign, Axis1Sign, *OrthoAngle, AngleSign;
		FLOAT DeltaPitch = DeltaRot.Pitch;
		FLOAT DeltaYaw   = DeltaRot.Yaw;
		FLOAT DeltaRoll  = DeltaRot.Roll;
		if( Viewport->Actor->RendMap == REN_OrthXY )
		{
			OrthoAxis1 = &Delta.X;  	Axis1Sign = +1;
			OrthoAxis2 = &Delta.Y;  	Axis2Sign = +1;
			OrthoAngle = &DeltaYaw;		AngleSign = +1;
		}
		else if( Viewport->Actor->RendMap==REN_OrthXZ )
		{
			OrthoAxis1 = &Delta.X; 		Axis1Sign = +1;
			OrthoAxis2 = &Delta.Z; 		Axis2Sign = -1;
			OrthoAngle = &DeltaPitch; 	AngleSign = +1;
		}
		else if( Viewport->Actor->RendMap==REN_OrthYZ )
		{
			OrthoAxis1 = &Delta.Y; 		Axis1Sign = +1;
			OrthoAxis2 = &Delta.Z; 		Axis2Sign = -1;
			OrthoAngle = &DeltaRoll; 	AngleSign = +1;
		}
		else
		{
			appErrorf( TEXT("Invalid rendering mode") );
			return;
		}

		// Special movement controls.
		if( (Buttons&(MOUSE_Left|MOUSE_Right))==MOUSE_Left )
		{
			// Left button: Move up/down/left/right.
			*OrthoAxis1 = Viewport->Actor->OrthoZoom/30000.0*(FLOAT)MouseX;
			if     ( MouseX<0 && *OrthoAxis1==0 ) *OrthoAxis1 = -Axis1Sign;
			else if( MouseX>0 && *OrthoAxis1==0 ) *OrthoAxis1 = +Axis1Sign;

			*OrthoAxis2 = Axis2Sign*Viewport->Actor->OrthoZoom/30000.0*(FLOAT)MouseY;
			if     ( MouseY<0 && *OrthoAxis2==0 ) *OrthoAxis2 = -Axis2Sign;
			else if( MouseY>0 && *OrthoAxis2==0 ) *OrthoAxis2 = +Axis2Sign;
		}
		else if( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left|MOUSE_Right) )
		{
			// Both buttons: Zoom in/out.
			Viewport->Actor->OrthoZoom -= Viewport->Actor->OrthoZoom/200.0 * (FLOAT)MouseY;
			if( Viewport->Actor->OrthoZoom<500.0     ) Viewport->Actor->OrthoZoom = 500.0;
			if( Viewport->Actor->OrthoZoom>2000000.0 ) Viewport->Actor->OrthoZoom = 2000000.0;
		}
		else if( (Buttons&(MOUSE_Left|MOUSE_Right))==MOUSE_Right )
		{
			// Right button: Rotate.
			if( OrthoAngle!=NULL )
				*OrthoAngle = -AngleSign*8.0*(FLOAT)MouseX;
		}
		DeltaRot.Pitch	= DeltaPitch;
		DeltaRot.Yaw	= DeltaYaw;
		DeltaRot.Roll	= DeltaRoll;
	}
	else
	{
		APlayerPawn* Actor = Viewport->Actor;
		if( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left) )
		{
			// Left button: move ahead and yaw.
			Delta.X      = -MouseY * GMath.CosTab(Actor->ViewRotation.Yaw);
			Delta.Y      = -MouseY * GMath.SinTab(Actor->ViewRotation.Yaw);
			DeltaRot.Yaw = +MouseX * 64.0 / 20.0;
		}
		else if( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left|MOUSE_Right) )
		{
			// Both buttons: Move up and left/right.
			Delta.X      = +MouseX * -GMath.SinTab(Actor->ViewRotation.Yaw);
			Delta.Y      = +MouseX *  GMath.CosTab(Actor->ViewRotation.Yaw);
			Delta.Z      = -MouseY;
		}
		else if( (Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Right) )
		{
			// Right button: Pitch and yaw.
			DeltaRot.Pitch = (64.0/12.0) * -MouseY;
			DeltaRot.Yaw   = (64.0/20.0) * +MouseX;
		}
	}
	unguard;
}

//
// Perform axial movement and rotation.
//
void CalcAxialMoveRot
(
	UViewport*	Viewport,
	FLOAT		MouseX,
	FLOAT		MouseY,
	DWORD		Buttons,
	FVector&	Delta,
	FRotator&	DeltaRot
)
{
	guard(CalcFreeMoveRot);
	if( Viewport->IsOrtho() )
	{
		// Figure out axes.
		FLOAT *OrthoAxis1,*OrthoAxis2,Axis2Sign,Axis1Sign,*OrthoAngle,AngleSign;
		FLOAT DeltaPitch = DeltaRot.Pitch;
		FLOAT DeltaYaw   = DeltaRot.Yaw;
		FLOAT DeltaRoll  = DeltaRot.Roll;
		if( Viewport->Actor->RendMap == REN_OrthXY )
		{
			OrthoAxis1 = &Delta.X;  	Axis1Sign = +1;
			OrthoAxis2 = &Delta.Y;  	Axis2Sign = +1;
			OrthoAngle = &DeltaYaw;		AngleSign = +1;
		}
		else if( Viewport->Actor->RendMap == REN_OrthXZ )
		{
			OrthoAxis1 = &Delta.X; 		Axis1Sign = +1;
			OrthoAxis2 = &Delta.Z;		Axis2Sign = -1;
			OrthoAngle = &DeltaPitch; 	AngleSign = +1;
		}
		else if( Viewport->Actor->RendMap == REN_OrthYZ )
		{
			OrthoAxis1 = &Delta.Y; 		Axis1Sign = +1;
			OrthoAxis2 = &Delta.Z; 		Axis2Sign = -1;
			OrthoAngle = &DeltaRoll; 	AngleSign = +1;
		}
		else
		{
			appErrorf( TEXT("Invalid rendering mode") );
			return;
		}

		// Special movement controls.
		if( Buttons & (MOUSE_Left | MOUSE_Right) )
		{
			// Left, right, or both are pressed.
			if( Buttons & MOUSE_Left )
			{
				// Left button: Screen's X-Axis.
      			*OrthoAxis1 = Viewport->Actor->OrthoZoom/30000.0*(FLOAT)MouseX;
      			if     ( MouseX<0 && *OrthoAxis1==0 ) *OrthoAxis1 = -Axis1Sign;
      			else if( MouseX>0 && *OrthoAxis1==0 ) *OrthoAxis1 = +Axis1Sign;
			}
			if( Buttons & MOUSE_Right )
			{
				// Right button: Screen's Y-Axis.
      			*OrthoAxis2 = Axis2Sign*Viewport->Actor->OrthoZoom/30000.0*(FLOAT)MouseY;
      			if     ( MouseY<0 && *OrthoAxis2==0 ) *OrthoAxis2 = -Axis2Sign;
      			else if( MouseY>0 && *OrthoAxis2==0 ) *OrthoAxis2 = +Axis2Sign;
			}
		}
		else if( Buttons & MOUSE_Middle )
		{
			// Middle button: Zoom in/out.
			Viewport->Actor->OrthoZoom -= Viewport->Actor->OrthoZoom/200.0 * (FLOAT)MouseY;
			if	   ( Viewport->Actor->OrthoZoom<500.0     ) Viewport->Actor->OrthoZoom = 500.0;
			else if( Viewport->Actor->OrthoZoom>2000000.0 ) Viewport->Actor->OrthoZoom = 2000000.0;
		}
		DeltaRot.Pitch	= DeltaPitch;
		DeltaRot.Yaw	= DeltaYaw;
		DeltaRot.Roll	= DeltaRoll;
	}
	else
	{
		// Do single-axis movement.
		if		((Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left))			   Delta.X = +MouseX;
		else if ((Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Right))			   Delta.Y = +MouseX;
		else if ((Buttons&(MOUSE_Left|MOUSE_Right))==(MOUSE_Left|MOUSE_Right)) Delta.Z = -MouseY;
	}
	unguard;
}

//
// Mixed movement and rotation.
//
void CalcMixedMoveRot
(
	UViewport*	Viewport,
	FLOAT		MouseX,
	FLOAT		MouseY,
	DWORD		Buttons,
	FVector&	Delta,
	FRotator&	DeltaRot
)
{
	guard(CalcMixedMoveRot);
	if( Viewport->IsOrtho() )
		CalcFreeMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
	else
		CalcAxialMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
	unguard;
}

/*-----------------------------------------------------------------------------
   Viewport movement computation.
-----------------------------------------------------------------------------*/

#if 1 //NEW: U2Ed
extern FVector GBoxSelStart, GBoxSelEnd;
extern UBOOL GbIsBoxSel;
#endif

//
// Move and rotate viewport freely.
//
void ViewportMoveRot
(
	UViewport*	Viewport,
	FVector&	Delta,
	FRotator&	DeltaRot
)
{
	guard(ViewportMoveRot);

	Viewport->Actor->ViewRotation.AddBounded( DeltaRot.Pitch, DeltaRot.Yaw, DeltaRot.Roll );
	Viewport->Actor->Location.AddBounded( Delta );

	unguard;
}

//
// Move and rotate viewport using gravity and collision where appropriate.
//
void ViewportMoveRotWithPhysics
(
	UViewport*	Viewport,
	FVector&	Delta,
	FRotator&	DeltaRot
)
{
	guard(ViewportMoveRotWithPhysics);

	Viewport->Actor->ViewRotation.AddBounded( 4.0*DeltaRot.Pitch, 4.0*DeltaRot.Yaw, 4.0*DeltaRot.Roll );
	Viewport->Actor->Location.AddBounded( Delta );

	unguard;
}

/*-----------------------------------------------------------------------------
   Scale functions.
-----------------------------------------------------------------------------*/

//
// See if a scale is within acceptable bounds:
//
UBOOL ScaleIsWithinBounds( FVector* V, FLOAT Min, FLOAT Max )
{
	guard(ScaleIsWithinBounds);
	FLOAT Temp;

	Temp = Abs(V->X);
	if( Temp<Min || Temp>Max )
		return 0;

	Temp = Abs (V->Y);
	if( Temp<Min || Temp>Max )
		return 0;

	Temp = Abs (V->Z);
	if( Temp<Min || Temp>Max )
		return 0;

	return 1;
	unguard;
}

/*-----------------------------------------------------------------------------
   Change transacting.
-----------------------------------------------------------------------------*/

//
// If this is the first time called since first click, note all selected actors.
//
void UEditorEngine::NoteActorMovement( ULevel* Level )
{
	guard(NoteActorMovement);
	UBOOL Found=0;
	if( !GUndo && !(GEditor->ClickFlags & CF_MOVE_ACTOR) )
	{
		GEditor->ClickFlags |= CF_MOVE_ACTOR;
		GEditor->Trans->Begin( TEXT("Actor movement") );
		GSnapping=0;
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && Actor->bSelected )
				break;
		}
		if( i==Level->Actors.Num() )
		{
			Level->Brush()->Modify();
			Level->Brush()->bSelected = 1;
			GEditor->NoteSelectionChange( Level );
		}
		for( i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && Actor->bSelected && Actor->bEdShouldSnap )
				GSnapping = 1;
		}
		for( i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && Actor->bSelected )
			{
				Actor->Modify();
				Actor->bEdSnap |= GSnapping;
				Found=1;
			}
		}
		GEditor->Trans->End();
	}
	unguard;
}

//
// Finish snapping all brushes in a level.
//
void UEditorEngine::FinishAllSnaps( ULevel* Level )
{
	guard(UEditorEngine::FinishAllSnaps);
	ClickFlags &= ~CF_MOVE_ACTOR;
	for( INT i=0; i<Level->Actors.Num(); i++ )
		if( Level->Actors(i) && Level->Actors(i)->bSelected )
			Level->Actors(i)->PostEditMove();
	unguard;
}

//
// Set the editor's pivot location.
//
void UEditorEngine::SetPivot( FVector NewPivot, UBOOL SnapPivotToGrid, UBOOL DoMoveActors )
{
	guard(UEditorEngine::SetPivot);

	// Set the pivot.
	GPivotLocation   = NewPivot;
	GPivotRotation   = FRotator(0,0,0);
	GGridBase        = FVector(0,0,0);
	GSnappedLocation = GPivotLocation;
	GSnappedRotation = GPivotRotation;
	if( GSnapping || SnapPivotToGrid )
		Constraints.Snap( Level, GSnappedLocation, GGridBase, GSnappedRotation );
	if( SnapPivotToGrid )
	{
		if( DoMoveActors )
			MoveActors( Level, GSnappedLocation-GPivotLocation, FRotator(0,0,0), 0, NULL );
		GPivotLocation = GSnappedLocation;
		GPivotRotation = GSnappedRotation;
	}
	else
	{
		GGridBase = GPivotLocation - GSnappedLocation;
		GSnappedLocation = GPivotLocation;
		Constraints.Snap( Level, GSnappedLocation, GGridBase, GSnappedRotation );
		GPivotLocation = GSnappedLocation;
	}

	// Check all actors.
	INT Count=0, SnapCount=0;
	AActor* SingleActor=NULL;
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		if( Level->Actors(i) && Level->Actors(i)->bSelected )
		{
			Count++;
			SnapCount += Level->Actors(i)->bEdShouldSnap;
			SingleActor = Level->Actors(i);
		}
	}

	// Apply to actors.
	if( Count==1 )
	{
		ABrush* Brush=Cast<ABrush>( SingleActor );
		if( Brush )
		{
			FModelCoords Coords, Uncoords;
			Brush->BuildCoords( &Coords, &Uncoords );
			Brush->Modify();
			Brush->PrePivot += (GSnappedLocation - Brush->Location).TransformVectorBy( Uncoords.PointXform );
			Brush->Location = GSnappedLocation;
			Brush->PostEditChange();
		}
	}

	// Update showing.
	GPivotShown = SnapCount>0 || Count>1;
	unguard;
}

//
// Reset the editor's pivot location.
//
void UEditorEngine::ResetPivot()
{
	guard(UEditorEngine::ResetPivot);
	GPivotShown = 0;
	GSnapping   = 0;
	unguard;
}

//
// Move a single actors.
//
void MoveSingleActor( AActor* Actor, FVector Delta, FRotator DeltaRot )
{
	guard(MoveSingleActor);
	if( Delta != FVector(0,0,0) )
	{
		Actor->bDynamicLight = 1;
		Actor->bLightChanged = 1;
	}
	Actor->Location.AddBounded( Delta );
	Actor->Rotation += DeltaRot;
	if( Cast<APawn>( Actor ) )
		Cast<APawn>( Actor )->ViewRotation = Actor->Rotation;
	unguard;
}

//
// Move and rotate actors.
//
void MoveActors( ULevel* Level, FVector Delta, FRotator DeltaRot, UBOOL Constrained, APlayerPawn* ViewActor )
{
	guard(MoveActors);

	// Transact the actors.
	GEditor->NoteActorMovement( Level );

	// Update global pivot.
	if( Constrained )
	{
		FVector   OldLocation = GSnappedLocation;
		FRotator OldRotation = GSnappedRotation;
		GSnappedLocation      = (GPivotLocation += Delta   );
		GSnappedRotation      = (GPivotRotation += DeltaRot);
		if( GSnapping )
			GEditor->Constraints.Snap( Level, GSnappedLocation, GGridBase, GSnappedRotation );
		Delta                 = GSnappedLocation - OldLocation;
		DeltaRot              = GSnappedRotation - OldRotation;
	}

	// Move the actors.
	if( Delta!=FVector(0,0,0) || DeltaRot!=FRotator(0,0,0) )
	{
		for( INT i=0; i<Level->Actors.Num(); i++ )
		{
			AActor* Actor = Level->Actors(i);
			if( Actor && (Actor->bSelected || Actor==ViewActor) )
			{
				FVector Arm   = GSnappedLocation - Actor->Location;
				FVector Rel   = Arm - Arm.TransformVectorBy(GMath.UnitCoords * DeltaRot);
				MoveSingleActor( Actor, Delta + Rel, DeltaRot );
			}
		}
	}

	unguard;
}

#if 1 //NEW: U2Ed (Vertex Editing)
TArray<FVertexHit> VertexHitList;
FVector VertexEditWkPos;
#endif

#if 1 //LEGEND
/*-----------------------------------------------------------------------------
   Vertex editing functions.
-----------------------------------------------------------------------------*/

struct FPolyVertex {
	FPolyVertex::FPolyVertex( INT i, INT j ) : PolyIndex(i), VertexIndex(j) {};
	INT PolyIndex;
	INT VertexIndex;
};

static AActor* VertexEditActor=NULL;
static TArray<FPolyVertex> VertexEditList;

//
// Find the selected brush and grab the closest vertex when <Alt> is pressed
//
void GrabVertex( ULevel* Level )
{
	guard(GrabVertex);

	if( VertexEditActor!=NULL )
		return;

	// Find the selected brush -- abort if none is found.
	AActor* Actor=NULL;
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		Actor = Level->Actors(i);
		if( Actor && Actor->bSelected && Actor->IsBrush() )
		{
			VertexEditActor = Actor;
			break;
		}
	}
	if( VertexEditActor==NULL )
		return;

	//!! Tim, Undo doesn't seem to work for vertex editing.  Do I need to set RF_Transactional? LEGEND
	VertexEditActor->Brush->Modify();

	// examine all the points and grab those that are within range of the pivot location
	UPolys* Polys = VertexEditActor->Brush->Polys;
	for( i=0; i<Polys->Element.Num(); i++ ) 
	{
		FCoords BrushC(VertexEditActor->ToWorld());
		for( INT j=0; j<Polys->Element(i).NumVertices; j++ ) 
		{
			FVector Location = Polys->Element(i).Vertex[j].TransformPointBy(BrushC);
			// match GPivotLocation against Brush's vertex positions -- find "close" vertex
			if( FDist( Location, GPivotLocation ) < GEditor->Constraints.SnapDistance ) {
				VertexEditList.AddItem( FPolyVertex( i, j ) );
			}
		}
	}

	unguard;
}

void RecomputePoly( FPoly* Poly, INT i )
{
	// force recalculation of normal, and texture U and V coordinates in FPoly::Finalize()
	Poly->Normal = FVector(0,0,0);
#if 1 //NEW: U2Ed
	if( !GEditor->Constraints.TextureLock )
	{
		Poly->TextureU = FVector(0,0,0);
		Poly->TextureV = FVector(0,0,0);
	}
#else
	Poly->TextureU = FVector(0,0,0);
	Poly->TextureV = FVector(0,0,0);
#endif
#if 1
	// catch normalization exceptions to warn about non-planar polys
	try
	{
		Poly->Finalize( 0 );
	}
	catch(...)
	{
		debugf( TEXT("WARNING: FPoly::Finalize() failed on Poly %d  (You broke the poly!)"), i );
	}
#else
	// ignore non-planar polys
	Poly->Finalize( 1 );
#endif
}

//
// Release the vertex when <Alt> is released, then update the brush
//
void ReleaseVertex( ULevel* Level )
{
	guard(ReleaseVertex);

	if( VertexEditActor==NULL )
		return;

	// finalize all the polys in the brush (recompute poly surface and TextureU/V normals)
	UPolys* Polys = VertexEditActor->Brush->Polys;
	for( INT i=0; i<Polys->Element.Num(); i++ ) 
		RecomputePoly( &Polys->Element(i), i );

	VertexEditActor->Brush->BuildBound();

	VertexEditActor=NULL;
	VertexEditList.Empty();

	unguard;
}

//
// Move a vertex.
//
void MoveVertex( ULevel* Level, FVector Delta, UBOOL Constrained )
{
	guard(MoveVertex);

	// Transact the actors.
	GEditor->NoteActorMovement( Level );

	if( VertexEditActor==NULL )
		return;

	// Update global pivot.
	if( Constrained )
	{
		FVector OldLocation = GSnappedLocation;
		GSnappedLocation = ( GPivotLocation += Delta );
		if( GSnapping )
		{
			GGridBase = FVector(0,0,0);
			GEditor->Constraints.Snap( Level, GSnappedLocation, GGridBase, GSnappedRotation );
		}
		Delta = GSnappedLocation - OldLocation;
	}

	// Move the vertex.
	if( Delta!=FVector(0,0,0) )
	{
		// examine all the points
		UPolys* Polys = VertexEditActor->Brush->Polys;

		Polys->Element.ModifyAllItems();

		FModelCoords Uncoords;
		((ABrush*)VertexEditActor)->BuildCoords( NULL, &Uncoords );
		VertexEditActor->Brush->Modify();
		for( INT k=0; k<VertexEditList.Num(); k++ ) 
		{
			INT i = VertexEditList(k).PolyIndex;
			INT j = VertexEditList(k).VertexIndex;
			Polys->Element(i).Vertex[j] += Delta.TransformVectorBy( Uncoords.PointXform );
		}
		VertexEditActor->Brush->PostEditChange();
	}

	unguard;
}
#endif

/*-----------------------------------------------------------------------------
   Editor surface transacting.
-----------------------------------------------------------------------------*/

//
// If this is the first time textures have been adjusted since the user first
// pressed a mouse button, save selected polygons transactionally so this can
// be undone/redone:
//
void NoteTextureMovement( ULevel* Level )
{
	guard(NoteTextureMovement);
	if( !GUndo && !(GEditor->ClickFlags & CF_MOVE_TEXTURE) )
	{
		GEditor->Trans->Begin( TEXT("Texture movement") );
		Level->Model->ModifySelectedSurfs(1);
		GEditor->Trans->End ();
		GEditor->ClickFlags |= CF_MOVE_TEXTURE;
	}
	unguard;
}

/*-----------------------------------------------------------------------------
   Editor viewport movement.
-----------------------------------------------------------------------------*/

//
// Move the edit-viewport.
//
void UEditorEngine::MouseDelta
(
	UViewport*	Viewport,
	DWORD		Buttons,
	FLOAT		MouseX,
	FLOAT		MouseY
)
{
	guard(UEditorEngine::MouseDelta);

	FVector     	Delta,Vector,SnapMin,SnapMax,DeltaMin,DeltaMax,DeltaFree;
	FRotator		DeltaRot;
	FLOAT			TempFloat,TempU,TempV,Speed;
	int				Temp;
	int				i;
	static FLOAT	TextureAngle=0.0;

#if 1 //NEW: U2Ed
	if( GbIsBoxSel
			&& Viewport->IsOrtho()
			&& (Viewport->Input->KeyDown(IK_Alt)
			&& Viewport->Input->KeyDown(IK_Ctrl) ) ) 
	{
		// Extend the box the user is dragging down the correct axis, depending on which ortho view they are in.
		switch(Viewport->Actor->RendMap) {

			case REN_OrthXY:
				GBoxSelEnd.X += Viewport->Actor->OrthoZoom/(ORTHO_LOW_DETAIL / 2)*(FLOAT)MouseX;
				GBoxSelEnd.Y += Viewport->Actor->OrthoZoom/(ORTHO_LOW_DETAIL / 2)*(FLOAT)MouseY;
				break;

			case REN_OrthXZ:
				GBoxSelEnd.X += Viewport->Actor->OrthoZoom/(ORTHO_LOW_DETAIL / 2)*(FLOAT)MouseX;
				GBoxSelEnd.Z -= Viewport->Actor->OrthoZoom/(ORTHO_LOW_DETAIL / 2)*(FLOAT)MouseY;
				break;

			case REN_OrthYZ:
				GBoxSelEnd.Y += Viewport->Actor->OrthoZoom/(ORTHO_LOW_DETAIL / 2)*(FLOAT)MouseX;
				GBoxSelEnd.Z -= Viewport->Actor->OrthoZoom/(ORTHO_LOW_DETAIL / 2)*(FLOAT)MouseY;
				break;
		}
	}
	if( GEditor->Constraints.ShowVertices
			&& Viewport->Input->KeyDown(IK_Ctrl)
			&& !Viewport->Input->KeyDown(IK_Alt)
			&& VertexHitList.Num()
			&& !GbIsBoxSel )
	{
		if( Buttons & MOUSE_FirstHit )
		{
			GEditor->Trans->Begin( TEXT("Multiple Vertex Editing") );
			GSnappedLocation = VertexEditWkPos = VertexHitList(0).pBrush->Brush->Polys->Element(VertexHitList(0).PolyIndex).Vertex[VertexHitList(0).VertexIndex];
		}
		else if( Buttons & MOUSE_LastRelease )
		{
			GEditor->Trans->End();
		}
		else
		{
			NoteActorMovement( Level );
			CalcFreeMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );

			// Constrain the delta.
			FVector OldLocation = GSnappedLocation;
			VertexEditWkPos += Delta;
			GSnappedLocation = VertexEditWkPos;
			GEditor->Constraints.Snap( GSnappedLocation, FVector(0,0,0) );
			Delta = GSnappedLocation - OldLocation;

			if( Delta != FVector(0,0,0) )
			{
				for( INT i=0; i<Level->Actors.Num(); i++ )
				{
					AActor* Actor = Level->Actors(i);
					if( Actor && Actor->bSelected && Actor->IsA(ABrush::StaticClass()) )
					{
						Actor->Brush->Modify();
						Actor->Brush->Polys->Element.ModifyAllItems();
					}
				}

				// Adjust the actual vertices
				for( int vertex = 0 ; vertex < VertexHitList.Num() ; vertex++ )
				{

					FVector* pVtx = &(VertexHitList(vertex).pBrush->Brush->Polys->Element(VertexHitList(vertex).PolyIndex).Vertex[VertexHitList(vertex).VertexIndex]);

					switch( Viewport->Actor->RendMap )
					{
						case REN_OrthXY:
							pVtx->X += Delta.X;
							pVtx->Y += Delta.Y;
							break;

						case REN_OrthXZ:
							pVtx->X += Delta.X;
							pVtx->Z += Delta.Z;
							break;

						case REN_OrthYZ:
							pVtx->Y += Delta.Y;
							pVtx->Z += Delta.Z;
							break;
					}

					RecomputePoly( &(VertexHitList(vertex).pBrush->Brush->Polys->Element(VertexHitList(vertex).PolyIndex)), VertexHitList(vertex).pBrush->Brush->Polys->Element(VertexHitList(vertex).PolyIndex).iLink );
				}

				for( i=0; i<Level->Actors.Num(); i++ )
				{
					AActor* Actor = Level->Actors(i);
					if( Actor && Actor->bSelected && Actor->IsA(ABrush::StaticClass()) )
					{
						Actor->Brush->BuildBound();
						Actor->Brush->PostEditChange();
					}
				}
			}
		}
	}
#endif

	if( Viewport->Actor->RendMap==REN_TexView )
	{
		if( Buttons & MOUSE_FirstHit )
		{
			Viewport->SetMouseCapture( 0, 1 );
		}
		else if( Buttons & MOUSE_LastRelease )
		{
			Viewport->SetMouseCapture( 0, 0 );
		}
		return;
	}
	else if( Viewport->Actor->RendMap==REN_TexBrowser )
	{
		return;
	}

	ABrush* BrushActor = Viewport->Actor->GetLevel()->Brush();

	Delta.X    		= 0.0;  Delta.Y  		= 0.0;  Delta.Z   		= 0.0;
	DeltaRot.Pitch	= 0.0;  DeltaRot.Yaw	= 0.0;  DeltaRot.Roll	= 0.0;
	//
	if( Buttons & MOUSE_FirstHit )
	{
		// Reset flags that last for the duration of the click.
		Viewport->SetMouseCapture( 1, 1 );
		ClickFlags &= ~(CF_MOVE_ALL);
		BrushActor->Modify();
		if( Mode==EM_BrushSnap )
		{
			BrushActor->TempScale = BrushActor->PostScale;
			GForceXSnap           = 0;
			GForceYSnap           = 0;
			GForceZSnap           = 0;
		}
		else if( Mode==EM_TextureRotate )
		{
			// Guarantee that each texture u and v vector on each selected polygon
			// is unique in the world.
			OriginalUVectors.Empty( Viewport->Actor->GetLevel()->Model->Surfs.Num() );
			OriginalVVectors.Empty( Viewport->Actor->GetLevel()->Model->Surfs.Num() );
			OrigNumVectors = Viewport->Actor->GetLevel()->Model->Vectors.Num();
			for( INT i=0; i<Viewport->Actor->GetLevel()->Model->Surfs.Num(); i++ )
			{
				FBspSurf* Surf = &Viewport->Actor->GetLevel()->Model->Surfs(i);
				OriginalUVectors.AddItem( Surf->vTextureU );
				OriginalVVectors.AddItem( Surf->vTextureV );
#if 1 //NEW: PolyFlagsEx
				if( Surf->PolyFlags[0] & PF_Selected )
#else
				if( Surf->PolyFlags & PF_Selected )
#endif
				{
					INT n			=  Viewport->Actor->GetLevel()->Model->Vectors.Add();
					FVector *V		= &Viewport->Actor->GetLevel()->Model->Vectors(n);
					*V				=  Viewport->Actor->GetLevel()->Model->Vectors(Surf->vTextureU);
					Surf->vTextureU = n;
					n				=  Viewport->Actor->GetLevel()->Model->Vectors.Add();
					V				= &Viewport->Actor->GetLevel()->Model->Vectors(n);
					*V				=  Viewport->Actor->GetLevel()->Model->Vectors(Surf->vTextureV);
					Surf->vTextureV = n;
					Surf->iLightMap = INDEX_NONE;
				}
			}
			TextureAngle = 0.0;
		}
#if 1 //NEW: U2Ed
		if( Viewport->IsOrtho()
				&& (Viewport->Input->KeyDown(IK_Alt)
				&& Viewport->Input->KeyDown(IK_Ctrl) ) ) 
		{
			// Start box selection
			GbIsBoxSel = true;
			GBoxSelStart = GBoxSelEnd = GEditor->ClickLocation;
		}
#endif
	}
	if( Buttons & MOUSE_LastRelease )
	{
		Viewport->SetMouseCapture( 0, 0 );
		FinishAllSnaps( Viewport->Actor->GetLevel() );
		if( OriginalUVectors.Num() )
		{
			// Finishing up texture rotate mode.  Go through and minimize the set of
			// vectors we've been adjusting by merging the new vectors in and eliminating
			// duplicates.
			FMemMark Mark(GMem);
			for( INT i=0; i<Viewport->Actor->GetLevel()->Model->Surfs.Num(); i++ )
			{
				FBspSurf *Surf = &Viewport->Actor->GetLevel()->Model->Surfs(i);
#if 1 //NEW: PolyFlagsEx
				if( Surf->PolyFlags[0] & PF_Selected )
#else
				if( Surf->PolyFlags & PF_Selected )
#endif
				{
					// Update master texture coordinates but not base.
					polyUpdateMaster (Viewport->Actor->GetLevel()->Model,i,1,0);

					// Add this poly's vectors, merging with the level's existing vectors.
					Surf->vTextureU = bspAddVector(Viewport->Actor->GetLevel()->Model,&Viewport->Actor->GetLevel()->Model->Vectors(Surf->vTextureU),0);
					Surf->vTextureV = bspAddVector(Viewport->Actor->GetLevel()->Model,&Viewport->Actor->GetLevel()->Model->Vectors(Surf->vTextureV),0);
				}
			}
			Mark.Pop();
			OriginalUVectors.Empty();
			OriginalVVectors.Empty();
		}
#if 1 //NEW: U2Ed
		if( GbIsBoxSel )
		{
			GbIsBoxSel = false;
			GEditor->edactBoxSelect( Viewport, GEditor->Level, GBoxSelStart, GBoxSelEnd );
		}
#endif
	}
	switch( Mode )
	{
		case EM_None:
			debugf( NAME_Warning, TEXT("Editor is disabled") );
			break;
		case EM_ViewportMove:
#if 1 //NEW: U2Ed
			// only enable <Alt> (vertex editing) while mode == EM_ViewportMove 
			// (not available from Zoom or other modes due to "goto")
			//
			// call always because MOUSE_FirstHit can come before or after MOUSE_Alt
			if( Buttons & MOUSE_Alt )
			{
				GrabVertex( Viewport->Actor->GetLevel() );

			// release the vertex if either the mouse button or <Alt> key is released
			}
			else if( Buttons & MOUSE_LastRelease || !( Buttons & MOUSE_Alt ) )
			{
				ReleaseVertex( Viewport->Actor->GetLevel() );
			}
#endif
		case EM_ViewportZoom:
			ViewportMove:
			if( Buttons & (MOUSE_FirstHit | MOUSE_LastRelease | MOUSE_SetMode | MOUSE_ExitMode) )
			{
				Viewport->Actor->Velocity = FVector(0,0,0);
			}
			else
			{
#if 1 //NEW: U2Ed
				if( Buttons & MOUSE_Alt )
				{
					if( !GbIsBoxSel )
					{
						// Move selected vertex.
						CalcFreeMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
						Delta *= 0.25*MovementSpeed;
					}
				}
				else
#endif
   				if( !(Buttons & (MOUSE_Ctrl | MOUSE_Shift) ) )
				{
					// Move camera.
					Speed = 0.30*MovementSpeed;
					if( Viewport->IsOrtho() && Buttons==MOUSE_Right )
					{
						Buttons = MOUSE_Left;
						Speed   = 0.60*MovementSpeed;
					}
					CalcFreeMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
					Delta *= Speed;
				}
				else
				{
					// Move actors.
					CalcMixedMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
					Delta *= 0.25*MovementSpeed;
				}
				if( Mode==EM_ViewportZoom )
				{
					Delta = (Viewport->Actor->Velocity += Delta);
				}
#if 1 //NEW: U2Ed
				if( Buttons & MOUSE_Alt )
				{
					if( !GbIsBoxSel )
					{
						// Move selected vertex.
						MoveVertex( Level, Delta, 1 );
					}
				}
				else
#endif
				if( !(Buttons & (MOUSE_Ctrl | MOUSE_Shift) ) )
				{
					// Move camera.
					ViewportMoveRotWithPhysics( Viewport, Delta, DeltaRot );
				}
				else
				{
					// Move actors.
#if 1 //NEW: U2Ed
					if( ( GEditor->Constraints.ShowVertices && !VertexHitList.Num() )
							|| !GEditor->Constraints.ShowVertices )
   						MoveActors( Level, Delta, DeltaRot, 1, (Buttons & MOUSE_Shift) ? Viewport->Actor : NULL );
#else
   					MoveActors( Level, Delta, DeltaRot, 1, (Buttons & MOUSE_Shift) ? Viewport->Actor : NULL );
#endif
				}
			}
			break;
		case EM_BrushRotate:
			if( !(Buttons&MOUSE_Ctrl) )
				goto ViewportMove;
			CalcAxialRot( Viewport, MouseX, MouseY, Buttons, DeltaRot );
			if( DeltaRot != FRotator(0,0,0) )
			{
				NoteActorMovement( Level );
 				MoveActors( Level, FVector(0,0,0), DeltaRot, 1, (Buttons & MOUSE_Shift) ? Viewport->Actor : NULL );
			}
			break;
		case EM_BrushSheer:
			if( !(Buttons&MOUSE_Ctrl) )
				goto ViewportMove;
			NoteActorMovement( Level );
#if 1 //LEGEND
			for( i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Level->Actors(i);
				if( Actor && Actor->IsBrush() && Actor->bSelected ) {
		   			((ABrush*)Actor)->Modify();
					((ABrush*)Actor)->MainScale.SheerRate = Clamp( ((ABrush*)Actor)->MainScale.SheerRate + (FLOAT)(-MouseY) / 240.0, -4.0, 4.0 );
				}
			}
#else
   			BrushActor->Modify();
			BrushActor->MainScale.SheerRate = Clamp( BrushActor->MainScale.SheerRate + (FLOAT)(-MouseY) / 240.0, -4.0, 4.0 );
#endif
			break;
		case EM_BrushScale:
			if( !(Buttons&MOUSE_Ctrl) )
				goto ViewportMove;
			NoteActorMovement( Level );
#if 1 //LEGEND
			for( i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Level->Actors(i);
				if( Actor && Actor->bSelected )
				{
					if( Actor->IsBrush() )
					{
						ABrush* Brush = Cast<ABrush>(Actor);
		   				Brush->Modify();
						Vector = Brush->MainScale.Scale * (1 + (FLOAT)(-MouseY) / 256.0);
						if( ScaleIsWithinBounds(&Vector,0.05,400.0) )
						{
							Brush->MainScale.Scale = Vector;
						}
					}
					if( Buttons&MOUSE_Alt )
					{
						Actor->Location *= (1 + (FLOAT)(-MouseY) / 256.0);
					}
				}
			}
#else
   			BrushActor->Modify();
			Vector = BrushActor->MainScale.Scale * (1 + (FLOAT)(-MouseY) / 256.0);
			if( ScaleIsWithinBounds(&Vector,0.05,400.0) )
				BrushActor->MainScale.Scale = Vector;
#endif
			break;
		case EM_BrushStretch:
			if (!(Buttons&MOUSE_Ctrl))
				goto ViewportMove;
			NoteActorMovement( Level );
#if 1 //LEGEND
			CalcAxialMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
			for( i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Level->Actors(i);
				if( Actor && Actor->bSelected ) {
					if( Actor->IsBrush() )
					{
						ABrush* Brush = Cast<ABrush>(Actor);
						Brush->Modify();
						Vector = Brush->MainScale.Scale;
						Vector.X *= (1 + Delta.X / 256.0);
						Vector.Y *= (1 + Delta.Y / 256.0);
						Vector.Z *= (1 + Delta.Z / 256.0);
						if( ScaleIsWithinBounds( &Vector, 0.05, 400.0 ) )
						{
							Brush->MainScale.Scale = Vector;
						}
					}
					if( Buttons&MOUSE_Alt )
					{
						Actor->Location.X *= (1 + Delta.X / 256.0);
						Actor->Location.Y *= (1 + Delta.Y / 256.0);
						Actor->Location.Z *= (1 + Delta.Z / 256.0);
					}
				}
			}
#else
			BrushActor->Modify();
			CalcAxialMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
			Vector = BrushActor->MainScale.Scale;
			Vector.X *= (1 + Delta.X / 256.0);
			Vector.Y *= (1 + Delta.Y / 256.0);
			Vector.Z *= (1 + Delta.Z / 256.0);
			if( ScaleIsWithinBounds(&Vector,0.05,400.0) )
				BrushActor->MainScale.Scale = Vector;
#endif
			break;
		case EM_BrushSnap:
			if( !(Buttons&MOUSE_Ctrl) )
				goto ViewportMove;
			NoteActorMovement( Level );
#if 1 //LEGEND
			CalcAxialMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
			for( i=0; i<Level->Actors.Num(); i++ )
			{
				AActor* Actor = Level->Actors(i);
				if( Actor && Actor->IsBrush() && Actor->bSelected ) {
		   			((ABrush*)Actor)->Modify();
					Vector = ((ABrush*)Actor)->TempScale.Scale;
					Vector.X *= (1 + Delta.X / 400.0);
					Vector.Y *= (1 + Delta.Y / 400.0);
					Vector.Z *= (1 + Delta.Z / 400.0);
					if( ScaleIsWithinBounds(&Vector,0.05,400.0) )
					{
						((ABrush*)Actor)->TempScale.Scale = Vector;
						((ABrush*)Actor)->PostScale.Scale = Vector;
						if( Viewport->Actor->GetLevel()->Brush()->Brush->Polys->Element.Num()==0 )
							break;
						FBox Box = ((ABrush*)Actor)->Brush->GetRenderBoundingBox( Actor, 1 );

						SnapMin   = Box.Min; Constraints.Snap(SnapMin,FVector(0,0,0));
						DeltaMin  = ((ABrush*)Actor)->Location - SnapMin;
						DeltaFree = ((ABrush*)Actor)->Location - Box.Min;
						SnapMin.X = ((ABrush*)Actor)->PostScale.Scale.X * DeltaMin.X/DeltaFree.X;
						SnapMin.Y = ((ABrush*)Actor)->PostScale.Scale.Y * DeltaMin.Y/DeltaFree.Y;
						SnapMin.Z = ((ABrush*)Actor)->PostScale.Scale.Z * DeltaMin.Z/DeltaFree.Z;

						SnapMax   = Box.Max; Constraints.Snap(SnapMax,FVector(0,0,0));
						DeltaMax  = ((ABrush*)Actor)->Location - SnapMax;
						DeltaFree = ((ABrush*)Actor)->Location - Box.Max;
						SnapMax.X = ((ABrush*)Actor)->PostScale.Scale.X * DeltaMax.X/DeltaFree.X;
						SnapMax.Y = ((ABrush*)Actor)->PostScale.Scale.Y * DeltaMax.Y/DeltaFree.Y;
						SnapMax.Z = ((ABrush*)Actor)->PostScale.Scale.Z * DeltaMax.Z/DeltaFree.Z;

						// Set PostScale so brush extents are grid snapped in all directions of movement.
						if( GForceXSnap || Delta.X!=0 )
						{
							GForceXSnap = 1;
							if( (SnapMin.X>0.05) &&
								((SnapMax.X<=0.05) ||
								(Abs(SnapMin.X-((ABrush*)Actor)->PostScale.Scale.X) < Abs(SnapMax.X-((ABrush*)Actor)->PostScale.Scale.X))))
								((ABrush*)Actor)->PostScale.Scale.X = SnapMin.X;
							else if( SnapMax.X>0.05 )
								((ABrush*)Actor)->PostScale.Scale.X = SnapMax.X;
						}
						if( GForceYSnap || Delta.Y!=0 )
						{
							GForceYSnap = 1;
							if( (SnapMin.Y>0.05) &&
								((SnapMax.Y<=0.05) ||
								(Abs(SnapMin.Y-((ABrush*)Actor)->PostScale.Scale.Y) < Abs(SnapMax.Y-((ABrush*)Actor)->PostScale.Scale.Y))))
								((ABrush*)Actor)->PostScale.Scale.Y = SnapMin.Y;
							else if( SnapMax.Y>0.05 )
								((ABrush*)Actor)->PostScale.Scale.Y = SnapMax.Y;
						}
						if( GForceZSnap || Delta.Z!=0 )
						{
							GForceZSnap = 1;
							if( (SnapMin.Z>0.05) &&
								((SnapMax.Z<=0.05) ||
								(Abs(SnapMin.Z-((ABrush*)Actor)->PostScale.Scale.Z) < Abs(SnapMax.Z-((ABrush*)Actor)->PostScale.Scale.Z))) )
								((ABrush*)Actor)->PostScale.Scale.Z = SnapMin.Z;
							else if( SnapMax.Z>0.05 )
								((ABrush*)Actor)->PostScale.Scale.Z = SnapMax.Z;
						}
					}
				}
			}
#else
			BrushActor->Modify();
			CalcAxialMoveRot( Viewport, MouseX, MouseY, Buttons, Delta, DeltaRot );
			Vector = BrushActor->TempScale.Scale;
			Vector.X *= (1 + Delta.X / 400.0);
			Vector.Y *= (1 + Delta.Y / 400.0);
			Vector.Z *= (1 + Delta.Z / 400.0);
			if( ScaleIsWithinBounds(&Vector,0.05,400.0) )
			{
				BrushActor->TempScale.Scale = Vector;
				BrushActor->PostScale.Scale = Vector;
				if( Viewport->Actor->GetLevel()->Brush()->Brush->Polys->Element.Num()==0 )
					break;
				FBox Box = BrushActor->Brush->GetRenderBoundingBox( BrushActor, 1 );

				SnapMin   = Box.Min; Constraints.Snap(SnapMin,FVector(0,0,0));
				DeltaMin  = BrushActor->Location - SnapMin;
				DeltaFree = BrushActor->Location - Box.Min;
				SnapMin.X = BrushActor->PostScale.Scale.X * DeltaMin.X/DeltaFree.X;
				SnapMin.Y = BrushActor->PostScale.Scale.Y * DeltaMin.Y/DeltaFree.Y;
				SnapMin.Z = BrushActor->PostScale.Scale.Z * DeltaMin.Z/DeltaFree.Z;

				SnapMax   = Box.Max; Constraints.Snap(SnapMax,FVector(0,0,0));
				DeltaMax  = BrushActor->Location - SnapMax;
				DeltaFree = BrushActor->Location - Box.Max;
				SnapMax.X = BrushActor->PostScale.Scale.X * DeltaMax.X/DeltaFree.X;
				SnapMax.Y = BrushActor->PostScale.Scale.Y * DeltaMax.Y/DeltaFree.Y;
				SnapMax.Z = BrushActor->PostScale.Scale.Z * DeltaMax.Z/DeltaFree.Z;

				// Set PostScale so brush extents are grid snapped in all directions of movement.
				if( GForceXSnap || Delta.X!=0 )
				{
					GForceXSnap = 1;
					if( (SnapMin.X>0.05) &&
						((SnapMax.X<=0.05) ||
						(Abs(SnapMin.X-BrushActor->PostScale.Scale.X) < Abs(SnapMax.X-BrushActor->PostScale.Scale.X))))
						BrushActor->PostScale.Scale.X = SnapMin.X;
					else if( SnapMax.X>0.05 )
						BrushActor->PostScale.Scale.X = SnapMax.X;
				}
				if( GForceYSnap || Delta.Y!=0 )
				{
					GForceYSnap = 1;
					if( (SnapMin.Y>0.05) &&
						((SnapMax.Y<=0.05) ||
						(Abs(SnapMin.Y-BrushActor->PostScale.Scale.Y) < Abs(SnapMax.Y-BrushActor->PostScale.Scale.Y))))
						BrushActor->PostScale.Scale.Y = SnapMin.Y;
					else if( SnapMax.Y>0.05 )
						BrushActor->PostScale.Scale.Y = SnapMax.Y;
				}
				if( GForceZSnap || Delta.Z!=0 )
				{
					GForceZSnap = 1;
					if( (SnapMin.Z>0.05) &&
						((SnapMax.Z<=0.05) ||
						(Abs(SnapMin.Z-BrushActor->PostScale.Scale.Z) < Abs(SnapMax.Z-BrushActor->PostScale.Scale.Z))) )
						BrushActor->PostScale.Scale.Z = SnapMin.Z;
					else if( SnapMax.Z>0.05 )
						BrushActor->PostScale.Scale.Z = SnapMax.Z;
				}
			}
#endif
			break;
		case EM_TexturePan:
		{
			if( !(Buttons&MOUSE_Ctrl) )
				goto ViewportMove;
			NoteTextureMovement( Level );
			if( Buttons == (MOUSE_Left | MOUSE_Right) )
			{
				GFixScale += Fix(MouseY)/64;
				TempFloat = 1.0;
				Temp = Unfix(GFixScale); 
				while( Temp>0 ) {TempFloat *= 0.5; Temp--;}
				while( Temp<0 ) {TempFloat *= 2.0; Temp++;}

				if( Buttons & MOUSE_Left  )	TempU = TempFloat; else TempU = 1.0;
				if( Buttons & MOUSE_Right ) TempV = TempFloat; else TempV = 1.0;

				if( TempU!=1.0 || TempV!=1.0 )
					polyTexScale(Viewport->Actor->GetLevel()->Model,TempU,0.0,0.0,TempV,0);
				GFixScale &= 0xffff;
			}
			else if( Buttons & MOUSE_Left )
			{
				GFixPanU += Fix(MouseX)/16;  GFixPanV += Fix(MouseY)/16;
				polyTexPan(Viewport->Actor->GetLevel()->Model,Unfix(GFixPanU),Unfix(0),0);
				GFixPanU &= 0xffff; GFixPanV &= 0xffff;
			}
			else
			{
				GFixPanU += Fix(MouseX)/16;  GFixPanV += Fix(MouseY)/16;
				polyTexPan(Viewport->Actor->GetLevel()->Model,Unfix(0),Unfix(GFixPanV),0);
				GFixPanU &= 0xffff; GFixPanV &= 0xffff;
			}
			break;
		}
		case EM_TextureRotate:
		{
			if( !(Buttons&MOUSE_Ctrl) )
				goto ViewportMove;
			check(OriginalUVectors.Num()==Viewport->Actor->GetLevel()->Model->Surfs.Num());
			check(OriginalVVectors.Num()==Viewport->Actor->GetLevel()->Model->Surfs.Num());
			NoteTextureMovement( Level );
			TextureAngle += (FLOAT)MouseX / 256.0;
			for( INT i=0; i<Viewport->Actor->GetLevel()->Model->Surfs.Num(); i++ )
			{
				FBspSurf* Surf = &Viewport->Actor->GetLevel()->Model->Surfs(i);
#if 1 //NEW: PolyFlagsEx
				if( Surf->PolyFlags[0] & PF_Selected )
#else
				if( Surf->PolyFlags & PF_Selected )
#endif
				{
					FVector U		=  Viewport->Actor->GetLevel()->Model->Vectors(OriginalUVectors(i));
					FVector V		=  Viewport->Actor->GetLevel()->Model->Vectors(OriginalVVectors(i));
					FVector* NewU	= &Viewport->Actor->GetLevel()->Model->Vectors(Surf->vTextureU);
					FVector* NewV	= &Viewport->Actor->GetLevel()->Model->Vectors(Surf->vTextureV);
					*NewU			= U * appCos(TextureAngle) + V * appSin(TextureAngle);
					*NewV			= V * appCos(TextureAngle) - U * appSin(TextureAngle);
				}
			}
			break;
		}
		default:
			debugf( NAME_Warning, TEXT("Unknown editor mode %i"), Mode );
			goto ViewportMove;
			break;
	}
	if( Viewport->Actor->RendMap != REN_MeshView )
	{
		Viewport->Actor->Rotation = Viewport->Actor->ViewRotation;
		Viewport->Actor->GetLevel()->SetActorZone( Viewport->Actor, 0, 0 );
	}

	unguardf(( TEXT("(Mode=%i)"), Mode ));
}

//
// Mouse position.
//
void UEditorEngine::MousePosition( UViewport* Viewport, DWORD Buttons, FLOAT X, FLOAT Y )
{
	guard(UEditorEngine::MousePosition);
	if( edcamMode(Viewport)==EM_TexView )
	{
		UTexture* Texture = (UTexture *)Viewport->MiscRes;
		X *= (FLOAT)Texture->USize/Viewport->SizeX;
		Y *= (FLOAT)Texture->VSize/Viewport->SizeY;
		if( X>=0 && X<Texture->USize && Y>=0 && Y<Texture->VSize )
			Texture->MousePosition( Buttons, X, Y );
	}
	unguard;
}

/*-----------------------------------------------------------------------------
   Keypress handling.
-----------------------------------------------------------------------------*/

//
// Handle a regular ASCII key that was pressed in UnrealEd.
// Returns 1 if proceesed, 0 if not.
//
INT UEditorEngine::Key( UViewport* Viewport, EInputKey Key )
{
	guard(UEditorEngine::Key);
	if( Viewport->Input->KeyDown(IK_Alt) )
	{
#if 1 //NEW: U2Ed
		FString Cmd;
		switch( Key )
		{
			case IK_1:	Cmd = TEXT("RMODE 1");	break;
			case IK_2:	Cmd = TEXT("RMODE 2");	break;
			case IK_3:	Cmd = TEXT("RMODE 3");	break;
			case IK_4:	Cmd = TEXT("RMODE 4");	break;
			case IK_5:	Cmd = TEXT("RMODE 5");	break;
			case IK_6:	Cmd = TEXT("RMODE 6");	break;
			case IK_7:	Cmd = TEXT("RMODE 13");	break;
			case IK_8:	Cmd = TEXT("RMODE 14");	break;
			case IK_9:	Cmd = TEXT("RMODE 15");	break;
			default:	return 0;
		}

		int iRet = Viewport->Exec( *Cmd );
		EdCallback( EDC_ViewportUpdateWindowFrame, 1 );
		return iRet;
#else
		switch( Key )
		{
			case IK_1:	return Viewport->Exec( TEXT("RMODE 1") );
			case IK_2:	return Viewport->Exec( TEXT("RMODE 2") );
			case IK_3:	return Viewport->Exec( TEXT("RMODE 3") );
			case IK_4:	return Viewport->Exec( TEXT("RMODE 4") );
			case IK_5:	return Viewport->Exec( TEXT("RMODE 5") );
			default:	break;
		}
#endif
	}
	if( UEngine::Key( Viewport, Key ) )
	{
		return 1;
	}
	else if( Viewport->Actor->RendMap==REN_TexView )
	{
		return 0;
	}
	else if( Viewport->Actor->RendMap==REN_TexBrowser )
	{
		if( appToUpper(Key)=='Q' && CurrentTexture )
		{
			debugf(TEXT("Removing texture %s from level"),CurrentTexture->GetFullName());
			for( TArray<AActor*>::TIterator ItA(Viewport->Actor->GetLevel()->Actors); ItA; ++ItA )
			{
				AActor* Actor = *ItA;
				if( Actor )
				{
					UModel* M = Actor->IsA(ALevelInfo::StaticClass()) ? Actor->GetLevel()->Model : Actor->Brush;
					if( M )
					{
						for( TArray<FBspSurf>::TIterator ItS(M->Surfs); ItS; ++ItS )
							if( ItS->Texture==CurrentTexture )
								ItS->Texture = NULL;
						if( M->Polys )
							for( TArray<FPoly>::TIterator ItP(M->Polys->Element); ItP; ++ItP )
								if( ItP->Texture==CurrentTexture )
									ItP->Texture = NULL;
					}
				}
			}
			RedrawLevel(NULL);
		}
		return 0;
	}
	else if( Viewport->Actor->RendMap==REN_MeshView )
	{
		return 0;
	}
#if 1 //NEW: U2Ed
	else if( Viewport->Input->KeyDown(IK_Shift) )
	{
		if( Viewport->Input->KeyDown(IK_A) ) {  Exec( TEXT("ACTOR SELECT ALL") ); return 1; }
		if( Viewport->Input->KeyDown(IK_B) ) {  Exec( TEXT("POLY SELECT MATCHING BRUSH") ); return 1; }
		if( Viewport->Input->KeyDown(IK_C) ) {  Exec( TEXT("POLY SELECT ADJACENT COPLANARS") ); return 1; }
		if( Viewport->Input->KeyDown(IK_D) ) {  Exec( TEXT("ACTOR DUPLICATE") ); return 1; }
		if( Viewport->Input->KeyDown(IK_F) ) {  Exec( TEXT("POLY SELECT ADJACENT FLOORS") ); return 1; }
		if( Viewport->Input->KeyDown(IK_G) ) {  Exec( TEXT("POLY SELECT MATCHING GROUPS") ); return 1; }
		if( Viewport->Input->KeyDown(IK_I) ) {  Exec( TEXT("POLY SELECT MATCHING ITEMS") ); return 1; }
		if( Viewport->Input->KeyDown(IK_J) ) {  Exec( TEXT("POLY SELECT ADJACENT ALL") ); return 1; }
		if( Viewport->Input->KeyDown(IK_M) ) {  Exec( TEXT("POLY SELECT MEMORY SET") ); return 1; }
		if( Viewport->Input->KeyDown(IK_N) ) {  Exec( TEXT("SELECT NONE") ); return 1; }
		if( Viewport->Input->KeyDown(IK_O) ) {  Exec( TEXT("POLY SELECT MEMORY INTERSECT") ); return 1; }
		if( Viewport->Input->KeyDown(IK_Q) ) {  Exec( TEXT("POLY SELECT REVERSE") ); return 1; }
		if( Viewport->Input->KeyDown(IK_R) ) {  Exec( TEXT("POLY SELECT MEMORY RECALL") ); return 1; }
		if( Viewport->Input->KeyDown(IK_S) ) {  Exec( TEXT("POLY SELECT ALL") ); return 1; }
		if( Viewport->Input->KeyDown(IK_T) ) {  Exec( TEXT("POLY SELECT MATCHING TEXTURE") ); return 1; }
		if( Viewport->Input->KeyDown(IK_U) ) {  Exec( TEXT("POLY SELECT MEMORY UNION") ); return 1; }
		if( Viewport->Input->KeyDown(IK_W) ) {  Exec( TEXT("POLY SELECT ADJACENT WALLS") ); return 1; }
		if( Viewport->Input->KeyDown(IK_Y) ) {  Exec( TEXT("POLY SELECT ADJACENT SLANTS") ); return 1; }
		if( Viewport->Input->KeyDown(IK_X) ) {  Exec( TEXT("POLY SELECT MEMORY XOR") ); return 1; }

		return 0;
	}
	else if( Viewport->Input->KeyDown(IK_Ctrl) )
	{
		if( Viewport->Input->KeyDown(IK_C) ) { Exec( TEXT("EDIT COPY") );	return 1; }
		if( Viewport->Input->KeyDown(IK_V) ) { Exec( TEXT("EDIT PASTE") );	return 1; }
		if( Viewport->Input->KeyDown(IK_W) ) { Exec( TEXT("ACTOR DUPLICATE") );	return 1; }
		if( Viewport->Input->KeyDown(IK_X) ) { Exec( TEXT("EDIT CUT") );	return 1; }
		if( Viewport->Input->KeyDown(IK_Y) ) { Exec( TEXT("TRANSACTION REDO") );	return 1; }
		if( Viewport->Input->KeyDown(IK_Z) ) { Exec( TEXT("TRANSACTION UNDO") );	return 1; }
		if( Viewport->Input->KeyDown(IK_A) ) { Exec( TEXT("BRUSH ADD") );	return 1; }
		if( Viewport->Input->KeyDown(IK_S) ) { Exec( TEXT("BRUSH SUBTRACT") );	return 1; }
		if( Viewport->Input->KeyDown(IK_I) ) { Exec( TEXT("BRUSH FROM INTERSECTION") );	return 1; }
		if( Viewport->Input->KeyDown(IK_D) ) { Exec( TEXT("BRUSH FROM DEINTERSECTION") );	return 1; }
		if( Viewport->Input->KeyDown(IK_L) ) { GEditor->EdCallback( EDC_SaveMap, 1 );	return 1; }
		if( Viewport->Input->KeyDown(IK_O) ) { GEditor->EdCallback( EDC_LoadMap, 1 );	return 1; }
		if( Viewport->Input->KeyDown(IK_P) ) { GEditor->EdCallback( EDC_PlayMap, 1 );	return 1; }

		return 0;
	}
	else if( !Viewport->Input->KeyDown(IK_Alt) )
	{
		if( Viewport->Input->KeyDown(IK_Delete) ) { Exec( TEXT("ACTOR DELETE") );	return 1; }
#if 0 //NEW: U2Ed -- We don't like this, so we disabled it.
		if( Viewport->Input->KeyDown(IK_1) ) { MovementSpeed = 1; return 1; }
		if( Viewport->Input->KeyDown(IK_2) ) {  MovementSpeed = 4; return 1; }
		if( Viewport->Input->KeyDown(IK_3) ) {  MovementSpeed = 16; return 1; }
#endif
		if( Viewport->Input->KeyDown(IK_B) ) {  Viewport->Actor->ShowFlags ^= SHOW_Brush; return 1; }
		if( Viewport->Input->KeyDown(IK_H) ) {  Viewport->Actor->ShowFlags ^= SHOW_Actors; return 1; }
		if( Viewport->Input->KeyDown(IK_K) ) {  Viewport->Actor->ShowFlags ^= SHOW_Backdrop; return 1; }
		if( Viewport->Input->KeyDown(IK_P) ) {  Viewport->Actor->ShowFlags ^= SHOW_PlayerCtrl; return 1; }

		return 0;
	}

	return 0;
#else
	else if( Viewport->Input->KeyDown(IK_Shift) )
	{
		switch( appToUpper(Key) )
		{
			case 'A': Exec( TEXT("ACTOR SELECT ALL") ); return 1;
			case 'B': Exec( TEXT("POLY SELECT MATCHING BRUSH") ); return 1;
			case 'C': Exec( TEXT("POLY SELECT ADJACENT COPLANARS") ); return 1;
			case 'D': Exec( TEXT("ACTOR DUPLICATE") ); return 1;
			case 'F': Exec( TEXT("POLY SELECT ADJACENT FLOORS") ); return 1;
			case 'G': Exec( TEXT("POLY SELECT MATCHING GROUPS") ); return 1;
			case 'I': Exec( TEXT("POLY SELECT MATCHING ITEMS") ); return 1;
			case 'J': Exec( TEXT("POLY SELECT ADJACENT ALL") ); return 1;
			case 'M': Exec( TEXT("POLY SELECT MEMORY SET") ); return 1;
			case 'N': Exec( TEXT("SELECT NONE") ); return 1;
			case 'O': Exec( TEXT("POLY SELECT MEMORY INTERSECT") ); return 1;
			case 'Q': Exec( TEXT("POLY SELECT REVERSE") ); return 1;
			case 'R': Exec( TEXT("POLY SELECT MEMORY RECALL") ); return 1;
			case 'S': Exec( TEXT("POLY SELECT ALL") ); return 1;
			case 'T': Exec( TEXT("POLY SELECT MATCHING TEXTURE") ); return 1;
			case 'U': Exec( TEXT("POLY SELECT MEMORY UNION") ); return 1;
			case 'W': Exec( TEXT("POLY SELECT ADJACENT WALLS") ); return 1;
			case 'Y': Exec( TEXT("POLY SELECT ADJACENT SLANTS") ); return 1;
			case 'X': Exec( TEXT("POLY SELECT MEMORY XOR") ); return 1;
			case 'Z': Exec( TEXT("SELECT NONE") ); return 1;
			default: return 0;
		}
	}
	else if( Viewport->Input->KeyDown(IK_Shift) )
	{
		switch( appToUpper( Key ) )
		{
			case 'C': Exec( TEXT("EDIT COPY") ); return 1;
			case 'V': Exec( TEXT("EDIT PASTE") ); return 1;
			case 'X': Exec( TEXT("EDIT CUT") ); return 1;
			case 'L': Viewport->Actor->ViewRotation.Pitch = 0; Viewport->Actor->ViewRotation.Roll  = 0; return 1;
			default: return 0;
		}
	}
	else if( !Viewport->Input->KeyDown(IK_Alt) )
	{
		switch( appToUpper(Key) )
		{
			case IK_Delete: Exec( TEXT("ACTOR DELETE") ); return 1;
			case '1': MovementSpeed = 1; return 1;
			case '2': MovementSpeed = 4; return 1;
			case '3': MovementSpeed = 16; return 1;
			case 'B': Viewport->Actor->ShowFlags ^= SHOW_Brush; return 1;
			case 'H': Viewport->Actor->ShowFlags ^= SHOW_Actors; return 1;
			case 'K': Viewport->Actor->ShowFlags ^= SHOW_Backdrop; return 1;
			case 'P': Viewport->Actor->ShowFlags ^= SHOW_PlayerCtrl; return 1;
			default: return 0;
		}
	}
	else
	{
		return 0;
	}
#endif
	unguard;
}

/*-----------------------------------------------------------------------------
   Texture browser routines.
-----------------------------------------------------------------------------*/

void DrawViewerBackground( FSceneNode* Frame )
{
	guard(DrawViewerBackground);
	Frame->Viewport->Canvas->DrawPattern( GEditor->Bkgnd, 0, 0, Frame->X, Frame->Y, 1.0, 0.0, 0.0, NULL, 1.0, FPlane(.4,.4,.4,0), FPlane(0,0,0,0), 0 );
	unguard;
}

int CDECL ResNameCompare(const void *A, const void *B)
{
	return appStricmp((*(UObject **)A)->GetName(),(*(UObject **)B)->GetName());
}

void DrawTextureBrowser( FSceneNode* Frame )
{
	guard(DrawTextureBrowser);
	UObject* Pkg = Frame->Viewport->MiscRes;
	if( Pkg && Frame->Viewport->Group!=NAME_None )
		Pkg = FindObject<UPackage>( Pkg, *Frame->Viewport->Group );

	FMemMark Mark(GMem);
	enum {MAX=16384};
	UTexture**  List    = new(GMem,MAX)UTexture*;
	INT			Size	= Frame->Viewport->Actor->Misc1;
	INT			PerRow	= Frame->X/Size;
	INT			Space	= (Frame->X - Size*PerRow)/(PerRow+1);
	INT			VSkip	= (Size>=64) ? 10 : 0;

	// Make the list.
	INT n = 0;
	for( TObjectIterator<UTexture> It; It && n<MAX; ++It )
		if( It->IsIn(Pkg) )
#if 1 //NEW: U2Ed
		{
			FString TexName = It->GetName();

			if( appStrstr( *(TexName.Caps()), *(GTexNameFilter.Caps()) ) )
				List[n++] = *It;
		}
#else
			List[n++] = *It;
#endif

	// Sort textures by name.
	appQsort( &List[0], n, sizeof(UTexture *), ResNameCompare );

	// Draw them.
	INT YL = Space+(Size+Space+VSkip)*((n+PerRow-1)/PerRow);
	if( YL > 0 )
	{
		INT YOfs = -((Frame->Viewport->Actor->Misc2*Frame->Y)/512);
		for( INT i=0; i<n; i++ )
		{
			UTexture* Texture = List[i];
			INT X = (Size+Space)*(i%PerRow);
			INT Y = (Size+Space+VSkip)*(i/PerRow)+YOfs;
			if( Y+Size+Space+VSkip>0 && Y<Frame->Y )
			{
				PUSH_HIT(Frame,HBrowserTexture(Texture));
				if( Texture==GEditor->CurrentTexture )
					Frame->Viewport->Canvas->DrawPattern( GEditor->BkgndHi, X+1, Y+1, Size+Space*2-2, Size+Space*2+VSkip-2, 1.0, 0.0, 0.0, NULL, 1.0, FPlane(1.,1.,1.,0), FPlane(0,0,0,0), 0 );
				FLOAT Scale=0.125;
				while( Texture->USize/Scale>Size || Texture->VSize/Scale>Size )
					Scale *= 2;
				Frame->Viewport->Canvas->DrawPattern( Texture, X+Space, Y+Space, Size, Size, Scale, X+Space, Y+Space, NULL, 1.0, FPlane(1.,1.,1.,0), FPlane(0,0,0,0), 0 );
				if( Size>=64 )
				{
					FString Temp = Texture->GetName();
					if( Size>=128 )
						Temp += FString::Printf( TEXT(" (%ix%i)"), Texture->USize, Texture->VSize );
					Frame->Viewport->Canvas->SetClip( X, Y+Size+1, Size, Frame->Y-Y-Size-1 );
					Frame->Viewport->Canvas->WrappedPrintf( Frame->Viewport->Canvas->MedFont, 1, TEXT("%s"), *Temp );
					Frame->Viewport->Canvas->SetClip( 0, 0, Frame->X, Frame->Y );
				}
				POP_HIT(Frame);
			}
		}
	}
	Mark.Pop();
	GLastScroll = Max(0,(512*(YL-Frame->Y))/Frame->Y);
	unguard;
}

/*-----------------------------------------------------------------------------
   Buttons.
-----------------------------------------------------------------------------*/

// Menu toggle button.
struct HMenuToggleButton : public HHitProxy
{
	void Click( const FHitCause& Cause )
	{
		Cause.Viewport->Actor->ShowFlags ^= SHOW_Menu;
		Cause.Viewport->UpdateWindowFrame();
	}
};

// Player control button.
struct HPlayerControlButton : public HHitProxy
{
	void Click( const FHitCause& Cause )
	{
		Cause.Viewport->Actor->ShowFlags ^= SHOW_PlayerCtrl;
		Cause.Viewport->Logf( NAME_Log, TEXT("Player controls are %s"), (Cause.Viewport->Actor->ShowFlags&SHOW_PlayerCtrl) ? TEXT("On") : TEXT("Off") );
	}
};

// Draw an onscreen mouseable button.
static INT DrawButton( FSceneNode* Frame, UTexture* Texture, INT X, INT Y )
{
	guard(DrawButton);
	Frame->Viewport->Canvas->DrawIcon( Texture, X+0.5, Y+0.5, Texture->USize, Texture->VSize, NULL, 0.0, FPlane(0.9,0.9,0.9,0), FPlane(0,0,0,0), 0 );
	return Texture->USize+2;
	unguard;
}

// Draw all buttons.
static void DrawButtons( FSceneNode* Frame )
{
	guard(DrawButtons);
	INT ButtonX=2;

	// Menu toggle button.
	PUSH_HIT(Frame,HMenuToggleButton());
	ButtonX += DrawButton( Frame, (Frame->Viewport->Actor->ShowFlags&SHOW_Menu) ? GEditor->MenuUp : GEditor->MenuDn, ButtonX, 2 );
	POP_HIT(Frame);

	// Player control button.
	PUSH_HIT(Frame,HPlayerControlButton());
	if( !Frame->Viewport->IsOrtho() )
		ButtonX += DrawButton( Frame, (Frame->Viewport->Actor->ShowFlags&SHOW_PlayerCtrl) ? GEditor->PlyrOn : GEditor->PlyrOff, ButtonX, 2 );
	POP_HIT(Frame);

	unguard;
}

/*-----------------------------------------------------------------------------
   Viewport frame drawing.
-----------------------------------------------------------------------------*/

#if 1
//!! hack to avoid modifying EditorEngine.uc, and rebuilding Editor.u
// (see UnEdRend.cpp for further details).
extern FLOAT EdClipZ;
#endif

#if 1 //NEW: Interpolation Paths (from Ion Storm, Austin) -- MERGED
//
// DEUS_EX CNN - for drawing interpolation paths
// copied from Engine.UnScript.cpp
//
FLOAT Splerp( FLOAT F )
{
	FLOAT S = Square(F);
	return (1.0/16.0)*S*S - (1.0/2.0)*S + 1;
}
#endif

//
// Draw the camera view.
//
void UEditorEngine::Draw( UViewport* Viewport, UBOOL Blit, BYTE* HitData, INT* HitCount )
{
	FVector			OriginalLocation;
	FRotator		OriginalRotation;
	DWORD			ShowFlags=0;
	guard(UEditorEngine::Draw);
	APlayerPawn* Actor = Viewport->Actor;
	ShowFlags = Actor->ShowFlags;

	// Lock the camera.
	DWORD LockFlags = 0;
	FPlane ScreenClear(0,0,0,0);
	if( Actor->RendMap==REN_MeshView )
	{
		LockFlags |= LOCKR_ClearScreen;
	}
	if(	Viewport->IsOrtho()
	||	Viewport->IsWire()
	|| !(Viewport->Actor->ShowFlags & SHOW_Backdrop) )
	{
		ScreenClear = Viewport->IsOrtho() ? C_OrthoBackground.Plane() : C_WireBackground.Plane();
		LockFlags |= LOCKR_ClearScreen;
	}
	if( !Viewport->Lock(FVector(.5,.5,.5),FVector(0,0,0),ScreenClear,LockFlags,HitData,HitCount) )
	{
		return;
	}

	FSceneNode* Frame = Render->CreateMasterFrame( Viewport, Viewport->Actor->Location, Viewport->Actor->ViewRotation, NULL );
	Render->PreRender( Frame );
	Viewport->Canvas->Update( Frame );
	switch( Actor->RendMap )
	{
		case REN_TexView:
		{
			guard(REN_TexView);
			check(Viewport->MiscRes!=NULL);
			Actor->bHiddenEd = 1;
			Actor->bHidden   = 1;
			UTexture* Texture = (UTexture*)Viewport->MiscRes;
			PUSH_HIT(Frame,HTextureView(Texture,Frame->X,Frame->Y));
			Viewport->Canvas->DrawIcon( Texture->Get(Viewport->CurrentTime), 0, 0, Frame->X, Frame->Y, NULL, 1.0, FPlane(1,1,1,0), FPlane(0,0,0,0), PF_TwoSided );
			POP_HIT(Frame);
			unguard;
			break;
		}
		case REN_TexBrowser:
		{
			guard(REN_TexBrowser);
			Actor->bHiddenEd = 1;
			Actor->bHidden   = 1;
			DrawViewerBackground( Frame );
			DrawTextureBrowser( Frame );
			unguard;
			break;
		}
		case REN_MeshView:
		{
			guard(REN_MeshView);
			FLOAT DeltaTime = Viewport->CurrentTime - Viewport->LastUpdateTime;

			// Rotate the view.
			FVector NewLocation = Viewport->Actor->ViewRotation.Vector() * (-Actor->Location.Size());
			if( FDist(Actor->Location, NewLocation) > 0.05 )
				Actor->Location = NewLocation;

			// Get animation.
			UMesh* Mesh = (UMesh*)Viewport->MiscRes;
			if( Actor->Misc1 < Mesh->AnimSeqs.Num() )
				Actor->AnimSequence = Mesh->AnimSeqs( Actor->Misc1 ).Name;
			else
				Actor->AnimSequence = NAME_None;
			const FMeshAnimSeq* Seq = Mesh->GetAnimSeq( Actor->AnimSequence );

			// Auto rotate if wanted.
			if( Actor->ShowFlags & SHOW_Brush )
				Actor->ViewRotation.Yaw += Clamp(DeltaTime,0.f,0.2f) * 8192.0;

			// Do coordinates.
			Frame->ComputeRenderCoords( Viewport->Actor->Location, Viewport->Actor->ViewRotation );
			PUSH_HIT(Frame,HCoords(Frame));

			// Remember.
			OriginalLocation		= Actor->Location;
			OriginalRotation		= Actor->ViewRotation;
			Actor->Location			= FVector(0,0,0);
			Actor->bHiddenEd		= 1;
			Actor->bHidden			= 1;
			Actor->bSelected        = 0;
			Actor->bMeshCurvy       = 0;
			Actor->DrawType			= DT_Mesh;
			Actor->Mesh				= Mesh;
			Actor->Region			= FPointRegion(NULL,INDEX_NONE,0);
			Actor->bCollideWorld	= 0;
			Actor->bCollideActors	= 0;
			Actor->AmbientGlow      = 255;

			// Update mesh.
			FLOAT NumFrames = Seq ? Seq->NumFrames : 1.0;
			if( ShowFlags & SHOW_Backdrop )
			{
				FLOAT Rate        = Seq ? Seq->Rate / NumFrames : 1.0;
				Actor->AnimFrame += Clamp(Rate*DeltaTime,0.f,1.f);
				Actor->AnimFrame -= appFloor(Actor->AnimFrame);
			}
			else Actor->AnimFrame = Actor->Misc2 / NumFrames;

			if     ( ShowFlags & SHOW_Frame  )	Viewport->Actor->RendMap = REN_Wire;
			else if( ShowFlags & SHOW_Coords )	Viewport->Actor->RendMap = REN_Polys;
			else								Viewport->Actor->RendMap = REN_PlainTex;

			// Draw it.
			Render->DrawActor( Frame, Viewport->Actor );
			Viewport->Canvas->CurX = 4;
			Viewport->Canvas->CurY = Frame->Y-12;
			Viewport->Canvas->WrappedPrintf//!!can bottom-justify.
			(
				Viewport->Canvas->MedFont,
				1,
 				TEXT("%s, Seq %i, Frame %i"),
				Viewport->MiscRes->GetName(),
				Viewport->Actor->Misc1,
				(INT)(Actor->AnimFrame / NumFrames)
			);
			Viewport->Actor->RendMap = REN_MeshView;
			Actor->Location		   = OriginalLocation;
			Actor->DrawType		   = DT_None;
			POP_HIT(Frame);
			unguard;
			break;
		}
		default:
		{
			Actor->bHiddenEd = Viewport->IsOrtho();

			// Draw background.
			if
			(	Viewport->IsOrtho()
			||	Viewport->IsWire()
			|| !(Viewport->Actor->ShowFlags & SHOW_Backdrop) )
#if 1 //TEMP_FIX: This can probably be undone if Jack (or whoever) ever makes the line drawing stuff z-buffer correctly.
			{
				DrawWireBackground( Frame );
				if( !Viewport->IsOrtho() )
					Viewport->RenDev->ClearZ( Frame );
			}
#else
				DrawWireBackground( Frame );
#endif


			PUSH_HIT(Frame,HCoords(Frame));

			// Draw the level.
			UBOOL bStaticBrushes = Viewport->IsWire();
			UBOOL bMovingBrushes = (Viewport->Actor->ShowFlags & SHOW_MovingBrushes)!=0;
			UBOOL bActiveBrush   = (Viewport->Actor->ShowFlags & SHOW_Brush)!=0;
			if( !Viewport->IsWire() )
				Render->DrawWorld( Frame );
			if( bStaticBrushes || bMovingBrushes || bActiveBrush )
				DrawLevelBrushes( Frame, bStaticBrushes, bMovingBrushes, bActiveBrush );

			// Draw all paths.
			if( (Viewport->Actor->ShowFlags&SHOW_Paths) && Viewport->Actor->GetLevel()->ReachSpecs.Num() )
			{
#if 1 //NEW: Pathing --  draw optionally 'clipped' paths with direction indicators
				FReachSpec::DrawReachSpecs( Frame->Viewport->Actor->GetLevel(), Viewport, Frame, true, ((Viewport->Actor->ShowFlags & SHOW_PathsClip) == SHOW_PathsClip) );
#else
				for( INT i=0; i<Viewport->Actor->GetLevel()->ReachSpecs.Num(); i++ )
				{
					FReachSpec& ReachSpec = Viewport->Actor->GetLevel()->ReachSpecs( i );

					if( ReachSpec.Start && ReachSpec.End && !ReachSpec.bPruned )
					{



						Viewport->RenDev->Draw3DLine
						(
							Frame,
							ReachSpec.MonsterPath() ? C_GroundHighlight.Plane() : C_ActorArrow.Plane(),
							LINE_DepthCued,
							ReachSpec.Start->Location, 
							ReachSpec.End->Location
						);

					}
				}
#endif
			}

			// Draw actors.
			if( Viewport->Actor->ShowFlags & SHOW_Actors )
			{
				// Draw actor extras.
				for( INT iActor=0; iActor<Viewport->Actor->GetLevel()->Actors.Num(); iActor++ )
				{
					AActor* Actor = Viewport->Actor->GetLevel()->Actors(iActor);
					if( Actor && !Actor->bHiddenEd )
					{
#if 1
						// if far-plane (Z) clipping is enabled, consider aborting this actor
						if( EdClipZ > 0.0 && !Frame->Viewport->IsOrtho() )
						{
							FVector	Temp = Actor->Location - Frame->Coords.Origin;
							Temp     = Temp.TransformVectorBy( Frame->Coords );
							FLOAT Z  = Temp.Z; if (Abs (Z)<0.01) Z+=0.02;

							if( Z < 1.0 || Z > EdClipZ )
								continue;
						}
#endif
						PUSH_HIT(Frame,HActor(Actor->GetHitActor()));
						// If this actor is an event source, draw event lines connecting it to
						// all corresponding event sinks.
						if
						(	Actor->Event!=NAME_None
						&&	Viewport->IsWire() )//SHOW_Events!!
						{
							for( INT iOther=0; iOther<Viewport->Actor->GetLevel()->Actors.Num(); iOther++ )
							{
								AActor* OtherActor = Viewport->Actor->GetLevel()->Actors( iOther );
								if
								(	(OtherActor)
								&&	(OtherActor->Tag == Actor->Event) 
								&&	(GIsEditor ? !Actor->bHiddenEd : !Actor->bHidden)
								&&  (!Actor->bOnlyOwnerSee || Actor->IsOwnedBy(Viewport->Actor))
								&&	(!Actor->IsOwnedBy(Frame->Viewport->Actor) || !Actor->bOwnerNoSee || (Actor->IsOwnedBy(Frame->Viewport->Actor) && Frame->Viewport->Actor->bBehindView)) )
								{
									Viewport->RenDev->Draw3DLine( Frame, C_ActorArrow.Plane(), LINE_None, Actor->Location, OtherActor->Location );
								}
							}
						}

						// Radii.
						if
						(	Viewport->IsOrtho()
						&&	(Viewport->Actor->ShowFlags & SHOW_ActorRadii)
						&&	Actor->bSelected )
						{
							if( !Actor->IsBrush() )
							{
								// Show collision radius
								if( Actor->bCollideActors && Viewport->Actor->RendMap==REN_OrthXY )
									Render->DrawCircle( Frame, C_ActorArrow.Plane(), LINE_None, Actor->Location, Actor->CollisionRadius );

								// Show collision height.
								FVector Ext(Actor->CollisionRadius,Actor->CollisionRadius,Actor->CollisionHeight);
								FVector Min(Actor->Location - Ext);
								FVector Max(Actor->Location + Ext);
								if( Actor->bCollideActors && Viewport->Actor->RendMap!=REN_OrthXY )
									Render->DrawBox( Frame, C_ActorArrow.Plane(), LINE_Transparent, Min, Max );
							}
							else if( Actor->IsMovingBrush() )
							{
								FBox Box = Actor->GetPrimitive()->GetRenderBoundingBox(Actor,0);
								Render->DrawBox( Frame, C_ActorArrow.Plane(), LINE_Transparent, Box.Min, Box.Max );
							}

							// Show light radius.
							if( Actor->LightType!=LT_None && Actor->bSelected && GIsEditor && Actor->LightBrightness && Actor->LightRadius )
								Render->DrawCircle( Frame, C_ActorArrow.Plane(), LINE_None, Actor->Location, Actor->WorldLightRadius() );

							// Show light radius.
							if( Actor->LightType!=LT_None && Actor->bSelected && GIsEditor && Actor->VolumeBrightness && Actor->VolumeRadius )
								Render->DrawCircle( Frame, C_Mover.Plane(), LINE_None, Actor->Location, Actor->WorldVolumetricRadius() );

							// Show sound radius.
							if( Actor->AmbientSound && Actor->bSelected && GIsEditor )
								Render->DrawCircle( Frame, C_GroundHighlight.Plane(), LINE_None, Actor->Location, Actor->WorldSoundRadius() );
						}

						// Direction arrow.
						if
						(	Viewport->IsOrtho()
						&&	Actor->bDirectional
						&&	(Cast<ACamera>(Actor) || Actor->bSelected) )
						{
							PUSH_HIT(Frame,HActor(Actor->GetHitActor()));
							FVector V = Actor->Location, A(0,0,0), B(0,0,0);
							FCoords C = GMath.UnitCoords / Actor->Rotation;
							Viewport->RenDev->Draw3DLine( Frame, C_ActorArrow.Plane(), LINE_None, V + C.XAxis * 48, V );
							Viewport->RenDev->Draw3DLine( Frame, C_ActorArrow.Plane(), LINE_None, V + C.XAxis * 48, V + C.XAxis * 16 + C.YAxis * 16 );
							Viewport->RenDev->Draw3DLine( Frame, C_ActorArrow.Plane(), LINE_None, V + C.XAxis * 48, V + C.XAxis * 16 - C.YAxis * 16 );
							Viewport->RenDev->Draw3DLine( Frame, C_ActorArrow.Plane(), LINE_None, V + C.XAxis * 48, V + C.XAxis * 16 + C.ZAxis * 16 );
							Viewport->RenDev->Draw3DLine( Frame, C_ActorArrow.Plane(), LINE_None, V + C.XAxis * 48, V + C.XAxis * 16 - C.ZAxis * 16 );
							POP_HIT(Frame);
						}
						POP_HIT(Frame);

						// Draw him.
						if( Viewport->IsWire() )
							Render->DrawActor( Frame, Actor );
					}
				}
			}

			// Show pivot.
#if 1 //NEW: U2Ed -- always show pivot, regardless of SHOW_Actor setting
			if( GPivotShown )
#else
			if( (Viewport->Actor->ShowFlags & SHOW_Actors) && GPivotShown )
#endif
			{
				FLOAT X, Y;
				FVector Location = GSnappedLocation;
				if( Render->Project( Frame, Location, X, Y, NULL ) )
				{
					PUSH_HIT(Frame,HGlobalPivot(Location));
         			Viewport->RenDev->Draw2DPoint( Frame, C_BrushWire.Plane(), LINE_None, X-1, Y-1, X+1, Y+1, Location.Z );
        			Viewport->RenDev->Draw2DPoint( Frame, C_BrushWire.Plane(), LINE_None, X,   Y-4, X,   Y+4, Location.Z );
         			Viewport->RenDev->Draw2DPoint( Frame, C_BrushWire.Plane(), LINE_None, X-4, Y,   X+4, Y,   Location.Z );
					POP_HIT(Frame);
				}
			}

			// Draw buttons.
			if( !(Viewport->Actor->ShowFlags & SHOW_NoButtons) && !Viewport->IsFullscreen() )
				DrawButtons( Frame );

#if 1 //NEW: Interpolation Paths (from Ion Storm, Austin) -- MERGED
			//
			// DEUS_EX CNN - Draw the spline curves for InterpolationPoints
			//
			
			// get the tag of the selected point
			FName matchTag = NAME_None;
			AActor* Actor;
			for (INT iActor=0; iActor<Viewport->Actor->GetLevel()->Actors.Num();
			iActor++)
			{
				Actor = Viewport->Actor->GetLevel()->Actors(iActor);
				if (Actor && Actor->IsA(AInterpolationPoint::StaticClass()) &&
					Actor->bSelected)
				{
					matchTag = Actor->Tag;
					break;
				}
			}
			
			// generate a list of all matching points
#define MAX_INTERP_POINTS 128
			AInterpolationPoint* PointList[MAX_INTERP_POINTS];
			INT numPoints = -1;
			for (INT i=0; i<MAX_INTERP_POINTS; i++)
				PointList[i] = NULL;
			for (iActor=0; iActor<Viewport->Actor->GetLevel()->Actors.Num(); iActor++)
			{
				Actor = Viewport->Actor->GetLevel()->Actors(iActor);
				if (Actor && Actor->IsA(AInterpolationPoint::StaticClass()) && (Actor->Tag == matchTag))
				{
					AInterpolationPoint* Dest = Cast<AInterpolationPoint>(Actor);
					if (Dest->Position < MAX_INTERP_POINTS)
					{
						PointList[Dest->Position] = Dest;
						if (Dest->Position > numPoints)
							numPoints = Dest->Position;
					}
				}
			}
			
			numPoints++;
			if (numPoints > 2)
			{
				if (numPoints < MAX_INTERP_POINTS-2)
				{
					// if this path should loop, link the points back to the beginning
					if (!PointList[numPoints-1]->bEndOfPath)
					{
						PointList[numPoints] = PointList[0];
						PointList[numPoints+1] = PointList[1];
						PointList[numPoints+2] = PointList[2];
					}
				}
				
				// draw the entire chain if something is selected
				INT cur = 1;
				
				while (PointList[cur])
				{
					FVector v1, v2;
					FRotator r1;
					FCoords C;
					if (PointList[cur-1] && PointList[cur+1] && PointList[cur+2])
					{
						FLOAT alpha = 0.0;
						FLOAT alphastep = 0.02;
						FLOAT arrow = 0.0;
						FLOAT arrowstep = PointList[cur]->RateModifier / 5.0;;
						FLOAT W0, W1, W2, W3, RW;
						
						while (alpha < 1.0-alphastep)
						{
							// Cubic spline interpolation.
							W0 = Splerp(alpha+1.0);
							W1 = Splerp(alpha+0.0);
							W2 = Splerp(alpha-1.0);
							W3 = Splerp(alpha-2.0);
							RW = 1.0 / (W0 + W1 + W2 + W3);
							v1 = (W0*PointList[cur-1]->Location + W1*PointList[cur]->Location +
								W2*PointList[cur+1]->Location + W3*PointList[cur+2]->Location)*RW;
							r1 = (W0*PointList[cur-1]->Rotation + W1*PointList[cur]->Rotation +
								W2*PointList[cur+1]->Rotation + W3*PointList[cur+2]->Rotation)*RW;
							
							W0 = Splerp(alpha+alphastep+1.0);
							W1 = Splerp(alpha+alphastep+0.0);
							W2 = Splerp(alpha+alphastep-1.0);
							W3 = Splerp(alpha+alphastep-2.0);
							RW = 1.0 / (W0 + W1 + W2 + W3);
							v2 = (W0*PointList[cur-1]->Location + W1*PointList[cur]->Location +
								W2*PointList[cur+1]->Location + W3*PointList[cur+2]->Location)*RW;
							
							// draw the path in white
							Viewport->RenDev->Draw3DLine(Frame, FPlane(1,1,1,0), LINE_None, v1, v2);
							
							// draw orientation axes on the path (Xfront = green, Yright = blue, Zup = red)
							if (arrow <= alpha)
							{
								C = GMath.UnitCoords / r1;
								Viewport->RenDev->Draw3DLine(Frame, FPlane(0,1,0,0), LINE_None, v1, v1 + C.XAxis * 32);
								Viewport->RenDev->Draw3DLine(Frame, FPlane(0,0,1,0), LINE_None, v1, v1 + C.YAxis * 32);
								Viewport->RenDev->Draw3DLine(Frame, FPlane(1,0,0,0), LINE_None, v1, v1 + C.ZAxis * 32);
								arrow += arrowstep;
							}
							
							// inc the alpha
							alpha += alphastep;
						}
					}
					
					// get the next point, but don't pass the end of the list
					cur++;
					if (cur >= MAX_INTERP_POINTS-2)
						break;
				}
			}
			
			//
			// DEUS_EX CNN - end changes
			//
			
#endif
#if 1 //NEW: U2Ed
			// If the user is doing a box selection in this viewport, draw the current selection box.
			//
			if( Viewport->IsOrtho() && GbIsBoxSel )
				Render->DrawBox( Frame, C_BrushWire.Plane(), LINE_None, GBoxSelStart, GBoxSelEnd );

			// If the user is brush clipping, draw lines to show them what's going on.
			//
			TArray<AActor*> ClipMarkers;

			// Gather a list of all the ClipMarkers in the level.
			//
			for( i = 0 ; i < GEditor->Level->Actors.Num() ; i++ )
			{
				AActor* pActor = GEditor->Level->Actors(i);
				if( pActor && pActor->IsA(AClipMarker::StaticClass()) )
					ClipMarkers.AddItem( pActor );
			}

			if( ClipMarkers.Num() > 1 )
			{
				// Draw a connecting line between them all.
				//
				for( int x = 1 ; x < ClipMarkers.Num() ; x++ )
					Viewport->RenDev->Draw3DLine(Frame, C_BrushWire.Plane(), LINE_None, ClipMarkers(x - 1)->Location, ClipMarkers(x)->Location);

				// Draw an arrow that shows the direction of the clipping plane.  This arrow should
				// appear halfway between the first and second markers.
				//
				FVector vtx1, vtx2, vtx3;
				FPoly NormalPoly;
				UBOOL bDrawOK = 1;

				vtx1 = ClipMarkers(0)->Location;
				vtx2 = ClipMarkers(1)->Location;

				if( ClipMarkers.Num() == 3 )
				{
					// If we have 3 points, just grab the third one to complete the plane.
					//
					vtx3 = ClipMarkers(2)->Location;
				}
				else
				{
					// If we only have 2 points, we will assume the third based on the viewport.
					// (With only 2 points, we can only render into the ortho viewports)
					//
					vtx3 = vtx1;
					if( Viewport->IsOrtho() )
					{
						switch( Viewport->Actor->RendMap )
						{
							case REN_OrthXY:
								vtx3.Z -= 64;
								break;

							case REN_OrthXZ:
								vtx3.Y -= 64;
								break;

							case REN_OrthYZ:
								vtx3.X -= 64;
								break;
						}
					}
					else
						bDrawOK = 0;
				}

				NormalPoly.NumVertices = 3;
				NormalPoly.Vertex[0] = vtx1;
				NormalPoly.Vertex[1] = vtx2;
				NormalPoly.Vertex[2] = vtx3;

				if( bDrawOK && !NormalPoly.CalcNormal(1) )
				{
					FVector Start = vtx1 + (( vtx2 - vtx1 ) / 2);
					Viewport->RenDev->Draw3DLine( Frame, C_BrushWire.Plane(), LINE_None, Start, Start + (NormalPoly.Normal * 48 ));
				}
			}
#endif
			POP_HIT(Frame);
			break;
		}
	}

	Render->PostRender( Frame );
	Viewport->Unlock( Blit );
	Render->FinishMasterFrame();

	unguardf(( TEXT("(Cam=%s,Flags=%i"), Viewport->GetName(), ShowFlags ));
}

/*-----------------------------------------------------------------------------
   Viewport mouse click handling.
-----------------------------------------------------------------------------*/

//
// Handle a mouse click in the camera window.
//
void UEditorEngine::Click
(
	UViewport*	Viewport, 
	DWORD		Buttons,
	FLOAT		MouseX,
	FLOAT		MouseY
)
{
	guard(UEditorEngine::Click);

	// Set hit-test location.
	Viewport->HitX  = Clamp(appFloor(MouseX)-2,0,Viewport->SizeX);
	Viewport->HitY  = Clamp(appFloor(MouseY)-2,0,Viewport->SizeY);
	Viewport->HitXL = Clamp(appFloor(MouseX)+3,0,Viewport->SizeX) - Viewport->HitX;
	Viewport->HitYL = Clamp(appFloor(MouseY)+3,0,Viewport->SizeY) - Viewport->HitY;

	// Draw with hit-testing.
	BYTE HitData[1024];
	INT HitCount=ARRAY_COUNT(HitData);
	Draw( Viewport, 0, HitData, &HitCount );

	// Update buttons.
	if( Viewport->Input->KeyDown(IK_Shift) )
		Buttons |= MOUSE_Shift;
	if( Viewport->Input->KeyDown(IK_Ctrl) )
		Buttons |= MOUSE_Ctrl;
	if( Viewport->Input->KeyDown(IK_Alt) )
		Buttons |= MOUSE_Alt;

	// Perform hit testing.
	FEditorHitObserver Observer;
	Viewport->ExecuteHits( FHitCause(&Observer,Viewport,Buttons,MouseX,MouseY), HitData, HitCount );

	unguard;
}

/*-----------------------------------------------------------------------------
   Editor camera mode.
-----------------------------------------------------------------------------*/

//
// Set the editor mode.
//
void UEditorEngine::edcamSetMode( int InMode )
{
	guard(UEditorEngine::edcamSetMode);

	// Clear old mode.
	if( Mode != EM_None )
		for( INT i=0; i<Client->Viewports.Num(); i++ )
			MouseDelta( Client->Viewports(i), MOUSE_ExitMode, 0, 0 );

	// Set new mode.
	Mode = InMode;
	if( Mode != EM_None )
		for( INT i=0; i<Client->Viewports.Num(); i++ )
			MouseDelta( Client->Viewports(i), MOUSE_SetMode, 0, 0 );

	unguard;
}

//
// Return editor camera mode given Mode and state of keys.
// This handlers special keyboard mode overrides which should
// affect the appearance of the mouse cursor, etc.
//
int UEditorEngine::edcamMode( UViewport* Viewport )
{
	guard(UEditorEngine::edcamMode);
	check(Viewport);
	check(Viewport->Actor);
	switch( Viewport->Actor->RendMap )
	{
		case REN_TexView:    return EM_TexView;
		case REN_TexBrowser: return EM_TexBrowser;
		case REN_MeshView:   return EM_MeshView;
	}
	return Mode;
	unguard;
}

/*-----------------------------------------------------------------------------
	Selection.
-----------------------------------------------------------------------------*/

#if 1 //NEW: U2Ed
// Checks the array of vertices and makes sure that the brushes in that list are still selected.  If not,
// the vertex is removed from the list.
void vertexedit_Refresh()
{
	guard(vertexedit_Refresh);

	for( int vertex = 0 ; vertex < VertexHitList.Num() ; vertex++ )
		if( !VertexHitList(vertex).pBrush->bSelected )
		{
			VertexHitList.Remove(vertex);
			vertex = 0;
		}

	unguard;
}
#endif

//
// Selection change.
//
void UEditorEngine::NoteSelectionChange( ULevel* Level )
{
	guard(UEditorEngine::NoteSelectionChange);

	// Notify the editor.
	EdCallback( EDC_SelChange, 0 );

	// Pick a new common pivot, or not.
	INT Count=0;
	AActor* SingleActor=NULL;
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		if( Level->Actors(i) && Level->Actors(i)->bSelected )
		{
			SingleActor=Level->Actors(i);
			Count++;
		}
	}
	if( Count==0 ) ResetPivot();
	else if( Count==1 ) SetPivot( SingleActor->Location, 0, 0 );

	// Update properties window.
	UpdatePropertiesWindows();

#if 1 //NEW: U2Ed
	vertexedit_Refresh();
#endif

	unguard;
}

//
// Select none.
//
void UEditorEngine::SelectNone( ULevel *Level, UBOOL Notify )
{
	guard(UEditorEngine::SelectNone);

	// Unselect all actors.
	for( INT i=0; i<Level->Actors.Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		if( Actor && Actor->bSelected )
		{
			Actor->Modify();
			Actor->bSelected = 0;
		}
	}

	// Unselect all surfaces.
	for( i=0; i<Level->Model->Surfs.Num(); i++ )
	{
		FBspSurf& Surf = Level->Model->Surfs(i);
#if 1 //NEW: PolyFlagsEx
		if( Surf.PolyFlags[0] & PF_Selected )
#else
		if( Surf.PolyFlags & PF_Selected )
#endif
		{
			Level->Model->ModifySurf( i, 0 );
#if 1 //NEW: PolyFlagsEx
			Surf.PolyFlags[0] &= ~PF_Selected;
#else
			Surf.PolyFlags &= ~PF_Selected;
#endif
		}
	}

	if( Notify )
		NoteSelectionChange( Level );
	unguard;
}

/*-----------------------------------------------------------------------------
	Ed link topic function.
-----------------------------------------------------------------------------*/

AUTOREGISTER_TOPIC(TEXT("Ed"),EdTopicHandler);
void EdTopicHandler::Get( ULevel* Level, const TCHAR* Item, FOutputDevice& Ar )
{
	guard(EdTopicHandler::Get);

	if		(!appStricmp(Item,TEXT("LASTSCROLL")))	Ar.Logf(TEXT("%i"),GLastScroll);
	else if (!appStricmp(Item,TEXT("CURTEX")))		Ar.Log(GEditor->CurrentTexture ? GEditor->CurrentTexture->GetName() : TEXT("None"));

	unguard;
}
void EdTopicHandler::Set( ULevel* Level, const TCHAR* Item, const TCHAR* Data )
{}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
