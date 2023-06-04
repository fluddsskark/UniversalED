#if 1 //NEW: U2Ed
/*=============================================================================
	BrowserActor : Browser window for actor classes
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

// --------------------------------------------------------------
//
// NEW CLASS Dialog
//
// --------------------------------------------------------------

class WDlgNewClass : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgNewClass,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WLabel ParentLabel;
	WEdit PackageEdit;
	WEdit NameEdit;

	FString ParentClass, Package, Name;

	// Constructor.
	WDlgNewClass( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("New Class"), IDDIALOG_NEW_CLASS, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)OnOk) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)EndDialogFalse) )
	,	ParentLabel		( this, IDSC_PARENT )
	,	PackageEdit		( this, IDEC_PACKAGE )
	,	NameEdit		( this, IDEC_NAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgNewClass::OnInitDialog);
		WDialog::OnInitDialog();

		ParentLabel.SetText( *ParentClass );
		PackageEdit.SetText( TEXT("MyPackage") );
		NameEdit.SetText( *(FString::Printf(TEXT("My%s"), *ParentClass) ) );
		::SetFocus( PackageEdit.hWnd );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgNewClass::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _ParentClass )
	{
		guard(WDlgNewClass::DoModal);

		ParentClass = _ParentClass;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgNewClass::OnOk);
		if( GetDataFromUser() )
		{
			// Check if class already exists.
			//

			// Create new class.
			//
			GEditor->Exec( *(FString::Printf( TEXT("CLASS NEW NAME=%s PACKAGE=%s PARENT=%s"),
				*Name, *Package, *ParentClass)) );
			GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%s"), *Name)) );

			// Create standard header for the new class.
			//
			char ch13 = '\x0d', ch10 = '\x0a';
			FString S = FString::Printf(
				TEXT("//=============================================================================%c%c// %s.%c%c//=============================================================================%c%cclass %s expands %s;%c%c"),
				ch13, ch10,
				*Name, ch13, ch10,
				ch13, ch10,
				*Name, *ParentClass, ch13, ch10);
			GEditor->Set(TEXT("SCRIPT"), *Name, *S);

			EndDialog(TRUE);
		}
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgNewClass::GetDataFromUser);
		Package = PackageEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
				|| !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}
		else
			return TRUE;
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBrowserActor
//
// --------------------------------------------------------------

class WBrowserActor : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserActor,WBrowser,Window)

	WTreeView* pTreeView;
	WCheckBox* pObjectsCheck;
	WLabel* pPackageLabel;
	WListBox* pPackagesList;
	WButton *pSaveButton, *pClearButton;
	HTREEITEM htiRoot, htiLastSel;

	UBOOL bShowPackages;

	// Structors.
	WBrowserActor( FName InPersistentName, WWindow* InOwnerWindow )
	:	WBrowser( InPersistentName, InOwnerWindow )
	{
		pTreeView = NULL;
		pObjectsCheck = NULL;
		pPackageLabel = NULL;
		htiRoot = htiLastSel = NULL;
	}

	// WBrowser interface.
	void OpenWindow()
	{
		guard(WBrowserActor::OpenWindow);
		WBrowser::OpenWindow();
		SetText( TEXT("Actor Browser") );
		Show(1);
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserActor::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserActor) );
		
		pObjectsCheck = new WCheckBox( this, IDCK_OBJECTS );
		pObjectsCheck->ClickDelegate = FDelegate(this, (TDelegate)OnObjectsClick);
		pObjectsCheck->OpenWindow( 1, 0, 0, 1, 1, TEXT("Actor classes only") );
		SendMessageX( pObjectsCheck->hWnd, BM_SETCHECK, BST_CHECKED, 0 );

		pPackageLabel = new WLabel( this, IDSC_PACKAGE );
		pPackageLabel->OpenWindow( 1, 0 );
		pPackageLabel->SetText(TEXT(""));

		pTreeView = new WTreeView( this, IDTV_TREEVIEW );
		pTreeView->OpenWindow( 1, 1, 0, 0, 1 );
		pTreeView->SelChangedDelegate = FDelegate(this, (TDelegate)OnTreeViewSelChanged);
		pTreeView->ItemExpandingDelegate = FDelegate(this, (TDelegate)OnTreeViewItemExpanding);
		
		pSaveButton	= new WButton( this, IDPB_SAVE, FDelegate(this,(TDelegate)OnSave) );
		pSaveButton->OpenWindow( 1, 0, 0, 0, 0, TEXT("Save Pkgs") );

		pClearButton = new WButton( this, IDPB_CLEAR, FDelegate(this,(TDelegate)OnClear ) );
		pClearButton->OpenWindow( 1, 0, 0, 0, 0, TEXT("Clear Sel") );

		pPackagesList = new WListBox( this, IDLB_PACKAGES );
		pPackagesList->OpenWindow( 1, 0, 1, 0, 1 );

		PositionChildControls();

		RefreshPackages();
		RefreshActorList();
		SendMessageX( pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot );

		if(!GConfig->GetInt( *PersistentName, TEXT("ShowPackages"), bShowPackages, TEXT("UnrealEd2.ini") ))		bShowPackages = 1;

		unguard;
	}
	void RefreshPackages(void)
	{
		guard(WBrowserActor::RefreshPackages);

		// PACKAGES
		//
		FStringOutputDevice GetPropResult = FStringOutputDevice();
	    GEditor->Get(TEXT("OBJ"), TEXT("PACKAGES CLASS=Class"), GetPropResult);

		TArray<FString> PkgArray;
		ParseStringToArray( TEXT(","), GetPropResult, &PkgArray );

		pPackagesList->Empty();

		for( int x = 0 ; x < PkgArray.Num() ; x++ )
			pPackagesList->AddString( *(FString::Printf( TEXT("%s"), *PkgArray(x))) );

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserActor::OnDestroy);

		delete pTreeView;
		delete pObjectsCheck;
		delete pPackageLabel;
		delete pSaveButton;
		delete pClearButton;
		delete pPackagesList;

		GConfig->SetInt( *PersistentName, TEXT("ShowPackages"), bShowPackages, TEXT("UnrealEd2.ini") );

		WBrowser::OnDestroy();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserActor::OnCommand);
		switch( Command )
		{
			case IDMN_AB_SHOWPACKAGES:
				{
					bShowPackages = !bShowPackages;
					PositionChildControls();
				}
				break;

			case IDMN_AB_NEW_CLASS:
				{
					WDlgNewClass l_dlg( NULL, this );
					if( l_dlg.DoModal( GEditor->CurrentClass ? GEditor->CurrentClass->GetName() : TEXT("Actor") ) )
					{
						// Open an editing window.
						//
						GCodeFrame->AddClass( GEditor->CurrentClass );
						RefreshActorList();
						RefreshPackages();
					}
				}
				break;

			case IDMN_AB_DELETE:
				{
					if( GEditor->CurrentClass )
					{
						FString CurName = GEditor->CurrentClass->GetName();
						GEditor->Exec( TEXT("SETCURRENTCLASS Class=Light") );

						TCHAR l_chCmd[256];
						FStringOutputDevice GetPropResult = FStringOutputDevice();
						appSprintf( l_chCmd, TEXT("DELETE CLASS=CLASS OBJECT=%s"), *CurName );
						
						GEditor->Get( TEXT("OBJ"), l_chCmd, GetPropResult);

						if( !GetPropResult.Len() )
						{
							// Try to cleanly update the actor list.  If this fails, just reload it from scratch...
							if( !SendMessageX( pTreeView->hWnd, TVM_DELETEITEM, 0, (LPARAM)htiLastSel ) )
								RefreshActorList();

							GCodeFrame->RemoveClass( CurName );
						}
						else
							appMsgf( TEXT("Can't delete class") );
					}
				}
				break;

			case IDMN_AB_DEF_PROP:
				GEditor->Exec( *(FString::Printf(TEXT("HOOK CLASSPROPERTIES CLASS=%s"), GEditor->CurrentClass->GetName())) );
				break;

			case IDMN_AB_FileSave:
				{
					// Save all selected packages
					OnSave();
				}
				break;

			case IDMN_AB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Class Packages (*.u)\0*.u\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\system";
					ofn.lpstrDefExt = "u";
					ofn.lpstrTitle = "Open Class Package";
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

						RefreshActorList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
					RefreshPackages();
				}
				break;

			case IDMN_AB_EDIT_SCRIPT:
				{
					GCodeFrame->AddClass( GEditor->CurrentClass );
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
		guard(WBrowserActor::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WBrowserActor::PositionChildControls);

		if( !pTreeView
				|| !pObjectsCheck
				|| !pPackageLabel 
				|| !pSaveButton 
				|| !pClearButton 
				|| !pPackagesList ) return;

		FRect R = GetClientRect();
		float Fraction = (R.Width() - 8) / 10.0f;
		int Height = R.Height();

		::MoveWindow( pObjectsCheck->hWnd, 4, (R.Height() - Height), Fraction * 10, 20, 1 );		Height -= 20;
		::MoveWindow( pPackageLabel->hWnd, 4, (R.Height() - Height), Fraction * 10, 20, 1 );		Height -= 20;
		if( bShowPackages )
		{
			::MoveWindow( pTreeView->hWnd, 4, (R.Height() - Height), Fraction * 10, Height - 256, 1 );	Height -= Height - 256;
			::MoveWindow( pSaveButton->hWnd, 0, (R.Height() - Height), Fraction * 5, 20, 1);
			::MoveWindow( pClearButton->hWnd, Fraction * 5, (R.Height() - Height), Fraction * 5, 20, 1);	Height -= 20;
			::MoveWindow( pPackagesList->hWnd, 4, (R.Height() - Height), Fraction * 10, Height, 1);
		}
		else
		{
			::MoveWindow( pTreeView->hWnd, 4, (R.Height() - Height), Fraction * 10, Height - 4, 1 );
			::MoveWindow( pSaveButton->hWnd, 0, 0, 0, 0, 1);
			::MoveWindow( pClearButton->hWnd, 0, 0, 0, 0, 1);
			::MoveWindow( pPackagesList->hWnd, 0, 0, 0, 0, 1);
		}

		::InvalidateRect( hWnd, NULL, 1);

		unguard;
	}
	void RefreshActorList( void )
	{
		pTreeView->Empty();

		if( pObjectsCheck->IsChecked() )
			htiRoot = pTreeView->AddItem( TEXT("Actor"), NULL, TRUE );
		else
			htiRoot = pTreeView->AddItem( TEXT("Object"), NULL, TRUE );

		htiLastSel = NULL;
	}
	void AddChildren( const TCHAR* pParentName, HTREEITEM hti )
	{
		::Sleep(0);

		HTREEITEM newhti;
		FString String, StringQuery;

		StringQuery = FString::Printf( TEXT("Query Parent=%s"), pParentName );
		Query( GEditor->Level, *StringQuery, &String );

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(","), String, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
		{
			FString NewName = *(StringArray(x)), Children;

			Children = NewName.Left(1);
			NewName = NewName.Right( NewName.Len() - 1 );

			newhti = pTreeView->AddItem( *NewName, hti, Children == TEXT("C") ? TRUE : FALSE );
		}
	}
	void OnTreeViewSelChanged( void )
	{
		NMTREEVIEW* pnmtv = (LPNMTREEVIEW)pTreeView->LastlParam;
		TCHAR chText[128] = TEXT("\0");
		TVITEMEX tvi;

		appMemzero( &tvi, sizeof(tvi));
		htiLastSel = tvi.hItem = pnmtv->itemNew.hItem;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = chText;
		tvi.cchTextMax = sizeof(chText);

		if( SendMessageX( pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)&tvi) )
		{
			GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%s"), tvi.pszText )));
			pPackageLabel->SetText( GEditor->CurrentClass->GetFullName() );
		}
	}
	void OnTreeViewItemExpanding( void )
	{
		NMTREEVIEW* pnmtv = (LPNMTREEVIEW)pTreeView->LastlParam;
		TCHAR chText[128] = TEXT("\0");

		TVITEMEX tvi;

		appMemzero( &tvi, sizeof(tvi));
		tvi.hItem = pnmtv->itemNew.hItem;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = chText;
		tvi.cchTextMax = sizeof(chText);

		// If this item already has children loaded, bail...
		if( SendMessageX( pTreeView->hWnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)pnmtv->itemNew.hItem ) )
			return;

		if( SendMessageX( pTreeView->hWnd, TVM_GETITEM, 0, (LPARAM)&tvi) )
			AddChildren( tvi.pszText, pnmtv->itemNew.hItem );
	}
	void OnObjectsClick()
	{
		guard(WBrowserTexture::OnObjectsClick);
		RefreshActorList();
		SendMessageX( pTreeView->hWnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiRoot );
		unguard;
	}
	void OnSave()
	{
		guard(WBrowserTexture::OnSave);

		FString Pkg;

		GWarn->BeginSlowTask( TEXT("Saving Packages"), 1, 0 );

		for( int x = 0 ; x < pPackagesList->GetCount() ; x++ )
			if( pPackagesList->GetSelected( x ) )
			{
				Pkg = *(pPackagesList->GetString( x ));
				GEditor->Exec( *(FString::Printf(TEXT("OBJ SAVEPACKAGE PACKAGE=%s FILE=%s.u"), *Pkg, *Pkg )) );
			}

		GWarn->EndSlowTask();

		unguard;
	}
	void OnClear()
	{
		guard(WBrowserTexture::OnClear);
		pPackagesList->ClearSel();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif