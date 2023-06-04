#if 1 //NEW: U2Ed
/*=============================================================================
	AddSpecial : Add special brushes
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>

class WDlgAddSpecial : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgAddSpecial,WDialog,UnrealEd)

	// Variables.
	WComboBox PrefabCombo;
	WButton OKButton;
	WButton CancelButton;

	// Constructor.
	WDlgAddSpecial( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Add Special"), IDDIALOG_ADD_SPECIAL, InOwnerWindow )
	,	PrefabCombo		( this, IDCB_PREFABS )
	,	OKButton		( this, IDOK, FDelegate(this,(TDelegate)OnOK) )
	,	CancelButton	( this, IDCANCEL, FDelegate(this,(TDelegate)EndDialogFalse) )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgAddSpecial::OnInitDialog);
		WDialog::OnInitDialog();

		PrefabCombo.AddString(TEXT("Invisible collision hull"));
		PrefabCombo.AddString(TEXT("Masked Decoration"));
		PrefabCombo.AddString(TEXT("Masked Wall"));
		PrefabCombo.AddString(TEXT("Regular Brush"));
		PrefabCombo.AddString(TEXT("Semisolid Pillar"));
		PrefabCombo.AddString(TEXT("Transparent Window"));
		PrefabCombo.AddString(TEXT("Water"));
		PrefabCombo.AddString(TEXT("Zone Portal"));
		PrefabCombo.SelectionChangeDelegate = FDelegate(this, (TDelegate)OnComboPrefabsSelChange);

		PrefabCombo.SetCurrent( 3 );
		OnComboPrefabsSelChange();

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgAddSpecial::OnDestroy);
		WDialog::OnDestroy();
		::DestroyWindow( hWnd );
		unguard;
	}
	virtual void DoModeless()
	{
		guard(WDlgAddSpecial::DoModeless);
		_Windows.AddItem( this );
		hWnd = CreateDialogParamA( hInstance, MAKEINTRESOURCEA(IDDIALOG_ADD_SPECIAL), OwnerWindow?OwnerWindow->hWnd:NULL, (DLGPROC)StaticDlgProc, (LPARAM)this);
		if( !hWnd )
			appGetLastError();
		Show( TRUE );
		unguard;
	}
	void OnOK()
	{
		guard(WDlgAddSpecial::OnCompileAll);

		int Flags = 0;

		if( SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Masked;
		if( SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Translucent;
		if( SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Portal;
		if( SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Invisible;
		if( SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_TwoSided;
		if( SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_Semisolid;
		if( SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_GETCHECK, 0, 0) == BST_CHECKED )
			Flags |= PF_NotSolid;

		GEditor->Exec( *(FString::Printf(TEXT("BRUSH ADD FLAGS=%d"), Flags)));

		EndDialog(TRUE);
		unguard;
	}
	void OnComboPrefabsSelChange()
	{
		switch( PrefabCombo.GetCurrent() )
		{
			case 0:	// Invisible collision hull
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 1:	// Masked Decoration
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 2:	// Masked Wall
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 3:	// Regular Brush
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 4:	// Semisolid Pillar
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				break;

			case 5:	// Transparent Window
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 6:	// Water
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;

			case 7:	// Zone Portal
				SendMessageX( ::GetDlgItem( hWnd, IDCK_MASKED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TRANSPARENT), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_ZONE_PORTAL), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_INVIS), BM_SETCHECK, BST_CHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDCK_TWO_SIDED), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_SEMI_SOLID), BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessageX( ::GetDlgItem( hWnd, IDRB_NON_SOLID), BM_SETCHECK, BST_CHECKED, 0);
				break;
		}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif