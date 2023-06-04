#if 1 //NEW: U2Ed
#ifndef _WDLGFINDREPLACE_H_
#define _WDLGFINDREPLACE_H_

/*=============================================================================
	FindReplace : Find/Replace text in the codeframe window
	Copyright 2000 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include "CodeFrame.h"
extern WCodeFrame* GCodeFrame;


TArray<FString> GFindHistory, GReplaceHistory;
FString GLastText;
int GLastScreenPos = 0;		// Pos that we use for selecting text on the screen
int GLastInternalPos = 0;	// Pos that we use for storing the actual search position
UBOOL GMoreText = 0, GMatchCase = 1;

class WDlgFindReplace : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgFindReplace,WDialog,UnrealEd)

	// Variables.
	WButton FindButton;
	WButton FindNextButton;
	WButton FindPrevButton;
	WButton ReplaceButton;
	WButton ReplaceAllButton;
	WButton CancelButton;
	WComboBox FindCombo;
	WComboBox ReplaceCombo;
	WCheckBox MatchCaseCheck;
	WEdit LineNumEdit;
	WButton GoLineNumButton;

	// Constructor.
	WDlgFindReplace( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Find/Replace"), IDDIALOG_FINDREPLACE, InOwnerWindow )
	,	FindButton			( this, IDPB_FIND,			FDelegate(this,(TDelegate)OnFind) )
	,	FindNextButton		( this, IDPB_FIND_NEXT,		FDelegate(this,(TDelegate)OnFindNext) )
	,	FindPrevButton		( this, IDPB_FIND_PREV,		FDelegate(this,(TDelegate)OnFindPrev) )
	,	ReplaceButton		( this, IDPB_REPLACE,		FDelegate(this,(TDelegate)OnReplace) )
	,	ReplaceAllButton	( this, IDPB_REPLACE_ALL,	FDelegate(this,(TDelegate)OnReplaceAll) )
	,	CancelButton		( this, IDCANCEL,			FDelegate(this,(TDelegate)EndDialogFalse) )
	,	FindCombo			( this, IDCB_FIND )
	,	ReplaceCombo		( this, IDCB_REPLACE )
	,	LineNumEdit			( this, IDEC_LINE_NUM )
	,	GoLineNumButton		( this, IDPB_GO_LINE_NUM,	FDelegate(this,(TDelegate)OnGoLineNUm) )
	,	MatchCaseCheck		( this, IDCK_MATCH_CASE)
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgFindReplace::OnInitDialog);
		WDialog::OnInitDialog();

		::SetFocus( FindCombo.hWnd );
		RefreshHistory();

		// If there is text selected, pull it in and make it the default text to search for.
		CHARRANGE range;
		SendMessageX( GCodeFrame->Edit.hWnd, EM_EXGETSEL, 0, (LPARAM)(CHARRANGE FAR*)&range );

		if( range.cpMax - range.cpMin )
		{
			TEXTRANGE txtrange;
			txtrange.chrg = range;
			txtrange.lpstrText = new TCHAR[range.cpMax - range.cpMin];
			SendMessageX( GCodeFrame->Edit.hWnd, EM_GETTEXTRANGE, 0, (LPARAM)(TEXTRANGE FAR*)&txtrange );

			FindCombo.SetText( txtrange.lpstrText );

			delete [] txtrange.lpstrText;
		}

		MatchCaseCheck.SetCheck( GMatchCase ? BST_CHECKED : BST_UNCHECKED );

		unguard;
	}
	void UpdateHistory( FString Find, FString Replace )
	{
		// FIND
		//
		if( Find.Len() )
		{
			// Check if value is already in the list.  If not, add it.
			for(int x = 0 ; x < GFindHistory.Num() ; x++)
			{
				if( GFindHistory(x) == Find )
					break;
			}

			if( x == GFindHistory.Num() )
				GFindHistory.AddItem( Find );
		}

		// REPLACE
		//
		if( Replace.Len() )
		{
			for(int x = 0 ; x < GReplaceHistory.Num() ; x++)
			{
				if( GReplaceHistory(x) == Replace )
					break;
			}

			if( x == GReplaceHistory.Num() )
				GReplaceHistory.AddItem( Replace );
		}
	}
	void RefreshHistory()
	{
		guard(WDlgFindReplace::RefreshHistory);

		FindCombo.Empty();
		for( int x = 0 ; x < GFindHistory.Num() ; x++ )
		{
			FindCombo.AddString( *(GFindHistory(x)) );
		}

		ReplaceCombo.Empty();
		for( x = 0 ; x < GReplaceHistory.Num() ; x++ )
		{
			ReplaceCombo.AddString( *(GReplaceHistory(x)) );
		}

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgFindReplace::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgFindReplace::DoModal);
		::BringWindowToTop( hWnd );
		return WDialog::DoModal( hInstance );
		unguard;
	}
	UBOOL LocateText( int* pScreenPos, int* pInternalPos, UBOOL bBackwards = 0 )
	{
		guard(WDlgFindReplace::LocateText);

		// Get the source text out of the richedit control
		int Length = SendMessageA( GCodeFrame->Edit.hWnd, WM_GETTEXTLENGTH, 0, 0 );
		char* pbuffer = new char[Length+1];
		::strcpy(pbuffer, "");
		SendMessageA( GCodeFrame->Edit.hWnd, WM_GETTEXT, Length+1, (LPARAM)pbuffer );
		TCHAR* pch = (TCHAR*)appFromAnsi( pbuffer );

		// Put the source text into a local var so we can convert it to upper case if we need
		// to search without matching case.
		FString SrcText = pch;

		// Put the search text into a local var for the same reason.
		FString SearchText = GLastText;

		GMatchCase = MatchCaseCheck.IsChecked();
		if( !GMatchCase )
		{
			SrcText = SrcText.Caps();
			SearchText = SearchText.Caps();
		}

		// Work variables.
		pch = (TCHAR*)*SrcText;

		int x, screenpos;

		UBOOL Match = 0;

		if( bBackwards )
			for( x = *pInternalPos, screenpos = *pScreenPos ; 
					x > -1 ; 
					x--, screenpos-- )
			{
				// End of line is represented with 2 chars in rich edit controls.  We need to only
				// count them as one for purposes of tracking position.
				if( *(pch+x) == 10 && *(pch+x-1) == 13 )
					x--;
				else
					if( !appStrncmp( (const TCHAR*)(pch+x), *SearchText, SearchText.Len() ) )
					{
						Match = 1;
						break;
					}
			}
		else
			for( x = *pInternalPos, screenpos = *pScreenPos ; 
					x < Length ; 
					x++, screenpos++ )
			{
				// End of line is represented with 2 chars in rich edit controls.  We need to only
				// count them as one for purposes of tracking position.
				if( *(pch+x) == 13 && *(pch+x+1) == 10 )
					x++;
				else
					if( !appStrncmp( (const TCHAR*)(pch+x), *SearchText, SearchText.Len() ) )
					{
						Match = 1;
						break;
					}
			}

		// If we didn't hit the end of the file, update the pos var.
		if( Match )
		{
			*pScreenPos = screenpos;
			*pInternalPos = x;
		}

		delete [] pbuffer;

		return Match;

		unguard;
	}
	int GetScreenPosFromInternal( int _InternalPos )
	{
		guard(WDlgFindReplace::GetScreenPosFromInternal);

		// Get the source text out of the richedit control
		int Length = SendMessageA( GCodeFrame->Edit.hWnd, WM_GETTEXTLENGTH, 0, 0 );
		char* pbuffer = new char[Length+1];
		::strcpy(pbuffer, "");
		SendMessageA( GCodeFrame->Edit.hWnd, WM_GETTEXT, Length+1, (LPARAM)pbuffer );
		TCHAR* pch = (TCHAR*)appFromAnsi( pbuffer );

		int x, screenpos = 0;
		for( x = 0, screenpos = 0 ; 
				x < _InternalPos ; 
				x++, screenpos++ )
		{
			if( *(pch+x) == 13 && *(pch+x+1) == 10 )
			{
				// End of line is represented with 2 chars in rich edit controls.  We need to only
				// count them as one for purposes of tracking position.
				x++;
			}
		}

		delete [] pbuffer;
				
		return screenpos;

		unguard;
	}
	void OnFind()
	{
		guard(WDlgFindReplace::OnFind);

		CHARRANGE range;
		SendMessageX( GCodeFrame->Edit.hWnd, EM_EXGETSEL, 0, (LPARAM)(CHARRANGE FAR*)&range );
		GLastInternalPos = range.cpMin;
		GLastScreenPos = GetScreenPosFromInternal( GLastInternalPos );
		GLastText = FindCombo.GetText();
		if( !GLastText.Len() ) return;

		if( LocateText( &GLastScreenPos, &GLastInternalPos ) )
		{
			//UpdateHistory( *GLastText, TEXT("") );

			SendMessageX( GCodeFrame->Edit.hWnd, EM_SETSEL, GLastScreenPos, GLastScreenPos + GLastText.Len() );
			SendMessageX( GCodeFrame->Edit.hWnd, EM_SCROLLCARET, 0, 0 );
		}
		else
		{
			::MessageBox( hWnd, TEXT("End of text reached."), TEXT("Find"), MB_OK);
		}

		EndDialogTrue();
		unguard;
	}
	void OnFindNext()
	{
		guard(WDlgFindReplace::OnFindNext);

		if( !GLastText.Len() ) return;

		GLastInternalPos++;
		GLastScreenPos++;
		if( LocateText( &GLastScreenPos, &GLastInternalPos ) )
		{
			SendMessageX( GCodeFrame->Edit.hWnd, EM_SETSEL, GLastScreenPos, GLastScreenPos + GLastText.Len() );
			SendMessageX( GCodeFrame->Edit.hWnd, EM_SCROLLCARET, 0, 0 );
		}
		else
			::MessageBox( hWnd, TEXT("End of text reached."), TEXT("Find"), MB_OK);

		unguard;
	}
	void OnFindPrev()
	{
		guard(WDlgFindReplace::OnFindPrev);

		if( !GLastText.Len() ) return;

		GLastInternalPos--;
		GLastScreenPos--;
		if( LocateText( &GLastScreenPos, &GLastInternalPos, 1 ) )
		{
			SendMessageX( GCodeFrame->Edit.hWnd, EM_SETSEL, GLastScreenPos, GLastScreenPos + GLastText.Len() );
			SendMessageX( GCodeFrame->Edit.hWnd, EM_SCROLLCARET, 0, 0 );
		}
		else
			::MessageBox( hWnd, TEXT("End of text reached."), TEXT("Find"), MB_OK);

		unguard;
	}
	void OnReplace()
	{
		guard(WDlgFindReplace::OnReplace);

		FString ReplaceString = ReplaceCombo.GetText();
		GLastText = FindCombo.GetText();

		if( GLastText.Len() )
		{
			UpdateHistory( GLastText, ReplaceString );

			DWORD Start, End;
			SendMessageX( GCodeFrame->Edit.hWnd, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End );

			if( Start && End )
			{
				/*
				FString OldText = GCodeFrame->Edit.GetText(1);
				FString NewText = OldText.Left(Start) + ReplaceString + OldText.Right( OldText.Len() - End );
				GCodeFrame->Edit.SetText( *NewText );
				*/
			}

			OnFindNext();
		}

		unguard;
	}
	void OnReplaceAll()
	{
		guard(WDlgFindReplace::OnReplaceAll);

		/*
		FString ReplaceString = ReplaceCombo.GetText();
		GLastText = FindCombo.GetText();

		if( GLastText.Len() )
		{
			GLastPos = 0;
			GMoreText = 1;

			UpdateHistory( GLastText, ReplaceString );

			OnFindNext();

			while( GMoreText )
			{
				DWORD Start, End;
				SendMessageX( GCodeFrame->Edit.hWnd, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End );

				OnFindNext();
			}
		}
		*/

		unguard;
	}
	void OnGoLineNUm()
	{
		guard(WDlgFindReplace::OnGoLineNUm);
		GCodeFrame->ScrollToLine( appAtoi( *(LineNumEdit.GetText()) ) );
		EndDialogTrue();
		unguard;
	}
};
#endif //_WDLGFINDREPLACE_H_

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif
