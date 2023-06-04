#if 1 //NEW: U2Ed
#ifndef _CODEFRAME_H_
#define _CODEFRAME_H_
/*=============================================================================
	CodeFrame : This window is where all UnrealScript editing takes place
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

extern void ShowBrowserActor( WWindow* Parent );
extern void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray);

#define ID_CF_TOOLBAR	29001
TBBUTTON tbCFButtons[] = {
	{ 0, IDMN_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 1, IDMN_CF_COMPILE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_CF_COMPILE_ALL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, ID_BrowserActor, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, IDMN_CF_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, IDMN_CF_FIND_PREV, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_CF_FIND_NEXT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_CF[] = {
	TEXT("Close Script"), IDMN_CLOSE,
	TEXT("Compile Changed Scripts"), IDMN_CF_COMPILE,
	TEXT("Compile ALL Scripts"), IDMN_CF_COMPILE_ALL,
	TEXT("Actor Browser"), ID_BrowserActor,
	TEXT("Find"), IDMN_CF_FIND,
	TEXT("Find Previous"), IDMN_CF_FIND_PREV,
	TEXT("Find Next"), IDMN_CF_FIND_NEXT,
	NULL, 0
};

// A code editing window.
class WCodeFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WCodeFrame,WWindow,UnrealEd)

	// Variables.
	UClass* Class;
	WRichEdit Edit;
	WListBox FilesList;
	BOOL bFirstTime;
	RECT rcStatus;
	BOOL m_bStatusError;
	FString m_StatusText;
	HWND hWndToolBar;
	HWND hWndTT;

	TArray<UClass*> m_Classes;	// the list of classes we are editing scripts for
	UClass* pCurrentClass;

	// Constructor.
	WCodeFrame( FName InPersistentName, WWindow* InOwnerWindow )
	: WWindow( InPersistentName, InOwnerWindow )
	,	Edit		( this )
	,	FilesList	( this, IDLB_FILES )
	{
		pCurrentClass = NULL;
		bFirstTime = TRUE;
		rcStatus.top = rcStatus.bottom = rcStatus.left = rcStatus.right = 0;
	}

	// WWindow interface.
	void OnSetFocus( HWND hWndLoser )
	{
		guard(WCodeFrame::OnSetFocus);
		WWindow::OnSetFocus( hWndLoser );
		SetFocus( Edit );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WCodeFrame::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );
		PositionChildControls();
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(WCodeFrame::PositionChildControls);

		if( !::IsWindow( GetDlgItem( hWnd, ID_CF_TOOLBAR )))	return;

		FRect CR = GetClientRect();
		RECT R;
		::GetWindowRect( GetDlgItem( hWnd, ID_CF_TOOLBAR ), &R );
		::MoveWindow( GetDlgItem( hWnd, ID_CF_TOOLBAR ), 0, 0, CR.Max.X, R.bottom, TRUE );

		FilesList.MoveWindow( FRect(0,(R.bottom - R.top) - 1,128,CR.Max.Y), TRUE );

		Edit.MoveWindow( FRect(128,(R.bottom - R.top) - 1,CR.Max.X,CR.Max.Y - 20), TRUE );
		//warren Edit.ScrollCaret();

		rcStatus.left = 128;
		rcStatus.right = CR.Max.X;
		rcStatus.top = CR.Max.Y - 20;
		rcStatus.bottom = CR.Max.Y;

		::InvalidateRect( hWnd, NULL, TRUE );

		unguard;
	}
	void UpdateStatus( BOOL bError, FString Text )
	{
		guard(WCodeFrame::UpdateStatus);
		m_bStatusError = bError;
		m_StatusText = Text;
		::InvalidateRect( hWnd, NULL, TRUE );
		unguard;
	}
	void OnPaint()
	{
		guard(WCodeFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		//
		// STATUS BAR
		//

		HPEN l_penError, l_penOK, l_penOld;
		HBRUSH l_brushError, l_brushOK, l_brushOld;

		::SetBkMode( hDC, TRANSPARENT );

		l_penError = ::CreatePen( PS_SOLID, 1, RGB(255, 0, 0) );
		l_penOK = ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DFACE ) );
		l_brushError = ::CreateSolidBrush( RGB(255, 0, 0) );
		l_brushOK = ::CreateSolidBrush( ::GetSysColor( COLOR_3DFACE ) );

		if( m_bStatusError )
		{
			l_penOld = (HPEN)::SelectObject( hDC, l_penError );
			l_brushOld = (HBRUSH)::SelectObject( hDC, l_brushError );
			::SetTextColor( hDC, RGB(255, 255, 255) );
		}
		else
		{
			l_penOld = (HPEN)::SelectObject( hDC, l_penOK );
			l_brushOld = (HBRUSH)::SelectObject( hDC, l_brushOK );
			::SetTextColor( hDC, ::GetSysColor( COLOR_BTNTEXT ) );
		}

		// Draw the background
		::Rectangle( hDC, rcStatus.left, rcStatus.top, rcStatus.right, rcStatus.bottom );

		// Draw the message
		::DrawTextA( hDC, TCHAR_TO_ANSI( *m_StatusText ), ::strlen( TCHAR_TO_ANSI( *m_StatusText ) ), &rcStatus, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

		// Clean up
		::SetBkMode( hDC, OPAQUE );

		::SelectObject( hDC, l_penOld );
		::SelectObject( hDC, l_brushOld );

		EndPaint( *this, &PS );

		::DeleteObject( l_penError );
		::DeleteObject( l_penOK );
		::DeleteObject( l_brushError );
		::DeleteObject( l_brushOK );

		unguard;
	}
	// Checks for script compile errors.
	//
	void ProcessResults(void)
	{
		guard(WCodeFrame::ProcessResults);
		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("TEXT"), TEXT("RESULTS"), GetPropResult );

		FString S, T, Msg;
		long i, Line;
		BOOL l_bError = FALSE;

		S = GetPropResult;

		if( !appStrcmp( *(S.Left(9)), TEXT("Error in ") )
				&& S.InStr(TEXT(":")) > -1 )
		{
			l_bError = TRUE;

			S = S.Mid(9);
			i = S.InStr(TEXT(", Line "));
			if(i != -1)
			{
				Line = appAtoi(*(S.Mid(i + 7)));	// Line number
				S = S.Left(i);						// Class name
				HighlightError( S, Line );
			}
		}

		S = GetPropResult;
		// Sometimes there's crap on the end of the message .. strip it off.
	    if( S.InStr(TEXT("\x0d")) != -1 )
			S = S.Left( S.InStr(TEXT("\x0d")) );
        UpdateStatus( l_bError, *S);

		unguard;
	}
	// Highlights a compilation error by opening up that classes script and moving to the appropriate line.
	//
	void HighlightError( FString Name, int Line )
	{
		guard(WCodeFrame::HighlightError);

		UClass* Class;
		if( ParseObject<UClass>( *(FString::Printf(TEXT("CLASS=%s"), *Name)), TEXT("CLASS="), Class, ANY_PACKAGE ) )
		{
			// Figure out where in the script the error line is, in chars.
			//
			char ch10 = '\x0a', *pch = TCHAR_TO_ANSI( *(Class->ScriptText->Text) );
			int iChar = 0, iLine = 1;

			while( *pch && iLine < Line )
			{
				if( *pch == ch10 )
					iLine++;

				iChar++;
				pch++;
			}

			AddClass( Class, iChar, Line - 1 );
		}
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WCodeFrame::OnCommand);
		switch( Command ) {

			case IDMN_CF_FIND:
				{
					SendMessageX( OwnerWindow->hWnd, WM_COMMAND, WM_SHOW_FINDREPLACE, 0L );
				}
				break;

			case IDMN_CF_FIND_NEXT:
				{
					SendMessageX( OwnerWindow->hWnd, WM_COMMAND, WM_FINDREPLACE_NEXT, 0L );
				}
				break;

			case IDMN_CF_FIND_PREV:
				{
					SendMessageX( OwnerWindow->hWnd, WM_COMMAND, WM_FINDREPLACE_PREV, 0L );
				}
				break;

			case IDMN_CF_EXPORT_CHANGED:
				{
					if( ::MessageBox( hWnd, TEXT("This option will export all modified classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
					{
						GEditor->Exec( TEXT("CLASS SPEW") );
					}
				}
				break;

			case IDMN_CF_EXPORT_ALL:
				{
					if( ::MessageBox( hWnd, TEXT("This option will export all classes to text .uc files which can later be rebuilt. Do you want to do this?"), TEXT("Export classes to *.uc files"), MB_YESNO) == IDYES)
					{
						GEditor->Exec( TEXT("CLASS SPEW ALL") );
					}
				}
				break;

			case IDMN_CF_COMPILE:
				{
					GWarn->BeginSlowTask( TEXT("Compiling changed scripts"), 1, 0 );
					Save();
					GEditor->Exec( TEXT("SCRIPT MAKE") );
					GWarn->EndSlowTask();

					ProcessResults();
				}
				break;

			case IDMN_CF_COMPILE_ALL:
				{
					GWarn->BeginSlowTask( TEXT("Compiling all scripts"), 1, 0 );
					Save();
					GEditor->Exec( TEXT("SCRIPT MAKE ALL") );
					GWarn->EndSlowTask();

					ProcessResults();
				}
				break;

			case IDMN_CLOSE:
				{
					Save();

					// Find the currently selected class and remove it from the list.
					//
					FString Name = FilesList.GetString( FilesList.GetCurrent() );
					RemoveClass( Name );
				}
				break;

			case ID_BrowserActor:
			{
				ShowBrowserActor( this );
			}
			break;

			default:
				WWindow::OnCommand(Command);
				break;
		}
		unguard;
	}
	void OnCreate()
	{
		guard(WCodeFrame::OnCreate);
		WWindow::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_CodeFrame) );

		// Load windows last position.
		//
		int X, Y, W, H;

		if(!GConfig->GetInt( *PersistentName, TEXT("X"), X, TEXT("UnrealEd2.ini") ))	X = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("Y"), Y, TEXT("UnrealEd2.ini") ))	Y = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("W"), W, TEXT("UnrealEd2.ini") ))	W = 512;
		if(!GConfig->GetInt( *PersistentName, TEXT("H"), H, TEXT("UnrealEd2.ini") ))	H = 384;

		if( !W ) W = 320;
		if( !H ) H = 200;

		::MoveWindow( hWnd, X, Y, W, H, TRUE );

		// Set up the main edit control.
		//
		Edit.OpenWindow(1,1,0,1,1);
		UINT Tabs[16];
		for( INT i=0; i<16; i++ )
			Tabs[i]=5*4*(i+1);
		SendMessageX( Edit.hWnd, EM_SETTABSTOPS, 16, (LPARAM)Tabs );
		Edit.SetFont( (HFONT)GetStockObject(ANSI_FIXED_FONT) );
		SendMessageX( Edit.hWnd, EM_EXLIMITTEXT, 0, 262144 );	// 512K of text
		SendMessageX( Edit.hWnd, EM_SETTEXTMODE, 0, TM_RICHTEXT | TM_MULTILEVELUNDO );	// 1MB of text & multiple levels of undo
		SendMessageX( Edit.hWnd, EM_SETBKGNDCOLOR, 0, (LPARAM)(COLORREF)RGB(0,0,64) );

		// warrenEdit.SetReadOnly( TRUE );
		Edit.SetText(TEXT("No scripts loaded."));

		hWndToolBar = CreateToolbarEx( 
			hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
			ID_CF_TOOLBAR,
			8,
			hInstance,
			IDB_CF_TOOLBAR,
			(LPCTBBUTTON)&tbCFButtons,
			10,
			16,16,
			16,16,
			sizeof(TBBUTTON));

		if( !hWndToolBar )
			appMsgf( TEXT("Toolbar not created!") );


		// TOOLTIPS
		hWndTT = CreateWindowA(TOOLTIPS_CLASSA, NULL, TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, (HMENU) NULL, hInstance, NULL);

		if( !hWndTT )
			appMsgf( TEXT("Tooltip control not created!") );
		else
		{
			int ID = 900;
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = hWndToolBar;
			ti.hinst = hInstance;

			for( int tooltip = 0 ; ToolTips_CF[tooltip].ID > 0 ; tooltip++ )
			{
				// Figure out the rectangle for the toolbar button.
				int index = SendMessageX( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_CF[tooltip].ID, 0 );
				RECT rect;
				SendMessageX( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

				// Add tooltip to tooltip control
				ti.uId = ID + tooltip;
				ti.lpszText = ToolTips_CF[tooltip].ToolTip;
				ti.rect = rect;

				SendMessageX(hWndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&ti);
			}
		}

		FilesList.OpenWindow( 1, 0, 0, 0, 1 );
		FilesList.DoubleClickDelegate = FDelegate(this, (TDelegate)OnFilesListDblClick);

		UpdateStatus( FALSE, TEXT("Ready."));

		unguard;
	}
	CORE_API TCHAR* LEGEND_ANSI_TO_TCHAR(char* str)
	{
		int iLength = winGetSizeUNICODE(str);
		TCHAR* pBuffer = new TCHAR[iLength];
		appStrcpy(pBuffer,TEXT(""));
		TCHAR* ret = winToUNICODE(pBuffer,str,iLength);
		return ret;
	}
	void Save(void)
	{
		guard(WCodeFrame::Save);

		if( !pCurrentClass )	return;
		if( !m_Classes.Num() )	return;

		int iLength = SendMessageA( Edit.hWnd, WM_GETTEXTLENGTH, 0, 0 );
		char* pchBuffer = new char[iLength];
		::strcpy(pchBuffer, "");
		const TCHAR* ptchBuffer;
		Edit.StreamTextOut( pchBuffer, iLength );

		ptchBuffer = LEGEND_ANSI_TO_TCHAR(pchBuffer);

		pCurrentClass->ScriptText->Text = ptchBuffer;
		SendMessageX( Edit.hWnd, EM_GETSEL, (WPARAM)&(pCurrentClass->ScriptText->Pos), 0 );
		pCurrentClass->ScriptText->Top = SendMessageX( Edit.hWnd, EM_GETFIRSTVISIBLELINE, 0, 0 );

		delete [] pchBuffer;

		unguard;
	}
	void OnDestroy()
	{
		guard(WCodeFrame::OnDestroy);

		Save();

		// Save Window position (base class doesn't always do this properly)
		// (Don't do this if the window is minimized.)
		//
		if( !::IsIconic( hWnd ) )
		{
			RECT R;
			::GetWindowRect(hWnd, &R);

			GConfig->SetInt( *PersistentName, TEXT("Active"), m_bShow, TEXT("UnrealEd2.ini") );
			GConfig->SetInt( *PersistentName, TEXT("X"), R.left, TEXT("UnrealEd2.ini") );
			GConfig->SetInt( *PersistentName, TEXT("Y"), R.top, TEXT("UnrealEd2.ini") );
			GConfig->SetInt( *PersistentName, TEXT("W"), R.right - R.left, TEXT("UnrealEd2.ini") );
			GConfig->SetInt( *PersistentName, TEXT("H"), R.bottom - R.top, TEXT("UnrealEd2.ini") );
		}

		::DestroyWindow( hWndToolBar );
		::DestroyWindow( hWndTT );

		WWindow::OnDestroy();
		unguard;
	}
	void OpenWindow( UBOOL bMdi=0, UBOOL AppWindow=0 )
	{
		guard(WCodeFrame::OpenWindow);
		MdiChild = bMdi;
		PerformCreateWindowEx
		(
			WS_EX_WINDOWEDGE,
			TEXT("Script Editor"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			384,
			512,
			OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		unguard;
	}
	void AddClass( UClass* pClass, int Pos = -1, int Top = -1 )
	{
		guard(WCodeFrame::AddClass);
		if( !pClass ) return;
		
		// Make sure this class has a script.
		//
		FStringOutputDevice GetPropResult = FStringOutputDevice();
	    GEditor->Get(TEXT("SCRIPTPOS"), pClass->GetName(), GetPropResult);
		if( !GetPropResult.Len() )
		{
			appMsgf( *(FString::Printf(TEXT("'%s' has no script to edit."), pClass->GetName())) );
			return;
		}

		// Only add this class to the list if it's not already there.
		//
		if( FilesList.FindString( pClass->GetName() ) == -1 )
			m_Classes.AddItem( pClass );

		RefreshScripts();
		SetClass( pClass->GetName(), Pos, Top );

		Show(1);
		::BringWindowToTop( hWnd );
		unguard;
	}
	void RemoveClass( FString Name )
	{
		guard(WCodeFrame::RemoveClass);

		// Remove the class from the internal list.
		//
		for( int x = 0 ; x < m_Classes.Num() ; x++ )
		{
			if( !appStrcmp( m_Classes(x)->GetName(), *Name ) )
			{
				m_Classes.Remove(x);
				break;
			}
		}

		RefreshScripts();
		FilesList.SetCurrent( 0, 1 );
		OnFilesListDblClick();

		if( !m_Classes.Num() )
			pCurrentClass = NULL;
		SetCaption();

		unguard;
	}
	void RefreshScripts(void)
	{
		guard(WCodeFrame::RefreshScripts);

		// LOADED SCRIPTS
		//
		FilesList.Empty();
		for( int x = 0 ; x < m_Classes.Num() ; x++ )
			FilesList.AddString( m_Classes(x)->GetName() );

		unguard;
	}
	// Saves the current script and selects a new script.
	//
	void SetClass( FString Name, int Pos = -1, int Top = -1 )
	{
		guard(WCodeFrame::SetClass);

		// If there are no classes loaded, just empty the edit control.
		//
		if( !m_Classes.Num() )
		{
			// warrenEdit.SetReadOnly( TRUE );
			Edit.SetText(TEXT("No scripts loaded."));
			pCurrentClass = NULL;
			return;
		}

		// Save the settings/script for the current class before changing.
		//
		if( pCurrentClass && !bFirstTime )
		{
			Save();
		}

		bFirstTime = FALSE;

		FilesList.SetCurrent( FilesList.FindString( *Name ), 1 );

		// warrenEdit.SetReadOnly( FALSE );

		// Locate the proper class pointer.
		//
		for( int x = 0 ; x < m_Classes.Num() ; x++ )
			if( !appStrcmp( m_Classes(x)->GetName(), *Name ) )
			{
				pCurrentClass = m_Classes(x);
				break;
			}

		// Override whatever is in the class if we need to.
		//
		if( Pos > -1 )		pCurrentClass->ScriptText->Pos = Pos;
		if( Top > -1 )		pCurrentClass->ScriptText->Top = Top;

		// Load current script into edit window.
		//
		SetCaption();

		// old code
		//Edit.SetText( *(pCurrentClass->ScriptText->Text) );

		// Get the script text in RTF format
		FStringOutputDevice GetPropResult = FStringOutputDevice();
		GEditor->Get( TEXT("RTF"), pCurrentClass->GetName(), GetPropResult );

		// Convert it to ANSI
		TCHAR* tchScriptText = new TCHAR[GetPropResult.Len()];
		appStrcpy( tchScriptText, TEXT("") ); 

		appStrcpy( tchScriptText, *GetPropResult ); 
		const char* chScriptText = TCHAR_TO_ANSI(tchScriptText);
		
		// Stream it into the RichEdit control
		LockWindowUpdate(Edit.hWnd);
		Edit.StreamTextIn( (char*)chScriptText, strlen(chScriptText) );
		LockWindowUpdate(NULL);

		delete [] tchScriptText;

		SendMessageX( Edit.hWnd, EM_SETSEL, pCurrentClass->ScriptText->Pos, pCurrentClass->ScriptText->Pos );
		SendMessageX( Edit.hWnd, EM_SCROLLCARET, 0, 0 );

		ScrollToLine( pCurrentClass->ScriptText->Top );

		::SetFocus( Edit.hWnd );
		unguard;
	}
	void SetCaption()
	{
		if( pCurrentClass )
			SetText( pCurrentClass->GetFullName() );
		else
			SetText( TEXT("") );
	}
	void ScrollToLine( int Line )
	{
		guard(WCodeFrame::ScrollToLine);

		// Stop the window from updating while scrolling to the requested line.  This makes
		// it go MUCH faster -- and it looks better.
		//
		LockWindowUpdate( hWnd );

		int CurTop = SendMessageX(Edit.hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
		while( CurTop > Line )
		{
			SendMessageX(Edit.hWnd, EM_SCROLL, SB_LINEUP, 0);
			CurTop--;
		}
		while( CurTop < Line )
		{
			SendMessageX(Edit.hWnd, EM_SCROLL, SB_LINEDOWN, 0);
			CurTop++;
		}

		LockWindowUpdate( NULL );
		unguard;
	}
	int OnSysCommand( INT Command )
	{
		guard(WCodeFrame::OnSysCommand);
		// Don't actually close the window when the user hits the "X" button.  Just hide it.
		if( Command == SC_CLOSE )
		{
			Show(0);
			return 1;
		}

		return 0;
		unguard;
	}

	// Notification delegates for child controls.
	//
	void OnFilesListDblClick()
	{
		guard(WCodeFrame::OnFilesListDblClick);
		FString Name = FilesList.GetString( FilesList.GetCurrent() );
		SetClass( Name );
		SetCaption();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif //_CODEFRAME_H_
#endif