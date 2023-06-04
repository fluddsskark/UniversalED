
#if 1 //NEW: U2Ed
/*=============================================================================
	Browser : Base class for browser windows
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

int CDECL ClassSortCompare( const void *elem1, const void *elem2 )
{
	return appStricmp((*(UClass**)elem1)->GetName(),(*(UClass**)elem2)->GetName());
}
void Query( ULevel* Level, const TCHAR* Item, FString* pOutput )
{
	guard(Query);
	enum	{MAX_RESULTS=1024};
	int		NumResults = 0;
	UClass	*Results[MAX_RESULTS];
	FString Work;

	if( ParseCommand(&Item,TEXT("QUERY")) )
	{
		UClass *Parent = NULL;
		ParseObject<UClass>(Item,TEXT("PARENT="),Parent,ANY_PACKAGE);

		// Make a list of all child classes.
		for( TObjectIterator<UClass> It; It && NumResults<MAX_RESULTS; ++It )
			if( It->GetSuperClass()==Parent )
				Results[NumResults++] = *It;

		// Return the results.
		for( INT i=0; i<NumResults; i++ )
		{
			// See if this item has children.
			INT Children = 0;
			for( TObjectIterator<UClass> It; It; ++It )
				if( It->GetSuperClass()==Results[i] )
					Children++;

			// Add to result string.
			if( i>0 ) Work += TEXT(",");
			Work += FString::Printf( TEXT("%s%s"), Children ? TEXT("C") : TEXT("_"), Results[i]->GetName() );
		}

		*pOutput = Work;
	}
	if( ParseCommand(&Item,TEXT("GETCHILDREN")) )
	{
		UClass *Parent = NULL;
		ParseObject<UClass>(Item,TEXT("CLASS="),Parent,ANY_PACKAGE);
		UBOOL Concrete=0; ParseUBOOL( Item, TEXT("CONCRETE="), Concrete );

		// Make a list of all child classes.
		for( TObjectIterator<UClass> It; It && NumResults<MAX_RESULTS; ++It )
			if( It->IsChildOf(Parent) && (!Concrete || !(It->ClassFlags & CLASS_Abstract)) )
				Results[NumResults++] = *It;

		// Sort them by name.
		appQsort( Results, NumResults, sizeof(UClass*), ClassSortCompare );

		// Return the results.
		for( int i=0; i<NumResults; i++ )
		{
			if( i>0 ) Work += TEXT(",");
			Work += FString::Printf( TEXT("%s"), Results[i]->GetName() );
		}

		*pOutput = Work;
	}
	unguard;
}
// Takes a delimited string and breaks it up into elements of a string array.
//
void ParseStringToArray( const TCHAR* pchDelim, FString String, TArray<FString>* _pArray)
{
	guard(ParseStringToArray);
	int i;
	FString S = String;

	i = S.InStr( pchDelim );

	while( i > 0 )
	{
		new(*_pArray)FString( S.Left(i) );
		S = S.Mid( i + 1, S.Len() );
		i = S.InStr( pchDelim );
	}

	new(*_pArray)FString( S );
	unguard;
}

// --------------------------------------------------------------
//
// WBrowser
//
// --------------------------------------------------------------

class WBrowser : public WWindow
{
	DECLARE_WINDOWCLASS(WBrowser,WWindow,Window)

#if 1 //NEW: U2Ed
	FString LastNameLoaded;
#endif

	// Structors.
	WBrowser( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WBrowser::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			320,
			200,
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		unguard;
	}
	int OnSysCommand( INT Command )
	{
		if( Command == SC_CLOSE )
		{
			Show(0);
			return 1;
		}

		return 0;
	}
	void OnCreate()
	{
		guard(WBrowser::OnCreate);
		WWindow::OnCreate();

#if 1 //NEW: U2Ed
		// Load windows last position.
		//
		int X, Y, W, H;

		if(!GConfig->GetInt( *PersistentName, TEXT("X"), X, TEXT("UnrealEd2.ini") ))	X = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("Y"), Y, TEXT("UnrealEd2.ini") ))	Y = 0;
		if(!GConfig->GetInt( *PersistentName, TEXT("W"), W, TEXT("UnrealEd2.ini") ))	W = 512;
		if(!GConfig->GetInt( *PersistentName, TEXT("H"), H, TEXT("UnrealEd2.ini") ))	X = 384;

		if( !W ) W = 320;
		if( !H ) H = 200;

		::MoveWindow( hWnd, X, Y, W, H, TRUE );
#endif

		unguard;
	}
	void OnDestroy()
	{
		guard(WBrowser::OnDestroy);

#if 1 //NEW: U2Ed
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
#endif

		WWindow::OnDestroy();
		unguard;
	}
	void OnPaint()
	{
		guard(WBrowser::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		FillRect( hDC, GetClientRect(), (HBRUSH)(COLOR_BTNFACE+1) );
		EndPaint( *this, &PS );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WBrowser::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);
		PositionChildControls();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	virtual void PositionChildControls( void )
	{
		guard(WBrowser::PositionChildControls);
		unguard;
	}
	// Searches a list of filenames and replaces all single NULL's with | characters.  This allows
	// the regular parse routine to work correctly.  The return value is the number of NULL's
	// that were replaced -- if this is greater than zero, you have multiple filenames.
	//
	int FormatFilenames( char* _pchFilenames )
	{
		guard(WBrowser::FormatFilenames);
		char *pch = _pchFilenames;
		int l_iNULLs = 0;

		while( true )
		{
			if( *pch == '\0' )
			{
				if( *(pch+1) == '\0') break;

				*pch = '|';
				l_iNULLs++;
			}
			pch++;
		}

		return l_iNULLs;
		unguard;
	}
	virtual FString GetCurrentPathName( void )
	{
		guard(WBrowser::GetCurrentPathName);
		return TEXT("");
		unguard;
	}
};

// Takes a fully pathed filename, and just returns the name.
// i.e. "c:\test\file.txt" gets returned as "file".
//
FString GetFilenameOnly( FString Filename)
{
	guard(GetFilenameOnly);
	FString NewFilename = Filename;

	while( NewFilename.InStr( TEXT("\\") ) != -1 )
		NewFilename = NewFilename.Mid( NewFilename.InStr( TEXT("\\") ) + 1, NewFilename.Len() );

	if( NewFilename.InStr( TEXT(".") ) != -1 )
		NewFilename = NewFilename.Left( NewFilename.InStr( TEXT(".") ) );

	return NewFilename;
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif