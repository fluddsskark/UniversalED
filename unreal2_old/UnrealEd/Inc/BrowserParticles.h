#if 1 //NEW: ParticleSystems
/*=============================================================================
	BrowserParticles : Browser window for ParticleSystems
	Copyright 2000 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Aaron Leiby

    Todo:
	    * Add a ParticleViewer similar to the MeshView.

=============================================================================*/

#include <stdio.h>

// --------------------------------------------------------------
//
// WBrowserParticles
//
// --------------------------------------------------------------

#define SHOW_VIEWPORT 0

class WBrowserParticles : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserParticles,WBrowser,Window)

	WListBox* pParticleList;
#if SHOW_VIEWPORT
	UViewport* pViewport;
#endif

	// Structors.
	WBrowserParticles( FName InPersistentName, WWindow* InOwnerWindow )
	:	WBrowser( InPersistentName, InOwnerWindow )
	{
		pParticleList = NULL;
#if SHOW_VIEWPORT
		pViewport = NULL;
#endif
	}

	// WBrowser interface.
	void OpenWindow()
	{
		guard(WBrowserParticles::OpenWindow);
		WBrowser::OpenWindow();
		SetText( TEXT("Particle Browser") );
		Show(1);
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserParticles::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserParticles) );

		pParticleList = new WListBox( this, IDLB_PARTICLES );
		pParticleList->OpenWindow( 1, 0, 1, 0, 1 );
		pParticleList->DoubleClickDelegate = FDelegate(this, (TDelegate)OnParticleListDoubleClick);
		pParticleList->SelectionChangeDelegate = FDelegate(this, (TDelegate)OnParticleListSelChange);
		
		RefreshParticlesList();
#if SHOW_VIEWPORT
		RefreshViewport();
#endif
		PositionChildControls();

		unguard;
	}
	void RefreshParticlesList(void)
	{
		guard(WBrowserActor::RefreshPackages);

		FStringOutputDevice GetPropResult = FStringOutputDevice();
	    GEditor->Get(TEXT("OBJ"), TEXT("OBJECTS CLASS=ParticleGenerator"), GetPropResult);

		TArray<FString> PkgArray;
		ParseStringToArray( TEXT(","), GetPropResult, &PkgArray );

		pParticleList->Empty();

		for( int x = 0 ; x < PkgArray.Num() ; x++ )
			pParticleList->AddString( *(FString::Printf( TEXT("%s"), *PkgArray(x))) );

		unguard;
	}
#if SHOW_VIEWPORT
	void RefreshViewport()
	{
		guard(WBrowserMesh::RefreshViewport);

		if( !pViewport )
		{
			// Create the mesh viewport
			//
			pViewport = GEditor->Client->NewViewport( TEXT("MeshViewer") );
			check(pViewport);
			GEditor->Level->SpawnViewActor( pViewport );
			pViewport->Input->Init( pViewport );
			check(pViewport->Actor);
			pViewport->Actor->ShowFlags = SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow;
			pViewport->Actor->RendMap   = REN_MeshView;
			pViewport->Group = NAME_None;
			pViewport->MiscRes = UObject::StaticFindObject( NULL, ANY_PACKAGE, TEXT("UnrealI.Flag1M") );
			check(pViewport->MiscRes);
			pViewport->Actor->Misc1 = 0;
			pViewport->Actor->Misc2 = 0;

			pViewport->OpenWindow( (DWORD)hWnd, 0, 256, 256, 0, 0 );
		}
		else
		{
			FString MeshName = TEXT("UnrealI.Flag1M");

			FStringOutputDevice GetPropResult = FStringOutputDevice();
			GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("ANIMSEQ NAME=%s NUM=%d"), *MeshName, TEXT("All"))), GetPropResult );

			GEditor->Exec( *(FString::Printf(TEXT("CAMERA UPDATE NAME=MeshViewer MESH=%s FLAGS=%d REN=%d MISC1=%s MISC2=%d"),
				*MeshName,
				true
				? SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_Backdrop | SHOW_RealTime
				: SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow,
				REN_MeshView,
				*((GetPropResult.Right(7)).Left(3)),
				0
				)));
		}

		unguard;
	}
#endif
	void OnDestroy()
	{
		guard(WBrowserParticles::OnDestroy);

		delete pParticleList;

		WBrowser::OnDestroy();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserParticles::OnCommand);
		switch( Command )
		{
			case IDMN_PB_FileImport:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Particle Packages (*.ups)\0*.ups\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\system";
					ofn.lpstrDefExt = "ups";
					ofn.lpstrTitle = "Open Particle Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );

						TArray<FString> StringArray;
						ParseStringToArray( TEXT("|"), appFromAnsi( File ), &StringArray );

						int iStart = 0;
						FString Prefix = TEXT("\0");

						if( iNULLs )
						{
							iStart = 1;
							Prefix = *(StringArray(0));
							Prefix += TEXT("\\");
						}

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							TCHAR l_chCmd[512];

							appSprintf( l_chCmd, TEXT("CLASS LOAD FILE=%s%s"), *Prefix, *(StringArray(x)) );
							GEditor->Exec( l_chCmd );
						}

						RefreshParticlesList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_PB_FileExport:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";
					TCHAR l_chCmd[512];

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Particle Packages (*.ups)\0*.ups\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\system";
					ofn.lpstrDefExt = "ups";
					ofn.lpstrTitle = "Save Particle Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;

					if( GetSaveFileNameA(&ofn) )
					{
						FString S = ANSI_TO_TCHAR(File);
						appSprintf( l_chCmd, TEXT("ACTOR PARTICLESYSTEMS EXPORT PACKAGE=%s"), S.Left( S.InStr( TEXT(".ups"), 1 ) ) );
						GEditor->Exec( l_chCmd );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_PB_FileRefresh:
				{
					RefreshParticlesList();
				}
				break;

			default:
				WBrowser::OnCommand(Command);
				break;
		}
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserParticles::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WBrowserParticles::PositionChildControls);

		if
		(	!pParticleList
#if SHOW_VIEWPORT
		||	!pViewport
#endif
		)	return;

#if SHOW_VIEWPORT
		LockWindowUpdate( hWnd );
#endif

		FRect R = GetClientRect();
		float Fraction = (R.Width() - 8) / 10.0f;
		int Height = R.Height();
		int Divider = (INT)(Fraction * 10 * 3/4);
		if( Height - Divider < 48 )
			Divider = 0;

#if SHOW_VIEWPORT
		::MoveWindow( pParticleList->hWnd, 4, (R.Height() - Height), Fraction * 10, Height - Divider, 1 );	Height -= Height - Divider;
		::MoveWindow( (HWND)pViewport->GetWindow(), 4, (R.Height() - Height) + 4, Fraction * 10, Height - 8, 1 );
#else
		::MoveWindow( pParticleList->hWnd, 4, (R.Height() - Height), Fraction * 10, Height, 1);
#endif
		::InvalidateRect( hWnd, NULL, 1);

#if SHOW_VIEWPORT
		pViewport->Repaint( 1 );
#endif

#if SHOW_VIEWPORT
		// Refresh the display.
		LockWindowUpdate( NULL );
#endif
		unguard;
	}
	void OnParticleListDoubleClick( void )
	{
		AActor* Actor;
		if( ParseObject<AActor>( *(FString(TEXT("OBJECT=")) + pParticleList->GetString( pParticleList->GetCurrent() )), TEXT("OBJECT="), Actor, ANY_PACKAGE ) )
		{
			TCHAR Temp[256];
			appSprintf( Temp, TEXT("Object %s Properties"), Actor->GetPathName() );
			WObjectProperties* ObjectProperties = new WObjectProperties( TEXT("ObjectProperties"), CPF_Edit, Temp, NULL, 1 );
			ObjectProperties->OpenWindow( ObjectProperties->List.hWnd );
			((FObjectsItem*)ObjectProperties->GetRoot())->SetObjects( (UObject**)&Actor, 1 );
			ObjectProperties->SetNotifyHook( GEditor );
			ObjectProperties->ForceRefresh();
			ObjectProperties->Show(1);
		}
	}
	void OnParticleListSelChange( void )
	{
		GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTPARTICLECLASS IMAGE=%s"), pParticleList->GetString( pParticleList->GetCurrent() ) )));
		SelectSelected();
	}
	void SelectSelected( void )
	{
		guard(WBrowserParticles::SelectSelected);

		GEditor->Trans->Begin( TEXT("Selecting selected ParticleGenerators") );
		GEditor->SelectNone( GEditor->Level, 0 );
		GEditor->Level->Modify();

		for( INT i=0; i<pParticleList->GetCount(); i++ )
		{
			AActor* Actor;
			if( ParseObject<AActor>( *(FString(TEXT("OBJECT=")) + pParticleList->GetString(i)), TEXT("OBJECT="), Actor, ANY_PACKAGE ) )
			{
				Actor->bSelected = pParticleList->GetSelected(i);
			}
		}

		GEditor->Trans->End();
		GEditor->NoteSelectionChange( GEditor->Level );

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif