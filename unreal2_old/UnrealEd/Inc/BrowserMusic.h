#if 1 //NEW: U2Ed
/*=============================================================================
	BrowserMusic : Browser window for music files
	Copyright 2000 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

// --------------------------------------------------------------
//
// IMPORT MUSIC Dialog
//
// --------------------------------------------------------------

class WDlgImportMusic : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgImportMusic,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton OkAllButton;
	WButton SkipButton;
	WButton CancelButton;
	WLabel FilenameStatic;
	WEdit NameEdit;

	TArray<FString>* paFilenames;

	FString Name;
	BOOL bOKToAll;
	int iCurrentFilename;

	// Constructor.
	WDlgImportMusic( UObject* InContext, WBrowser* InOwnerWindow )
	:	WDialog			( TEXT("Import Music"), IDDIALOG_IMPORT_MUSIC, InOwnerWindow )
	,	OkButton		( this, IDOK,			FDelegate(this,(TDelegate)OnOk) )
	,	OkAllButton		( this, IDPB_OKALL,		FDelegate(this,(TDelegate)OnOkAll) )
	,	SkipButton		( this, IDPB_SKIP,		FDelegate(this,(TDelegate)OnSkip) )
	,	CancelButton	( this, IDCANCEL,		FDelegate(this,(TDelegate)EndDialogFalse) )
	,	NameEdit		( this, IDEC_NAME )
	,	FilenameStatic	( this, IDSC_FILENAME )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgImportMusic::OnInitDialog);
		WDialog::OnInitDialog();

		::SetFocus( NameEdit.hWnd );

		bOKToAll = FALSE;
		iCurrentFilename = -1;
		SetNextFilename();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgImportMusic::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal( TArray<FString>* _paFilenames)
	{
		guard(WDlgImportMusic::DoModal);

		paFilenames = _paFilenames;

		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgImportMusic::OnOk);
		if( GetDataFromUser() )
		{
			ImportFile( (*paFilenames)(iCurrentFilename) );
			SetNextFilename();
		}
		unguard;
	}
	void OnOkAll()
	{
		guard(WDlgImportMusic::OnOkAll);
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
		guard(WDlgImportMusic::OnSkip);
		if( GetDataFromUser() )
			SetNextFilename();
		unguard;
	}
	void ImportTexture( void )
	{
		guard(WDlgImportMusic::ImportTexture);
		unguard;
	}
	void RefreshName( void )
	{
		guard(WDlgImportMusic::RefreshName);
		FilenameStatic.SetText( *(*paFilenames)(iCurrentFilename) );

		FString Name = GetFilenameOnly( (*paFilenames)(iCurrentFilename) );
		NameEdit.SetText( *Name );
		unguard;
	}
	BOOL GetDataFromUser( void )
	{
		guard(WDlgImportMusic::GetDataFromUser);
		Name = NameEdit.GetText();

		if( !Name.Len() )
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
		guard(WDlgImportMusic::SetNextFilename);
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
		guard(WDlgImportMusic::ImportFile);
		TCHAR l_chCmd[512];

		appSprintf( l_chCmd, TEXT("OBJ IMPORT STANDALONE TYPE=MUSIC FILE=%s NAME=%s PACKAGE=%s"),
			*Filename, *Name, *Name );
		GEditor->Exec( l_chCmd );
		unguard;
	}
};

// --------------------------------------------------------------
//
// WBrowserMusic
//
// --------------------------------------------------------------

class WBrowserMusic : public WBrowser
{
	DECLARE_WINDOWCLASS(WBrowserMusic,WBrowser,Window)

	WListBox *pListMusic;
	WButton *pButtonPlay, *pButtonStop;

	// Structors.
	WBrowserMusic( FName InPersistentName, WWindow* InOwnerWindow )
	:	WBrowser( InPersistentName, InOwnerWindow )
	{
		pListMusic = NULL;
		pButtonPlay = pButtonStop = NULL;
	}

	// WBrowser interface.
	void OpenWindow()
	{
		guard(WBrowserMusic::OpenWindow);
		WBrowser::OpenWindow();
		SetText( TEXT("Music Browser") );
		Show(1);
		unguard;
	}
	void OnCreate()
	{
		guard(WBrowserMusic::OnCreate);
		WBrowser::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_BrowserMusic) );

		// MUSIC LIST
		//
		pListMusic = new WListBox( this, IDLB_MUSIC );
		pListMusic->OpenWindow( 1, 0, 0, 0, 1 );
		pListMusic->DoubleClickDelegate = FDelegate(this, (TDelegate)OnListMusicDblClick);

		// BUTTONS
		//
		pButtonPlay = new WButton( this, IDPB_PLAY, FDelegate(this, (TDelegate)OnPlayClick) );
		pButtonPlay->OpenWindow( 1, 0, 0, 1, 1, TEXT("&Play") );

		pButtonStop = new WButton( this, IDPB_STOP, FDelegate(this, (TDelegate)OnStopClick) );
		pButtonStop->OpenWindow( 1, 0, 0, 1, 1, TEXT("&Stop") );

		RefreshMusicList();
		PositionChildControls();

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowserMusic::OnDestroy);

		delete pListMusic;
		delete pButtonPlay;
		delete pButtonStop;

		WBrowser::OnDestroy();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WBrowserMusic::OnCommand);
		switch( Command ) {

			case IDMN_MB_EXPORT:
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
					ofn.lpstrTitle = "Export Music";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

					// Display the Open dialog box. 
					//
					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[512];
						FString Name = pListMusic->GetString( pListMusic->GetCurrent() );

						appSprintf( l_chCmd, TEXT("OBJ EXPORT TYPE=MUSIC PACKAGE=%s NAME=%s FILE=%s"),
							*Name, *Name, appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_MB_IMPORT:
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
					ofn.lpstrTitle = "Import Music";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

					// Display the Open dialog box. 
					//
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

						TArray<FString> FilenamesArray;

						for( int x = iStart ; x < StringArray.Num() ; x++ )
						{
							FString NewString;

							NewString = FString::Printf( TEXT("%s%s"), *Prefix, *(StringArray(x)) );
							new(FilenamesArray)FString( NewString );
						}

						WDlgImportMusic l_dlg( NULL, this );
						l_dlg.DoModal( &FilenamesArray );

						RefreshMusicList();
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_MB_FileSave:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Music Packages (*.umx)\0*.umx\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\music";
					ofn.lpstrDefExt = "umx";
					ofn.lpstrTitle = "Save Music Package";
					ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

					if( GetSaveFileNameA(&ofn) )
					{
						TCHAR l_chCmd[256];
						appSprintf( l_chCmd, TEXT("OBJ SAVEPACKAGE PACKAGE=%s FILE=%s"),
							*(pListMusic->GetString( pListMusic->GetCurrent())), appFromAnsi( File ) );
						GEditor->Exec( l_chCmd );
					}

					GFileManager->SetDefaultDirectory(appBaseDir());
				}
				break;

			case IDMN_MB_FileOpen:
				{
					OPENFILENAMEA ofn;
					char File[256] = "\0";

					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = File;
					ofn.nMaxFile = sizeof(char) * 256;
					ofn.lpstrFilter = "Music Packages (*.umx)\0*.umx\0All Files\0*.*\0\0";
					ofn.lpstrInitialDir = "..\\music";
					ofn.lpstrDefExt = "umx";
					ofn.lpstrTitle = "Open Music Package";
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
						}

						RefreshMusicList();
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
		guard(WBrowserMusic::OnSize);
		WBrowser::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void RefreshMusicList( void )
	{
		guard(WBrowserMusic::RefreshMusicList);

		// MUSIC
		//
		pListMusic->Empty();
		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("OBJ"), TEXT("QUERY TYPE=MUSIC"), GetPropResult );

		//appMsgf(*GetPropResult);
		//pListMusic->AddString( *(StringArray(x)) );

		pListMusic->SetCurrent( 0, 1 );

		unguard;
	}
	// Moves the child windows around so that they best match the window size.
	//
	void PositionChildControls( void )
	{
		guard(WBrowserMusic::PositionChildControls);

		if( !pListMusic
			|| !pButtonPlay
			|| !pButtonStop ) return;

		FRect R = GetClientRect();

		float Fraction = (R.Width() - 8) / 10.0f;

		::MoveWindow( pListMusic->hWnd, 4, 4, R.Width() - 8, R.Height() - 20, 1 );
		::MoveWindow( pButtonPlay->hWnd, 4, R.Height() - 20, Fraction * 5, 20, 1 );
		::MoveWindow( pButtonStop->hWnd, 4 + (Fraction * 5), R.Height() - 20, Fraction * 5, 20, 1 );

		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnListMusicDblClick()
	{
		guard(WBrowserMusic::OnListMusicDblClick);
		OnPlayClick();
		unguard;
	}
	void OnPlayClick()
	{
		guard(WBrowserMusic::OnPlayClick);

		TCHAR l_chCmd[256];
		FString Name = pListMusic->GetString( pListMusic->GetCurrent() );
		appSprintf( l_chCmd, TEXT("MUSIC PLAY NAME=%s"), *Name );
		GEditor->Exec( l_chCmd );
		unguard;
	}
	void OnStopClick()
	{
		guard(WBrowserMusic::OnStopClick);
		GEditor->Exec( TEXT("MUSIC PLAY NAME=None") );
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowserMusic::GetCurrentPathName);
		return *(pListMusic->GetString( pListMusic->GetCurrent() ));
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif