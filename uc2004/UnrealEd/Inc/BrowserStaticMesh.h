/*=============================================================================
	BrowserStaticMesh : Browser window for static meshes
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

extern void Query( ULevel* Level, const TCHAR* Item, FString* pOutput );
extern FString GLastDir[eLASTDIR_MAX];
#ifdef WITH_KARMA
extern void KAse2me2(char* infilename, UStaticMesh* smesh);
#endif
extern TMap<INT,UOptionsProxy*>* GOptionProxies;

// --------------------------------------------------------------
//
// IMPORT STATIC MESH Dialog
//
// --------------------------------------------------------------

class WDlgImportStaticMesh : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportStaticMesh,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton OkAllButton;
	WButton SkipButton;
	WButton CancelButton;
	WLabel FilenameStatic;
	WEdit PackageEdit;
	WEdit GroupEdit;
	WEdit NameEdit;

	FString defPackage, defGroup;
	TArray<FString>* paFilenames;

	FString Package, Group, Name;
	BOOL bOKToAll;
	INT iCurrentFilename;

	// Constructor.
	WDlgImportStaticMesh( UObject* InContext, WBrowser* InOwnerWindow )
	:	WDialog			( TEXT("Import Static Mesh"), IDDIALOG_IMPORT_STATIC_MESH, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)OnOk) )
	,	OkAllButton		( this, IDPB_OKALL,		FDelegate(this,(TDelegate)OnOkAll) )
	,	SkipButton		( this, IDPB_SKIP,		FDelegate(this,(TDelegate)OnSkip) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)EndDialogFalse) )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	GroupEdit		( this, IDEC_GROUP )
	,	NameEdit		( this, IDEC_NAME )
	,	FilenameStatic	( this, IDSC_FILENAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportStaticMesh::OnInitDialog);
		WDialog::OnInitDialog();

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );
		::SetFocus( NameEdit.hWnd );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();

		unguard;
	}
	virtual INT DoModal( FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgImportStaticMesh::DoModal);

		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportStaticMesh::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportStaticMesh::OnOkAll);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			bOKToAll = TRUE;
			SetNextFilename();
		}
		unguard;
	}
	void OnSkip()
	{
		guard(WDlgImportStaticMesh::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void RefreshName()
	{
		guard(WDlgImportStaticMesh::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = GetFilenameOnly( (*paFilenames)(iCurrentFilename) );
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser()
	{
		guard(WDlgImportStaticMesh::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
				|| !Name.Len() )
		{
			appMsgf( 0, TEXT("Invalid input.") );
			return FALSE;
		}

		return TRUE;
		unguard;
	}
	void SetNextFilename()
	{
		guard(WDlgImportStaticMesh::SetNextFilename);
		iCurrentFilename++;
		if( iCurrentFilename == paFilenames->Num() )
		{
			EndDialogTrue();
			return;
		}

		if( bOKToAll )
		{
			RefreshName();
			GetDataFromUser();
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
			return;
		};

		RefreshName();

		unguard;
	}
	void ImportFile( FString Filename )
	{
		guard(WDlgImportStaticMesh::ImportFile);
		TCHAR l_chCmd[512];

		if( Group.Len() )
			appSprintf( l_chCmd, TEXT("STATICMESH IMPORT FILE=\"%s\" NAME=\"%s\" PACKAGE=\"%s\" GROUP=\"%s\""),
				*(*paFilenames)(iCurrentFilename), *Name, *Package, *Group );
		else
			appSprintf( l_chCmd, TEXT("STATICMESH IMPORT FILE=\"%s\" NAME=\"%s\" PACKAGE=\"%s\""),
				*(*paFilenames)(iCurrentFilename), *Name, *Package );

		GUnrealEd->Exec( l_chCmd );

#ifdef WITH_KARMA
		// Get filename we just opened as chars (thats what Karma uses...)
		char cFilename[512];
		wcstombs(cFilename, *(*paFilenames)(iCurrentFilename), 512);

		// Then find the static mesh..
		UStaticMesh* madeMesh = FindObject<UStaticMesh>(ANY_PACKAGE, *FName(*FString::Printf(TEXT("%s%s%s.%s"), *Package, Group.Len() ? TEXT(".") : TEXT(""), *Group, *Name)));
		
		// ..and add Karma prims from the ASE->geom parser.
		if(madeMesh && (appStrstr(*(*paFilenames)(iCurrentFilename),TEXT(".ase")) ||
			appStrstr(*(*paFilenames)(iCurrentFilename),TEXT(".ase"))))
			KAse2me2(cFilename, madeMesh); //#SKEL - integrated exp/importer issues to fix ? - Erik

#endif
		unguard;
	}
};

#include "GeomFitUtils.h"

// --------------------------------------------------------------
//
// WBrowserStaticMesh
//
// --------------------------------------------------------------

#define ID_SM_TOOLBAR	29040
TBBUTTON tbBSMuttons[] = {
	{ 0, IDMN_MB_DOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_SM_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_SM_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, IDMN_SM_LOAD_ENTIRE_PACKAGE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_SM_CREATE_FROM_SELECTION, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_SM_ADD_TO_LEVEL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}

	// 3DBuzz
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 8, IDMN_CENTERVIEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}

	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 6, IDMN_SM_PREV_GRP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 7, IDMN_SM_NEXT_GRP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	INT ID;
} ToolTips_SM[] = {
	TEXT("Toggle Dock Status"), IDMN_MB_DOCK,
	TEXT("Open Package"), IDMN_SM_FileOpen,
	TEXT("Save Package"), IDMN_SM_FileSave,
	TEXT("Load Entire Package"), IDMN_SM_LOAD_ENTIRE_PACKAGE,
	TEXT("Insert Static Mesh into Level"), IDMN_SM_ADD_TO_LEVEL,
	TEXT("Create Static Mesh from Selection"), IDMN_SM_CREATE_FROM_SELECTION,
	TEXT("Previous Group"), IDMN_SM_PREV_GRP,
	TEXT("Next Group"), IDMN_SM_NEXT_GRP,

        // 3DBuzz
	TEXT("Frame Current"), IDMN_CENTERVIEW,
	NULL, 0
};

class WBrowserStaticMesh : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserStaticMesh,WBrowser,Window)

	TMap<DWORD,FWindowAnchor> Anchors;

	FContainer *Container;
	WLabel* ViewportLabel;
	WComboBox *PackageCombo, *GroupCombo;
	WCheckBox *GroupAllCheck, *RealtimeCheck;
	WListBox *MeshList;
	WObjectProperties *PropertyWindow;
	HWND hWndToolBar;
	WToolTip *ToolTipCtrl;
	MRUList* mrulist;
	WSplitterContainer* SplitterContainer;

	// 3DBuzz
	WSplitterContainer* SplitterContainer2;
	WGroupBox *ExtraContainer;

	UBOOL ShowRealtime;
	bool bAutoFrameSelect;

	UViewport *Viewport;

	// Structors.
	WBrowserStaticMesh( FName InPersistentName, WWindow* InOwnerWindow, HWND InEditorFrame )
	:	WBrowser( InPersistentName, InOwnerWindow, InEditorFrame )
	{
		Container = NULL;
		ViewportLabel = NULL;
		PackageCombo = GroupCombo = NULL;
		GroupAllCheck = NULL;
		Viewport = NULL;
		MeshList = NULL;
		PropertyWindow = NULL;
		SplitterContainer = NULL;

		// 3DBuzz
		SplitterContainer2 = NULL;
		ExtraContainer = NULL;

		RealtimeCheck = NULL;
		ShowRealtime = 0;

		bAutoFrameSelect = false;

		MenuID = IDMENU_BrowserStaticMesh;
		BrowserID = eBROWSER_STATICMESH;
		Description = TEXT("Static Meshes");
		mrulist = NULL;
	}

	// WBrowser interface.
	void OpenWindow( UBOOL bChild )
	{
		guard(WBrowserStaticMesh::OpenWindow);
		WBrowser::OpenWindow( bChild );
		SetCaption();
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserStaticMesh::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserStaticMesh) );

		Container = NEW FContainer();

		// 3DBuzz
		SplitterContainer2 = new WSplitterContainer( this );
		SplitterContainer2->OpenWindow( 0 );
		SplitterContainer2->ParentContainer = Container;
		SplitterContainer2->SetPct(80);

		ExtraContainer = new WGroupBox( SplitterContainer2->Pane1 );
		ExtraContainer->OpenWindow( 1, 0 );
		ExtraContainer->SetText( TEXT("Extra Container"));

		SplitterContainer = new WSplitterContainer( ExtraContainer );
		SplitterContainer->OpenWindow( 1 );
		SplitterContainer->ParentContainer = Container;

		ViewportLabel = NEW WLabel( SplitterContainer->Pane2, IDSC_VIEWPORT );
		ViewportLabel->OpenWindow( 1, 0 );

		PackageCombo = NEW WComboBox( this, IDCB_PACKAGE );
		PackageCombo->OpenWindow( 1, 1 );
		PackageCombo->SelectionChangeDelegate = FDelegate(this, (TDelegate)OnPackageComboSelChange);

		GroupCombo = NEW WComboBox( this, IDCB_GROUP );
		GroupCombo->OpenWindow( 1, 1 );
		GroupCombo->SelectionChangeDelegate = FDelegate(this, (TDelegate)OnGroupComboSelChange);

		GroupAllCheck = NEW WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)OnGroupAllClick) );
		GroupAllCheck->OpenWindow( 1, 0, 0, 1, 1, TEXT("All"), 1, 0, BS_PUSHLIKE );

                // 3DBuzz
		RealtimeCheck = new WCheckBox( this, IDCK_REALTIME, FDelegate(this, (TDelegate)OnRealtimeClick) );
		RealtimeCheck->OpenWindow( 1, 0, 0, 1, 1, TEXT("!"), 1, 0, BS_PUSHLIKE );

		MeshList = new WListBox( ExtraContainer, IDLB_STATIC_MESHES );
		MeshList->OpenWindow( 1, 0, 0, 0, 0, WS_VSCROLL | LBS_SORT );
		MeshList->SelectionChangeDelegate = FDelegate(this,(TDelegate)OnMeshSelectionChange);

		// 3DBuzz
		SetWindowPos(MeshList->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE);

		PropertyWindow = new WObjectProperties( TEXT("StaticMeshProperties"), CPF_Edit, TEXT("Properties"), SplitterContainer2->Pane2, 1 );
		PropertyWindow->ShowTreeLines = 0;
		PropertyWindow->SetNotifyHook( GUnrealEd );
		PropertyWindow->OpenChildWindow(0);
		PropertyWindow->Root.Sorted = 1;		


		// Create the static mesh browser viewport
		//
		FName Name = TEXT("StaticMeshBrowser");
		Viewport = GUnrealEd->Client->NewViewport( Name );
		GUnrealEd->Level->SpawnViewActor( Viewport );
		Viewport->Actor->ShowFlags = SHOW_StandardView | SHOW_ChildWindow | SHOW_Frame | SHOW_StaticMeshes | SHOW_SelectionHighlight;
		Viewport->Actor->RendMap   = REN_StaticMeshBrowser;
		Viewport->Actor->Misc1 = 0;
		Viewport->Actor->Misc2 = 0;
		Viewport->Group = NAME_None;
		Viewport->MiscRes = NULL;
		Viewport->Input->Init( Viewport );
		Viewport->OpenWindow( (DWORD)ViewportLabel->hWnd, 0, 320, 200, 0, 0 );

		// Set the rendering device from the INI file
		FString Device;
		GConfig->GetString( *PersistentName, TEXT("Device"), Device, TEXT("UnrealEd.ini") );
		Viewport->TryRenderDevice( *Device, Viewport->SizeX, Viewport->SizeY, 0 );

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
			IDB_BrowserStaticMesh_TOOLBAR,
                        // 3DBuzz - An extra button for the Frame Selection
			8,
			hInstance,
			IDB_BrowserStaticMesh_TOOLBAR,
			(LPCTBBUTTON)&tbBSMuttons,
			12,
			16,16,
			16,16,
			sizeof(TBBUTTON));
		check(hWndToolBar);

		ToolTipCtrl = NEW WToolTip(this);
		ToolTipCtrl->OpenWindow();
		for( INT tooltip = 0 ; ToolTips_SM[tooltip].ID > 0 ; ++tooltip )
		{
			// Figure out the rectangle for the toolbar button.
			INT index = SendMessageX( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_SM[tooltip].ID, 0 );
			RECT rect;
			SendMessageX( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

			ToolTipCtrl->AddTool( hWndToolBar, ToolTips_SM[tooltip].ToolTip, tooltip, &rect );
		}

		mrulist = NEW MRUList( *PersistentName );
		mrulist->ReadINI();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

		INT Top = 0;
		Anchors.Set( (DWORD)hWndToolBar, FWindowAnchor( hWnd, hWndToolBar,					ANCHOR_TL, 0, 0,			ANCHOR_RIGHT|ANCHOR_HEIGHT, 0, STANDARD_TOOLBAR_HEIGHT ) );
		Top += STANDARD_TOOLBAR_HEIGHT+4;

                // 3DBuzz - Quite a few changes here just to make the extra
                //          splitter bar work properly

		Anchors.Set((DWORD)PackageCombo->hWnd, FWindowAnchor( hWnd, PackageCombo->hWnd,		ANCHOR_TL, 4, Top,			ANCHOR_RIGHT|ANCHOR_HEIGHT, -4, STANDARD_CTRL_HEIGHT ) );

		Top += STANDARD_CTRL_HEIGHT+2;
		Anchors.Set( (DWORD)RealtimeCheck->hWnd, FWindowAnchor( hWnd, RealtimeCheck->hWnd,	ANCHOR_TL, 4, Top,				ANCHOR_WIDTH|ANCHOR_HEIGHT, 21, STANDARD_CTRL_HEIGHT ) );
		Anchors.Set( (DWORD)GroupAllCheck->hWnd, FWindowAnchor( hWnd, GroupAllCheck->hWnd,	ANCHOR_TL, 4+21+2, Top,			ANCHOR_WIDTH|ANCHOR_HEIGHT, 64, STANDARD_CTRL_HEIGHT ) );
		Anchors.Set( (DWORD)GroupCombo->hWnd, FWindowAnchor( hWnd, GroupCombo->hWnd,		ANCHOR_TL, 4+64+21+2, Top,			ANCHOR_RIGHT|ANCHOR_HEIGHT, -4, STANDARD_CTRL_HEIGHT ) );
		Top += STANDARD_CTRL_HEIGHT+2;

		Anchors.Set( (DWORD)SplitterContainer2->hWnd, FWindowAnchor( hWnd, SplitterContainer2->hWnd,					ANCHOR_TL, 0, Top,	ANCHOR_BR, 0, 0 ) );

		Anchors.Set( (DWORD)ExtraContainer->hWnd, FWindowAnchor( SplitterContainer2->Pane1->hWnd, ExtraContainer->hWnd,	ANCHOR_TL,0,0,						ANCHOR_BR,0,0 ) );
		Anchors.Set( (DWORD)PropertyWindow->hWnd, FWindowAnchor( SplitterContainer2->Pane2->hWnd, PropertyWindow->hWnd,	ANCHOR_TL, 0, 0,					ANCHOR_BR, 0, 0 ) );

		Anchors.Set( (DWORD)SplitterContainer->hWnd, FWindowAnchor( ExtraContainer->hWnd, SplitterContainer->hWnd,						ANCHOR_TL, 0, 0,					ANCHOR_BR,0,0 ) );

		Anchors.Set( (DWORD)MeshList->hWnd, FWindowAnchor( SplitterContainer->Pane1->hWnd, MeshList->hWnd,				ANCHOR_TL,0,0,						ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0 ) );

		Anchors.Set( (DWORD)ViewportLabel->hWnd, FWindowAnchor( SplitterContainer->Pane2->hWnd, ViewportLabel->hWnd,	ANCHOR_TL,0,0,						ANCHOR_BR,0,0 ) );
		Anchors.Set( (DWORD)Viewport->GetWindow(), FWindowAnchor( ViewportLabel->hWnd, (HWND)Viewport->GetWindow(),		ANCHOR_TL,0,0,						ANCHOR_BR,0,0 ) );

		Container->SetAnchors( &Anchors );

		PositionChildControls();
		RefreshPackages();
		RefreshGroups();
		RefreshStaticMeshList();

		SetCaption();

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserStaticMesh::OnDestroy);

		FString Device = Viewport->RenDev->GetClass()->GetFullName();
		Device = Device.Right( Device.Len() - Device.InStr( TEXT(" "), 0 ) - 1 );
		GConfig->SetString( *PersistentName, TEXT("Device"), *Device, TEXT("UnrealEd.ini") );

		delete Viewport;

		delete Container;
		delete PackageCombo;
		delete GroupCombo;
		delete GroupAllCheck;
		delete MeshList;
		delete SplitterContainer;
		delete ViewportLabel;

		// 3DBuzz - Cleanup duty
		delete SplitterContainer2;
		delete RealtimeCheck;

		PropertyWindow->Root.SetObjects( NULL, 0 );
		delete PropertyWindow;

		::DestroyWindow( hWndToolBar );
		delete ToolTipCtrl;

		mrulist->WriteINI();
		delete mrulist;

		WBrowser::OnDestroy();
		unguard;
	}
	virtual void UpdateMenu()
	{
		guard(WBrowserStaticMesh::UpdateMenu);

		HWND hwnd = IsDocked() ? OwnerWindow->hWnd : hWnd;
		HMENU menu = GetMenu( hwnd );

		if( mrulist 
				&& GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hwnd, menu );

		if(menu)
		{
                        // 3DBuzz - Manages the Auto Frame Selection menu item
			CheckMenuItem( menu, ID_SM_AUTOFRAMESELECTION, (bAutoFrameSelect) ? MF_CHECKED : MF_UNCHECKED  );
			CheckMenuItem( menu, ID_SM_SHOW_COLLISION, (Viewport->Actor->ShowFlags&SHOW_Collision) ? MF_CHECKED : MF_UNCHECKED  );
			CheckMenuItem( menu, ID_SM_SHOW_KPRIMS, (Viewport->Actor->ShowFlags&SHOW_KarmaPrimitives) ? MF_CHECKED : MF_UNCHECKED  );
			CheckMenuItem( menu, ID_SM_SHOW_KMASSPROPS, (Viewport->Actor->ShowFlags&SHOW_KarmaMassProps) ? MF_CHECKED : MF_UNCHECKED  );
			CheckMenuItem( menu, ID_SM_WIREFRAME, (Viewport->Actor->RendMap == REN_Wire) ? MF_CHECKED : MF_UNCHECKED  );
		}

		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserStaticMesh::OnCommand);
		switch( Command )
		{
			case WM_POSITIONCHILDCONTROLS:
				PositionChildControls();
				break;
			case IDMN_SM_IMPORT_ASE:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Supported formats (*.ase,*.lwo)\0*.ase;*.lwo\0ASE Files (*.ase)\0*.ase\0Lightwave Files (*.lwo)\0*.lwo\0\0"; // sjs
				ofn.nFilterIndex = 1;
				ofn.lpstrDefExt = "ase";
				ofn.lpstrTitle = "Import Static Mesh";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_ASE]) );
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

				// Display the Open dialog box. 
				//
				if( GetOpenFileNameA(&ofn) )
				{
					INT NumNULLs = FormatFilenames( File );
					FString Package = PackageCombo->GetString( PackageCombo->GetCurrent() );
					FString Group = GroupCombo->GetString( GroupCombo->GetCurrent() );
	
					TArray<FString> StringArray;
					FString S = appFromAnsi( File );
					S.ParseIntoArray( TEXT("|"), &StringArray );

					INT iStart = 0;
					FString Prefix = TEXT("\0");

					if( NumNULLs )
					{
						iStart = 1;
						Prefix = *(StringArray(0));
						Prefix += TEXT("\\");
					}

					if( StringArray.Num() == 1 )
						GLastDir[eLASTDIR_ASE] = StringArray(0).Left( StringArray(0).InStr( TEXT("\\"), 1 ) );
					else
						GLastDir[eLASTDIR_ASE] = StringArray(0);

					TArray<FString> FilenamesArray;

					for( INT x = iStart ; x < StringArray.Num() ; ++x )
					{
						FString NewString;

						NewString = FString::Printf( TEXT("%s%s"), *Prefix, *(StringArray(x)) );
						new(FilenamesArray)FString( NewString );

						FString S = NewString;
					}

					WDlgImportStaticMesh l_dlg( NULL, this );
					if( l_dlg.DoModal( Package, Group, &FilenamesArray ) )
					{
						// Flip to the package/group that was used for importing
						GBrowserMaster->RefreshAll();
						PackageCombo->SetCurrent( PackageCombo->FindStringExact( *l_dlg.Package) );
						RefreshGroups();
						GroupCombo->SetCurrent( GroupCombo->FindStringExact( *l_dlg.Group) );
						RefreshStaticMeshList();
					}

					StringArray.Empty();
					FilenamesArray.Empty();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDMN_SM_LOAD_ENTIRE_PACKAGE:
			{
				FString Package = PackageCombo->GetString( PackageCombo->GetCurrent() );
				if( Package != TEXT("MyLevel") )
				{
					GUnrealEd->LoadPackage( NULL, *Package, LOAD_NoWarn );
					RefreshAll();
				}
			}
			break;

			case IDMN_SM_ADD_TO_LEVEL:
			{
				if( !GUnrealEd->CurrentStaticMesh )
				{
					appMsgf(0, TEXT("Select a static mesh first."));
					break;
				}

				UViewport* Viewport = GUnrealEd->GetCurrentViewport();
				if( !Viewport )
				{
					appMsgf(0, TEXT("No active viewport."));
					break;
				}

				GUnrealEd->Exec( *FString::Printf( TEXT("STATICMESH ADD NAME=%s SNAP=1 X=%1.1f Y=%1.1f Z=%1.1f"),
					GUnrealEd->CurrentStaticMesh->GetPathName(),
					Viewport->Actor->Location.X, Viewport->Actor->Location.Y, Viewport->Actor->Location.Z
					) );
				GUnrealEd->Exec( TEXT("POLY SELECT NONE") );

				GUnrealEd->RedrawLevel( GUnrealEd->Level );
			}
			break;

			case IDMN_SM_CREATE_FROM_SELECTION:
			{
				CreateFromSelection();
			}
			break;

			// 3DBuzz - Centers the camera on the current Static Mesh
			case IDMN_CENTERVIEW:
			{
				FBox Box = GUnrealEd->CurrentStaticMesh->GetRenderBoundingBox( Viewport->Actor );

				FVector Center = Box.GetCenter();
				FLOAT Extent = (Box.Max - Box.Min).Size();
				Extent *= 1.2f;
				FVector Distance = Viewport->Actor->Rotation.Vector();
				Distance = Distance.GetNormalized() * Extent;

                                Viewport->Actor->Location = Center;
				Viewport->Actor->Location -= Distance;

				Viewport->Repaint(1);
			}
			break;

			case IDMN_SM_DELETE:
			{
				if( !GUnrealEd->CurrentStaticMesh )
				{
					appMsgf( 0, TEXT("Select a static mesh first.") );
					break;
				}

				INT Sel = MeshList->GetCurrent();

				FString Name = GUnrealEd->CurrentStaticMesh->GetPathName();
				FStringOutputDevice GetPropResult = FStringOutputDevice();

				GUnrealEd->CurrentStaticMesh = NULL;
				PropertyWindow->Root.SetObjects( NULL, 0 );
				GUnrealEd->Get( TEXT("Obj"), *FString::Printf(TEXT("DELETE CLASS=STATICMESH OBJECT=\"%s\""), *Name), GetPropResult);

				if( GetPropResult.Len() )
					appMsgf( 0, TEXT("Can't delete static mesh.\n\n%s"), *GetPropResult );

				RefreshPackages();
				RefreshGroups();
				RefreshStaticMeshList();

				if( MeshList->SetCurrent( Sel ) == LB_ERR )
					MeshList->SetCurrent( 0 );
				OnMeshSelectionChange();
			}
			break;

			case IDMN_SM_PREV_GRP:
			{
				INT Sel = GroupCombo->GetCurrent();
				Sel--;
				if( Sel < 0 ) Sel = GroupCombo->GetCount() - 1;
				GroupCombo->SetCurrent(Sel);
				RefreshStaticMeshList();
			}
			break;

			case IDMN_SM_NEXT_GRP:
			{
				INT Sel = GroupCombo->GetCurrent();
				Sel++;
				if( Sel >= GroupCombo->GetCount() ) Sel = 0;
				GroupCombo->SetCurrent(Sel);
				RefreshStaticMeshList();
			}
			break;

			case IDMN_SM_RENAME:
			{
				if( !GUnrealEd->CurrentStaticMesh )
				{
					appMsgf( 0, TEXT("Select a static mesh first.") );
					break;
				}

				WDlgRename dlg( NULL, this );
				FString Group, Package;
				if( !Cast<UPackage>(GUnrealEd->CurrentStaticMesh->GetOuter()->GetOuter()) )
				{
					Group = TEXT("");
					Package = GUnrealEd->CurrentStaticMesh->GetOuter()->GetName();
				}
				else
				{			
					Group = GUnrealEd->CurrentStaticMesh->GetOuter()->GetName();
					Package = GUnrealEd->CurrentStaticMesh->GetOuter()->GetOuter()->GetName();
				}
				if( dlg.DoModal( GUnrealEd->CurrentStaticMesh->GetName(), Group, Package ) )
					GUnrealEd->Exec(*FString::Printf(TEXT("OBJ RENAME OLDNAME=\"%s\" OLDGROUP=\"%s\" OLDPACKAGE=\"%s\" NEWNAME=\"%s\" NEWGROUP=\"%s\" NEWPACKAGE=\"%s\""), *dlg.OldName, *dlg.OldGroup, *dlg.OldPackage, *dlg.NewName, *dlg.NewGroup, *dlg.NewPackage) );
				RefreshPackages();
				RefreshGroups();
				RefreshStaticMeshList();
			}
			break;

			//3dbuzz
			case IDMN_TB_INUSE:
			{
				(new WDlgPackageBuilder( NULL, this, FDelegate(this, (TDelegate)RefreshAll) ))->DoModeless( );
			}
			break;

			case IDMN_SM_FileOpen:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Static Mesh Packages (*.usx)\0*.usx\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_USX]) );
				ofn.lpstrDefExt = "usx";
				ofn.lpstrTitle = "Open Static Mesh Package";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

				if( GetOpenFileNameA(&ofn) )
				{
					INT NumNULLs = FormatFilenames( File );
	
					TArray<FString> StringArray;
					FString S = appFromAnsi( File );
					S.ParseIntoArray( TEXT("|"), &StringArray );

					INT iStart = 0;
					FString Prefix = TEXT("\0");

					if( NumNULLs )
					{
						iStart = 1;
						Prefix = *(StringArray(0));
						Prefix += TEXT("\\");
					}

					if( StringArray.Num() > 0 )
					{
						if( StringArray.Num() == 1 )
						{
							SavePkgName = *(StringArray(0));
							SavePkgName = SavePkgName.Right( SavePkgName.Len() - (SavePkgName.Left( SavePkgName.InStr(TEXT("\\"), 1)).Len() + 1 ));
						}
						else
							SavePkgName = *(StringArray(1));
						SavePkgName = SavePkgName.Left( SavePkgName.InStr(TEXT(".")) );
					}

					if( StringArray.Num() == 1 )
						GLastDir[eLASTDIR_USX] = StringArray(0).Left( StringArray(0).InStr( TEXT("\\"), 1 ) );
					else
						GLastDir[eLASTDIR_USX] = StringArray(0);

					GWarn->BeginSlowTask( TEXT(""), 1 );

					for( INT x = iStart ; x < StringArray.Num() ; ++x )
					{
						GWarn->StatusUpdatef( x, StringArray.Num(), TEXT("Loading %s"), *(StringArray(x)) );
						GUnrealEd->Exec( *FString::Printf(TEXT("OBJ LOAD FILE=\"%s%s\""), *Prefix, *(StringArray(x))) );

						mrulist->AddItem( *(StringArray(x)) );
						if( GBrowserMaster->GetCurrent()==BrowserID )
							mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
					}

					GWarn->EndSlowTask();

					GBrowserMaster->RefreshAll();
					PackageCombo->SetCurrent( PackageCombo->FindStringExact( *SavePkgName ) );
					RefreshGroups();
					RefreshStaticMeshList();

					StringArray.Empty();
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case IDMN_SM_FileSave:
			{
				OPENFILENAMEA ofn;
				char File[8192] = "\0";
				FString Package = PackageCombo->GetString( PackageCombo->GetCurrent() );

				::sprintf( File, "%s.usx", TCHAR_TO_ANSI( *Package ) );

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 8192;
				ofn.lpstrFilter = "Static Mesh Packages (*.usx)\0*.usx\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_USX]) );
				ofn.lpstrDefExt = "usx";
				ofn.lpstrTitle = "Save Static Mesh Package";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

				if( GetSaveFileNameA(&ofn) )
				{
					TCHAR l_chCmd[512];

					appSprintf( l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=\"%s\" FILE=\"%s\""),
						*Package, appFromAnsi( File ) );
					if( GUnrealEd->Exec( l_chCmd ) )
					{
						FString S = appFromAnsi( File );
						mrulist->AddItem( S );
						if( GBrowserMaster->GetCurrent()==BrowserID )
							mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
						GLastDir[eLASTDIR_USX] = S.Left( S.InStr( TEXT("\\"), 1 ) );
					}
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_SM_SHOW_KPRIMS:
				{
					Viewport->Actor->ShowFlags ^= SHOW_KarmaPrimitives;
					Viewport->Repaint( 1 );
					UpdateMenu();
				}
			break;

			case ID_SM_SHOW_COLLISION:
				{
					Viewport->Actor->ShowFlags ^= SHOW_Collision;
					Viewport->Repaint( 1 );
					UpdateMenu();
				}
			break;

			case ID_SM_SHOW_KMASSPROPS:
				{
					Viewport->Actor->ShowFlags ^= SHOW_KarmaMassProps;
					Viewport->Repaint( 1 );
					UpdateMenu();
				}
			break;

                        // 3DBuzz - The event triggered by the Auto Frame Selection menu item
			case ID_SM_AUTOFRAMESELECTION:
				{
					bAutoFrameSelect = (bAutoFrameSelect) ? false : true;
					UpdateMenu();
				}
			break;

			case IDMN_MRU1:
			case IDMN_MRU2:
			case IDMN_MRU3:
			case IDMN_MRU4:
			case IDMN_MRU5:
			case IDMN_MRU6:
			case IDMN_MRU7:
			case IDMN_MRU8:
			{
				FString Filename = mrulist->Items[Command - IDMN_MRU1];
				GUnrealEd->Exec( *FString::Printf(TEXT("OBJ LOAD FILE=\"%s\""), *Filename ));

				mrulist->MoveToTop( Command - IDMN_MRU1 );
				mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );

				FString Package = Filename.Right( Filename.Len() - (Filename.InStr( TEXT("\\"), 1) + 1) );
				Package = Package.Left( Package.InStr( TEXT(".")) );

				GBrowserMaster->RefreshAll();
				RefreshPackages();
				PackageCombo->SetCurrent( PackageCombo->FindStringExact( *Package ) );
				RefreshGroups();
				RefreshStaticMeshList();
			}
			break;

                        case ID_SM_USE_6DOP_COLLISION:
			{
				TArray<FVector> dirs;
				for(INT i=0; i<6; i++)
					dirs.AddItem(KDopDir6[i]);

				GenerateKDopAsCollisionModel(dirs);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_10DOPX_COLLISION:
			{
				TArray<FVector> dirs;
				for(INT i=0; i<10; i++)
					dirs.AddItem(KDopDir10X[i]);

				GenerateKDopAsCollisionModel(dirs);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_10DOPY_COLLISION:
			{
				TArray<FVector> dirs;
				for(INT i=0; i<10; i++)
					dirs.AddItem(KDopDir10Y[i]);

				GenerateKDopAsCollisionModel(dirs);
				Viewport->DirtyViewport = 1;
			}
			break;			
			
			case ID_SM_USE_10DOPZ_COLLISION:
			{
				TArray<FVector> dirs;
				for(INT i=0; i<10; i++)
					dirs.AddItem(KDopDir10Z[i]);

				GenerateKDopAsCollisionModel(dirs);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_18DOP_COLLISION:
			{
				TArray<FVector> dirs;
				for(INT i=0; i<18; i++)
					dirs.AddItem(KDopDir18[i]);

				GenerateKDopAsCollisionModel(dirs);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_26DOP_COLLISION:
			{
				TArray<FVector> dirs;
				for(INT i=0; i<26; i++)
					dirs.AddItem(KDopDir26[i]);

				GenerateKDopAsCollisionModel(dirs);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_AUTOOBB:
			{
				GenerateOBBAsCollisionModel();
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_KARMA_SPHERE:
			{
				GenerateSphereAsKarmaCollision();
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_KARMA_CYLX:
			{
				GenerateCylinderAsKarmaCollision(0);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_KARMA_CYLY:
			{
				GenerateCylinderAsKarmaCollision(1);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_USE_KARMA_CYLZ:
			{
				GenerateCylinderAsKarmaCollision(2);
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_IMPORT_ASE_COLLISION:
			{
				// TODO
				appMsgf( 0, TEXT("Sorry. Not Yet Implemented! (jag)") );
				Viewport->DirtyViewport = 1;
			}
			break;

			case ID_SM_REFRESH_KPRIMS:
			{
#ifdef WITH_KARMA
				if( !GUnrealEd->CurrentStaticMesh )
				{
					appMsgf( 0, TEXT("Select a static mesh first.") );
					break;
				}

				UStaticMesh* StaticMesh = GUnrealEd->CurrentStaticMesh;

				KModelToHulls(&StaticMesh->KPhysicsProps->AggGeom, StaticMesh->CollisionModel, FVector(0, 0, 0));
				KUpdateMassProps(StaticMesh->KPhysicsProps);

				UObject* Outer = StaticMesh->GetOuter();
				while( Outer && Outer->GetOuter() )
					Outer = Outer->GetOuter();
				if( Outer && Cast<UPackage>(Outer) )
					Cast<UPackage>(Outer)->bDirty = 1;
#endif
			}
			break;

			case ID_SM_REMOVE_COLLISION:
				{
					if( !GUnrealEd->CurrentStaticMesh )
					{
						appMsgf( 0, TEXT("Select a static mesh first.") );
						break;
					}

					UStaticMesh* StaticMesh = GUnrealEd->CurrentStaticMesh;

					UBOOL doIt = appMsgf(1, TEXT("This will remove all collision geometry from this Static Mesh.\nAre you sure?"));
					if(!doIt)
						return;

					UBOOL didSomething = false;

					// For UModel collision model, remove reference and let garbage collection clean it up.
					if(StaticMesh->CollisionModel)
					{
						StaticMesh->CollisionModel = NULL;
						didSomething = true;
					}

#ifdef WITH_KARMA
					// For UKMeshProps, we explicitly delete it.
					if(StaticMesh->KPhysicsProps)
					{
						delete StaticMesh->KPhysicsProps;
						StaticMesh->KPhysicsProps = NULL;						
						didSomething = true;
					}
#endif

					// If we actually changed something, mark this package as dirty.
					if(didSomething)
					{
						UObject* Outer = StaticMesh->GetOuter();
						while( Outer && Outer->GetOuter() )
							Outer = Outer->GetOuter();
						if( Outer && Cast<UPackage>(Outer) )
							Cast<UPackage>(Outer)->bDirty = 1;
					}

				}
			break;
				
			case IDMN_COPY_MESH_PHYSPROPS:
			{
#ifdef WITH_KARMA
				if( !GUnrealEd->CurrentStaticMesh )
				{
					appMsgf( 0, TEXT("Select a static mesh first.") );
					break;
				}

				UStaticMesh* StaticMesh = GUnrealEd->CurrentStaticMesh;

				if(StaticMesh->KPhysicsProps)
					KCopyMassPropsToClipboard(StaticMesh->KPhysicsProps);
#endif
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
		guard(WBrowserStaticMesh::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		UpdateMenu();
		unguard;
	}
	//3DBuzz
	UStaticMesh *CreateNewStaticMesh(UStaticMesh *OldStaticMesh, FName PkgName, FName GroupName, FName Name)
	{
		guard(WDlgPackageBuilder::CreateNewStaticMesh);

		TArray<FStaticMeshTriangle>	Triangles;
		TArray<FStaticMeshMaterial>	Materials;

		GUnrealEd->Trans->Begin(TEXT("STATICMESH FROM SELECTION"));
		GUnrealEd->Level->Modify();
		GUnrealEd->FinishAllSnaps(GUnrealEd->Level);

		UPackage *Pkg = GUnrealEd->CreatePackage(NULL,*PkgName);
		Pkg->bDirty = 1;

		if( GroupName != NAME_None )
			Pkg = GUnrealEd->CreatePackage(Pkg,*GroupName);

		check(OldStaticMesh);

		if(!OldStaticMesh->RawTriangles.Num())
			OldStaticMesh->RawTriangles.Load();

		// Create the UStaticMesh object.
		UStaticMesh*	StaticMesh = new(Pkg,Name,RF_Public|RF_Standalone) UStaticMesh;

		StaticMesh->RawTriangles.Add(OldStaticMesh->RawTriangles.Num());
		appMemcpy(&StaticMesh->RawTriangles(0),&OldStaticMesh->RawTriangles(0),OldStaticMesh->RawTriangles.Num() * sizeof(FStaticMeshTriangle));

		StaticMesh->Materials.Add(OldStaticMesh->Materials.Num());
		appMemcpy(&StaticMesh->Materials(0),&OldStaticMesh->Materials(0),OldStaticMesh->Materials.Num() * sizeof(FStaticMeshMaterial));

		/*
		//merge
		StaticMesh->MaxSmoothingAngles.Add(OldStaticMesh->MaxSmoothingAngles.Num());
		appMemcpy(&StaticMesh->MaxSmoothingAngles(0),&OldStaticMesh->MaxSmoothingAngles(0),OldStaticMesh->MaxSmoothingAngles.Num() * sizeof(FLOAT));

		// if there are any collision triangles, then create a collision model from them for this static mesh
		if (OldStaticMesh->CollisionModel)
		{
			StaticMesh->CollisionModel = new(StaticMesh->GetOuter()) UModel(NULL,1);

			for(int i = 0; i < OldStaticMesh->CollisionModel->Polys->Element.Num(); i++)
			{
				FPoly*	Poly = new(StaticMesh->CollisionModel->Polys->Element) FPoly();
				Poly->Init();
				for (int iv = 0; iv < OldStaticMesh->CollisionModel->Polys->Element(i).NumVertices; iv++)
				{
					Poly->Vertex[iv] = OldStaticMesh->CollisionModel->Polys->Element(i).Vertex[iv];
				}

				Poly->iLink = INDEX_NONE;
				Poly->NumVertices = iv;
				Poly->CalcNormal(1);
				Poly->Finalize(0);
			}

			StaticMesh->CollisionModel->BuildBound();

			GEditor->bspBuild(StaticMesh->CollisionModel,BSP_Good,15,70,1,0);
			GEditor->bspRefresh(StaticMesh->CollisionModel,1);
			GEditor->bspBuildBounds(StaticMesh->CollisionModel);
		}

		//--------------------------------
		// KARMA
		//--------------------------------

		// If we dont already have physics props, construct them here.
		if(OldStaticMesh->KPhysicsProps && !StaticMesh->KPhysicsProps)
		{
			StaticMesh->KPhysicsProps = ConstructObject<UKMeshProps>(UKMeshProps::StaticClass(), StaticMesh);

			StaticMesh->KPhysicsProps->COMOffset = OldStaticMesh->KPhysicsProps->COMOffset;
			appMemcpy(&StaticMesh->KPhysicsProps->InertiaTensor,&OldStaticMesh->KPhysicsProps->InertiaTensor,6 * sizeof(FLOAT));

			StaticMesh->KPhysicsProps->AggGeom.SphereElems.Add(OldStaticMesh->KPhysicsProps->AggGeom.SphereElems.Num());
			appMemcpy(&StaticMesh->KPhysicsProps->AggGeom.SphereElems(0),&OldStaticMesh->KPhysicsProps->AggGeom.SphereElems(0),OldStaticMesh->KPhysicsProps->AggGeom.SphereElems.Num() * sizeof(FKSphereElem));

			StaticMesh->KPhysicsProps->AggGeom.BoxElems.Add(OldStaticMesh->KPhysicsProps->AggGeom.BoxElems.Num());
			appMemcpy(&StaticMesh->KPhysicsProps->AggGeom.BoxElems(0),&OldStaticMesh->KPhysicsProps->AggGeom.BoxElems(0),OldStaticMesh->KPhysicsProps->AggGeom.BoxElems.Num() * sizeof(FKBoxElem));

			StaticMesh->KPhysicsProps->AggGeom.CylinderElems.Add(OldStaticMesh->KPhysicsProps->AggGeom.CylinderElems.Num());
			appMemcpy(&StaticMesh->KPhysicsProps->AggGeom.CylinderElems(0),&OldStaticMesh->KPhysicsProps->AggGeom.CylinderElems(0),OldStaticMesh->KPhysicsProps->AggGeom.CylinderElems.Num() * sizeof(FKCylinderElem));

			StaticMesh->KPhysicsProps->AggGeom.ConvexElems.AddZeroed(OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems.Num());

			for (int i = 0; i < StaticMesh->KPhysicsProps->AggGeom.ConvexElems.Num(); i++)
			{
				appMemcpy(&StaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).TM,&OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).TM,sizeof(FMatrix));

				StaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).VertexData.Add(OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).VertexData.Num());
				appMemcpy(&StaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).VertexData(0),&OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).VertexData(0),OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).VertexData.Num() * sizeof(FVector));

				StaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).PlaneData.Add(OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).PlaneData.Num());
				appMemcpy(&StaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).PlaneData(0),&OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).PlaneData(0),OldStaticMesh->KPhysicsProps->AggGeom.ConvexElems(i).PlaneData.Num() * sizeof(FPlane));
			}
		}

		// copy over the karma collision object
		StaticMesh->KCollisionGeom.Add(OldStaticMesh->KCollisionGeom.Num());
		appMemcpy(&StaticMesh->KCollisionGeom(0),&OldStaticMesh->KCollisionGeom(0),OldStaticMesh->KCollisionGeom.Num() * sizeof(McdGeometryID));

		StaticMesh->KCollisionGeomScale3D = OldStaticMesh->KCollisionGeomScale3D;

		if (StaticMesh->KPhysicsProps)
			KUpdateMassProps(StaticMesh->KPhysicsProps);
	*/
		//--------------------------------

		StaticMesh->UseSimpleBoxCollision = OldStaticMesh->UseSimpleBoxCollision;
		StaticMesh->UseSimpleKarmaCollision = OldStaticMesh->UseSimpleKarmaCollision;
		StaticMesh->UseSimpleLineCollision = OldStaticMesh->UseSimpleLineCollision;
		StaticMesh->UseVertexColor = OldStaticMesh->UseVertexColor;

		StaticMesh->Build();

		// Assign a valid content authentication key. 
		//merge StaticMesh->AuthenticationKey = StaticMesh->CreateAuthenticationKey();

		GUnrealEd->Trans->End();
		GUnrealEd->RedrawLevel(GUnrealEd->Level);

		return StaticMesh;

		unguard;
	}
	virtual void RefreshAll()
	{
		guard(WBrowserStaticMesh::RefreshAll);
		RefreshPackages();
		RefreshGroups();
		RefreshStaticMeshList();
		if( GBrowserMaster->GetCurrent()==BrowserID )
			mrulist->AddToMenu( hWnd, GetMenu( IsDocked() ? OwnerWindow->hWnd : hWnd ) );
		unguard;
	}
	void RefreshPackages()
	{
		guard(WBrowserStaticMesh::RefreshPackages);

		INT Current = PackageCombo->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;

		// PACKAGES
		//
		PackageCombo->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GUnrealEd->Get( TEXT("OBJ"), TEXT("PACKAGES CLASS=StaticMesh"), GetPropResult );

		TArray<FString> StringArray;
		GetPropResult.ParseIntoArray( TEXT(","), &StringArray );

		for( INT x = 0 ; x < StringArray.Num() ; ++x )
			PackageCombo->AddString( *(StringArray(x)) );

		PackageCombo->SetCurrent( Current );
		StringArray.Empty();

		unguard;
	}
	void RefreshGroups()
	{
		guard(WBrowserStaticMesh::RefreshGroups);

		FString Package = PackageCombo->GetString( PackageCombo->GetCurrent() );
		INT Current = GroupCombo->GetCurrent();
		Current = (Current != CB_ERR) ? Current : 0;

		// GROUPS
		//
		GroupCombo->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		TCHAR l_ch[256];
		appSprintf( l_ch, TEXT("GROUPS CLASS=StaticMesh PACKAGE=\"%s\""), *Package );
		GUnrealEd->Get( TEXT("OBJ"), l_ch, GetPropResult );

		TArray<FString> StringArray;
		GetPropResult.ParseIntoArray( TEXT(","), &StringArray );

		// scion sz
		// make sure first item in combo list is empty so that
		// we can see items with no group specified
		GroupCombo->AddString( TEXT("") );
		for( INT x = 0 ; x < StringArray.Num() ; ++x )
		{
			GroupCombo->AddString( *(StringArray(x)) );
		}

		GroupCombo->SetCurrent(Current);
		StringArray.Empty();

		unguard;
	}
	void RefreshStaticMeshList()
	{
		guard(WBrowserStaticMesh::RefreshStaticMeshList);

		INT Sel = MeshList->GetCurrent();

		FString Package = PackageCombo->GetString( PackageCombo->GetCurrent() );
		FString Group = GroupCombo->GetString( GroupCombo->GetCurrent() );

		MeshList->Empty();

		UObject* Pkg = FindObject<UPackage>( ANY_PACKAGE, *Package );
		UObject* OuterPkg = Pkg;
		if( !GroupAllCheck->IsChecked() )
			if( Pkg && Group.Len() )
				Pkg = FindObject<UPackage>( Pkg, *Group );

		// Make a short list of filtered static meshes.
		for( TObjectIterator<UStaticMesh> It ; It ; ++It )
		{
			if( It->IsIn(Pkg) )
			{
				// scion sz 
				// If the group name is empty then just show the objects with no group outers
				if( (Group.Len() == 0 && It->GetOuter() == OuterPkg) ||
					Group.Len() > 0 )
				{
                    MeshList->AddString( It->GetName() );                    
				}				
			}
		}

		if( MeshList->SetCurrent( Sel ) == LB_ERR )
			MeshList->SetCurrent( 0 );
		OnMeshSelectionChange();

		UpdateMenu();

		unguard;
	}

	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls()
	{
		guard(WBrowserStaticMesh::PositionChildControls);
		if( Container )
			Container->RefreshControls();

                // 3DBuzz - Fixes some annoying refresh problems with the
                //          property window
		if (PropertyWindow)
		{
                        PropertyWindow->ForceRefresh();
			SetWindowPos(PropertyWindow->hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE);
		}

		InvalidateRect( hWnd, NULL, 1 );
		unguard;
	}
	virtual void SetCaption( FString* Tail = NULL )
	{
		guard(WBrowserStaticMesh::SetCaption);

		FString Extra;
		if( GUnrealEd->CurrentMaterial )
			Extra = *FString::Printf( TEXT("%s"), GUnrealEd->CurrentMaterial->GetPathName() );

		WBrowser::SetCaption( &Extra );
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnPackageComboSelChange()
	{
		guard(WBrowserStaticMesh::OnPackageComboSelChange);
		RefreshGroups();
		RefreshStaticMeshList();
		unguard;
	}
	void OnGroupComboSelChange()
	{
		guard(WBrowserStaticMesh::OnGroupComboSelChange);
		RefreshStaticMeshList();
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserStaticMesh::OnGroupAllClick);
		EnableWindow( GroupCombo->hWnd, !GroupAllCheck->IsChecked() );
		RefreshStaticMeshList();
		unguard;
	}
        // 3DBuzz - Handles the click event for the realtime check button
	void OnRealtimeClick()
	{
		guard(WBrowserStaticMesh::OnRealtimeClick);
		ShowRealtime = RealtimeCheck->IsChecked();
		Viewport->Actor->ShowFlags &= ~SHOW_RealTime;
		if( ShowRealtime )
			Viewport->Actor->ShowFlags |= SHOW_RealTime;
		unguard;
	}	
	void OnMeshSelectionChange()
	{
		FString Package = PackageCombo->GetString( PackageCombo->GetCurrent() );
		FString Group = GroupCombo->GetString( GroupCombo->GetCurrent() );
		FString StaticMesh = MeshList->GetString( MeshList->GetCurrent() );

		UObject* Pkg = FindObject<UPackage>( ANY_PACKAGE, *Package );
		if( !GroupAllCheck->IsChecked() )
			if( Pkg && Group.Len() )
				Pkg = FindObject<UPackage>( Pkg, *Group );

		GUnrealEd->CurrentStaticMesh = NULL;
		PropertyWindow->Root.SetObjects( NULL, 0 );
		for( TObjectIterator<UStaticMesh> It ; It ; ++It )
		{
			if( StaticMesh == It->GetName() && It->IsIn(Pkg) )
			{
				GUnrealEd->CurrentStaticMesh = (*It);
				PropertyWindow->Root.SetObjects( (UObject**)&GUnrealEd->CurrentStaticMesh, 1 );
			}
		}

                // 3DBuzz - If Auto Frame Select is checked, center
                //          the camera on the Static Mesh
		if (bAutoFrameSelect)
			OnCommand(IDMN_CENTERVIEW);

		Viewport->Repaint( 1 );
	}

	void CreateFromSelection()
	{
		guard(WBrowserStaticMesh::CreateFromSelection);

		WDlgGeneric dlg( NULL, this, OPTIONS_MATNEWSTATICMESH, TEXT("New Static Mesh") );
		if( dlg.DoModal( TEXT("") ) )
		{
			UOptionsMatNewStaticMesh* Proxy = Cast<UOptionsMatNewStaticMesh>(dlg.Proxy);
			GUnrealEd->Exec( *FString::Printf( TEXT("STATICMESH FROM SELECTION PACKAGE=%s %s%s%s NAME=%s"),
				*Proxy->PkgName,
				Proxy->Group.Len() ? TEXT("GROUP=\"") : TEXT(""),
				*Proxy->Group,
				Proxy->Group.Len() ? TEXT("\"") : TEXT(""),
				*Proxy->Name )
			);
			GBrowserMaster->RefreshAll();
		}

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
