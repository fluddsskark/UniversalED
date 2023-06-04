#if 1 //NEW: U2Ed
/*=============================================================================
	BrowserMesh : Browser window for meshes
	Copyright 2000 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

__declspec(dllimport) INT GLastScroll;

extern void Query( ULevel* Level, const TCHAR* Item, FString* pOutput );
extern void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);

// --------------------------------------------------------------
//
// WBrowserMesh
//
// --------------------------------------------------------------

class WBrowserMesh : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserMesh,WBrowser,Window)

	WComboBox* pMeshCombo;
	WListBox* pAnimList;
	WButton* pPlayButton;
	WButton* pStopButton;
	WLabel* pMeshLabel;

	UViewport *pViewport;

	UBOOL bPlaying;

	// Structors.
	WBrowserMesh( FName InPersistentName, WWindow* InOwnerWindow )
	:	WBrowser( InPersistentName, InOwnerWindow )
	{
		pViewport = NULL;
		bPlaying = FALSE;
	}

	// WBrowser interface.
	void OpenWindow()
	{
		guard(WBrowserMesh::OpenWindow);
		WBrowser::OpenWindow();
		SetText( TEXT("Mesh Browser") );
		Show(1);
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserMesh::OnCreate);
		WBrowser::OnCreate();

		//SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserMesh) );
		
		pMeshCombo = new WComboBox( this, IDCB_MESH );
		pMeshCombo->OpenWindow( 1, 1 );

		pAnimList = new WListBox( this, IDLB_ANIMATIONS );
		pAnimList->OpenWindow( 1, 0, 0, 0, 0 );

		pPlayButton = new WButton( this, IDPB_PLAY,	FDelegate(this,(TDelegate)OnPlay) );
		pPlayButton->OpenWindow( 1, 0, 0, 50, 50, TEXT("Play") );

		pStopButton = new WButton( this, IDPB_STOP,	FDelegate(this,(TDelegate)OnStop) );
		pStopButton->OpenWindow( 1, 0, 0, 50, 50, TEXT("Stop") );

		pMeshLabel = new WLabel( this, IDSC_MESH );
		pMeshLabel->OpenWindow( 1, 0 );
		pMeshLabel->SetText( TEXT("Mesh :") );

		pMeshCombo->SelectionChangeDelegate = FDelegate(this,(TDelegate)OnMeshSelectionChange);
		pAnimList->DoubleClickDelegate = FDelegate(this,(TDelegate)OnAnimDoubleClick);
		pAnimList->SelectionChangeDelegate = FDelegate(this,(TDelegate)OnAnimSelectionChange);

		RefreshMeshList();
		RefreshAnimList();
		RefreshViewport();
		SetCaption();

		PositionChildControls();

		unguard;
	}
	void SetCaption( void )
	{
		guard(WBrowserMesh::SetCaption);

		FString Caption = TEXT("Mesh Browser");

		if( GetCurrentMeshName().Len() )
			Caption += FString::Printf( TEXT(" - %s"),
				GetCurrentMeshName() );

		SetText( *Caption );
		unguard;
	}
	void RefreshMeshList()
	{
		guard(WBrowserMesh::RefreshMeshList);

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("OBJ"), TEXT("Query Type=Mesh"), GetPropResult );

		pMeshCombo->Empty();

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(" "), *GetPropResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
			pMeshCombo->AddString( *(StringArray(x)) );

		pMeshCombo->SetCurrent(0);

		unguard;
	}
	FString GetCurrentMeshName()
	{
		guard(WBrowserMesh::GetCurrentMeshName);
		return pMeshCombo->GetString( pMeshCombo->GetCurrent() );
		unguard;
	}
	void RefreshAnimList()
	{
		guard(WBrowserMesh::RefreshAnimList);

		FString MeshName = GetCurrentMeshName();

		pAnimList->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("NUMANIMSEQS NAME=%s"), *MeshName)), GetPropResult );
		int NumAnims = appAtoi( *GetPropResult );

		for( int anim = 0 ; anim < NumAnims ; anim++ )
		{
			FStringOutputDevice GetPropResult = FStringOutputDevice();
			GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("ANIMSEQ NAME=%s NUM=%d"), *MeshName, anim)), GetPropResult );

			int NumFrames = appAtoi( *(GetPropResult.Right(3)) );
			FString Name = GetPropResult.Left( GetPropResult.InStr(TEXT(" ")));

			pAnimList->AddString( *(FString::Printf(TEXT("%s [ %d ]"), *Name, NumFrames )) );
		}

		pAnimList->SetCurrent(0, 1);

		unguard;
	}
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
			pViewport->MiscRes = UObject::StaticFindObject( NULL, ANY_PACKAGE, *(pMeshCombo->GetString(pMeshCombo->GetCurrent())) );
			check(pViewport->MiscRes);
			pViewport->Actor->Misc1 = 0;
			pViewport->Actor->Misc2 = 0;

			pViewport->OpenWindow( (DWORD)hWnd, 0, 256, 256, 0, 0 );
		}
		else
		{
			FString MeshName = pMeshCombo->GetString(pMeshCombo->GetCurrent());

			FStringOutputDevice GetPropResult = FStringOutputDevice();
			GEditor->Get( TEXT("MESH"), *(FString::Printf(TEXT("ANIMSEQ NAME=%s NUM=%d"), *MeshName, pAnimList->GetCurrent())), GetPropResult );

			GEditor->Exec( *(FString::Printf(TEXT("CAMERA UPDATE NAME=MeshViewer MESH=%s FLAGS=%d REN=%d MISC1=%s MISC2=%d"),
				*MeshName,
				bPlaying
				? SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow | SHOW_Backdrop | SHOW_RealTime
				: SHOW_StandardView | SHOW_NoButtons | SHOW_ChildWindow,
				REN_MeshView,
				*((GetPropResult.Right(7)).Left(3)),
				0
				)));
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMesh::OnDestroy);

		GEditor->Exec( TEXT("CAMERA CLOSE NAME=MeshViewer") );

		delete pMeshCombo;
		delete pAnimList;
		delete pPlayButton;
		delete pStopButton;
		delete pMeshLabel;

		WBrowser::OnDestroy();
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowserMesh::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls()
	{
		guard(WBrowserMesh::PositionChildControls);
		if(	!::IsWindow( GetDlgItem( hWnd, IDCB_MESH ) )
				|| !::IsWindow( GetDlgItem( hWnd, IDLB_ANIMATIONS ) )
				|| !::IsWindow( GetDlgItem( hWnd, IDPB_PLAY ) )
				|| !::IsWindow( GetDlgItem( hWnd, IDPB_STOP ) )
				|| !pViewport )
			return;

		LockWindowUpdate( hWnd );

		RECT rcClient;
		::GetClientRect( hWnd, &rcClient );

		//::SetWindowPos( GetDlgItem( hWnd, IDGP_CTRLS ), HWND_TOP,
		//	rcClient.right - 175, rcClient.top + 8, 170, rcClient.bottom - 12, SWP_SHOWWINDOW );

		::SetWindowPos( GetDlgItem( hWnd, IDSC_MESH ), HWND_TOP,
			rcClient.right - 175, rcClient.top + 8, 35, 16, SWP_SHOWWINDOW );
		::SetWindowPos( GetDlgItem( hWnd, IDCB_MESH ), HWND_TOP,
			rcClient.right - 130, rcClient.top + 6, 130, 16, SWP_SHOWWINDOW );

		::SetWindowPos( GetDlgItem( hWnd, IDLB_ANIMATIONS ), HWND_TOP,
			rcClient.right - 175, rcClient.top + 34, 175, (rcClient.bottom - 40) - (rcClient.top + 34), SWP_SHOWWINDOW );

		::SetWindowPos( GetDlgItem( hWnd, IDPB_PLAY ), HWND_TOP,
			rcClient.right - 175, rcClient.bottom - 32, 85, 22, SWP_SHOWWINDOW );
		::SetWindowPos( GetDlgItem( hWnd, IDPB_STOP ), HWND_TOP,
			rcClient.right - 85, rcClient.bottom - 32, 85, 22, SWP_SHOWWINDOW );

		FRect R = GetClientRect();
		::MoveWindow( (HWND)pViewport->GetWindow(), R.Min.X, R.Min.Y, R.Width() - 180, R.Height(), 1 );
		pViewport->Repaint( 1 );

		// Refresh the display.
		LockWindowUpdate( NULL );
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnPlay()
	{
		guard(WBrowserMesh::OnPlay);
		bPlaying = 1;
		RefreshViewport();
		::EnableWindow( GetDlgItem( hWnd, IDPB_STOP ), 1);
		::EnableWindow( GetDlgItem( hWnd, IDPB_PLAY ), 0);
		unguard;
	}
	void OnStop()
	{
		guard(WBrowserMesh::OnStop);
		bPlaying = 0;
		RefreshViewport();
		::EnableWindow( GetDlgItem( hWnd, IDPB_STOP ), 0);
		::EnableWindow( GetDlgItem( hWnd, IDPB_PLAY ), 1);
		unguard;
	}
	void OnMeshSelectionChange()
	{
		guard(WBrowserMesh::OnMeshSelectionChange);
		RefreshAnimList();
		RefreshViewport();
		SetCaption();
		unguard;
	}
	void OnAnimDoubleClick()
	{
		guard(WBrowserMesh::OnAnimDoubleClick);
		OnPlay();
		unguard;
	}
	void OnAnimSelectionChange()
	{
		guard(WBrowserMesh::OnAnimSelectionChange);
		RefreshViewport();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif