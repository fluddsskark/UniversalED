#if 1 //NEW: U2Ed
/*=============================================================================
	ViewportFrame : Simple window to hold a viewport into a level
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include "..\..\core\inc\UnMsg.h"

extern TSurfPropSheet* GSurfPropSheet;
extern TBuildSheet* GBuildSheet;

class WViewportFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WViewportFrame,WWindow,Window)

	UViewport* m_pViewport;	// The viewport that this frame contains
	
	// Structors.
	WViewportFrame( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		m_pViewport = NULL;
	}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WViewportFrame::OpenWindow);
		MdiChild = 0;

		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			320,
			200,
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		SetCaption();
		unguard;
	}
	void OnCreate()
	{
		guard(WViewportFrame::OnCreate);
		WWindow::OnCreate();
		unguard;
	}
	void OnPaint()
	{
		guard(WViewportFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		FillRect( hDC, GetClientRect(), (HBRUSH)(COLOR_BTNFACE+1) );
		EndPaint( *this, &PS );
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WViewportFrame::OnSize);
		WWindow::OnSize(Flags, NewX, NewY);

		if( m_pViewport )
		{
			FRect R = GetClientRect();
			::MoveWindow( (HWND)m_pViewport->GetWindow(), R.Min.X, R.Min.Y, R.Width(), R.Height(), 1 );
			m_pViewport->Repaint( 1 );
		}

		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void OnKeyUp( WPARAM wParam, LPARAM lParam )
	{
		// A hack to get the familiar hotkeys working again.  This should really go through
		// Proper Windows acclerators, but I can't get them to work.
		switch( wParam )
		{
			case VK_F4:
				GEditor->Exec( TEXT("HOOK ACTORPROPERTIES") );
				break;

			case VK_F5:
				GSurfPropSheet->Show( TRUE );
				break;

			case VK_F6:
				if( !GEditor->LevelProperties )
				{
					GEditor->LevelProperties = new WObjectProperties( TEXT("LevelProperties"), CPF_Edit, TEXT("Level Properties"), NULL, 1 );
					GEditor->LevelProperties->OpenWindow( hWnd );
					GEditor->LevelProperties->SetNotifyHook( GEditor );
				}
				GEditor->LevelProperties->Root.SetObjects( (UObject**)&GEditor->Level->Actors(0), 1 );
				GEditor->LevelProperties->Show(1);
				break;

			case VK_F7:
				GWarn->BeginSlowTask( TEXT("Compiling changed scripts"), 1, 0 );
				GEditor->Exec( TEXT("SCRIPT MAKE") );
				GWarn->EndSlowTask();
				break;

			case VK_F8:
				GBuildSheet->Show(1);
				break;

			case VK_DELETE:
				GEditor->Exec( TEXT("DELETE") );
				break;
		}
	}
	void OnCommand( INT Command )
	{
		guard(WViewportFrame::OnCommand);
		switch( Command ) {

			case WM_VIEWPORT_UPDATEWINDOWFRAME:
				SetCaption();
				break;

			default:
				WWindow::OnCommand(Command);
				break;
		}
		unguard;
	}
	void SetCaption( void )
	{
		if( !m_pViewport ) 
		{
			SetText( TEXT("Viewport Frame") );
			return;
		}

		switch( m_pViewport->Actor->RendMap )
		{
			case REN_Wire:
				SetText( TEXT("Wireframe") );
				break;

			case REN_Zones:
				SetText( TEXT("Zone/Portal View") );
				break;

			case REN_Polys:
				SetText( TEXT("Texture Usage") );
				break;

			case REN_PolyCuts:
				SetText( TEXT("BSP Cuts") );
				break;

			case REN_DynLight:
				SetText( TEXT("Dynamic Light") );
				break;

			case REN_PlainTex:
				SetText( TEXT("Texture") );
				break;

			case REN_OrthXY:
				SetText( TEXT("Overhead") );
				break;

			case REN_OrthXZ:
				SetText( TEXT("Ortho XZ") );
				break;

			case REN_OrthYZ:
				SetText( TEXT("Ortho YZ") );
				break;

			case REN_TexView:
				SetText( TEXT("Texture View") );
				break;

			case REN_TexBrowser:
				SetText( TEXT("Texture Browser") );
				break;

			case REN_MeshView:
				SetText( TEXT("Mesh Viewer") );
				break;

			default:
				SetText( TEXT("Unknown") );
				break;
		}
	}
	void SetViewport( UViewport* pViewport )
	{
		m_pViewport = pViewport;

		FRect R = GetClientRect();
		m_pViewport->OpenWindow( (DWORD)hWnd, 0, R.Width(), R.Height(), R.Min.X, R.Min.Y );

		SetCaption();
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif