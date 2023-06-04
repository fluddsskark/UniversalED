#if 1 //NEW: U2Ed
/*=============================================================================
	BrowserSound : Browser window for sound effects
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>
extern FString GLastDir[eLASTDIR_MAX];

// --------------------------------------------------------------
//
// IMPORT SOUND Dialog
//
// --------------------------------------------------------------

class WDlgImportSound : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportSound,WDialog,UnrealEd)

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
	int iCurrentFilename;

	// Constructor.
	WDlgImportSound( UObject* InContext, WBrowser* InOwnerWindow )
	:	WDialog			( TEXT("Import Sound"), IDDIALOG_IMPORT_SOUND, InOwnerWindow )
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
		guard(WDlgImportSound::OnInitDialog);
		WDialog::OnInitDialog();

		PackageEdit.SetText( *defPackage );
		GroupEdit.SetText( *defGroup );
		::SetFocus( NameEdit.hWnd );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgImportSound::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( FString _defPackage, FString _defGroup, TArray<FString>* _paFilenames)
	{
		guard(WDlgImportSound::DoModal);

		defPackage = _defPackage;
		defGroup = _defGroup;
		paFilenames = _paFilenames;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportSound::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportSound::OnOkAll);
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
		guard(WDlgImportSound::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void ImportTexture( void )
	{
		guard(WDlgImportSound::ImportTexture);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportSound::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = GetFilenameOnly( (*paFilenames)(iCurrentFilename) );
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportSound::GetDataFromUser);
		Package = PackageEdit.GetText();
		Group = GroupEdit.GetText();
		Name = NameEdit.GetText();

		if( !Package.Len()
				|| !Group.Len()
				|| !Name.Len() )
		{
			appMsgf( TEXT("Invalid input.") );
			return FALSE;
		}
		else
			return TRUE;
		unguard;
	}
	void SetNextFilename( void )
	{
		guard(WDlgImportSound::SetNextFilename);
		iCurrentFilename++;
		if( iCurrentFilename == paFilenames->Num() ) {
			EndDialogTrue();
			return;
		}

		if( bOKToAll ) {
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
		guard(WDlgImportSound::ImportFile);
		TCHAR l_chCmd[512];

		appSprintf( l_chCmd, TEXT("AUDIO IMPORT FILE=%s NAME=%s PACKAGE=%s GROUP=%s"),
			*(*paFilenames)(iCurrentFilename), *Name, *Package, *Group );
		GEditor->Exec( l_chCmd );
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBROWSERSOUND
//
// --------------------------------------------------------------

class WBrowserSound : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserSound,WBrowser,Window)

	WComboBox *pComboPackage, *pComboGroup;
	WListBox *pListSounds;
	WButton *pButtonPlay, *pButtonStop;
	WCheckBox *pCheckGroupAll;

	// Structors.
	WBrowserSound( FName InPersistentName, WWindow* InOwnerWindow )
	:	WBrowser( InPersistentName, InOwnerWindow )
	{
		pComboPackage = pComboGroup = NULL;
		pListSounds = NULL;
		pButtonPlay = pButtonStop = NULL;
		pCheckGroupAll = NULL;
	}

	// WBrowser interface.
	void OpenWindow()
	{
		guard(WBrowserSound::OpenWindow);
		WBrowser::OpenWindow();
		SetText( TEXT("Sound Browser") );
		Show(1);
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserSound::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserSound) );

		// PACKAGES
		//
		pComboPackage = new WComboBox( this, IDCB_PACKAGE );
		pComboPackage->OpenWindow( 1, 1 );
		pComboPackage->SelectionChangeDelegate = FDelegate(this, (TDelegate)OnComboPackageSelChange);

		// GROUP
		//
		pComboGroup = new WComboBox( this, IDCB_GROUP );
		pComboGroup->OpenWindow( 1, 1 );
		pComboGroup->SelectionChangeDelegate = FDelegate(this, (TDelegate)OnComboGroupSelChange);

		// SOUND LIST
		//
		pListSounds = new WListBox( this, IDLB_SOUNDS );
		pListSounds->OpenWindow( 1, 1, 0, 0, 1 );
		pListSounds->DoubleClickDelegate = FDelegate(this, (TDelegate)OnListSoundsDblClick);

		// BUTTONS
		//
		pButtonPlay = new WButton( this, IDPB_PLAY, FDelegate(this, (TDelegate)OnPlayClick) );
		pButtonPlay->OpenWindow( 1, 0, 0, 1, 1, TEXT("&Play") );

		pButtonStop = new WButton( this, IDPB_STOP, FDelegate(this, (TDelegate)OnStopClick) );
		pButtonStop->OpenWindow( 1, 0, 0, 1, 1, TEXT("&Stop") );

		// CHECK BOXES
		//
		pCheckGroupAll = new WCheckBox( this, IDCK_GRP_ALL, FDelegate(this, (TDelegate)OnGroupAllClick) );
		pCheckGroupAll->OpenWindow( 1, 0, 0, 1, 1, TEXT("All") );

		RefreshPackages();
		RefreshGroups();
		RefreshSoundList();
		PositionChildControls();

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserSound::OnDestroy);

		delete pComboPackage;
		delete pComboGroup;
		delete pListSounds;
		delete pButtonPlay;
		delete pButtonStop;
		delete pCheckGroupAll;

		WBrowser::OnDestroy();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserSound::OnCommand);
		switch( Command ) {

			case IDMN_SB_DELETE:
				{
					FString Name = pListSounds->GetString( pListSounds->GetCurrent() );
					FStringOutputDevice GetPropResult = FStringOutputDevice();
					TCHAR l_chCmd[256];

					appSprintf( l_chCmd, TEXT("DELETE CLASS=SOUND OBJECT=%s"), *Name);
				    GEditor->Get( TEXT("Obj"), l_chCmd, GetPropResult);

					if( !GetPropResult.Len() )
						RefreshSoundList();
					else
						appMsgf( TEXT("Can't delete sound") );
				}
				break;

			case IDMN_SB_EXPORT_WAV:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "WAV Files (*.wav)\0*.wav\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "wav";
					ofn.lpstrTitle = "Export Sound";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_WAV]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

					// Display the Open dialog box. 
					//
					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[512];
						FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
						FString Name = pListSounds->GetString( pListSounds->GetCurrent() );

						appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=SOUND PACKAGE=%s NAME=%s FILE=%s"),
							*Package, *Name, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_WAV] = S.Left( S.InStr( TEXT("\\"), 1 ) );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_SB_IMPORT_WAV:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "WAV Files (*.wav)\0*.wav\0All Files\0*.*\0\0";
					ofn.lpstrDefExt = "wav";
					ofn.lpstrTitle = "Import Sounds";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_WAV]) );
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					// Display the Open dialog box. 
					//
					if( GetOpenFileNameA(&ofn) )
					{
						int iNULLs = FormatFilenames( File );
						FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
						FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );
		
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

						TArray<FString> FilenamesArray;

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							FString NewString;

							NewString = FString::Printf( TEXT("%s%s"), *Prefix, *(StringArray(x)) );
							new(FilenamesArray)FString( NewString );
						
							FString S = NewString;
							GLastDir[eLASTDIR_WAV] = S.Left( S.InStr( TEXT("\\"), 1 ) );
						}

						WDlgImportSound l_dlg( NULL, this );
						l_dlg.DoModal( Package, Group, &FilenamesArray );

						RefreshPackages();
						RefreshGroups();
						RefreshSoundList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_SB_FileSave:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Sound Packages (*.uax)\0*.uax\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UAX]) );
					ofn.lpstrDefExt = "uax";
					ofn.lpstrTitle = "Save Sound Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[256];
						FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

						appSprintf( l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=%s FILE=%s"),
							*Package, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_UAX] = S.Left( S.InStr( TEXT("\\"), 1 ) );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_SB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Sound Packages (*.uax)\0*.uax\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UAX]) );
					ofn.lpstrDefExt = "uax";
					ofn.lpstrTitle = "Open Sound Package";
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
							appSprintf( l_chCmd, TEXT("OBJ LOAD FILE=%s%s"), *Prefix, *(StringArray(x)) );
							GEditor->Exec( l_chCmd );
							LastNameLoaded = *(StringArray(x));
								if( StringArray.Num() == 1 )
								LastNameLoaded = LastNameLoaded.Right( LastNameLoaded.Len() - (LastNameLoaded.Left( LastNameLoaded.InStr(TEXT("\\"), 1)).Len() + 1 ));
							LastNameLoaded = LastNameLoaded.Left( LastNameLoaded.InStr(TEXT(".")) );

							FString S = *(StringArray(x));
							GLastDir[eLASTDIR_UAX] = S.Left( S.InStr( TEXT("\\"), 1 ) );
						}

						RefreshPackages();
						RefreshGroups();
						RefreshSoundList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
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
		guard(WBrowserSound::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	
	void RefreshPackages( void )
	{
		guard(WBrowserSound::RefreshPackages);

		// PACKAGES
		//
		pComboPackage->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("OBJ"), TEXT("PACKAGES CLASS=Sound"), GetPropResult );

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(","), *GetPropResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
		{
			pComboPackage->AddString( *(StringArray(x)) );
		}

		if( LastNameLoaded.Len() )
			pComboPackage->SetCurrent( pComboPackage->FindString( *LastNameLoaded ) );
		else
			pComboPackage->SetCurrent( 0 );
		LastNameLoaded = TEXT("");
		unguard;
	}
	void RefreshGroups( void )
	{
		guard(WBrowserSound::RefreshGroups);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );

		// GROUPS
		//
		pComboGroup->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		TCHAR l_ch[256];
		appSprintf( l_ch, TEXT("GROUPS CLASS=Sound PACKAGE=%s"), *Package );
		GEditor->Get( TEXT("OBJ"), l_ch, GetPropResult );

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(","), *GetPropResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
		{
			pComboGroup->AddString( *(StringArray(x)) );
		}

		pComboGroup->SetCurrent( 0 );

		unguard;
	}
	void RefreshSoundList( void )
	{
		guard(WBrowserSound::RefreshSoundList);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );

		// SOUNDS
		//
		pListSounds->Empty();

		FStringOutputDevice GetPropResult = FStringOutputDevice();
		TCHAR l_ch[256];

		if( pCheckGroupAll->IsChecked() )
			appSprintf( l_ch, TEXT("QUERY TYPE=Sound PACKAGE=%s"), *Package );
		else
			appSprintf( l_ch, TEXT("QUERY TYPE=Sound PACKAGE=%s GROUP=%s"), *Package, *Group );

		GEditor->Get( TEXT("OBJ"), l_ch, GetPropResult );

		TArray<FString> StringArray;
		ParseStringToArray( TEXT(" "), *GetPropResult, &StringArray );

		for( int x = 0 ; x < StringArray.Num() ; x++ )
		{
			pListSounds->AddString( *(StringArray(x)) );
		}

		pListSounds->SetCurrent( 0, 1 );

		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserSound::PositionChildControls);

		if( !pComboPackage 
			|| !pComboGroup
			|| !pListSounds
			|| !pButtonPlay
			|| !pButtonStop 
			|| !pCheckGroupAll ) return;

		FRect R = GetClientRect();

		float Fraction = (R.Width() - 8) / 10.0f;

		::MoveWindow( pCheckGroupAll->hWnd, 4, 26, Fraction, 20, 1 );
		::MoveWindow( pComboPackage->hWnd, 4, 4, Fraction * 8, 20, 1 );
		::MoveWindow( pComboGroup->hWnd, 4 + (Fraction), 26, Fraction * 7, 20, 1 );
		::MoveWindow( pButtonPlay->hWnd, 4 + (Fraction * 8), 4, Fraction * 2, 20, 1 );
		::MoveWindow( pButtonStop->hWnd, 4 + (Fraction * 8), 26, Fraction * 2, 20, 1 );
		::MoveWindow( pListSounds->hWnd, 4, 52, R.Width(), R.Height() - 54, 1 );

		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnComboPackageSelChange()
	{
		guard(WBrowserSound::OnComboPackageSelChange);
		RefreshGroups();
		RefreshSoundList();
		unguard;
	}
	void OnComboGroupSelChange()
	{
		guard(WBrowserSound::OnComboGroupSelChange);
		RefreshSoundList();
		unguard;
	}
	void OnListSoundsDblClick()
	{
		guard(WBrowserSound::OnListSoundsDblClick);
		OnPlayClick();
		unguard;
	}
	void OnPlayClick()
	{
		guard(WBrowserSound::OnPlayClick);

		TCHAR l_chCmd[256];
		FString Name = pListSounds->GetString( pListSounds->GetCurrent() );
		appSprintf( l_chCmd, TEXT("AUDIO PLAY NAME=%s"), *Name );
		GEditor->Exec( l_chCmd );
		unguard;
	}
	void OnStopClick()
	{
		guard(WBrowserSound::OnStopClick);
		GEditor->Exec( TEXT("AUDIO PLAY NAME=None") );
		unguard;
	}
	void OnGroupAllClick()
	{
		guard(WBrowserSound::OnGroupAllClick);
		EnableWindow( pComboGroup->hWnd, !pCheckGroupAll->IsChecked() );
		RefreshSoundList();
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowserSound::GetCurrentPathName);

		FString Package = pComboPackage->GetString( pComboPackage->GetCurrent() );
		FString Group = pComboGroup->GetString( pComboGroup->GetCurrent() );
		FString Name = pListSounds->GetString( pListSounds->GetCurrent() );

		if( Group.Len() )
			return *(FString::Printf(TEXT("%s.%s.%s"), *Package, *Group, *Name ));
		else
			return *(FString::Printf(TEXT("%s.%s"), *Package, *Name ));

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif