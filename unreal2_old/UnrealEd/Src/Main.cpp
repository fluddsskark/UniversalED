#if 1 //NEW: U2Ed
/*=============================================================================
	Main.cpp: UnrealEd Windows startup.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

    Revision history:
		* Created by Tim Sweeney.

    Work-in-progress todo's:

=============================================================================*/

enum eLASTDIR {
	eLASTDIR_UNR	= 0,
	eLASTDIR_UTX	= 1,
	eLASTDIR_PCX	= 2,
	eLASTDIR_UAX	= 3,
	eLASTDIR_WAV	= 4,
	eLASTDIR_BRUSH	= 5,
	eLASTDIR_2DS	= 6,
	eLASTDIR_MAX	= 7
};

#pragma warning( disable : 4201 )
#define STRICT
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include "Engine.h"
#include "UnRender.h"
#include "Window.h"
#include "..\..\Editor\Src\EditorPrivate.h"
#include "Res\resource.h"
#include "UnEngineWin.h"
#include "DlgProgress.h"
#include "DlgFindReplace.h"
#include "DlgSearchActors.h"
#include "CodeFrame.h"
#include "DlgTexProp.h"
#include "DlgAddSpecial.h"
#include "DlgScaleLights.h"
#include "SurfPropSheet.h"
#include "BuildSheet.h"
#include "DlgBrushImport.h"
#include "DlgMapImport.h"
#include "TwoDeeShapeEditor.h"
#include "Extern.h"
#include "Browser.h"
#include "BrowserSound.h"
#include "BrowserMUsic.h"
#include "BrowserTexture.h"
#include "BrowserMesh.h"
#include "..\..\core\inc\unmsg.h"
#include "ViewportFrame.h"

FString GLastDir[eLASTDIR_MAX];
FString GMapExt;

extern "C" {HINSTANCE hInstance;}
extern "C" {TCHAR GPackage[64]=TEXT("U2Ed");}

// Brushes.
HBRUSH hBrushMode = CreateSolidBrush( RGB(0,96,0) );

extern FString GLastText;
extern FString GMapExt;

// MRU
void MRU_ReadINI();
void MRU_WriteINI();
void MRU_AddFilename( FString Filename );
void MRU_AddToMenu();

// Docking frame positions.
enum EDockingFramePosition
{
	DOCKF_Top=0,
	DOCKF_Bottom=1,
	DOCKF_Left=2,
	DOCKF_Right=3,
};

// Classes.
class WMdiClient;
class WMdiFrame;
class WEditorFrame;
class WMdiDockingFrame;
class WLevelFrame;

// Memory allocator.
#include "FMallocWindows.h"
FMallocWindows Malloc;

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#if 1 //NEW
#include "FOutputDeviceEditorWindowsError.h"
FOutputDeviceEditorWindowsError Error;
#else
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;
#endif

// Feedback.
#include "FFeedbackContextWindows.h"
FFeedbackContextWindows Warn;

// File manager.
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;

// Config.
#include "FConfigCacheIni.h"

WCodeFrame* GCodeFrame = NULL;
#include "BrowserActor.h"

#if 1 //NEW: ParticleSystems
#include "BrowserParticles.h"
#endif

WEditorFrame* GEditorFrame = NULL;
WLevelFrame* GLevelFrame = NULL;
W2DShapeEditor* G2DShapeEditor = NULL;
TSurfPropSheet* GSurfPropSheet = NULL;
TBuildSheet* GBuildSheet = NULL;
WBrowserSound* GBrowserSound = NULL;
WBrowserMusic* GBrowserMusic = NULL;
WBrowserActor* GBrowserActor = NULL;
#if 1 //NEW: ParticleSystems
WBrowserParticles* GBrowserParticles = NULL;
#endif
WBrowserTexture* GBrowserTexture = NULL;
WBrowserMesh* GBrowserMesh = NULL;
WDlgAddSpecial* GDlgAddSpecial = NULL;
WDlgScaleLights* GDlgScaleLights = NULL;
WDlgProgress* GDlgProgress = NULL;
WProperties* GToolBar;
WDlgSearchActors* GDlgSearchActors = NULL;

void FileOpen( HWND hWnd );

/*-----------------------------------------------------------------------------
	Document manager crappy abstraction.
-----------------------------------------------------------------------------*/

struct FDocumentManager
{
	virtual void OpenLevelView()=0;
	virtual void OpenScriptView( UClass* Class )=0;
} *GDocumentManager=NULL;

/*-----------------------------------------------------------------------------
	WMdiClient.
-----------------------------------------------------------------------------*/

// An MDI client window.
class WMdiClient : public WControl
{
	DECLARE_WINDOWSUBCLASS(WMdiClient,WControl,UnrealEd)
	WMdiClient( WWindow* InOwner )
	: WControl( InOwner, 0, SuperProc )
	{}
	void OpenWindow( CLIENTCREATESTRUCT* ccs )
	{
		guard(WMdiFrame::OpenWindow);
		//must make nccreate work!! GetWindowClassName(),
		//!! WS_VSCROLL | WS_HSCROLL
        HWND hWndCreated = TCHAR_CALL_OS(CreateWindowEx(0,TEXT("MDICLIENT"),NULL,WS_CHILD|WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,0,0,0,OwnerWindow->hWnd,(HMENU)0xCAC,hInstance,ccs),CreateWindowExA(0,"MDICLIENT",NULL,WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,0,0,0,0,OwnerWindow->hWnd,(HMENU)0xCAC,hInstance,ccs));
		check(hWndCreated);
		check(!hWnd);
		_Windows.AddItem( this );
		hWnd = hWndCreated;
		Show( 1 );
		unguard;
	}
};
WNDPROC WMdiClient::SuperProc;

/*-----------------------------------------------------------------------------
	WDockingFrame.
-----------------------------------------------------------------------------*/

// One of four docking frame windows on a MDI frame.
class WDockingFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WDockingFrame,WWindow,UnrealEd)

	// Variables.
	EDockingFramePosition DockingPosition;
	INT DockDepth;
	WWindow* Child;

	// Functions.
	WDockingFrame( FName InPersistentName, WMdiFrame* InFrame, INT InDockDepth, EDockingFramePosition InPos )
	:	WWindow			( InPersistentName, (WWindow*)InFrame )
	,	DockingPosition	( InPos )
	,   DockDepth       ( InDockDepth )
	,	Child			( NULL )
	{}
	void OpenWindow()
	{
		guard(WDockingFrame::OpenWindow);
		PerformCreateWindowEx
		(
			0,
			NULL,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0, 0, 0, 0,
			OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		Show(1);
		unguard;
	}
	void Dock( WWindow* InChild )
	{
		guard(WDockingFrame::Dock);
		Child = InChild;
		unguard;
	}
	void OnSize( DWORD Flags, INT InX, INT InY )
	{
		guard(WDockingFrame::OnSize);
		if( Child )
			Child->MoveWindow( GetClientRect(), TRUE );
		unguard;
	}
	void OnPaint()
	{
		guard(WDockingFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		FRect Rect = GetClientRect();
		FillRect( hDC, Rect, (HBRUSH)(COLOR_BTNFACE+1) );
		DrawEdge( hDC, Rect, BDR_RAISEDINNER, BF_TOPLEFT|BF_BOTTOMRIGHT );

		EndPaint( *this, &PS );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WMdiFrame.
-----------------------------------------------------------------------------*/

// An MDI frame window.
class WMdiFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WMdiFrame,WWindow,UnrealEd)

	// Variables.
	WMdiClient MdiClient;
	WDockingFrame BottomFrame, LeftFrame;

	// Functions.
	WMdiFrame( FName InPersistentName )
	:	WWindow		( InPersistentName )
	,	MdiClient	( this )
	,	BottomFrame	( TEXT("MdiFrameBottom"), this, 0,   DOCKF_Bottom )
	,	LeftFrame	( TEXT("MdiFrameLeft"),	  this, 224, DOCKF_Left )
	{}
	INT CallDefaultProc( UINT Message, UINT wParam, LONG lParam )
	{
		return DefFrameProcX( hWnd, MdiClient.hWnd, Message, wParam, lParam );
	}
	void OnCreate()
	{
		guard(WMdiFrame::OnCreate);
		WWindow::OnCreate();

		// Create docking frames.
		BottomFrame.OpenWindow();
		LeftFrame  .OpenWindow();

		GSurfPropSheet = new TSurfPropSheet;
		GSurfPropSheet->OpenWindow( hInstance, hWnd );
		GSurfPropSheet->Show( FALSE );

		GBuildSheet = new TBuildSheet;
		GBuildSheet->OpenWindow( hInstance, hWnd );
		GBuildSheet->Show( FALSE );

		unguard;
	}
	virtual void RepositionClient()
	{
		guard(WMdiFrame::RepositionClient);

		// Reposition docking frames.
		FRect Client = GetClientRect();
		BottomFrame.MoveWindow( FRect(0, Client.Max.Y-BottomFrame.DockDepth, Client.Max.X, Client.Max.Y), 1 );
		LeftFrame  .MoveWindow( FRect(0, 28, LeftFrame.DockDepth, Client.Max.Y-BottomFrame.DockDepth), 1 );

		// Reposition MDI client window.
		MdiClient  .MoveWindow( FRect(LeftFrame.DockDepth, 28, Client.Max.X, Client.Max.Y-BottomFrame.DockDepth), 1 );

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WMdiFrame::OnSize);
		RepositionClient();
		throw TEXT("NoRoute");
		unguard;
	}
	void OpenWindow()
	{
		guard(WMdiFrame::OpenWindow);
		TCHAR Title[256];
		appSprintf( Title, LocalizeGeneral(TEXT("FrameWindow"),TEXT("U2Ed")), LocalizeGeneral(TEXT("Product"),TEXT("Core")) );
		PerformCreateWindowEx
		(
			WS_EX_APPWINDOW,
			Title,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			640,
			480,
			NULL,
			NULL,
			hInstance
		);
		ShowWindow( *this, SW_SHOWMAXIMIZED );
		unguard;
	}
	void OnSetFocus()
	{
		guard(WMdiFrame::OnSetFocus);
		SetFocus( MdiClient );
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WBackgroundHolder.
-----------------------------------------------------------------------------*/

// Test.
class WBackgroundHolder : public WWindow
{
	DECLARE_WINDOWCLASS(WBackgroundHolder,WWindow,Window)

	// Structors.
	WBackgroundHolder( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{}

	// WWindow interface.
	void OpenWindow()
	{
		guard(WBackgroundHolder::OpenWindow);
		MdiChild = 0;
		PerformCreateWindowEx
		(
			WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE,
			NULL,
			WS_CHILD | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			0,
			0,
			512,
			256,
			OwnerWindow ? OwnerWindow->hWnd : NULL,
			NULL,
			hInstance
		);
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WLevelFrame.
-----------------------------------------------------------------------------*/

enum eBIMODE {
	eBIMODE_CENTER	= 0,
	eBIMODE_TILE	= 1,
	eBIMODE_STRETCH	= 2
};

class WLevelFrame : public WWindow
{
	DECLARE_WINDOWCLASS(WLevelFrame,WWindow,Window)

	// Variables.
	ULevel* Level;
	WViewportFrame* m_pViewportFrames[dED_MAX_VIEWPORTS];
	HBITMAP hImage;
	FString BIFilename;
	int BIMode;	// eBIMODE_

	// Structors.
	WLevelFrame( ULevel* InLevel, FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	,	Level( InLevel )
	{
		SetMapFilename( TEXT("") );
		hImage = NULL;
		BIMode = eBIMODE_CENTER;
		BIFilename = TEXT("");

		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			m_pViewportFrames[x] = NULL;
		}
	}
	void SetMapFilename( TCHAR* _MapFilename )
	{
		appStrcpy( MapFilename, _MapFilename );
		if( ::IsWindow( hWnd ) )
			SetText( MapFilename );
	}
	TCHAR* GetMapFilename()
	{
		return MapFilename;
	}

	void OnDestroy()
	{
		guard(WLevelFrame::OnDestroy);

		// Save data out to config file, and clean up...
		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			TCHAR l_chName[20];
			appSprintf( l_chName, TEXT("U2Viewport%d"), x);

			if( m_pViewportFrames[x] && ::IsWindow( m_pViewportFrames[x]->hWnd ) )
			{
				FRect R = m_pViewportFrames[x]->GetWindowRect();
			
				GConfig->SetInt( l_chName, TEXT("Active"), 1, TEXT("UnrealEd2.ini") );
				GConfig->SetInt( l_chName, TEXT("X"), R.Min.X, TEXT("UnrealEd2.ini") );
				GConfig->SetInt( l_chName, TEXT("Y"), R.Min.Y, TEXT("UnrealEd2.ini") );
				GConfig->SetInt( l_chName, TEXT("W"), R.Width(), TEXT("UnrealEd2.ini") );
				GConfig->SetInt( l_chName, TEXT("H"), R.Height(), TEXT("UnrealEd2.ini") );
				GConfig->SetInt( l_chName, TEXT("RendMap"), m_pViewportFrames[x]->m_pViewport->Actor->RendMap, TEXT("UnrealEd2.ini") );
			}
			else {

				GConfig->SetInt( l_chName, TEXT("Active"), 0, TEXT("UnrealEd2.ini") );
			}

			delete m_pViewportFrames[x];
		}

		// "Last Directory"
		GConfig->SetString( TEXT("Directories"), TEXT("PCX"), *GLastDir[eLASTDIR_PCX], TEXT("UnrealEd2.ini") );
		GConfig->SetString( TEXT("Directories"), TEXT("WAV"), *GLastDir[eLASTDIR_WAV], TEXT("UnrealEd2.ini") );
		GConfig->SetString( TEXT("Directories"), TEXT("BRUSH"), *GLastDir[eLASTDIR_BRUSH], TEXT("UnrealEd2.ini") );
		GConfig->SetString( TEXT("Directories"), TEXT("2DS"), *GLastDir[eLASTDIR_2DS], TEXT("UnrealEd2.ini") );

		// Background image
		GConfig->SetInt( TEXT("Background Image"), TEXT("Active"), (hImage != NULL), TEXT("UnrealEd2.ini") );
		GConfig->SetInt( TEXT("Background Image"), TEXT("Mode"), BIMode, TEXT("UnrealEd2.ini") );
		GConfig->SetString( TEXT("Background Image"), TEXT("Filename"), *BIFilename, TEXT("UnrealEd2.ini") );

		::DeleteObject( hImage );

		unguard;
	}
	// Looks for an empty viewport slot, allocates a viewport and returns a pointer to it.
	WViewportFrame* NewViewportFrame( FName* pName )
	{
		guard(WLevelFrame::NewViewportFrame);
		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			// Compensate for the window being closed.
			if( m_pViewportFrames[x] && !::IsWindow( m_pViewportFrames[x]->hWnd ) )
			{
				m_pViewportFrames[x] = NULL;
			}

			if( !m_pViewportFrames[x] )
			{
				// Make up a unique name for this viewport.
				TCHAR l_chName[20];
				appSprintf( l_chName, TEXT("U2Viewport%d"), x);
				*pName = l_chName;

				// Create the viewport.
				m_pViewportFrames[x] = new WViewportFrame( *pName, this );

				return m_pViewportFrames[x];
			}
		}

		appMsgf( TEXT("You are at the limit for open viewports.") );
		return NULL;
		unguard;
	}

	// WWindow interface.
	void OnKillFocus( HWND hWndNew )
	{
		guard(WLevelFrame::OnKillFocus);
		GEditor->Client->MakeCurrent( NULL );
		unguard;
	}
	void Serialize( FArchive& Ar )
	{
		guard(WLevelFrame::Serialize);
		WWindow::Serialize( Ar );
		Ar << Level;
		unguard;
	}
	void OpenWindow( UBOOL bMdi, UBOOL bMax )
	{
		guard(WLevelFrame::OpenWindow);
		MdiChild = bMdi;
		PerformCreateWindowEx
		(
			MdiChild
			?	(WS_EX_MDICHILD)
			:	(0),
			TEXT("Level"),
			(bMax ? WS_MAXIMIZE : 0 ) |
			(MdiChild
			?	(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
			:	(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)),
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			512,
			384,
			MdiChild ? OwnerWindow->OwnerWindow->hWnd : OwnerWindow->hWnd,
			NULL,
			hInstance
		);
		if( !MdiChild )
		{
			SetWindowLongX( hWnd, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );
			OwnerWindow->Show(1);
		}

		// Open the proper configuration of viewports.
		for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
		{
			TCHAR l_chName[20];
			appSprintf( l_chName, TEXT("U2Viewport%d"), x);
			int Active, X, Y, W, H, RendMap;

			if(!GConfig->GetInt( l_chName, TEXT("Active"), Active, TEXT("UnrealEd2.ini") ))		Active = 0;

			if( Active )
			{
				if(!GConfig->GetInt( l_chName, TEXT("X"), X, TEXT("UnrealEd2.ini") ))	X = 0;
				if(!GConfig->GetInt( l_chName, TEXT("Y"), Y, TEXT("UnrealEd2.ini") ))	Y = 0;
				if(!GConfig->GetInt( l_chName, TEXT("W"), W, TEXT("UnrealEd2.ini") ))	W = 512;
				if(!GConfig->GetInt( l_chName, TEXT("H"), H, TEXT("UnrealEd2.ini") ))	H = 384;
				if(!GConfig->GetInt( l_chName, TEXT("RendMap"), RendMap, TEXT("UnrealEd2.ini") ))	RendMap = REN_OrthXY;

				OpenFrameViewport( RendMap, X, Y, W, H, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes );
			}
		}

		// Background image
		UBOOL bActive;
		if(!GConfig->GetInt( TEXT("Background Image"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = 0;

		if( bActive )
		{
			if(!GConfig->GetInt( TEXT("Background Image"), TEXT("Mode"), BIMode, TEXT("UnrealEd2.ini") ))	BIMode = eBIMODE_CENTER;
			if(!GConfig->GetString( TEXT("Background Image"), TEXT("Filename"), BIFilename, TEXT("UnrealEd2.ini") ))	BIFilename.Empty();
			LoadBackgroundImage(BIFilename);
		}

		unguard;
	}
	void LoadBackgroundImage( FString Filename )
	{
		guard(WLevelFrame::LoadBackgroundImage);

		if( hImage ) 
			DeleteObject( hImage );

		hImage = (HBITMAP)LoadImageA( hInstance, appToAnsi( *Filename ), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );

		if( hImage )
			BIFilename = Filename;
		else
			appMsgf ( TEXT("Error loading bitmap for background image.") );

		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(WLevelFrame::OnSize);
		WWindow::OnSize( Flags, NewX, NewY );
		unguard;
	}
	INT OnSetCursor()
	{
		guard(WLevelFrame::OnSetCursor);
		WWindow::OnSetCursor();
		SetCursor(LoadCursorIdX(NULL,IDC_ARROW));
		return 0;
		unguard;
	}
	void OnPaint()
	{
		guard(WLevelFrame::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );
		FillRect( hDC, GetClientRect(), (HBRUSH)(COLOR_WINDOW+1) );
		DrawImage( hDC );
		EndPaint( *this, &PS );

		// Put the name of the map into the titlebar.
		SetText( GetMapFilename() );

		unguard;
	}
	void DrawImage( HDC _hdc )
	{
		guard(WLevelFrame::DrawImage);
		if( !hImage ) return;

		HDC hdcMem;
		HBITMAP hbmOld;
		BITMAP bitmap;

		// Prepare the bitmap.
		//
		GetObjectA( hImage, sizeof(BITMAP), (LPSTR)&bitmap );
		hdcMem = CreateCompatibleDC(_hdc);
		hbmOld = (HBITMAP)SelectObject(hdcMem, hImage);

		// Display it.
		//
		RECT l_rc;
		::GetClientRect( hWnd, &l_rc );
		switch( BIMode )
		{
			case eBIMODE_CENTER:
			{
				BitBlt(_hdc,
				   (l_rc.right - bitmap.bmWidth) / 2, (l_rc.bottom - bitmap.bmHeight) / 2,
				   bitmap.bmWidth, bitmap.bmHeight,
				   hdcMem,
				   0, 0,
				   SRCCOPY);
			}
			break;

			case eBIMODE_TILE:
			{
				int XSteps = (int)((l_rc.right / bitmap.bmWidth)) + 1;
				int YSteps = (int)((l_rc.bottom / bitmap.bmHeight)) + 1;

				for( int x = 0 ; x < XSteps ; x++ )
					for( int y = 0 ; y < YSteps ; y++ )
						BitBlt(_hdc,
						   (x * bitmap.bmWidth), (y * bitmap.bmHeight),
						   bitmap.bmWidth, bitmap.bmHeight,
						   hdcMem,
						   0, 0,
						   SRCCOPY);
			}
			break;

			case eBIMODE_STRETCH:
			{
				StretchBlt(
					_hdc,
				   0, 0,
				   l_rc.right, l_rc.bottom,
				   hdcMem,
				   0, 0,
				   bitmap.bmWidth, bitmap.bmHeight,
				   SRCCOPY);
			}
			break;
		}

		// Clean up.
		//
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		unguard;
	}

	// WLevelFrame interface.
	virtual void OpenFrameViewport( INT RendMap, int X, int Y, int W, int H, DWORD ShowFlags )
	{
		guard(WLevelFrame::OpenFrameViewport);

		FName Name = TEXT("");

		// Open a viewport frame.
		WViewportFrame* pViewportFrame = NewViewportFrame( &Name );

		if( pViewportFrame ) 
		{
			pViewportFrame->OpenWindow();

			// Create the viewport inside of the frame.
			UViewport* Viewport = GEditor->Client->NewViewport( Name );
			Level->SpawnViewActor( Viewport );
			Viewport->Actor->ShowFlags = ShowFlags;
			Viewport->Actor->RendMap   = RendMap;
			Viewport->Input->Init( Viewport );
			pViewportFrame->SetViewport( Viewport );

			::MoveWindow( (HWND)pViewportFrame->hWnd, X, Y, W, H, 1 );
			::BringWindowToTop( pViewportFrame->hWnd );
		}

		unguard;
	}
private:

	TCHAR MapFilename[256];
};

/*-----------------------------------------------------------------------------
	WNewObject.
-----------------------------------------------------------------------------*/

// New object window.
class WNewObject : public WDialog
{
	DECLARE_WINDOWCLASS(WNewObject,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WListBox TypeList;
	WObjectProperties Props;
	UObject* Context;
	UObject* Result;
 
	// Constructor.
	WNewObject( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog		( TEXT("NewObject"), IDDIALOG_NewObject, InOwnerWindow )
	,	OkButton    ( this, IDOK,     FDelegate(this,(TDelegate)OnOk) )
	,	CancelButton( this, IDCANCEL, FDelegate(this,(TDelegate)EndDialogFalse) )
	,	TypeList	( this, IDC_TypeList )
	,	Props		( NAME_None, CPF_Edit, TEXT(""), this, 0 )
	,	Context     ( InContext )
	,	Result		( NULL )
	{
		Props.ShowTreeLines = 0;
		TypeList.DoubleClickDelegate=FDelegate(this,(TDelegate)OnOk);
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WNewObject::OnInitDialog);
		WDialog::OnInitDialog();
		for( TObjectIterator<UClass> It; It; ++It )
		{
			if( It->IsChildOf(UFactory::StaticClass()) )
			{
				UFactory* Default = (UFactory*)It->GetDefaultObject();
				if( Default->bCreateNew )
					TypeList.SetItemData( TypeList.AddString( *Default->Description ), *It );
			}
		}
		Props.OpenChildWindow( IDC_PropHolder );
		TypeList.SetCurrent( 0, 1 );
		TypeList.SelectionChangeDelegate = FDelegate(this,(TDelegate)OnSelChange);
		OnSelChange();
		unguard;
	}
	void OnDestroy()
	{
		guard(WNewObject::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual UObject* DoModal()
	{
		guard(WNewObject::DoModal);
		WDialog::DoModal( hInstance );
		return Result;
		unguard;
	}

	// Notifications.
	void OnSelChange()
	{
		guard(WNewObject::OnSelChange);
		INT Index = TypeList.GetCurrent();
		if( Index>=0 )
		{
			UClass*   Class   = (UClass*)TypeList.GetItemData(Index);
			UObject*  Factory = ConstructObject<UFactory>( Class );
			Props.Root.SetObjects( &Factory, 1 );
			EnableWindow( OkButton, 1 );
		}
		else
		{
			Props.Root.SetObjects( NULL, 0 );
			EnableWindow( OkButton, 0 );
		}
		unguard;
	}
	void OnOk()
	{
		guard(WNewObject::OnOk);
		if( Props.Root._Objects.Num() )
		{
			UFactory* Factory = CastChecked<UFactory>(Props.Root._Objects(0));
			Result = Factory->FactoryCreateNew( Factory->SupportedClass, NULL, NAME_None, 0, Context, GWarn );
			if( Result )
				EndDialogTrue();
		}
		unguard;
	}

	// WWindow interface.
	void Serialize( FArchive& Ar )
	{
		guard(WNewObject::Serialize);
		WDialog::Serialize( Ar );
		Ar << Context;
		for( INT i=0; i<TypeList.GetCount(); i++ )
		{
			UObject* Obj = (UClass*)TypeList.GetItemData(i);
			Ar << Obj;
		}
		unguard;
	}
};

void FileSaveAs( HWND hWnd )
{
	// Make sure we have a level loaded...
	if( !GLevelFrame ) { return; }

	OPENFILENAMEA ofn;
	char File[256], *pFilename;
	TCHAR l_chCmd[255];

	pFilename = TCHAR_TO_ANSI( GLevelFrame->GetMapFilename() );
	strcpy( File, pFilename );

	ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = File;
	ofn.nMaxFile = sizeof(char) * 256;
	char Filter[255];
  ::sprintf( Filter, 
        "Map Files (*.%s)%c*.%s%cAll Files%c*.*%c%c",
		appToAnsi( *GMapExt ), 
        '\0', 
        appToAnsi( *GMapExt ), 
        '\0', 
        '\0', 
        '\0', 
        '\0' );
	ofn.lpstrFilter = Filter;
	ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
	ofn.lpstrDefExt = appToAnsi( *GMapExt );
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	// Display the Open dialog box. 
	if( GetSaveFileNameA(&ofn) )
	{
		// Convert the ANSI filename to UNICODE, and tell the editor to open it.
		GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
		appSprintf( l_chCmd, TEXT("MAP SAVE FILE=%s"), ANSI_TO_TCHAR(File));
		GEditor->Exec( l_chCmd );

		// Save the filename.
		GLevelFrame->SetMapFilename( ANSI_TO_TCHAR(File) );

		FString S = ANSI_TO_TCHAR(File);
		GLastDir[eLASTDIR_UNR] = S.Left( S.InStr( TEXT("\\"), 1 ) );
	}

	GFileManager->SetDefaultDirectory(appBaseDir());
}

void FileSave( HWND hWnd )
{
	TCHAR l_chCmd[255];

	if( GLevelFrame ) {

		if( ::appStrlen( GLevelFrame->GetMapFilename() ) )
		{
			GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
			appSprintf( l_chCmd, TEXT("MAP SAVE FILE=%s"), GLevelFrame->GetMapFilename() );
			GEditor->Exec( l_chCmd );

			MRU_AddFilename( GLevelFrame->GetMapFilename() );
			MRU_AddToMenu();
		}
		else
			FileSaveAs( hWnd );
	}
}

void FileSaveChanges( HWND hWnd )
{
	// If a level has been loaded and there is something in the undo buffer, ask the user
	// if they want to save.
	if( GLevelFrame 
			&& GEditor->Trans->CanUndo() )
	{
		TCHAR l_chMsg[256];

		appSprintf( l_chMsg, TEXT("Save changes to %s?"), GLevelFrame->GetMapFilename() );

		if( ::MessageBox( hWnd, l_chMsg, TEXT("U2Ed"), MB_YESNO) == IDYES )
			FileSave( hWnd );
	}
}

enum eGI {
	eGI_NUM_SELECTED		= 1,
	eGI_CLASSNAME_SELECTED	= 2,
	eGI_NUM_SURF_SELECTED	= 4,
	eGI_CLASS_SELECTED		= 8
};

typedef struct tag_GetInfoRet {
	int iValue;
	FString String;
	UClass*	pClass;
} t_GetInfoRet;

t_GetInfoRet GetInfo( ULevel* Level, int Item )
{
	guard(GetInfo);

	t_GetInfoRet Ret;

	Ret.iValue = 0;
	Ret.String = TEXT("");

	// ACTORS
	if( Item & eGI_NUM_SELECTED
			|| Item & eGI_CLASSNAME_SELECTED 
			|| Item & eGI_CLASS_SELECTED )
	{
		int NumActors = 0;
		BOOL bAnyClass = FALSE;
		UClass*	AllClass = NULL;

		for( int i=0; i<Level->Actors.Num(); i++ )
		{
			if( Level->Actors(i) && Level->Actors(i)->bSelected )
			{
				if( bAnyClass && Level->Actors(i)->GetClass() != AllClass ) 
					AllClass = NULL;
				else 
					AllClass = Level->Actors(i)->GetClass();

				bAnyClass = TRUE;
				NumActors++;
			}
		}

		if( Item & eGI_NUM_SELECTED )
		{
			Ret.iValue = NumActors;
		}
		if( Item & eGI_CLASSNAME_SELECTED )
		{
			if( bAnyClass && AllClass )
				Ret.String = AllClass->GetName();
			else 
				Ret.String = TEXT("Actor");
		}
		if( Item & eGI_CLASS_SELECTED )
		{
			if( bAnyClass && AllClass )
				Ret.pClass = AllClass;
			else 
				Ret.pClass = NULL;
		}
	}

	// SURFACES
	if( Item & eGI_NUM_SURF_SELECTED)
	{
		int NumSurfs = 0;

		for( INT i=0; i<Level->Model->Surfs.Num(); i++ )
		{
			FBspSurf *Poly = &Level->Model->Surfs(i);

#if 1 //NEW: PolyFlagsEx
			if( Poly->PolyFlags[0] & PF_Selected )
#else
			if( Poly->PolyFlags & PF_Selected )
#endif
			{
				NumSurfs++;
			}
		}

		if( Item & eGI_NUM_SURF_SELECTED )
		{
			Ret.iValue = NumSurfs;
		}
	}

	return Ret;

	unguard;
}

void ShowBrowserTexture( WWindow* Parent )
{
	if( GBrowserTexture
			&& ::IsWindow( GBrowserTexture->hWnd ) )
	{
		GBrowserTexture->Show(1);
		::BringWindowToTop( GBrowserTexture->hWnd );
	}
	else 
	{
		delete GBrowserTexture;

		GBrowserTexture = new WBrowserTexture( TEXT("Texture Browser"), Parent );
		GBrowserTexture->OpenWindow();
	}
}

void ShowBrowserMesh( WWindow* Parent )
{
	if( GBrowserMesh
			&& ::IsWindow( GBrowserMesh->hWnd ) )
	{
		GBrowserMesh->Show(1);
		::BringWindowToTop( GBrowserMesh->hWnd );
	}
	else 
	{
		delete GBrowserMesh;

		GBrowserMesh = new WBrowserMesh( TEXT("Mesh Browser"), Parent );
		GBrowserMesh->OpenWindow();
	}
}

void ShowBrowserActor( WWindow* Parent )
{
	if( GBrowserActor
			&& ::IsWindow( GBrowserActor->hWnd ) )
	{
		GBrowserActor->Show(1);
		::BringWindowToTop( GBrowserActor->hWnd );
	}
	else 
	{
		delete GBrowserActor;

		GBrowserActor = new WBrowserActor( TEXT("Actor Browser"), Parent );
		GBrowserActor->OpenWindow();
	}
}

#if 1 //NEW: ParticleSystems
void ShowBrowserParticles( WWindow* Parent )
{
	if( GBrowserParticles
			&& ::IsWindow( GBrowserParticles->hWnd ) )
	{
		GBrowserParticles->Show(1);
		::BringWindowToTop( GBrowserParticles->hWnd );
	}
	else 
	{
		delete GBrowserParticles;

		GBrowserParticles = new WBrowserParticles( TEXT("Particle Browser"), Parent );
		GBrowserParticles->OpenWindow();
	}
}
#endif

void ShowBrowserSound( WWindow* Parent )
{
	if( GBrowserSound
			&& ::IsWindow( GBrowserSound->hWnd ) )
	{
		GBrowserSound->Show(1);
		::BringWindowToTop( GBrowserSound->hWnd );
	}
	else 
	{
		delete GBrowserSound;

		GBrowserSound = new WBrowserSound( TEXT("Sound Browser"), Parent );
		GBrowserSound->OpenWindow();
	}
}

void ShowBrowserMusic( WWindow* Parent )
{
	if( GBrowserMusic
			&& ::IsWindow( GBrowserMusic->hWnd ) )
	{
		GBrowserMusic->Show(1);
		::BringWindowToTop( GBrowserMusic->hWnd );
	}
	else 
	{
		delete GBrowserMusic;

		GBrowserMusic = new WBrowserMusic( TEXT("Music Browser"), Parent );
		GBrowserMusic->OpenWindow();
	}
}

void ShowCodeFrame( WWindow* Parent )
{
	if( GCodeFrame
			&& ::IsWindow( GCodeFrame->hWnd ) )
	{
		GCodeFrame->Show(1);
		::BringWindowToTop( GCodeFrame->hWnd );
	}
}


/*-----------------------------------------------------------------------------
	WEditorFrame.
-----------------------------------------------------------------------------*/
#define ID_TOOLBAR	29000
TBBUTTON tbButtons[] = {
	{ 0, ID_FileNew, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 1, ID_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, ID_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, IDMN_EDIT_SEARCH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 4, ID_BrowserTexture, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 5, ID_BrowserActor, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, ID_BrowserMesh, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 7, ID_BrowserSound, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 8, ID_BrowserMusic, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 9, IDMN_CODE_FRAME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 10, ID_Tools2DEditor, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 11, ID_BuildGeometry, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 12, ID_BuildLighting, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 13, ID_BuildPaths, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 14, ID_BuildOptions, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 15, ID_BuildPlay, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_Main[] = {
	TEXT("New"), ID_FileNew,
	TEXT("Open"), ID_FileOpen,
	TEXT("Save"), ID_FileSave,
	TEXT("Search for Actors"), IDMN_EDIT_SEARCH,
	TEXT("Texture Browser"), ID_BrowserTexture,
	TEXT("Actor Browser"), ID_BrowserActor,
	TEXT("Mesh Viewer"), ID_BrowserMesh,
	TEXT("Sound Browser"), ID_BrowserSound,
	TEXT("Music Browser"), ID_BrowserMusic,
	TEXT("UnrealScript Editing Window"), IDMN_CODE_FRAME,
	TEXT("2D Shape Editor"), ID_Tools2DEditor,
	TEXT("Compute Geometry"), ID_BuildGeometry,
	TEXT("Compute Lighting"), ID_BuildLighting,
	TEXT("Compute Paths"), ID_BuildPaths,
	TEXT("Rebuild Options"), ID_BuildOptions,
	TEXT("Play Map"), ID_BuildPlay,
	NULL, 0
};
HWND GHwndToolbar = NULL;
HWND GhWndTT = NULL;
void CreateToolbar( HWND hwndParent )
{
	if( GHwndToolbar )
		DestroyWindow(GHwndToolbar);

	GHwndToolbar = CreateToolbarEx( 
		hwndParent, WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
		ID_TOOLBAR,
		16,
		hInstance,
		IDB_TOOLBAR,
		(LPCTBBUTTON)&tbButtons,
		21,
		16,16,
		16,16,
		sizeof(TBBUTTON));
	check( GHwndToolbar );

	// TOOLTIPS
	GhWndTT = CreateWindowA(TOOLTIPS_CLASSA, NULL, TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, (HMENU) NULL, hInstance, NULL);

	if( !GhWndTT )
		appMsgf( TEXT("Tooltip control not created!") );
	else
	{
		int ID = 900;
		TOOLINFO ti;
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS;
		ti.hwnd = GHwndToolbar;
		ti.hinst = hInstance;

		for( int tooltip = 0 ; ToolTips_Main[tooltip].ID > 0 ; tooltip++ )
		{
			// Figure out the rectangle for the toolbar button.
			int index = SendMessageX( GHwndToolbar, TB_COMMANDTOINDEX, ToolTips_Main[tooltip].ID, 0 );
			RECT rect;
			SendMessageX( GHwndToolbar, TB_GETITEMRECT, index, (LPARAM)&rect);

			// Add tooltip to tooltip control
			ti.uId = ID + tooltip;
			ti.lpszText = ToolTips_Main[tooltip].ToolTip;
			ti.rect = rect;

			SendMessageX(GhWndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&ti);
		}
	}
}

#define MRU_MAX_FILES 8

// Editor frame window.
class WEditorFrame : public WMdiFrame, public FNotifyHook, public FDocumentManager
{
	DECLARE_WINDOWCLASS(WEditorFrame,WMdiFrame,UnrealEd)

	// Variables.
	WBackgroundHolder BackgroundHolder;
	WConfigProperties* Preferences;

	TArray<FString> MRUFilenames;

	// Constructors.
	WEditorFrame()
	: WMdiFrame( TEXT("EditorFrame") )
	, BackgroundHolder( NAME_None, &MdiClient )
	, Preferences( NULL )
	{ }

	// WWindow interface.
	void OnCreate()
	{
		guard(WEditorFrame::OnCreate);
		WMdiFrame::OnCreate();
		SetText( *FString::Printf( LocalizeGeneral(TEXT("FrameWindow"),TEXT("U2Ed")), LocalizeGeneral(TEXT("Product"),TEXT("Core"))) );

		// Create MDI client.
		CLIENTCREATESTRUCT ccs;
        ccs.hWindowMenu = NULL; 
        ccs.idFirstChild = 60000;
		MdiClient.OpenWindow( &ccs );

		// Background.
		BackgroundHolder.OpenWindow();

		NE_EdStarting();
		NE_EdInitServer( hWnd, hWnd );

		// Set up progress dialog.
		GDlgProgress = new WDlgProgress( NULL, this );
		GDlgProgress->DoModeless();

		Warn.hWndProgressBar = (DWORD)::GetDlgItem( GDlgProgress->hWnd, IDPG_PROGRESS);
		Warn.hWndProgressText = (DWORD)::GetDlgItem( GDlgProgress->hWnd, IDSC_MSG);
		Warn.hWndProgressDlg = (DWORD)GDlgProgress->hWnd;

		GDlgSearchActors = new WDlgSearchActors( NULL, this );
		GDlgSearchActors->DoModeless();
		GDlgSearchActors->Show(0);

		GDlgScaleLights = new WDlgScaleLights( NULL, this );
		GDlgScaleLights->DoModeless();
		GDlgScaleLights->Show(0);

		GEditorFrame = this;

		// MRU setup.
		MRU_ReadINI();
		PostMessageX( hWnd, WM_COMMAND, WM_MRU_INITMENU, 0 );

		unguard;
	}
	virtual void OnTimer()
	{
		guard(WEditorFrame::OnTimer);
		GEditor->Exec( TEXT("MAYBEAUTOSAVE") );
		unguard;
	}
	void RepositionClient()
	{
		guard(WEditorFrame::RepositionClient);
		WMdiFrame::RepositionClient();
		BackgroundHolder.MoveWindow( MdiClient.GetClientRect(), 1 );
		unguard;
	}
	void OnClose()
	{
		guard(WEditorFrame::OnClose);

		KillTimer( hWnd, 900 );

		MRU_WriteINI();
		MRUFilenames.Empty();

		delete GSurfPropSheet;
		delete GBuildSheet;
		delete G2DShapeEditor;
		delete GBrowserSound;
		delete GBrowserMusic;
		delete GBrowserActor;
#if 1 //NEW: ParticleSystems
		delete GBrowserParticles;
#endif
		delete GBrowserTexture;
		delete GBrowserMesh;
		delete GCodeFrame;
		delete GDlgAddSpecial;
		delete GDlgScaleLights;
		delete GDlgProgress;
		delete GDlgSearchActors;

		appRequestExit( 0 );
		WMdiFrame::OnClose();
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(WEditorFrame::OnCommand);
		TCHAR l_chCmd[255];

		switch( Command )
		{

			case WM_MRU_INITMENU:
				{
					// Doing this here allows us to set up the MRU list, and at the same time,
					// destroy the system menu of the MDI client -- which means the user can't close
					// the MDI client and break the editor.
					MRU_AddToMenu();
				}
				break;

			case IDC_FileNew:
			{
				FileSaveChanges( hWnd );
				WNewObject Dialog( NULL, this );
				UObject* Result = Dialog.DoModal();
				if( Cast<ULevel>(Result) )
				{
					GLevelFrame->SetMapFilename( TEXT("") );
					OpenLevelView();
				}
			}
			break;

			case ID_FILE_IMPORT:
			{
				OPENFILENAMEA ofn;
				ANSICHAR File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Import Map";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				// Display the Open dialog box. 
				if( GetOpenFileNameA(&ofn) )
				{
					WDlgMapImport l_dlg( this );
					if( l_dlg.DoModal( appFromAnsi( File ) ) )
					{
						GWarn->BeginSlowTask( TEXT("Importing Map"), 1, 0 );
						TCHAR l_chCmd[256];
						if( l_dlg.bNewMapCheck )
							appSprintf( l_chCmd, TEXT("MAP IMPORTADD FILE=%s"), appFromAnsi( File ) );
						else
						{
							GLevelFrame->SetMapFilename( TEXT("") );
							OpenLevelView();
							appSprintf( l_chCmd, TEXT("MAP IMPORT FILE=%s"), appFromAnsi( File ) );
						}
						GEditor->Exec( l_chCmd );
						GWarn->EndSlowTask();
						GEditor->RedrawLevel( GEditor->Level );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_UNR] = S.Left( S.InStr( TEXT("\\"), 1 ) );
					}
				}

				GFileManager->SetDefaultDirectory(appBaseDir());

				GBuildSheet->RefreshStats();
			}
			break;

			case ID_FILE_EXPORT:
			{
				OPENFILENAMEA ofn;
				char File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Export Map";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				if( GetSaveFileNameA(&ofn) )
				{
					GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
					GEditor->Exec( *(FString::Printf(TEXT("MAP EXPORT FILE=%s"), appFromAnsi( File ))));

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_UNR] = S.Left( S.InStr( TEXT("\\"), 1 ) );
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
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
				GLevelFrame->SetMapFilename( (TCHAR*)(*(MRUFilenames( Command - IDMN_MRU1 ) ) ) );
				GEditor->Exec( *(FString::Printf(TEXT("MAP LOAD FILE=%s"), *MRUFilenames( Command - IDMN_MRU1 ) )) );
				GBuildSheet->RefreshStats();
			}
			break;

			case IDMN_LOAD_BACK_IMAGE:
			{
				OPENFILENAMEA ofn;
				char File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Bitmaps (*.bmp)\0*.bmp\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = "..\\maps";
				ofn.lpstrTitle = "Open Image";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UTX]) );
				ofn.lpstrDefExt = "bmp";
				ofn.Flags = OFN_NOCHANGEDIR;

				// Display the Open dialog box. 
				//
				if( GetOpenFileNameA(&ofn) )
				{
					GLevelFrame->LoadBackgroundImage(appFromAnsi( File ));

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_UTX] = S.Left( S.InStr( TEXT("\\"), 1 ) );
				}

				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_CLEAR_BACK_IMAGE:
			{
				::DeleteObject( GLevelFrame->hImage );
				GLevelFrame->hImage = NULL;
				GLevelFrame->BIFilename = TEXT("");
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_BI_CENTER:
			{
				GLevelFrame->BIMode = eBIMODE_CENTER;
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_BI_TILE:
			{
				GLevelFrame->BIMode = eBIMODE_TILE;
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case IDMN_BI_STRETCH:
			{
				GLevelFrame->BIMode = eBIMODE_STRETCH;
				InvalidateRect( GLevelFrame->hWnd, NULL, FALSE );
			}
			break;

			case ID_FileOpen:
			{
				FileOpen( hWnd );
			}
			break;

			case ID_FileClose:
			{
				FileSaveChanges( hWnd );

				if( GLevelFrame )
				{
					GLevelFrame->_CloseWindow();
					delete GLevelFrame;
					GLevelFrame = NULL;
				}
			}
			break;

			case ID_FileSave:
			{
				FileSave( hWnd );
			}
			break;

			case ID_FileSaveAs:
			{
				FileSaveAs( hWnd );
			}
			break;

			case ID_BrowserTexture:
			{
				ShowBrowserTexture( this );
			}
			break;

			case ID_BrowserMesh:
			{
				ShowBrowserMesh( this );
			}
			break;

			case ID_BrowserActor:
			{
				ShowBrowserActor( this );
			}
			break;

#if 1 //NEW: ParticleSystems
			case ID_BrowserParticles:
			{
				ShowBrowserParticles( this );
			}
			break;
#endif

			case ID_BrowserSound:
			{
				ShowBrowserSound( this );
			}
			break;

			case ID_BrowserMusic:
			{
				ShowBrowserMusic( this );
			}
			break;

			case IDMN_CODE_FRAME:
			{
				ShowBrowserActor( this );
				ShowCodeFrame( this );
			}
			break;

			case ID_FileExit:
			{
				OnClose();
			}
			break;

			case ID_EditUndo:
			{
				GEditor->Exec( TEXT("TRANSACTION UNDO") );
			}
			break;

			case ID_EditRedo:
			{
				GEditor->Exec( TEXT("TRANSACTION REDO") );
			}
			break;

			case ID_EditDuplicate:
			{
				GEditor->Exec( TEXT("DUPLICATE") );
			}
			break;

			case IDMN_EDIT_SEARCH:
			{
				GDlgSearchActors->Show(1);
			}
			break;

			case IDMN_EDIT_SCALE_LIGHTS:
			{
				GDlgScaleLights->Show(1);
			}
			break;

			case ID_EditDelete:
			{
				GEditor->Exec( TEXT("DELETE") );
			}
			break;

			case ID_EditCut:
			{
				GEditor->Exec( TEXT("EDIT CUT") );
			}
			break;

			case ID_EditCopy:
			{
				GEditor->Exec( TEXT("EDIT COPY") );
			}
			break;

			case ID_EditPaste:
			{
				GEditor->Exec( TEXT("EDIT PASTE") );
			}
			break;

			case ID_EditSelectNone:
			{
				GEditor->Exec( TEXT("SELECT NONE") );
			}
			break;

			case ID_EditSelectAllActors:
			{
				GEditor->Exec( TEXT("ACTOR SELECT ALL") );
			}
			break;

			case ID_EditSelectAllSurfs:
			{
				GEditor->Exec( TEXT("POLY SELECT ALL") );
			}
			break;

			case ID_ViewActorProp:
			{
				if( !GEditor->ActorProperties )
				{
					GEditor->ActorProperties = new WObjectProperties( TEXT("ActorProperties"), CPF_Edit, TEXT(""), NULL, 1 );
					GEditor->ActorProperties->OpenWindow( hWnd );
					GEditor->ActorProperties->SetNotifyHook( GEditor );
				}
				GEditor->UpdatePropertiesWindows();
				GEditor->ActorProperties->Show(1);
			}
			break;

			case ID_ViewSurfaceProp:
			{
				GSurfPropSheet->Show( TRUE );
			}
			break;

			case ID_ViewLevelProp:
			{
				if( !GEditor->LevelProperties )
				{
					GEditor->LevelProperties = new WObjectProperties( TEXT("LevelProperties"), CPF_Edit, TEXT("Level Properties"), NULL, 1 );
					GEditor->LevelProperties->OpenWindow( hWnd );
					GEditor->LevelProperties->SetNotifyHook( GEditor );
				}
				GEditor->LevelProperties->Root.SetObjects( (UObject**)&GEditor->Level->Actors(0), 1 );
				GEditor->LevelProperties->Show(1);
			}
			break;

			case ID_BrushAdd:
			{
				GEditor->Exec( TEXT("BRUSH ADD") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushSubtract:
			{
				GEditor->Exec( TEXT("BRUSH SUBTRACT") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushIntersect:
			{
				GEditor->Exec( TEXT("BRUSH FROM INTERSECTION") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushDeintersect:
			{
				GEditor->Exec( TEXT("BRUSH FROM DEINTERSECTION") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushAddMover:
			{
				GEditor->Exec( TEXT("BRUSH ADDMOVER") );
				GEditor->RedrawLevel( GEditor->Level );
			}
			break;

			case ID_BrushAddSpecial:
			{
				delete GDlgAddSpecial;

				GDlgAddSpecial = new WDlgAddSpecial( NULL, GEditorFrame );
				GDlgAddSpecial->DoModeless();
			}
			break;

			case ID_BrushOpen:
			{
				OPENFILENAMEA ofn;
				char File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Brushes (*.u3d)\0*.u3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = "..\\maps";
				ofn.lpstrDefExt = "u3d";
				ofn.lpstrTitle = "Open Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				// Display the Open dialog box. 
				if( GetOpenFileNameA(&ofn) )
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH LOAD FILE=%s"), appFromAnsi( File ))));
					GEditor->RedrawLevel( GEditor->Level );
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_BrushSaveAs:
			{
				OPENFILENAMEA ofn;
				char File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Brushes (*.u3d)\0*.u3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = "..\\maps";
				ofn.lpstrDefExt = "u3d";
				ofn.lpstrTitle = "Save Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				if( GetSaveFileNameA(&ofn) )
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH SAVE FILE=%s"), appFromAnsi( File ))));
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_BRUSH_IMPORT:
			{
				OPENFILENAMEA ofn;
				char File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Import Types (*.t3d, *.dxf, *.asc)\0*.t3d;*.dxf;*.asc;\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_BRUSH]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Import Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				// Display the Open dialog box. 
				if( GetOpenFileNameA(&ofn) )
				{
					WDlgBrushImport l_dlg( NULL, this );
					l_dlg.DoModal( appFromAnsi( File ) );
					GEditor->RedrawLevel( GEditor->Level );

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_BRUSH] = S.Left( S.InStr( TEXT("\\"), 1 ) );
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_BRUSH_EXPORT:
			{
				OPENFILENAMEA ofn;
				char File[256] = "\0";

				ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = File;
				ofn.nMaxFile = sizeof(char) * 256;
				ofn.lpstrFilter = "Unreal Text (*.t3d)\0*.t3d\0All Files\0*.*\0\0";
				ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_BRUSH]) );
				ofn.lpstrDefExt = "t3d";
				ofn.lpstrTitle = "Export Brush";
				ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

				if( GetSaveFileNameA(&ofn) )
				{
					GEditor->Exec( *(FString::Printf(TEXT("BRUSH EXPORT FILE=%s"), appFromAnsi( File ))));

					FString S = appFromAnsi( File );
					GLastDir[eLASTDIR_BRUSH] = S.Left( S.InStr( TEXT("\\"), 1 ) );
				}

				GFileManager->SetDefaultDirectory(appBaseDir());
			}
			break;

			case ID_BuildPlay:
			{
				GEditor->Exec( TEXT("HOOK PLAYMAP") );
			}
			break;

			case ID_BuildGeometry:
			{
				GEditor->Exec( TEXT("MAP REBUILD") );
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildLighting:
			{
				GEditor->Exec( TEXT("LIGHT APPLY SELECTED=off") );
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildPaths:
			{
				GEditor->Exec( TEXT("PATHS DEFINE") );
				GBuildSheet->RefreshStats();
			}
			break;

			case ID_BuildOptions:
			{
				GBuildSheet->Show( TRUE );
			}
			break;

			case ID_ToolsLog:
			{
				if( GLogWindow )
				{
					GLogWindow->Show(1);
					SetFocus( *GLogWindow );
					GLogWindow->Display.ScrollCaret();
				}
			}
			break;

			case ID_Tools2DEditor:
			{
				delete G2DShapeEditor;

				G2DShapeEditor = new W2DShapeEditor( TEXT("2D Shape Editor"), this );
				G2DShapeEditor->OpenWindow();
			}
			break;

			case ID_ViewNewFree:
			{
				GLevelFrame->OpenFrameViewport( REN_OrthXY, 0, 0, 320, 200, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes );
			}
			break;

			case IDMN_VIEWPORT_CLASSIC:
			{
				// Get rid of any existing viewports.
				for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
				{
					delete GLevelFrame->m_pViewportFrames[x];
					GLevelFrame->m_pViewportFrames[x] = NULL;
				}

				RECT R;
				::GetClientRect( GLevelFrame->hWnd, &R );
				float fFactor = R.right / 10, fLeftSide = 6.5;

				GLevelFrame->OpenFrameViewport( REN_OrthXY,		R.left,					R.top,			fFactor * fLeftSide,				R.bottom / 2, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes );
				GLevelFrame->OpenFrameViewport( REN_OrthXZ,		fFactor * fLeftSide,	R.top,			R.right - (fFactor * fLeftSide),	R.bottom / 2, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes );
				GLevelFrame->OpenFrameViewport( REN_DynLight,	R.left,					R.bottom / 2,	fFactor * fLeftSide,				R.bottom / 2, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes );
				GLevelFrame->OpenFrameViewport( REN_OrthYZ,		fFactor * fLeftSide,	R.bottom / 2,	R.right - (fFactor * fLeftSide),	R.bottom / 2, SHOW_Menu | SHOW_Frame | SHOW_Actors | SHOW_Brush | SHOW_StandardView | SHOW_ChildWindow | SHOW_MovingBrushes );
			}
			break;

			case ID_ToolsPrefs:
			{
				if( !Preferences )
				{
					Preferences = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral(TEXT("AdvancedOptionsTitle"),TEXT("Window")) );
					Preferences->OpenWindow( *this );
					Preferences->SetNotifyHook( this );
					Preferences->ForceRefresh();
				}
				Preferences->Show(1);
			}
			break;

			case WM_SHOW_FINDREPLACE:
			{
				WDlgFindReplace l_dlg( NULL, GCodeFrame );
				l_dlg.DoModal();
			}
			break;

			case WM_FINDREPLACE_NEXT:
			{
				WDlgFindReplace l_dlg( NULL, GCodeFrame );
				if( GLastText.Len() )
					l_dlg.OnFindNext();
				else
				{
					l_dlg.DoModal();
				}
			}
			break;

			case WM_FINDREPLACE_PREV:
			{
				WDlgFindReplace l_dlg( NULL, GCodeFrame );
				if( GLastText.Len() )
					l_dlg.OnFindPrev();
				else
				{
					l_dlg.DoModal();
				}
			}
			break;

			case WM_EDC_SAVEMAP:
			{
				FileSave( hWnd );
			}
			break;

			case WM_EDC_CAMMODECHANGE:
			{
				if( GToolBar )
				{
					GEditor->Exec( TEXT("BRUSHCLIP DELETE") );
					InvalidateRect( GToolBar->List, NULL, 0 );
					UpdateWindow( GToolBar->List );
				}
			}
			break;

			case WM_EDC_LOADMAP:
			{
				FileOpen( hWnd );
			}
			break;

			case WM_EDC_PLAYMAP:
			{
				GEditor->Exec( TEXT("HOOK PLAYMAP") );
			}
			break;

			case WM_EDC_BROWSE:
			{
				*GetPropResult = FStringOutputDevice();
				GEditor->Get( TEXT("OBJ"), TEXT("BROWSECLASS"), *GetPropResult );

				if( !appStrcmp( **GetPropResult, TEXT("Texture") ) )
					ShowBrowserTexture( this );

				if( !appStrcmp( **GetPropResult, TEXT("Sound") ) )
					ShowBrowserSound( this );

				if( !appStrcmp( **GetPropResult, TEXT("Music") ) )
					ShowBrowserMusic( this );

				if( !appStrcmp( **GetPropResult, TEXT("Class") ) )
					ShowBrowserActor( this );
#if 1 //NEW: ParticleSystems
				if( !appStrcmp( **GetPropResult, TEXT("Particle") ) )
					ShowBrowserParticles( this );
#endif
				if( !appStrcmp( **GetPropResult, TEXT("Mesh") ) )
					ShowBrowserMesh( this );
					
			}
			break;

			case WM_EDC_USECURRENT:
			{
				*GetPropResult = FStringOutputDevice();
				GEditor->Get( TEXT("OBJ"), TEXT("BROWSECLASS"), *GetPropResult );

				FString Cur;

				if( !appStrcmp( **GetPropResult, TEXT("Texture") ) )
					if( GEditor->CurrentTexture )
						Cur = GEditor->CurrentTexture->GetPathName();

				if( !appStrcmp( **GetPropResult, TEXT("Sound") ) )
					if( GBrowserSound )
						Cur = *GBrowserSound->GetCurrentPathName();

				if( !appStrcmp( **GetPropResult, TEXT("Music") ) )
					if( GBrowserMusic )
						Cur = *GBrowserMusic->GetCurrentPathName();

				if( !appStrcmp( **GetPropResult, TEXT("Class") ) )
					if( GEditor->CurrentClass )
						Cur = GEditor->CurrentClass->GetPathName();
#if 1 //NEW: ParitcleSystems
				if( !appStrcmp( **GetPropResult, TEXT("ParticleGenerator") ) )
					if( GEditor->CurrentParticleClass )
						Cur = GEditor->CurrentParticleClass->GetPathName();
#endif
				if( !appStrcmp( **GetPropResult, TEXT("Mesh") ) )
					if( GBrowserMesh )
						Cur = GBrowserMesh->GetCurrentMeshName();

				if( Cur.Len() )
					GEditor->Set( TEXT("OBJ"), TEXT("NOTECURRENT"), *(FString::Printf(TEXT("CLASS=%s OBJECT=%s"), **GetPropResult, *Cur)));
			}
			break;

			case WM_EDC_CURTEXCHANGE:
			{
				if( GBrowserTexture ) GBrowserTexture->SetCaption();
			}
			break;

			case WM_EDC_SELPOLYCHANGE:
			{
				GSurfPropSheet->GetDataFromSurfs1();
				GSurfPropSheet->GetDataFromSurfs2();
				GSurfPropSheet->RefreshStats();
			}
			break;

			case WM_EDC_SELCHANGE:
			{
				GSurfPropSheet->GetDataFromSurfs1();
				GSurfPropSheet->GetDataFromSurfs2();
				GSurfPropSheet->RefreshStats();
			}
			break;

			case WM_EDC_RTCLICKTEXTURE:
			{
				POINT pt;
				HMENU menu = GetSubMenu( LoadMenuIdX(hInstance, IDMENU_BrowserTexture_Context), 0 );
				::GetCursorPos( &pt );
				TrackPopupMenu( menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					pt.x, pt.y, 0,
					GBrowserTexture->hWnd, NULL);
			}
			break;

			case WM_EDC_RTCLICKPOLY:
			{
				POINT l_point;

				::GetCursorPos( &l_point );
				HMENU l_menu = GetSubMenu( LoadMenuIdX(hInstance, IDMENU_SurfPopup), 0 );

				// Customize the menu options we need to.
				MENUITEMINFOA mif;
				char l_ch[255];

				mif.cbSize = sizeof(MENUITEMINFO);
				mif.fMask = MIIM_TYPE;
				mif.fType = MFT_STRING;

				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_NUM_SURF_SELECTED );

				sprintf( l_ch, "Surface &Properties (%i Selected)\tF5", gir.iValue );
				mif.dwTypeData = l_ch;
				SetMenuItemInfoA( l_menu, ID_SurfProperties, FALSE, &mif );

				if( GEditor->CurrentClass )
				{
					sprintf( l_ch, "&Add %s Here", TCHAR_TO_ANSI( GEditor->CurrentClass->GetName() ) );
					mif.dwTypeData = l_ch;
					SetMenuItemInfoA( l_menu, ID_SurfPopupAddClass, FALSE, &mif );
				}
				else {

					DeleteMenu( l_menu, ID_SurfPopupAddClass, MF_BYCOMMAND );
				}
#if 1 //NEW: ParticleSystems
				if( GEditor->CurrentParticleClass )
				{
					sprintf( l_ch, "&Add %s Here", TCHAR_TO_ANSI( GEditor->CurrentParticleClass->GetFullName() ) );
					mif.dwTypeData = l_ch;
					SetMenuItemInfoA( l_menu, ID_SurfPopupAddParticleClass, FALSE, &mif );
				}
				else {

					DeleteMenu( l_menu, ID_SurfPopupAddParticleClass, MF_BYCOMMAND );
				}
#endif

				TrackPopupMenu( l_menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					l_point.x, l_point.y, 0,
					hWnd, NULL);
			}
			break;

			case WM_EDC_RTCLICKACTOR:
			{
				POINT l_point;

				::GetCursorPos( &l_point );
				HMENU l_menu = GetSubMenu( LoadMenuIdX(hInstance, IDMENU_ActorPopup), 0 );

				// Customize the menu options we need to.
				MENUITEMINFOA mif;
				char l_ch[255];

				mif.cbSize = sizeof(MENUITEMINFO);
				mif.fMask = MIIM_TYPE;
				mif.fType = MFT_STRING;

				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_NUM_SELECTED | eGI_CLASSNAME_SELECTED | eGI_CLASS_SELECTED );

				sprintf( l_ch, "%s &Properties (%i Selected)", TCHAR_TO_ANSI( *gir.String ), gir.iValue );
				mif.dwTypeData = l_ch;
				SetMenuItemInfoA( l_menu, IDMENU_ActorPopupProperties, FALSE, &mif );

				sprintf( l_ch, "&Select All %s", TCHAR_TO_ANSI( *gir.String ) );
				mif.dwTypeData = l_ch;
				SetMenuItemInfoA( l_menu, IDMENU_ActorPopupSelectAllClass, FALSE, &mif );

				EnableMenuItem( l_menu, IDMENU_ActorPopupEditScript, (gir.pClass == NULL) );
				EnableMenuItem( l_menu, IDMENU_ActorPopupMakeCurrent, (gir.pClass == NULL) );

				TrackPopupMenu( l_menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					l_point.x, l_point.y, 0,
					hWnd, NULL);
			}
			break;

			case WM_EDC_RTCLICKWINDOW:
			case WM_EDC_RTCLICKWINDOWCANADD:
			{
				POINT l_point;

				::GetCursorPos( &l_point );
				HMENU l_menu = GetSubMenu( LoadMenuIdX(hInstance, IDMENU_BackdropPopup), 0 );

				// Customize the menu options we need to.
				MENUITEMINFOA mif;
				char l_ch[255];

				mif.cbSize = sizeof(MENUITEMINFO);
				mif.fMask = MIIM_TYPE;
				mif.fType = MFT_STRING;

				if( GEditor->CurrentClass )
				{
					sprintf( l_ch, "&Add %s here", TCHAR_TO_ANSI( GEditor->CurrentClass->GetName() ) );
					mif.dwTypeData = l_ch;
					SetMenuItemInfoA( l_menu, ID_BackdropPopupAddClassHere, FALSE, &mif );
				}
				else {

					DeleteMenu( l_menu, ID_BackdropPopupAddClassHere, MF_BYCOMMAND );
				}
#if 1 //NEW: ParticleSystems
				if( GEditor->CurrentParticleClass )
				{
					sprintf( l_ch, "&Add %s here", TCHAR_TO_ANSI( GEditor->CurrentParticleClass->GetFullName() ) );
					mif.dwTypeData = l_ch;
					SetMenuItemInfoA( l_menu, ID_BackdropPopupAddParticleGeneratorHere, FALSE, &mif );
				}
				else {

					DeleteMenu( l_menu, ID_BackdropPopupAddParticleGeneratorHere, MF_BYCOMMAND );
				}
#endif

				TrackPopupMenu( l_menu,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					l_point.x, l_point.y, 0,
					hWnd, NULL);
			}
			break;

			case WM_EDC_MAPCHANGE:
			{
			}
			break;

			case WM_EDC_VIEWPORTUPDATEWINDOWFRAME:
			{
				for( int x = 0 ; x < dED_MAX_VIEWPORTS ; x++)
					if( GLevelFrame->m_pViewportFrames[x] && ::IsWindow( GLevelFrame->m_pViewportFrames[x]->hWnd ) )
						GLevelFrame->m_pViewportFrames[x]->SetCaption();
			}
			break;

			case WM_EDC_SURFPROPS:
			{
				GSurfPropSheet->Show( TRUE );
			}
			break;

			//
			// BACKDROP POPUP
			//

			// Root
			case ID_BackdropPopupAddClassHere:
			{
				GEditor->Exec( *(FString::Printf( TEXT("ACTOR ADD CLASS=%s"), GEditor->CurrentClass->GetName() ) ) );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;
#if 1 //NEW: ParticleSystems
			case ID_BackdropPopupAddParticleGeneratorHere:
			{
				FString S = GEditor->CurrentParticleClass->GetFullName();
				GEditor->Exec( *(FString::Printf( TEXT("ACTOR PARTICLESYSTEMS NEW IMAGE=%s"), S.Mid(S.InStr(TEXT(" "))+1) ) ) );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;
#endif
			case ID_BackdropPopupAddLightHere:
			{
				GEditor->Exec( TEXT("ACTOR ADD CLASS=LIGHT") );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_BackdropPopupLevelProperties:
			{
				if( !GEditor->LevelProperties )
				{
					GEditor->LevelProperties = new WObjectProperties( TEXT("LevelProperties"), CPF_Edit, TEXT("Level Properties"), NULL, 1 );
					GEditor->LevelProperties->OpenWindow( hWnd );
					GEditor->LevelProperties->SetNotifyHook( GEditor );
				}
				GEditor->LevelProperties->Root.SetObjects( (UObject**)&GEditor->Level->Actors(0), 1 );
				GEditor->LevelProperties->Show(1);
			}
			break;

		// Grid
			case ID_BackdropPopupGrid1:
			{
				GEditor->Exec( TEXT("MAP GRID X=1 Y=1 Z=1") );
			}
			break;

			case ID_BackdropPopupGrid2:
			{
				GEditor->Exec( TEXT("MAP GRID X=2 Y=2 Z=2") );
			}
			break;

			case ID_BackdropPopupGrid4:
			{
				GEditor->Exec( TEXT("MAP GRID X=4 Y=4 Z=4") );
			}
			break;

			case ID_BackdropPopupGrid8:
			{
				GEditor->Exec( TEXT("MAP GRID X=8 Y=8 Z=8") );
			}
			break;

			case ID_BackdropPopupGrid16:
			{
				GEditor->Exec( TEXT("MAP GRID X=16 Y=16 Z=16") );
			}
			break;

			case ID_BackdropPopupGrid32:
			{
				GEditor->Exec( TEXT("MAP GRID X=32 Y=32 Z=32") );
			}
			break;

			case ID_BackdropPopupGrid64:
			{
				GEditor->Exec( TEXT("MAP GRID X=64 Y=64 Z=64") );
			}
			break;

			case ID_BackdropPopupGrid128:
			{
				GEditor->Exec( TEXT("MAP GRID X=128 Y=128 Z=128") );
			}
			break;

			case ID_BackdropPopupGrid256:
			{
				GEditor->Exec( TEXT("MAP GRID X=256 Y=256 Z=256") );
			}
			break;

			// Pivot
			case ID_BackdropPopupPivotSnapped:
			{
				GEditor->Exec( TEXT("PIVOT SNAPPED") );
			}
			break;

			case ID_BackdropPopupPivot:
			{
				GEditor->Exec( TEXT("PIVOT HERE") );
			}
			break;

			//
			// SURFACE POPUP MENU
			//

			// Root
			case ID_SurfProperties:
			{
				GSurfPropSheet->Show( TRUE );
			}
			break;

			case ID_SurfPopupAddClass:
			{
				if( GEditor->CurrentClass )
				{
					GEditor->Exec( *(FString::Printf(TEXT("ACTOR ADD CLASS=%s"), GEditor->CurrentClass->GetName())));
					GEditor->Exec( TEXT("POLY SELECT NONE") );
				}
			}
			break;
#if 1 //NEW: ParticleSystems
			case ID_SurfPopupAddParticleClass:
			{
				if( GEditor->CurrentParticleClass )
				{
					FString S = GEditor->CurrentParticleClass->GetFullName();
					GEditor->Exec( *(FString::Printf( TEXT("ACTOR PARTICLESYSTEMS NEW IMAGE=%s"), S.Mid(S.InStr(TEXT(" "))+1) ) ) );
					GEditor->Exec( TEXT("POLY SELECT NONE") );
				}
			}
			break;
#endif
			case ID_SurfPopupAddLight:
			{
				GEditor->Exec( TEXT("ACTOR ADD CLASS=LIGHT") );
				GEditor->Exec( TEXT("POLY SELECT NONE") );
			}
			break;

			case ID_SurfPopupApplyTexture:
			{
				GEditor->Exec( TEXT("POLY SETTEXTURE") );
			}
			break;

			// Align Selected
			case ID_SurfPopupAlignFloor:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN FLOOR") );
			}
			break;

			case ID_SurfPopupAlignWallDirection:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN WALLDIR") );
			}
			break;

			case ID_SurfPopupAlignWallPanning:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN WALLPAN") );
			}
			break;

			case ID_SurfPopupUnalign:
			{
				GEditor->Exec( TEXT("POLY TEXALIGN DEFAULT") );
			}
			break;

			// Select Surfaces
			case ID_SurfPopupSelectMatchingGroups:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING GROUPS") );
			}
			break;

			case ID_SurfPopupSelectMatchingItems:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING ITEMS") );
			}
			break;

			case ID_SurfPopupSelectMatchingBrush:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING BRUSH") );
			}
			break;

			case ID_SurfPopupSelectMatchingTexture:
			{
				GEditor->Exec( TEXT("POLY SELECT MATCHING TEXTURE") );
			}
			break;

			case ID_SurfPopupSelectAllAdjacents:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT ALL") );
			}
			break;

			case ID_SurfPopupSelectAdjacentCoplanars:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT COPLANARS") );
			}
			break;

			case ID_SurfPopupSelectAdjacentWalls:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT WALLS") );
			}
			break;

			case ID_SurfPopupSelectAdjacentFloors:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT FLOORS") );
			}
			break;

			case ID_SurfPopupSelectAdjacentSlants:
			{
				GEditor->Exec( TEXT("POLY SELECT ADJACENT SLANTS") );
			}
			break;

			case ID_SurfPopupSelectReverse:
			{
				GEditor->Exec( TEXT("POLY SELECT REVERSE") );
			}
			break;

			case ID_SurfPopupMemorize:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY SET") );
			}
			break;

			case ID_SurfPopupRecall:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY RECALL") );
			}
			break;

			case ID_SurfPopupOr:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY INTERSECTION") );
			}
			break;

			case ID_SurfPopupAnd:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY UNION") );
			}
			break;

			case ID_SurfPopupXor:
			{
				GEditor->Exec( TEXT("POLY SELECT MEMORY XOR") );
			}
			break;


			//
			// ACTOR POPUP MENU
			//

			// Root
			case IDMENU_ActorPopupProperties:
			{
				GEditor->Exec( TEXT("HOOK ACTORPROPERTIES") );
			}
			break;

			case IDMENU_ActorPopupSelectAllClass:
			{
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_NUM_SELECTED | eGI_CLASSNAME_SELECTED );

				if( gir.iValue )
				{
					appSprintf( l_chCmd, TEXT("ACTOR SELECT OFCLASS CLASS=%s"), *gir.String );
					GEditor->Exec( l_chCmd );
				}
			}
			break;

			case IDMENU_ActorPopupSelectAll:
			{
				GEditor->Exec( TEXT("ACTOR SELECT ALL") );
			}
			break;

			case IDMENU_ActorPopupSelectNone:
			{
				GEditor->Exec( TEXT("SELECT NONE") );
			}
			break;

			case IDMENU_ActorPopupDuplicate:
			{
				GEditor->Exec( TEXT("ACTOR DUPLICATE") );
			}
			break;

			case IDMENU_ActorPopupDelete:
			{
				GEditor->Exec( TEXT("ACTOR DELETE") );
			}
			break;

			case IDMENU_ActorPopupEditScript:
			{
				ShowBrowserActor( this );
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_CLASS_SELECTED );
				GCodeFrame->AddClass( gir.pClass );
			}
			break;

			case IDMENU_ActorPopupMakeCurrent:
			{
				t_GetInfoRet gir = GetInfo( GEditor->Level, eGI_CLASSNAME_SELECTED );
				GEditor->Exec( *(FString::Printf(TEXT("SETCURRENTCLASS CLASS=%s"), *gir.String)) );
			}
			break;

			case IDMENU_ActorPopupMerge:
			{
				GWarn->BeginSlowTask( TEXT("Merging Faces"), 1, 0 );
				for( int i=0; i<GEditor->Level->Actors.Num(); i++ )
				{
					GWarn->StatusUpdatef( i, GEditor->Level->Actors.Num(), TEXT("Merging Faces") );
					AActor* pActor = GEditor->Level->Actors(i);
					if( pActor && pActor->bSelected && pActor->IsBrush() )
						GEditor->bspValidateBrush( pActor->Brush, 1, 1 );
				}
				GEditor->RedrawLevel( GEditor->Level );
				GWarn->EndSlowTask();
			}
			break;

			case IDMENU_ActorPopupSeparate:
			{
				GWarn->BeginSlowTask( TEXT("Separating Faces"), 1, 0 );
				for( int i=0; i<GEditor->Level->Actors.Num(); i++ )
				{
					GWarn->StatusUpdatef( i, GEditor->Level->Actors.Num(), TEXT("Separating Faces") );
					AActor* pActor = GEditor->Level->Actors(i);
					if( pActor && pActor->bSelected && pActor->IsBrush() )
						GEditor->bspUnlinkPolys( pActor->Brush );
				}
				GEditor->RedrawLevel( GEditor->Level );
				GWarn->EndSlowTask();
			}
			break;

			// Select Brushes
			case IDMENU_ActorPopupSelectBrushesAdd:
			{
				GEditor->Exec( TEXT("MAP SELECT ADDS") );
			}
			break;

			case IDMENU_ActorPopupSelectBrushesSubtract:
			{
				GEditor->Exec( TEXT("MAP SELECT SUBTRACTS") );
			}
			break;

			case IDMENU_ActorPopupSubtractBrushesSemisolid:
			{
				GEditor->Exec( TEXT("MAP SELECT SEMISOLIDS") );
			}
			break;

			case IDMENU_ActorPopupSelectBrushesNonsolid:
			{
				GEditor->Exec( TEXT("MAP SELECT NONSOLIDS") );
			}
			break;

			// Mover Keyframe
			case IDMENU_ActorPopupKey0:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=0") );
			}
			break;

			case IDMENU_ActorPopupKey1:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=1") );
			}
			break;

			case IDMENU_ActorPopupKey2:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=2") );
			}
			break;

			case IDMENU_ActorPopupKey3:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=3") );
			}
			break;

			case IDMENU_ActorPopupKey4:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=4") );
			}
			break;

			case IDMENU_ActorPopupKey5:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=5") );
			}
			break;

			case IDMENU_ActorPopupKey6:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=6") );
			}
			break;

			case IDMENU_ActorPopupKey7:
			{
				GEditor->Exec( TEXT("ACTOR KEYFRAME NUM=7") );
			}
			break;

			// Reset
			case IDMENU_ActorPopupResetOrigin:
			{
				GEditor->Exec( TEXT("ACTOR RESET LOCATION") );
			}
			break;

			case IDMENU_ActorPopupResetPivot:
			{
				GEditor->Exec( TEXT("ACTOR RESET PIVOT") );
			}
			break;

			case IDMENU_ActorPopupResetRotation:
			{
				GEditor->Exec( TEXT("ACTOR RESET ROTATION") );
			}
			break;

			case IDMENU_ActorPopupResetScaling:
			{
				GEditor->Exec( TEXT("ACTOR RESET SCALE") );
			}
			break;

			case IDMENU_ActorPopupResetAll:
			{
				GEditor->Exec( TEXT("ACTOR RESET ALL") );
			}
			break;

			// Transform
			case IDMENU_ActorPopupMirrorX:
			{
				GEditor->Exec( TEXT("ACTOR MIRROR X=-1") );
			}
			break;

			case IDMENU_ActorPopupMirrorY:
			{
				GEditor->Exec( TEXT("ACTOR MIRROR Y=-1") );
			}
			break;

			case IDMENU_ActorPopupMirrorZ:
			{
				GEditor->Exec( TEXT("ACTOR MIRROR Z=-1") );
			}
			break;

			case IDMENU_ActorPopupPerm:
			{
				GEditor->Exec( TEXT("ACTOR APPLYTRANSFORM") );
			}
			break;

			// Order
			case IDMENU_ActorPopupToFirst:
			{
				GEditor->Exec( TEXT("MAP SENDTO FIRST") );
			}
			break;

			case IDMENU_ActorPopupToLast:
			{
				GEditor->Exec( TEXT("MAP SENDTO LAST") );
			}
			break;

			// Copy Polygons
			case IDMENU_ActorPopupToBrush:
			{
				GEditor->Exec( TEXT("MAP BRUSH GET") );
			}
			break;

			case IDMENU_ActorPopupFromBrush:
			{
				GEditor->Exec( TEXT("MAP BRUSH PUT") );
			}
			break;

			// Solidity
			case IDMENU_ActorPopupMakeSolid:
			{
				appSprintf( l_chCmd, TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, 0);
				GEditor->Exec( l_chCmd );
			}
			break;

			case IDMENU_ActorPopupMakeSemisolid:
			{
				appSprintf( l_chCmd, TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, PF_Semisolid );
				GEditor->Exec( l_chCmd );
			}
			break;

			case IDMENU_ActorPopupMakeNonSolid:
			{
				appSprintf( l_chCmd, TEXT("MAP SETBRUSH CLEARFLAGS=%d SETFLAGS=%d"), PF_Semisolid + PF_NotSolid, PF_NotSolid );
				GEditor->Exec( l_chCmd );
			}
			break;

			// CSG
			case IDMENU_ActorPopupMakeAdd:
			{
				GEditor->Exec( *(FString::Printf(TEXT("MAP SETBRUSH CSGOPER=%d"), CSG_Add) ) );
			}
			break;

			case IDMENU_ActorPopupMakeSubtract:
			{
				GEditor->Exec( *(FString::Printf(TEXT("MAP SETBRUSH CSGOPER=%d"), CSG_Subtract) ) );
			}
			break;
		
			default:
				WMdiFrame::OnCommand(Command);
			}
		unguard;
	}
	void NotifyDestroy( void* Other )
	{
		if( Other==Preferences )
			Preferences=NULL;
	}

	// FDocumentManager interface.
	virtual void OpenLevelView()
	{
		guard(WEditorFrame::OpenLevelView);

		// This is making it so you can only open one level window - it will reuse it for each
		// map you load ... which is not really MDI.  But the editor has problems with 2 level windows open.  
		// Fix if you can...
		if( !GLevelFrame )
		{
			GLevelFrame = new WLevelFrame( GEditor->Level, TEXT("LevelFrame"), &BackgroundHolder );
			GLevelFrame->OpenWindow( 1, 1 );
		}

		unguard;
	}
	virtual void OpenScriptView( UClass* Class )
	{
		guard(WEditorFrame::OpenScriptView);
		//WCodeFrame* CodeFrame = new WCodeFrame( Class, *(FString(TEXT("ScriptEd"))+Class->GetPathName()), &BackgroundHolder );
		//CodeFrame->OpenWindow(1,0);
		//CodeFrame->Show(1);
		unguard;
	}
};

void MRU_ReadINI()
{
	guard(MRU_ReadINI);
	GEditorFrame->MRUFilenames.Empty();
	for( int mru = 0 ; mru < MRU_MAX_FILES ; mru++ )
	{
		FString Filename;
		GConfig->GetString( TEXT("MRU"), *(FString::Printf(TEXT("Filename%d"),mru)), Filename, TEXT("UnrealEd2.ini") );

		// If we get a filename, add it to the top of the MRU list.
		if( Filename.Len() )
			GEditorFrame->MRUFilenames.AddItem(Filename);
	}
	unguard;
}
void MRU_WriteINI()
{
	guard(MRU_WriteINI);
	for( int mru = 0 ; mru < GEditorFrame->MRUFilenames.Num() ; mru++ )
	{
		GConfig->SetString( TEXT("MRU"), *(FString::Printf(TEXT("Filename%d"),mru)), *(GEditorFrame->MRUFilenames(mru)), TEXT("UnrealEd2.ini") );
	}
	unguard;
}

// Adds a filename to the MRU list.  New filenames are added to the bottom of the list.
void MRU_AddFilename( FString Filename )
{
	guard(MRU_AddFilename);
	// See if the filename already exists in the list...
	for( int mru = 0 ; mru < GEditorFrame->MRUFilenames.Num() ; mru++ )
	{
		if( GEditorFrame->MRUFilenames(mru) == Filename )
			return;
	}

	// ...if not, add it to the top of the list.
	if( Filename.Len() )
	{
		GEditorFrame->MRUFilenames.AddItem( Filename );
	}

	// Make sure the array doesn't exceed MRU_MAX_FILES entries.
	while( GEditorFrame->MRUFilenames.Num() > MRU_MAX_FILES )
		GEditorFrame->MRUFilenames.Remove( 0 );

	unguard;
}

// Adds all currently known MRU filenames to the "File" menu.  This completely
// replaces the editor frames menu with a new one.
void MRU_AddToMenu()
{
	guard(MRU_AddToMenu);

	::LockWindowUpdate( GEditorFrame->hWnd );

	// Kill current menu
	SetMenu( GEditorFrame->hWnd, NULL );

	// Create a new menu
	HMENU menu = LoadMenuIdX(hInstance,IDMENU_MainMenu);
	check(menu);
	HMENU filemenu = GetSubMenu( menu, 0 );
	check( filemenu );

	MENUITEMINFOA mif;
	TCHAR tchFilename[256] = TEXT("\0");
	int menuid = IDMN_MRU1;

	mif.cbSize = sizeof(MENUITEMINFO);
	mif.fMask = MIIM_TYPE | MIIM_ID;
	mif.fType = MFT_STRING;

	for( int x = 0 ; x < GEditorFrame->MRUFilenames.Num() ; x++ )
	{
		appSprintf(tchFilename, TEXT("&%d %s"), x + 1, *(GEditorFrame->MRUFilenames(x)) );
		mif.dwTypeData = TCHAR_TO_ANSI( tchFilename );
		mif.wID = menuid + x;

		InsertMenuItemA( filemenu, 99999, FALSE, &mif );
	}

	// Only add this seperator if we actually have MRU filenames... looks weird otherwise.
	if( GEditorFrame->MRUFilenames.Num() )
	{
		mif.fType = MFT_SEPARATOR;
		mif.wID = IDMN_MRU_SEP;
		InsertMenuItemA( filemenu, 99999, FALSE, &mif );
	}

	mif.fType = MFT_STRING;
	mif.dwTypeData = "Exit";
	mif.wID = ID_FileExit;
	InsertMenuItemA( filemenu, 99999, FALSE, &mif );

	SetMenu( GEditorFrame->hWnd, menu );

	::LockWindowUpdate( NULL );

	unguard;
}

void FileOpen( HWND hWnd )
{
	guard(FileOpen);
	FileSaveChanges( hWnd );

	OPENFILENAMEA ofn;
	char File[255] = "\0";

	ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = File;
	ofn.nMaxFile = sizeof(File);
	char Filter[255];
  ::sprintf( Filter, 
        "Map Files (*.%s)%c*.%s%cAll Files%c*.*%c%c",
		appToAnsi( *GMapExt ), 
        '\0', 
        appToAnsi( *GMapExt ), 
        '\0', 
        '\0', 
        '\0', 
        '\0' );
	ofn.lpstrFilter = Filter;
	ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_UNR]) );
	ofn.lpstrDefExt = appToAnsi( *GMapExt );
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	// Display the Open dialog box. 
	if( GetOpenFileNameA(&ofn) )
	{
		// Make sure there's a level frame open.
		GEditorFrame->OpenLevelView();
			
		// Convert the ANSI filename to UNICODE, and tell the editor to open it.
		GLevelFrame->SetMapFilename( (TCHAR*)appFromAnsi(File) );
		GEditor->Exec( *(FString::Printf(TEXT("MAP LOAD FILE=%s"), GLevelFrame->GetMapFilename() ) ) );

		FString S = GLevelFrame->GetMapFilename();
		GLastDir[eLASTDIR_UNR] = S.Left( S.InStr( TEXT("\\"), 1 ) );

#if 1 //NEW: U2Ed -- add to MRU on opening file as well as saving, e.g. for SaveAs
		MRU_AddFilename( GLevelFrame->GetMapFilename() );
		MRU_AddToMenu();
#endif
	}

	// Make sure that the browsers reflect any new data the map brought with it.
	if( GBrowserTexture )
	{
		GBrowserTexture->RefreshPackages();
		GBrowserTexture->RefreshGroups();
		GBrowserTexture->RefreshTextureList();
	}
	if( GBrowserSound )
	{
		GBrowserSound->RefreshPackages();
		GBrowserSound->RefreshGroups();
		GBrowserSound->RefreshSoundList();
	}
	if( GBrowserMusic )
	{
		GBrowserMusic->RefreshMusicList();
	}

	GBuildSheet->RefreshStats();

	GFileManager->SetDefaultDirectory(appBaseDir());
	unguard;
}

/*-----------------------------------------------------------------------------
	Class browser.
-----------------------------------------------------------------------------*/

// An category header list item.
class FObjectBrowserItem : public FHeaderItem
{
public:
	// Variables.
	UObject* Object;

	// Constructors.
	FObjectBrowserItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UObject* InObject, UBOOL InExpandable )
	:	FHeaderItem( InOwnerProperties, InParent, InExpandable )
	,	Object( InObject )
	{}

	// FTreeItem interface.
	void Serialize( FArchive& Ar )
	{
		guard(FObjectBrowserItem::Serialize);
		FHeaderItem::Serialize( Ar );
		Ar << Object;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FObjectBrowserItem::GetId);
		return Object->GetIndex() + ((QWORD)5<<32);
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FObjectBrowserItem::GetText);
		return Object ? Object->GetName() : TEXT("");
		unguard;
	}
	void Collapse()
	{
		guard(FCategoryItem::Collapse);
		FTreeItem::Collapse();
		EmptyChildren();
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	WToolbar.
-----------------------------------------------------------------------------*/

// An category header list item.
class FToolbarItem : public FHeaderItem
{
public:
	// Variables.
	FString Caption;

	// Constructors.
	FToolbarItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable, const TCHAR* InCaption )
	:	FHeaderItem( InOwnerProperties, InParent, InExpandable )
	,	Caption( InCaption )
	{
		Sorted = 0;
	}

	// FTreeItem interface.
	INT GetIndentPixels( UBOOL Text )
	{
		return Expandable ? FHeaderItem::GetIndentPixels(Text) : 16;
	}
	UBOOL GetSelected()
	{
		return 0;
	}
	HBRUSH GetBackgroundBrush( UBOOL Selected )
	{
		guard(FPropertyItem::GetBackgroundBrush);
		return Selected ? hBrushCurrent : Children.Num() ? hBrushHeadline : hBrushOffWhite;
		unguard;
	}
	QWORD GetId() const
	{
		guard(FObjectBrowserItem::GetId);
		return appStrCrc( *Caption );
		unguard;
	}
	virtual FString GetCaption() const
	{
		guard(FObjectBrowserItem::GetCaption);
		return Caption;
		unguard;
	}
	virtual void SetCaption( FString NewCaption )
	{
		guard(FObjectBrowserItem::SetCaption);
		Caption = NewCaption;
		unguard;
	}
};

// An editor mode.
class FToolbarModeItem : public FToolbarItem
{
public:
	EEditorMode Mode;
	FToolbarModeItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable, const TCHAR* InCaption, EEditorMode InMode )
	:	FToolbarItem( InOwnerProperties, InParent, InExpandable, InCaption )
	,	Mode( InMode )
	{}
	UBOOL GetSelected()
	{
		return GEditor->Mode==Mode;
	}
	HBRUSH GetBackgroundBrush( UBOOL Selected )
	{
		guard(FPropertyItem::GetBackgroundBrush);
		return Selected ? hBrushMode : hBrushOffWhite;
		unguard;
	}
	void OnItemLeftMouseDown( FPoint P )
	{
		GEditor->edcamSetMode( Mode );
		GEditor->RedrawLevel( GEditor->Level );
		InvalidateRect( OwnerProperties->List, NULL, 0 );
		UpdateWindow( OwnerProperties->List );
	}
};

// A brush builder.
class FToolbarBrushItem : public FObjectsItem
{
public:
	UBrushBuilder* Builder;
	FToolbarBrushItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UClass* Class )
	:	FObjectsItem( InOwnerProperties, InParent, CPF_Edit, Class->GetName(), 0 )
	,	Builder(NULL)
	{
		guard(FToolbarBrushItem::FToolbarBrushItem);
		Builder = ConstructObject<UBrushBuilder>(Class);
		SetObjects( (UObject**)&Builder, 1 );
		Sorted = 0;
		unguard;
	}
	void OnBuild()
	{
		guard(FToolbarBrushItem::OnBuild);
		OwnerProperties->SetItemFocus(0);
		UBOOL GIsSavedScriptableSaved = 1;
		Exchange(GIsScriptable,GIsSavedScriptableSaved);
		Builder->eventBuild();
		Exchange(GIsScriptable,GIsSavedScriptableSaved);
		unguard;
	}
	void OnItemSetFocus()
	{
		FObjectsItem::OnItemSetFocus();
		AddButton( TEXT("Build"), FDelegate(this,(TDelegate)OnBuild) );
	}
	void Serialize( FArchive& Ar )
	{
		guard(FPropertyItemBase::Serialize);
		FObjectsItem::Serialize( Ar );
		Ar << Builder;
		unguard;
	}
};

// An editor command.
class FToolbarCommandItem : public FToolbarItem
{
public:
	FString Cmd;
	UBOOL Executing;
	FToolbarCommandItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable, const TCHAR* InCaption, const TCHAR* InCmd )
	:	FToolbarItem( InOwnerProperties, InParent, InExpandable, InCaption )
	,	Cmd( InCmd )
	,	Executing( 0 )
	{}
	HBRUSH GetBackgroundBrush( UBOOL Selected )
	{
		guard(FPropertyItem::GetBackgroundBrush);
		return Executing ? hBrushCurrent : hBrushOffWhite;
		unguard;
	}
	void OnItemLeftMouseDown( FPoint P )
	{
		Executing=1;
		InvalidateRect( OwnerProperties->List, NULL, 0 );
		UpdateWindow( OwnerProperties->List );
		GEditor->Exec( *Cmd );
		GEditor->RedrawLevel( GEditor->Level );
		Executing=0;
		InvalidateRect( OwnerProperties->List, NULL, 0 );
		UpdateWindow( OwnerProperties->List );
	}
};

// A misc item.
enum eMISCITEM {
	eMISCITEM_ADDSPECIAL		= 0,
	eMISCITEM_GRID				= 1,
	eMISCITEM_ROTGRID			= 2,
	eMISCITEM_CAMSPEED			= 3,
	eMISCITEM_SNAPVERTEX		= 4,
	eMISCITEM_REPLACESELACTORS	= 5,
	eMISCITEM_AFFECTREGION		= 6,
	eMISCITEM_SHOWVERTICES		= 7,
	eMISCITEM_TEXTURELOCK		= 8,
};

class FToolbarMiscItem : public FToolbarItem
{
public:

	int Item;

	FToolbarMiscItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable, const TCHAR* InCaption, int InItem )
	:	FToolbarItem( InOwnerProperties, InParent, InExpandable, InCaption )
	,	Item( InItem )
	{}
	virtual COLORREF GetTextColor( UBOOL Selected )
	{
		guard(FToolbarMiscItem::GetTextColor);

		switch( Item )
		{
			case eMISCITEM_GRID:
				return GEditor->Constraints.GridEnabled ? RGB(255,255,255) : RGB(0,0,0);
			case eMISCITEM_ROTGRID:
				return GEditor->Constraints.RotGridEnabled ? RGB(255,255,255) : RGB(0,0,0);
			case eMISCITEM_SNAPVERTEX:
				return GEditor->Constraints.SnapVertices ? RGB(255,255,255) : RGB(0,0,0);
			case eMISCITEM_AFFECTREGION:
				return GEditor->Constraints.AffectRegion ? RGB(255,255,255) : RGB(0,0,0);
			case eMISCITEM_SHOWVERTICES:
				return GEditor->Constraints.ShowVertices ? RGB(255,255,255) : RGB(0,0,0);
			case eMISCITEM_TEXTURELOCK:
				return GEditor->Constraints.TextureLock ? RGB(255,255,255) : RGB(0,0,0);
		}

		return RGB(0,0,0);
		unguard;
	}
	HBRUSH GetBackgroundBrush( UBOOL Selected )
	{
		guard(FPropertyItem::GetBackgroundBrush);

		if( Selected ) {

			return hBrushMode;
		}
		else {

			switch( Item )
			{
				case eMISCITEM_GRID:
					return GEditor->Constraints.GridEnabled ? hBrushMode : hBrushOffWhite;
				case eMISCITEM_ROTGRID:
					return GEditor->Constraints.RotGridEnabled ? hBrushMode : hBrushOffWhite;
				case eMISCITEM_SNAPVERTEX:
					return GEditor->Constraints.SnapVertices ? hBrushMode : hBrushOffWhite;
				case eMISCITEM_AFFECTREGION:
					return GEditor->Constraints.AffectRegion ? hBrushMode : hBrushOffWhite;
				case eMISCITEM_SHOWVERTICES:
					return GEditor->Constraints.ShowVertices ? hBrushMode : hBrushOffWhite;
				case eMISCITEM_TEXTURELOCK:
					return GEditor->Constraints.TextureLock ? hBrushMode : hBrushOffWhite;
			}
		}

		return hBrushOffWhite;

		unguard;
	}
	void OnItemLeftMouseDown( FPoint P )
	{
		switch( Item )
		{
			case eMISCITEM_ADDSPECIAL:
				delete GDlgAddSpecial;

				GDlgAddSpecial = new WDlgAddSpecial( NULL, GEditorFrame );
				GDlgAddSpecial->DoModeless();
				break;

			case eMISCITEM_GRID:
				GEditor->Exec( *(FString::Printf(TEXT("MODE GRID=%d"), !GEditor->Constraints.GridEnabled)) );
				break;

			case eMISCITEM_ROTGRID:
				GEditor->Exec( *(FString::Printf(TEXT("MODE ROTGRID=%d"), !GEditor->Constraints.RotGridEnabled)) );
				break;

			case eMISCITEM_CAMSPEED:
				switch( (int)GEditor->MovementSpeed )
				{
					case 16:
						GEditor->Exec( TEXT("MODE SPEED=1") );
						break;

					case 1:
						GEditor->Exec( TEXT("MODE SPEED=4") );
						break;

					case 4:
					default:
						GEditor->Exec( TEXT("MODE SPEED=16") );
						break;
				}
				SetCaption( *(FString::Printf(TEXT("Camera Speed=%d"), (int)GEditor->MovementSpeed)) );
				break;

			case eMISCITEM_SNAPVERTEX:
				GEditor->Exec( *(FString::Printf(TEXT("MODE SNAPVERTEX=%d"), !GEditor->Constraints.SnapVertices)) );
				break;

			case eMISCITEM_REPLACESELACTORS:
				if( GEditor->CurrentClass )
					GEditor->Exec( *(FString::Printf(TEXT("ACTOR REPLACE CLASS=%s"), GEditor->CurrentClass->GetName())) );
				break;

			case eMISCITEM_AFFECTREGION:
				GEditor->Exec( *(FString::Printf(TEXT("MODE AFFECTREGION=%d"), !GEditor->Constraints.AffectRegion)) );
				break;

			case eMISCITEM_SHOWVERTICES:
				GEditor->Exec( *(FString::Printf(TEXT("MODE SHOWVERTICES=%d"), !GEditor->Constraints.ShowVertices)) );
				break;

			case eMISCITEM_TEXTURELOCK:
				GEditor->Exec( *(FString::Printf(TEXT("MODE TEXTURELOCK=%d"), !GEditor->Constraints.TextureLock)) );
				break;
		}
		UpdateWindow( OwnerProperties->List );
		InvalidateRect( OwnerProperties->List.hWnd, NULL, 1 );
	}
};

// An category header list item.
class FToolbarRootItem : public FToolbarItem
{
public:
	// Variables.
	FString Caption;

	// Constructors.
	FToolbarRootItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable, const TCHAR* InCaption )
	:	FToolbarItem( InOwnerProperties, InParent, InExpandable, InCaption )
	,	Caption( TEXT("Toolbar") )
	{}

	virtual ~FToolbarRootItem()
	{
		guard(FToolbarRootItem::~FToolbarRootItem);

		// Save states of toolbar items
		for( int x = 0 ; x < Children.Num() ; x++ )
			GConfig->SetInt( TEXT("ToolbarStates"), *(Children(x)->GetCaption()), Children(x)->Expanded, TEXT("UnrealEd2.ini") );

		unguard;;
	}

	// FTreeItem interface.
	void Expand()
	{
		guard(FToolbarItem::Expand);
		FToolbarItem* T;
		Children.AddItem(T=new FToolbarItem(OwnerProperties,this,1,TEXT("Modes")));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Camera Move"),EM_ViewportMove));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Camera Zoom"),EM_ViewportZoom));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Brush Rotate"),EM_BrushRotate));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Brush Sheer"),EM_BrushSheer));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Brush Scale Uniform"),EM_BrushScale));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Brush Scale Axial"),EM_BrushStretch));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Brush Scale Snap"),EM_BrushSnap));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Pan Textures"),EM_TexturePan));
			T->Children.AddItem(new FToolbarModeItem(OwnerProperties,T,0,TEXT("Rotate Textures"),EM_TextureRotate));
		Children.AddItem(T=new FToolbarItem(OwnerProperties,this,1,TEXT("Brush Operations")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Add"),TEXT("BRUSH ADD")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Subtract"),TEXT("BRUSH SUBTRACT")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Intersect"),TEXT("BRUSH FROM INTERSECTION")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Deintersect"),TEXT("BRUSH FROM DEINTERSECTION")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Add Mover"),TEXT("BRUSH ADDMOVER")));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("Add Special..."),eMISCITEM_ADDSPECIAL));
		Children.AddItem(T=new FToolbarItem(OwnerProperties,this,1,TEXT("Brush Factories")));
			for( TObjectIterator<UClass> ItC; ItC; ++ItC )
				if( ItC->IsChildOf(UBrushBuilder::StaticClass()) && !(ItC->ClassFlags&CLASS_Abstract) )
					T->Children.AddItem(new FToolbarBrushItem(OwnerProperties,T,*ItC));
		Children.AddItem(T=new FToolbarItem(OwnerProperties,this,1,TEXT("Select")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("All Actors"),TEXT("ACTOR SELECT ALL")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("All Surfaces"),TEXT("POLY SELECT ALL")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Actors Inside Brush"),TEXT("ACTOR SELECT INSIDE")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Actors Invert"),TEXT("ACTOR SELECT INVERT")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("None"),TEXT("SELECT NONE")));
		Children.AddItem(T=new FToolbarItem(OwnerProperties,this,1,TEXT("Tools")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Replace Sel Brushes"),TEXT("ACTOR REPLACE BRUSH")));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("Replace Sel Actors"),eMISCITEM_REPLACESELACTORS));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("Camera Speed=4"),eMISCITEM_CAMSPEED));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("[Texture Lock]"),eMISCITEM_TEXTURELOCK));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("[Affect Region]"),eMISCITEM_AFFECTREGION));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("[Show Vertices]"),eMISCITEM_SHOWVERTICES));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("[Vertex Snap]"),eMISCITEM_SNAPVERTEX));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("[Grid]"),eMISCITEM_GRID));
			T->Children.AddItem(new FToolbarMiscItem(OwnerProperties,T,0,TEXT("[Rotation Grid]"),eMISCITEM_ROTGRID));
			Children.AddItem(T=new FToolbarItem(OwnerProperties,this,1,TEXT("Visibility")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Show All"),TEXT("ACTOR UNHIDE ALL")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Show Sel Actors"),TEXT("ACTOR HIDE UNSELECTED")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Hide Sel Actors"),TEXT("ACTOR HIDE SELECTED")));
			T->Children.AddItem(new FToolbarCommandItem(OwnerProperties,T,0,TEXT("Toggle Z Region"),TEXT("ACTOR CLIP Z")));

		FToolbarItem::Expand();

		// Restore the toolbar states
		BOOL bExpanded;
		for( int x = 0 ; x < Children.Num() ; x++ )
		{
			if(!GConfig->GetInt( TEXT("ToolbarStates"), *(Children(x)->GetCaption()), bExpanded, TEXT("UnrealEd2.ini") ))	bExpanded = FALSE;
			if( bExpanded ) Children(x)->Expand();
		}
		unguard;
	}
};

// Multiple selection object properties.
class WToolbar : public WProperties
{
	DECLARE_WINDOWCLASS(WToolbar,WProperties,Window)

	// Variables.
	FToolbarRootItem Root;

	// Structors.
	WToolbar( FName InPersistentName, WWindow* InOwnerWindow )
	:	WProperties	( NAME_None/*InPersistentName!!*/, InOwnerWindow )
	,	Root		( this, NULL, 1, TEXT("Toolbar") )
	{
		ShowTreeLines=0;
		PropertiesWindows.RemoveItem( this );//!!hack while observer code is unimplemented
	}

	// WPropertiesBase interface.
	FTreeItem* GetRoot()
	{
		return &Root;
	}
};

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main window entry point.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )
{
	// Remember instance.
	GIsStarted = 1;
	hInstance = hInInstance;

	// Set package name.
	appStrcpy( GPackage, appPackage() );

	// Begin.
#ifndef _DEBUG
	try
	{
#endif
		// Set mode.
		GIsClient = GIsServer = GIsEditor = GLazyLoad = 1;
		GIsScriptable = 0;

		// Start main loop.
		GIsGuarded=1;

		// Create a fully qualified pathname for the log file.  If we don't do this, pieces of the log file
		// tends to get written into various directories as the editor starts up.
		TCHAR chLogFilename[256] = TEXT("\0");
		appSprintf( chLogFilename, TEXT("%s%s"), appBaseDir(), TEXT("Editor.log"));
		appStrcpy( Log.Filename, chLogFilename );

		appInit( TEXT("Unreal"), GetCommandLine(), &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		GetPropResult = new FStringOutputDevice;

		// Init windowing.
		InitWindowing();
		IMPLEMENT_WINDOWCLASS(WMdiFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WEditorFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBackgroundHolder,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WLevelFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WDockingFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WToolbar,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WCodeFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(W2DShapeEditor,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WViewportFrame,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBrowserSound,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBrowserMusic,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBrowserTexture,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBrowserMesh,CS_DBLCLKS);
		IMPLEMENT_WINDOWCLASS(WBrowserActor,CS_DBLCLKS);
#if 1 //NEW: ParticleSystems
		IMPLEMENT_WINDOWCLASS(WBrowserParticles,CS_DBLCLKS);
#endif
		IMPLEMENT_WINDOWSUBCLASS(WMdiClient,TEXT("MDICLIENT"));

		// Windows.
		WEditorFrame Frame;
		GDocumentManager = &Frame;
		Frame.OpenWindow();
		InvalidateRect( Frame, NULL, 1 );
		UpdateWindow( Frame );
		UBOOL ShowLog = ParseParam(appCmdLine(),TEXT("log"));
		if( !ShowLog && !ParseParam(appCmdLine(),TEXT("server")) )
#if 1 //NEW: U2Ed
			InitSplash( TEXT("U2EdLogo.bmp") );
#else
			InitSplash( TEXT("..\\Help\\Logo.bmp") );
#endif

		// Init.
		GLogWindow = new WLog( Log.Filename, Log.LogAr, TEXT("EditorLog"), &Frame );
		GLogWindow->OpenWindow( ShowLog, 0 );

		// Init engine.
		GEditor = CastChecked<UEditorEngine>(InitEngine());

		// Set up autosave timer.  We ping the engine once a minute and it determines when and 
		// how to do the autosave.
		SetTimer( GEditorFrame->hWnd, 900, 60000, NULL);

		// Initialize "last dir" array
		GLastDir[eLASTDIR_UNR] = TEXT("..\\maps");
		GLastDir[eLASTDIR_UTX] = TEXT("..\\textures");
		GLastDir[eLASTDIR_UAX] = TEXT("..\\sounds");

		if( !GConfig->GetString( TEXT("Directories"), TEXT("PCX"), GLastDir[eLASTDIR_PCX], TEXT("UnrealEd2.ini") ) )		GLastDir[eLASTDIR_PCX] = TEXT("..\\textures");
		if( !GConfig->GetString( TEXT("Directories"), TEXT("WAV"), GLastDir[eLASTDIR_WAV], TEXT("UnrealEd2.ini") ) )		GLastDir[eLASTDIR_WAV] = TEXT("..\\sounds");
		if( !GConfig->GetString( TEXT("Directories"), TEXT("BRUSH"), GLastDir[eLASTDIR_BRUSH], TEXT("UnrealEd2.ini") ) )		GLastDir[eLASTDIR_BRUSH] = TEXT("..\\maps");
		if( !GConfig->GetString( TEXT("Directories"), TEXT("2DS"), GLastDir[eLASTDIR_2DS], TEXT("UnrealEd2.ini") ) )		GLastDir[eLASTDIR_2DS] = TEXT("..\\maps");

		if( !GConfig->GetString( TEXT("URL"), TEXT("MapExt"), GMapExt, TEXT("Unreal.ini") ) )		GMapExt = TEXT("unr");
		GEditor->Exec( *(FString::Printf(TEXT("MODE MAPEXT=%s"), *GMapExt ) ) );

		// Init input.
		UInput::StaticInitInput();

		// Toolbar.
		GToolBar = new WToolbar( TEXT("EditorToolbar"), &Frame.LeftFrame );
		GToolBar->OpenChildWindow( 0 );
		GToolBar->ForceRefresh();
		Frame.LeftFrame.Dock( GToolBar );

		CreateToolbar( Frame.hWnd );

		ExitSplash();

		// Open a blank level on startup.
		Frame.OpenLevelView();

		// Reopen whichever windows we need to.
		BOOL bActive;

		if(!GConfig->GetInt( TEXT("Texture Browser"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = FALSE;
		if( bActive )	ShowBrowserTexture( GEditorFrame );
		if(!GConfig->GetInt( TEXT("Sound Browser"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = FALSE;
		if( bActive )	ShowBrowserSound( GEditorFrame );
		if(!GConfig->GetInt( TEXT("Music Browser"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = FALSE;
		if( bActive )	ShowBrowserMusic( GEditorFrame );
		if(!GConfig->GetInt( TEXT("Actor Browser"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = FALSE;
		if( bActive )	ShowBrowserActor( GEditorFrame );
#if 1 //NEW: ParticleSystems
		if(!GConfig->GetInt( TEXT("Particle Browser"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))bActive = FALSE;
		if( bActive )	ShowBrowserParticles( GEditorFrame );
#endif
		if(!GConfig->GetInt( TEXT("CodeFrame"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = FALSE;
		if( bActive )	ShowCodeFrame( GEditorFrame );
		if(!GConfig->GetInt( TEXT("Mesh Browser"), TEXT("Active"), bActive, TEXT("UnrealEd2.ini") ))	bActive = FALSE;
		if( bActive )	ShowBrowserMesh( GEditorFrame );


		GCodeFrame = new WCodeFrame( TEXT("CodeFrame"), GEditorFrame );
		GCodeFrame->OpenWindow( 0, 0 );

		if( !GIsRequestingExit )
			MainLoop( GEditor );

		::DestroyWindow( GHwndToolbar );
		::DestroyWindow( GhWndTT );

		GDocumentManager=NULL;
		GFileManager->Delete(TEXT("Running.ini"),0,0);
		if( GLogWindow )
			delete GLogWindow;
		appPreExit();
		GIsGuarded = 0;
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		Error.HandleError();
	}
#endif

	// Shut down.
	appExit();
	GIsStarted = 0;
	return 0;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif
