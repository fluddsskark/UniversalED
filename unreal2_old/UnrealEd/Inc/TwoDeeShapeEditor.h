#if 1 //NEW: U2Ed
/*=============================================================================
	TwoDeeShapeEditor : 2D Shape Editor conversion from VB code
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#include <stdio.h>
#include <math.h>
extern FString GLastDir[eLASTDIR_MAX];

// --------------------------------------------------------------
//
// EXTRUDE Dialog
//
// --------------------------------------------------------------

int GExtrudeDepth = 256;

class WDlgExtrude : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgExtrude,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit DepthEdit;

	int Depth;
 
	// Constructor.
	WDlgExtrude( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog		( TEXT("Extrude"), IDDIALOG_2DShapeEditor_Extrude, InOwnerWindow )
	,	OkButton    ( this, IDOK,     FDelegate(this,(TDelegate)OnOk) )
	,	CancelButton( this, IDCANCEL, FDelegate(this,(TDelegate)EndDialogFalse) )
	,	DepthEdit	( this, IDEC_DEPTH )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgExtrude::OnInitDialog);
		WDialog::OnInitDialog();

		DepthEdit.SetText( *(FString::Printf(TEXT("%d"), GExtrudeDepth) ) );
		::SetFocus( DepthEdit.hWnd );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgExtrude::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgExtrude::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgExtrude::OnOk);

		Depth = appAtoi( *DepthEdit.GetText() );

		if( Depth <= 0 )
		{
			appMsgf( TEXT("Invalid input.") );
			return;
		}

		GExtrudeDepth = Depth;
	
		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// REVOLVE Dialog
//
// --------------------------------------------------------------

int GRevolveTotalSides = 8, GRevolveSides = 4;

class WDlgRevolve : public WDialog
{
	DECLARE_WINDOWCLASS(WDlgRevolve,WDialog,UnrealEd)

	// Variables.
	WButton OkButton;
	WButton CancelButton;
	WEdit TotalSidesEdit;
	WEdit SidesEdit;

	int TotalSides, Sides;
 
	// Constructor.
	WDlgRevolve( UObject* InContext, WWindow* InOwnerWindow )
	:	WDialog			( TEXT("Revolve"), IDDIALOG_2DShapeEditor_Revolve, InOwnerWindow )
	,	OkButton		( this, IDOK,     FDelegate(this,(TDelegate)OnOk) )
	,	CancelButton	( this, IDCANCEL, FDelegate(this,(TDelegate)EndDialogFalse) )
	,	TotalSidesEdit	( this, IDEC_TOTAL_SIDES )
	,	SidesEdit		( this, IDEC_SIDES )
	{
	}

	// WDialog interface.
	void OnInitDialog()
	{
		guard(WDlgRevolve::OnInitDialog);
		WDialog::OnInitDialog();

		TotalSidesEdit.SetText( *(FString::Printf(TEXT("%d"), GRevolveTotalSides) ) );
		::SetFocus( TotalSidesEdit.hWnd );
		SidesEdit.SetText( *(FString::Printf(TEXT("%d"), GRevolveSides) ) );

		unguard;
	}
	void OnDestroy()
	{
		guard(WDlgRevolve::OnDestroy);
		WDialog::OnDestroy();
		unguard;
	}
	virtual int DoModal()
	{
		guard(WDlgRevolve::DoModal);
		return WDialog::DoModal( hInstance );
		unguard;
	}
	void OnOk()
	{
		guard(WDlgRevolve::OnOk);

		TotalSides = appAtoi( *TotalSidesEdit.GetText() );
		Sides = appAtoi( *SidesEdit.GetText() );

		if( TotalSides < 1
				|| Sides > TotalSides )
		{
			appMsgf( TEXT("Invalid input.") );
			return;
		}

		GRevolveTotalSides = TotalSides;
		GRevolveSides = Sides;
	
		EndDialogTrue();
		unguard;
	}
};

// --------------------------------------------------------------
//
// DATA TYPES
//
// --------------------------------------------------------------

class F2DSEVector : public FVector
{
public:
	F2DSEVector()
	{
		X = Y = Z = 0;
		bSelected = 0;
	}
	F2DSEVector( float x, float y, float z)
		: FVector( x, y, z )
	{
		bSelected = 0;
		bIsEdge = 0;
		bUsed = 0;
	}
	~F2DSEVector()
	{}

	inline F2DSEVector operator=( F2DSEVector Other )
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		bSelected = Other.bSelected;
		return *this;
	}
	inline F2DSEVector operator=( FVector Other )
	{
		X = Other.X;
		Y = Other.Y;
		Z = Other.Z;
		return *this;
	}
	inline UBOOL operator!=( F2DSEVector Other )
	{
		return (X != Other.X && Y != Other.Y && Z != Other.Z);
	}
	inline UBOOL operator==( F2DSEVector Other )
	{
		return (X == Other.X && Y == Other.Y && Z == Other.Z);
	}
	void SelectToggle()
	{
		bSelected = !bSelected;
	}
	void Select( BOOL bSel )
	{
		bSelected = bSel;
	}
	BOOL IsSel( void )
	{
		return bSelected;
	}

	float TempX, TempY;
	UBOOL bIsEdge, bUsed;

private:
	UBOOL bSelected;
};

class FTriangle
{
public:
	FTriangle()
	{
		bCCW = 0;
	}
	FTriangle( F2DSEVector vtx1, F2DSEVector vtx2, F2DSEVector vtx3 )
	{
		Vertex[0] = vtx1;
		Vertex[1] = vtx2;
		Vertex[2] = vtx3;
	}
	~FTriangle()
	{}

	inline FTriangle operator=( FTriangle Other )
	{
		Vertex[0] = Other.Vertex[0];
		Vertex[1] = Other.Vertex[1];
		Vertex[2] = Other.Vertex[2];
		return *this;
	}
	inline UBOOL operator==( FTriangle Other )
	{
		return( Vertex[0] == Other.Vertex[0] && Vertex[1] == Other.Vertex[1] && Vertex[2] == Other.Vertex[2] );
	}

	int HitVertex;			// Used when splitting sides
	F2DSEVector Vertex[3];
	UBOOL bCCW;				// Is this triangle wound CCW?
};

#define d2dSE_SELECT_TOLERANCE 4

// --------------------------------------------------------------
//
// W2DSHAPEEDITOR
//
// --------------------------------------------------------------

#define ID_2DSE_TOOLBAR	29002
TBBUTTON tb2DSEButtons[] = {
	{ 0, IDMN_2DSE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 1, IDMN_2DSE_FileOpen, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 2, IDMN_2DSE_FileSave, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 3, IDMN_2DSE_ROTATE90, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 4, IDMN_2DSE_ROTATE45, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 5, IDMN_2DSE_FLIP_VERT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 6, IDMN_2DSE_FLIP_HORIZ, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0}
	, { 7, IDMN_2DSE_PROCESS_SHEET, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 8, IDMN_2DSE_PROCESS_EXTRUDE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
	, { 9, IDMN_2DSE_PROCESS_REVOLVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
};
struct {
	TCHAR ToolTip[64];
	int ID;
} ToolTips_2DSE[] = {
	TEXT("New"), IDMN_2DSE_NEW,
	TEXT("Open"), IDMN_2DSE_FileOpen,
	TEXT("Save"), IDMN_2DSE_FileSave,
	TEXT("Rotate 90 Degrees"), IDMN_2DSE_ROTATE90,
	TEXT("Rotate 45 Degrees"), IDMN_2DSE_ROTATE45,
	TEXT("Flip Vertically"), IDMN_2DSE_FLIP_VERT,
	TEXT("Flip Horizontally"), IDMN_2DSE_FLIP_HORIZ,
	TEXT("Create a Sheet"), IDMN_2DSE_PROCESS_SHEET,
	TEXT("Create an Extruded Shape"), IDMN_2DSE_PROCESS_EXTRUDE,
	TEXT("Create a Revolved Shape"), IDMN_2DSE_PROCESS_REVOLVE,
	NULL, 0
};

class W2DShapeEditor : public WWindow
{
	DECLARE_WINDOWCLASS(W2DShapeEditor,WWindow,Window)

	HWND hWndTT;
	HWND hWndToolBar;

	// Structors.
	W2DShapeEditor( FName InPersistentName, WWindow* InOwnerWindow )
	:	WWindow( InPersistentName, InOwnerWindow )
	{
		New();
		m_bDraggingCamera = m_bDraggingVerts = FALSE;
		::ZeroMemory( MapFilename, sizeof(TCHAR) * 256);
		m_iGridSize = 16;
		hImage = NULL;
		hWndToolBar = hWndTT = NULL;
	}

	FVector m_camera;			// The viewing camera position
	F2DSEVector m_origin;		// The origin point used for revolves and such
	BOOL m_bDraggingCamera, m_bDraggingVerts, m_bMouseHasMoved;
	FPoint m_pointOldPos;
	TCHAR MapFilename[256];
	int m_iGridSize;
	HBITMAP hImage;
	RECT m_rcWnd;
	POINT m_ContextPos;

	TArray<FTriangle> m_tris;

	// WWindow interface.
	void OpenWindow()
	{
		guard(W2DShapeEditor::OpenWindow);
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
		SetCaption();
		unguard;
	}
	void OnDestroy()
	{
		guard(W2DShapeEditor::OnDestroy);
		::DestroyWindow( hWndTT );
		::DestroyWindow( hWndToolBar );
		WWindow::OnDestroy();
		unguard;
	}
	void OnCreate()
	{
		guard(W2DShapeEditor::OnCreate);
		WWindow::OnCreate();

		SetMenu( hWnd, LoadMenuIdX(hInstance, IDMENU_2DShapeEditor) );

		// TOOLBAR
		HWND hWndToolBar = CreateToolbarEx( 
			hWnd,
			WS_CHILD | WS_BORDER | WS_VISIBLE | CCS_ADJUSTABLE,
			ID_2DSE_TOOLBAR,
			10,
			hInstance,
			IDB_2DSE_TOOLBAR,
			(LPCTBBUTTON)&tb2DSEButtons,
			13,
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

			for( int tooltip = 0 ; ToolTips_2DSE[tooltip].ID > 0 ; tooltip++ )
			{
				// Figure out the rectangle for the toolbar button.
				int index = SendMessageX( hWndToolBar, TB_COMMANDTOINDEX, ToolTips_2DSE[tooltip].ID, 0 );
				RECT rect;
				SendMessageX( hWndToolBar, TB_GETITEMRECT, index, (LPARAM)&rect);

				// Add tooltip to tooltip control
				ti.uId = ID + tooltip;
				ti.lpszText = ToolTips_2DSE[tooltip].ToolTip;
				ti.rect = rect;

				SendMessageX(hWndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&ti);
			}
		}

		unguard;
	}
	void OnPaint()
	{
		guard(W2DShapeEditor::OnPaint);
		PAINTSTRUCT PS;
		HDC hDC = BeginPaint( *this, &PS );

		HDC hdcWnd, hdcMem;
		HBITMAP hBitmap;

		::GetClientRect( hWnd, &m_rcWnd );

		hdcWnd = GetDC(hWnd);
		hdcMem = CreateCompatibleDC(hdcWnd);
		hBitmap = CreateCompatibleBitmap(hdcWnd, m_rcWnd.right, m_rcWnd.bottom );
		SelectObject(hdcMem, hBitmap);

		HBRUSH l_brush, l_brushOld;
		l_brush = CreateSolidBrush( RGB(255, 255, 255) );
		l_brushOld = (HBRUSH)SelectObject( hdcMem, l_brush);
		FillRect( hdcMem, GetClientRect(), l_brush );
		SelectObject( hdcMem, l_brushOld);

		DrawGrid( hdcMem );
		DrawImage( hdcMem );
		DrawOrigin( hdcMem );
		DrawLines( hdcMem );
		DrawEdges( hdcMem );
		DrawVertices( hdcMem );

		BitBlt(hDC,
			   0, 0,
			   m_rcWnd.right, m_rcWnd.bottom,
			   hdcMem,
			   0, 0,
			   SRCCOPY);

		EndPaint( *this, &PS );

		DeleteObject( l_brush );
		DeleteDC(hdcMem);
		ReleaseDC( hWnd, hdcWnd );
		DeleteObject( hBitmap );
		unguard;
	}
	void OnCommand( INT Command )
	{
		guard(W2DShapeEditor::OnCommand);
		switch( Command ) {

			case IDMN_2DSE_ROTATE90:
				RotateShape( 90 );
				break;

			case IDMN_2DSE_ROTATE45:
				RotateShape( 45 );
				break;

			case IDMN_2DSE_FLIP_VERT:
				Flip(0);
				break;

			case IDMN_2DSE_FLIP_HORIZ:
				Flip(1);
				break;

			case IDMN_2DSE_NEW:
				appStrcpy( MapFilename, TEXT("") );
				SetCaption();
				New();
				break;

			case IDMN_2DSE_SPLIT_SIDE:
				SplitSides();
				break;

			case IDMN_2DSE_DELETE_VERTEX:
				Delete();
				break;

			case IDMN_2DSE_FileSave:
				FileSave( hWnd );
				break;

			case IDMN_2DSE_FileSaveAs:
				FileSaveAs( hWnd );
				break;

			case IDMN_2DSE_FileOpen:
				FileOpen( hWnd );
				break;

			case IDMN_2DSEC_SET_ORIGIN:
				SetOrigin();
				break;

			case IDMN_2DSE_PROCESS_SHEET:
				ProcessSheet();
				break;

			case IDMN_2DSE_PROCESS_EXTRUDE:
				{
					WDlgExtrude Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessExtrude( Dialog.Depth );
				}
				break;

			case IDMN_2DSE_PROCESS_REVOLVE:
				{
					WDlgRevolve Dialog( NULL, this );
					if( Dialog.DoModal() == IDOK )
						ProcessRevolve( Dialog.TotalSides, Dialog.Sides );
				}
				break;

			case IDMN_GRID_1:	m_iGridSize = 1;	InvalidateRect( hWnd, NULL, FALSE );	break;
			case IDMN_GRID_2:	m_iGridSize = 2;	InvalidateRect( hWnd, NULL, FALSE );	break;
			case IDMN_GRID_4:	m_iGridSize = 4;	InvalidateRect( hWnd, NULL, FALSE );	break;
			case IDMN_GRID_8:	m_iGridSize = 8;	InvalidateRect( hWnd, NULL, FALSE );	break;
			case IDMN_GRID_16:	m_iGridSize = 16;	InvalidateRect( hWnd, NULL, FALSE );	break;
			case IDMN_GRID_32:	m_iGridSize = 32;	InvalidateRect( hWnd, NULL, FALSE );	break;
			case IDMN_GRID_64:	m_iGridSize = 64;	InvalidateRect( hWnd, NULL, FALSE );	break;

			case IDMN_2DSE_OPEN_IMAGE:
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
						if( hImage ) 
							DeleteObject( hImage );

						hImage = (HBITMAP)LoadImageA( hInstance, File, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE );

						if( !hImage )
							appMsgf ( TEXT("Error loading bitmap.") );

						FString S = appFromAnsi( File );
						GLastDir[eLASTDIR_UTX] = S.Left( S.InStr( TEXT("\\"), 1 ) );
					}

					InvalidateRect( hWnd, NULL, FALSE );
				}
				break;

			case IDMN_2DSE_DELETE_IMAGE:
				DeleteObject( hImage );
				hImage = NULL;
				InvalidateRect( hWnd, NULL, FALSE );
				break;

			default:
				WWindow::OnCommand(Command);
				break;
		}
		unguard;
	}
	void OnRightButtonDown()
	{
		guard(W2DShapeEditor::OnRightButtonDown);

		m_bDraggingCamera = TRUE;
		m_pointOldPos = GetCursorPos();
		SetCapture( hWnd );
		m_bMouseHasMoved = FALSE;

		WWindow::OnRightButtonDown();
		unguard;
	}
	void OnRightButtonUp()
	{
		guard(W2DShapeEditor::OnRightButtonUp);

		ReleaseCapture();
		m_bDraggingCamera = FALSE;

		if( !m_bMouseHasMoved )
		{
			::GetCursorPos( &m_ContextPos );
			HMENU l_menu = GetSubMenu( LoadMenuIdX(hInstance, IDMENU_2DShapeEditor_Context), 0 );
			TrackPopupMenu( l_menu,
				TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
				m_ContextPos.x, m_ContextPos.y, 0,
				hWnd, NULL);

			::ScreenToClient( hWnd, &m_ContextPos );
		}

		WWindow::OnRightButtonUp();
		unguard;
	}
	void OnMouseMove( DWORD Flags, FPoint MouseLocation )
	{
		guard(W2DShapeEditor::OnMouseMove);

		m_bMouseHasMoved = TRUE;
		if( m_bDraggingCamera )
		{
			m_camera.X += MouseLocation.X - m_pointOldPos.X;
			m_camera.Y += MouseLocation.Y - m_pointOldPos.Y;

			m_pointOldPos = MouseLocation;
			InvalidateRect( hWnd, NULL, FALSE );
		}

		if( m_bDraggingVerts )
		{
			// Origin
			if( m_origin.IsSel() )
			{
				m_origin.TempX += (MouseLocation.X - m_pointOldPos.X);
				m_origin.TempY -= (MouseLocation.Y - m_pointOldPos.Y);

				m_origin.X = m_origin.TempX - ((int)m_origin.TempX % m_iGridSize);
				m_origin.Y = m_origin.TempY - ((int)m_origin.TempY % m_iGridSize);
			}

			// Vertices
			for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
					if( m_tris(tri).Vertex[vertex].IsSel() )
					{
						m_tris(tri).Vertex[vertex].TempX += (MouseLocation.X - m_pointOldPos.X);
						m_tris(tri).Vertex[vertex].TempY -= (MouseLocation.Y - m_pointOldPos.Y);

						m_tris(tri).Vertex[vertex].X = m_tris(tri).Vertex[vertex].TempX - ((int)m_tris(tri).Vertex[vertex].TempX % m_iGridSize);
						m_tris(tri).Vertex[vertex].Y = m_tris(tri).Vertex[vertex].TempY - ((int)m_tris(tri).Vertex[vertex].TempY % m_iGridSize);
					}

			m_pointOldPos = MouseLocation;
			InvalidateRect( hWnd, NULL, FALSE );
		}

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );

		WWindow::OnMouseMove( Flags, MouseLocation );
		unguard;
	}
	void OnLeftButtonDown()
	{
		guard(W2DShapeEditor::OnLeftButtonDown);

		m_bDraggingVerts = TRUE;
		m_pointOldPos = GetCursorPos();

		if( GetAsyncKeyState(VK_CONTROL) & 0x8000 )
			ProcessHits( TRUE );
		else 
		{
			// If the user has clicked on a vertex, then select that vertex and put them into drag mode.  Otherwise,
			// leave the current selections alone and just drag them.
			if( CheckHits() )
				ProcessHits( FALSE );
			else
				DeselectAllVerts();
		}

		InvalidateRect( hWnd, NULL, FALSE );

		WWindow::OnLeftButtonDown();
		unguard;
	}
	void OnLeftButtonUp()
	{
		guard(W2DShapeEditor::OnLeftButtonUp);

		ReleaseCapture();
		m_bDraggingVerts = FALSE;

		if( !m_bMouseHasMoved && !(GetAsyncKeyState(VK_CONTROL) & 0x8000) )
			DeselectAllVerts();

		WWindow::OnLeftButtonUp();
		unguard;
	}
	void OnSize( DWORD Flags, INT NewX, INT NewY )
	{
		guard(W2DShapeEditor::OnSize);
		PositionChildControls();
		WWindow::OnSize(Flags, NewX, NewY);
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void PositionChildControls( void )
	{
		guard(W2DShapeEditor::PositionChildControls);

		if( !::IsWindow( GetDlgItem( hWnd, ID_2DSE_TOOLBAR )))	return;

		LockWindowUpdate( hWnd );

		FRect CR = GetClientRect();
		RECT R;
		::GetWindowRect( GetDlgItem( hWnd, ID_2DSE_TOOLBAR ), &R );
		::MoveWindow( GetDlgItem( hWnd, ID_2DSE_TOOLBAR ), 0, 0, CR.Max.X, R.bottom, TRUE );

		LockWindowUpdate( NULL );

		unguard;
	}
	virtual void OnKeyDown( TCHAR Ch )
	{
		guard(W2DShapeEditor::OnKeyDown);
		// Hot keys from old version
		if( Ch == 'I' && GetKeyState(VK_CONTROL) & 0x8000)
			SplitSides();
		if( Ch == VK_DELETE )
			Delete();
		unguard;
	}
	// Rotate the shapes by the speifued angle, around the origin,
	void RotateShape( int _Angle )
	{
		guard(W2DShapeEditor::RotateShape);
		FVector l_vec;
		FRotator StepRotation( 0, (65536.0f / 360.0f)  * _Angle, 0 );

		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				l_vec.X = m_tris(tri).Vertex[vertex].X;	l_vec.Y = m_tris(tri).Vertex[vertex].Y;	l_vec.Z = m_tris(tri).Vertex[vertex].Z;
				l_vec = m_origin + ( l_vec - m_origin ).TransformVectorBy( GMath.UnitCoords * StepRotation);
				m_tris(tri).Vertex[vertex].X = l_vec.X;	m_tris(tri).Vertex[vertex].Y = l_vec.Y;	m_tris(tri).Vertex[vertex].Z = l_vec.Z;
			}

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	// Flips the shape across the origin.
	void Flip( BOOL _bHoriz )
	{
		guard(W2DShapeEditor::Flip);

		// Flip the vertices across the origin.
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				FVector Dist = m_tris(tri).Vertex[vertex] - m_origin;

				if( _bHoriz )
					m_tris(tri).Vertex[vertex].X -= (Dist.X * 2);
				else
					m_tris(tri).Vertex[vertex].Y -= (Dist.Y * 2);

			}

			F2DSEVector Save = m_tris(tri).Vertex[0];
			m_tris(tri).Vertex[0] = m_tris(tri).Vertex[2];
			m_tris(tri).Vertex[2] = Save;
		}

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void DeselectAllVerts()
	{
		guard(W2DShapeEditor::DeselectAllVerts);
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			m_tris(tri).Vertex[0].Select(0);
			m_tris(tri).Vertex[1].Select(0);
			m_tris(tri).Vertex[2].Select(0);
		}
		m_origin.Select(0);
		unguard;
	}
	BOOL CheckHits(void)
	{
		guard(W2DShapeEditor::CheckHits);

		// Get the click position in world space.
		//
		FPoint l_click = GetCursorPos();

		l_click.X += -m_camera.X - (m_rcWnd.right / 2);
		l_click.Y += -m_camera.Y - (m_rcWnd.bottom / 2);

		//
		// See if any vertex comes within the selection radius to this point.  If so, select it...
		//

		// Check origin first
		F2DSEVector l_vtxTest;
		l_vtxTest.X = (float)l_click.X - m_origin.X;
		l_vtxTest.Y = (float)l_click.Y + m_origin.Y;
		if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
			return TRUE;

		// Check the rest of the vertices
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				l_vtxTest.X = (float)l_click.X - m_tris(tri).Vertex[vertex].X;
				l_vtxTest.Y = (float)l_click.Y + m_tris(tri).Vertex[vertex].Y;

				if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
					return TRUE;
			}

		return FALSE;
		unguard;
	}
	BOOL ProcessHits( BOOL _bCumulative )
	{
		guard(W2DShapeEditor::ProcessHits);

		if( !_bCumulative )
			DeselectAllVerts();

		// Get the click position in world space.
		//
		FPoint l_click = GetCursorPos();

		l_click.X += -m_camera.X - (m_rcWnd.right / 2);
		l_click.Y += -m_camera.Y - (m_rcWnd.bottom / 2);

		//
		// See if any vertex comes within the selection radius to this point.  If so, select it...
		//

		// Check origin first
		F2DSEVector l_vtxTest;
		l_vtxTest.X = (float)l_click.X - m_origin.X;
		l_vtxTest.Y = (float)l_click.Y + m_origin.Y;
		if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
		{
			m_origin.SelectToggle();
			m_origin.TempX = m_origin.X;
			m_origin.TempY = m_origin.Y;
		}

		// Check the rest of the vertices
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				l_vtxTest.X = (float)l_click.X - m_tris(tri).Vertex[vertex].X;
				l_vtxTest.Y = (float)l_click.Y + m_tris(tri).Vertex[vertex].Y;

				if( l_vtxTest.Size() <= d2dSE_SELECT_TOLERANCE )
				{
					m_tris(tri).Vertex[vertex].SelectToggle();
					m_tris(tri).Vertex[vertex].TempX = m_tris(tri).Vertex[vertex].X;
					m_tris(tri).Vertex[vertex].TempY = m_tris(tri).Vertex[vertex].Y;
				}
			}

		return FALSE;
		unguard;
	}
	void New( void )
	{
		guard(W2DShapeEditor::New);
		m_camera.X = m_camera.Y = 0;
		m_origin.X = m_origin.Y = 0;

		m_tris.Empty();
		new(m_tris)FTriangle( F2DSEVector(-64,-64,0), F2DSEVector(0,64,0), F2DSEVector(64,-64,0) );

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void DrawGrid( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawGrid);
		int l_iXStart, l_iYStart, l_iXEnd, l_iYEnd;
		FVector l_vecTopLeft;
		HPEN l_penOriginLines, l_penMajorLines, l_penMinorLines, l_penOld;

		l_vecTopLeft.X = (m_camera.X * -1) - (m_rcWnd.right / 2);
		l_vecTopLeft.Y = (m_camera.Y * -1) - (m_rcWnd.bottom / 2);

		l_penMinorLines = CreatePen( PS_SOLID, 1, RGB( 235, 235, 235 ) );
		l_penMajorLines = CreatePen( PS_SOLID, 1, RGB( 215, 215, 215 ) );
		l_penOriginLines = CreatePen( PS_SOLID, 3, RGB( 225, 225, 225 ) );

		// Snap the starting position to the grid size.
		//
		l_iXStart = m_iGridSize - (int)l_vecTopLeft.X % m_iGridSize;
		l_iYStart = m_iGridSize - (int)l_vecTopLeft.Y % m_iGridSize;

		l_iXEnd = l_iXStart + m_rcWnd.right;
		l_iYEnd = l_iYStart + m_rcWnd.bottom;
		
		// Draw the lines.
		//
		l_penOld = (HPEN)SelectObject( _hdc, l_penMinorLines );

		static BOOL l_bFirst = TRUE;

		for( int y = l_iYStart ; y < l_iYEnd ; y += m_iGridSize )
		{
			if( l_vecTopLeft.Y + y == 0 )
				SelectObject( _hdc, l_penOriginLines );
			else
				if( !((int)(l_vecTopLeft.Y + y) % 128) )
					SelectObject( _hdc, l_penMajorLines );
				else
					SelectObject( _hdc, l_penMinorLines );

			::MoveToEx( _hdc, 0, y, NULL );
			::LineTo( _hdc, m_rcWnd.right, y );
		}

		for( int x = l_iXStart ; x < l_iXEnd ; x += m_iGridSize )
		{
			if( l_vecTopLeft.X + x == 0 )
				SelectObject( _hdc, l_penOriginLines );
			else
				if( !((int)(l_vecTopLeft.X + x) % 128) )
					SelectObject( _hdc, l_penMajorLines );
				else
					SelectObject( _hdc, l_penMinorLines );

			::MoveToEx( _hdc, x, 0, NULL );
			::LineTo( _hdc, x, m_rcWnd.bottom );
		}

		l_bFirst = FALSE;

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penOriginLines );
		DeleteObject( l_penMinorLines );
		DeleteObject( l_penMajorLines );
		unguard;
	}
	void DrawImage( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawImage);
		if( !hImage ) return;

		HDC hdcMem;
		HBITMAP hbmOld;
		BITMAP bitmap;
		FVector l_vecLoc;

		l_vecLoc.X = m_origin.X + m_camera.X + (m_rcWnd.right / 2);
		l_vecLoc.Y = m_origin.Y + m_camera.Y + (m_rcWnd.bottom / 2);

		// Prepare the bitmap.
		//
		GetObjectA( hImage, sizeof(BITMAP), (LPSTR)&bitmap );
		hdcMem = CreateCompatibleDC(_hdc);
		hbmOld = (HBITMAP)SelectObject(hdcMem, hImage);

		// Display it.
		//
		BitBlt(_hdc,
			   l_vecLoc.X - (bitmap.bmWidth / 2), l_vecLoc.Y - (bitmap.bmHeight / 2),
			   bitmap.bmWidth, bitmap.bmHeight,
			   hdcMem,
			   0, 0,
			   SRCCOPY);

		// Clean up.
		//
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		unguard;
	}
	// Counts how many triangles share the specified edge
	int CountSharingTris( F2DSEVector vtx1, F2DSEVector vtx2 )
	{
		TArray<FTriangle> List, FinalList;

		// Build a short list of the tris that contain the first vtx.
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				if( m_tris(tri).Vertex[vertex] == vtx1 )
					new(List)FTriangle( m_tris(tri).Vertex[0], m_tris(tri).Vertex[1], m_tris(tri).Vertex[2] );

		// Build another list of tris that contain BOTH vtxs.
		for( tri = 0 ; tri < List.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				if( List(tri).Vertex[vertex] == vtx2 )
					new(FinalList)FTriangle( List(tri).Vertex[0], List(tri).Vertex[1], List(tri).Vertex[2] );


		return FinalList.Num();
	}
	void Compute(void)
	{
		//
		// Look through all the triangles and mark the vertices which are the beginnings of edges.
		//

		// Clear all edges.
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				m_tris(tri).Vertex[vertex].bIsEdge = 0;

		// Mark current edges.
		for( tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				if( CountSharingTris( m_tris(tri).Vertex[vertex], m_tris(tri).Vertex[(vertex+1)%3] ) == 1)
					m_tris(tri).Vertex[vertex].bIsEdge = 1;

		//
		// Look for polys which are counter clockwise and mark them so they can be rendered in RED.
		//
		for( tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			FPlane Plane( m_tris(tri).Vertex[0], m_tris(tri).Vertex[1], m_tris(tri).Vertex[2] );
			m_tris(tri).bCCW = (Plane.Z == 1);
		}
	}
	int CountEdges(void)
	{
		int Count = 0;

		// Clear all edges.
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				if( m_tris(tri).Vertex[vertex].bIsEdge )
					Count++;

		return Count;
	}
	// Creates a list of vertices that represent the outer edge of the shape.
	void MakeShape( TArray<F2DSEVector>* pArray )
	{
		pArray->Empty();

		F2DSEVector NextVertex(0,0,0);
		int NumEdges = 0, warren;

		// Find the first edge
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				m_tris(tri).Vertex[vertex].bUsed = 0;
				if( m_tris(tri).Vertex[vertex].bIsEdge )
					NumEdges++;
			}

		for( int edge = 0 ; edge < NumEdges ; edge++ )
		{
			for( tri = 0 ; tri < m_tris.Num() ; tri++ )
			{
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
				{
					if( m_tris(tri).Vertex[vertex].bIsEdge && !m_tris(tri).Vertex[vertex].bUsed )
					{
						if( NextVertex == F2DSEVector(0,0,0) )
						{
							new(*pArray)F2DSEVector(m_tris(tri).Vertex[vertex]);
							NextVertex = m_tris(tri).Vertex[(vertex+1)%3];
							m_tris(tri).Vertex[vertex].bUsed = 1;
							goto BAIL;
						}
						else
						{
							if( m_tris(tri).Vertex[vertex] == NextVertex )
							{
								new(*pArray)F2DSEVector(m_tris(tri).Vertex[vertex]);
								NextVertex = m_tris(tri).Vertex[(vertex+1)%3];
								m_tris(tri).Vertex[vertex].bUsed = 1;
								goto BAIL;
							}
						}
					}
				}
			}
			BAIL:
			warren = 0;
		}
	}
	void DrawOrigin( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawOrigin);
		HPEN l_pen, l_penSel, l_penOld;

		FVector l_vecLoc = m_camera;
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		l_pen = CreatePen( PS_SOLID, 2, RGB( 0, 255, 0 ) );
		l_penSel = CreatePen( PS_SOLID, 4, RGB( 255, 0, 0 ) );

		if( m_origin.IsSel() )
			l_penOld = (HPEN)SelectObject( _hdc, l_penSel );
		else
			l_penOld = (HPEN)SelectObject( _hdc, l_pen );

		Rectangle( _hdc,
			l_vecLoc.X + m_origin.X - 4,
			l_vecLoc.Y - m_origin.Y + 4,
			l_vecLoc.X + m_origin.X + 4,
			l_vecLoc.Y - m_origin.Y - 4 );

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_pen );
		DeleteObject( l_penSel );
		unguard;
	}
	void DrawLines( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawLines);
		FVector l_vecLoc = m_camera;
		HPEN l_penLine, l_penRed, l_penOld;

		l_penLine = CreatePen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
		l_penRed = CreatePen( PS_SOLID, 1, RGB( 255, 0, 0 ) );

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		l_penOld = (HPEN)SelectObject( _hdc, l_penLine );

		//
		// CLOCKWISE
		//

		SelectObject( _hdc, l_penLine );
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			if( !m_tris(tri).bCCW )
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
				{
					::MoveToEx( _hdc,	l_vecLoc.X + m_tris(tri).Vertex[vertex].X,		l_vecLoc.Y - m_tris(tri).Vertex[vertex].Y, NULL );
					::LineTo( _hdc,		l_vecLoc.X + m_tris(tri).Vertex[(vertex+1) % 3].X,	l_vecLoc.Y - m_tris(tri).Vertex[(vertex+1) % 3].Y );
				}

		//
		// COUNTER CLOCKWISE
		//

		SelectObject( _hdc, l_penRed );
		for( tri = 0 ; tri < m_tris.Num() ; tri++ )
			if( m_tris(tri).bCCW )
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
				{
					::MoveToEx( _hdc,	l_vecLoc.X + m_tris(tri).Vertex[vertex].X,		l_vecLoc.Y - m_tris(tri).Vertex[vertex].Y, NULL );
					::LineTo( _hdc,		l_vecLoc.X + m_tris(tri).Vertex[(vertex+1) % 3].X,	l_vecLoc.Y - m_tris(tri).Vertex[(vertex+1) % 3].Y );
				}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penLine );
		DeleteObject( l_penRed );
		unguard;
	}
	void DrawEdges( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawEdges);
		FVector l_vecLoc = m_camera;
		HPEN l_penEdge, l_penRed, l_penOld;

		l_penEdge = CreatePen( PS_SOLID, 3, RGB( 0, 0, 0 ) );
		l_penRed = CreatePen( PS_SOLID, 3, RGB( 255, 0, 0 ) );

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		// Draw the lines.
		//
		l_penOld = (HPEN)SelectObject( _hdc, l_penEdge );

		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			if( m_tris(tri).bCCW )
				SelectObject( _hdc, l_penRed );
			else
				SelectObject( _hdc, l_penEdge );

			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				if( m_tris(tri).Vertex[vertex].bIsEdge )
				{
					::MoveToEx( _hdc,	l_vecLoc.X + m_tris(tri).Vertex[vertex].X,		l_vecLoc.Y - m_tris(tri).Vertex[vertex].Y, NULL );
					::LineTo( _hdc,		l_vecLoc.X + m_tris(tri).Vertex[(vertex+1) % 3].X,	l_vecLoc.Y - m_tris(tri).Vertex[(vertex+1) % 3].Y );
				}
		}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penEdge );
		DeleteObject( l_penRed );
		unguard;
	}
	void DrawVertices( HDC _hdc )
	{
		guard(W2DShapeEditor::DrawVertices);
		FVector l_vecLoc = m_camera;
		HPEN l_penVertex, l_penVertexSel, l_penOld;

		l_penVertex = CreatePen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
		l_penVertexSel = CreatePen( PS_SOLID, 3, RGB( 255, 0, 0 ) );

		// Figure out where the top left corner of the window is in world coords.
		//
		l_vecLoc.X += m_rcWnd.right / 2;
		l_vecLoc.Y += m_rcWnd.bottom / 2;

		// Draw the vertices.
		//
		l_penOld = (HPEN)SelectObject( _hdc, l_penVertex );
		SelectObject( _hdc, GetStockObject(WHITE_BRUSH) );

		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				if( m_tris(tri).Vertex[vertex].IsSel() )
					SelectObject( _hdc, l_penVertexSel );
				else
					SelectObject( _hdc, l_penVertex );

				Rectangle( _hdc,
					l_vecLoc.X + m_tris(tri).Vertex[vertex].X - 4,
					l_vecLoc.Y - m_tris(tri).Vertex[vertex].Y + 4,
					l_vecLoc.X + m_tris(tri).Vertex[vertex].X + 4,
					l_vecLoc.Y - m_tris(tri).Vertex[vertex].Y - 4 );
			}
		}

		SelectObject( _hdc, l_penOld );
		DeleteObject( l_penVertex );
		DeleteObject( l_penVertexSel );
		unguard;
	}
	void GetUniqueSelVerts( TArray<F2DSEVector>* _parray )
	{
		guard(W2DShapeEditor::CountUniqueSelVertices);

		_parray->Empty();

		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
				if( m_tris(tri).Vertex[vertex].IsSel() )
				{
					// If this vertex is not already in the list, add it.
					for( int test = 0 ; test < _parray->Num() ; test++ )
						if( (*_parray)(test) == m_tris(tri).Vertex[vertex] )
							break;

					if( test == _parray->Num() )
						new(*_parray)F2DSEVector(m_tris(tri).Vertex[vertex]);
				}
		unguard;
	}
	// Locates a triangle in the list and deletes it.
	void DeleteTriangle( FTriangle Other )
	{
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			if( m_tris(tri) == Other )
			{
				m_tris.Remove(tri);
				return;
			}
	}
	void SplitSides( void )
	{
		guard(W2DShapeEditor::SplitSides);

		TArray<F2DSEVector> UniqueVerts;
		GetUniqueSelVerts( &UniqueVerts );

		if( UniqueVerts.Num() != 2 )
		{
			appMsgf(TEXT("You must have 2 vertices selected to do a split."));
			return;
		}

		TArray<FTriangle> ShortList;

		// Short list the first vertex.
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				if( m_tris(tri).Vertex[vertex] == UniqueVerts(0) )
				{
					new(ShortList)FTriangle(m_tris(tri));
					ShortList(ShortList.Num() - 1).HitVertex = vertex;
					break;
				}
			}

		// Now that we have a short list of triangles, loop through them and split them along
		// the appropriate edges.
		F2DSEVector SelVtx1, SelVtx2, OtherVtx, MidPoint;

		for( tri = 0 ; tri < ShortList.Num() ; tri++ )
		{
			if( ShortList(tri).Vertex[ (ShortList(tri).HitVertex+2)%3 ] == UniqueVerts(1) )
			{
				SelVtx1 = ShortList(tri).Vertex[ ShortList(tri).HitVertex ];
				SelVtx2 = ShortList(tri).Vertex[ (ShortList(tri).HitVertex+2)%3 ];
				OtherVtx = ShortList(tri).Vertex[ (ShortList(tri).HitVertex+1)%3 ];

				MidPoint = (SelVtx1 + SelVtx2) / 2;

				new(m_tris)FTriangle( SelVtx1, OtherVtx, MidPoint );
				new(m_tris)FTriangle( MidPoint, OtherVtx, SelVtx2 );
				DeleteTriangle( ShortList(tri) );
			}
			if( ShortList(tri).Vertex[ (ShortList(tri).HitVertex+1)%3 ] == UniqueVerts(1) )
			{

				SelVtx1 = ShortList(tri).Vertex[ ShortList(tri).HitVertex ];
				SelVtx2 = ShortList(tri).Vertex[ (ShortList(tri).HitVertex+1)%3 ];
				OtherVtx = ShortList(tri).Vertex[ (ShortList(tri).HitVertex+2)%3 ];

				MidPoint = (SelVtx1 + SelVtx2) / 2;

				new(m_tris)FTriangle( SelVtx1, MidPoint, OtherVtx );
				new(m_tris)FTriangle( MidPoint, SelVtx2, OtherVtx );

				DeleteTriangle( ShortList(tri) );
			}
		}

		for( tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				if( m_tris(tri).Vertex[vertex] == SelVtx1 )
					m_tris(tri).Vertex[vertex].Select(0);
				if( m_tris(tri).Vertex[vertex] == SelVtx2 )
					m_tris(tri).Vertex[vertex].Select(0);
				if( m_tris(tri).Vertex[vertex] == MidPoint )
				{
					m_tris(tri).Vertex[vertex].Select(1);
					m_tris(tri).Vertex[vertex].TempX = m_tris(tri).Vertex[vertex].X;
					m_tris(tri).Vertex[vertex].TempY = m_tris(tri).Vertex[vertex].Y;
				}
			}

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void Delete( void )
	{
		guard(W2DShapeEditor::Delete);

		//
		// Delete all triangles which are touching selected vertices.
		//

		TArray<F2DSEVector> UniqueVerts;
		GetUniqueSelVerts( &UniqueVerts );

		for( int uniq = 0 ; uniq < UniqueVerts.Num() ; uniq++ )
			for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
					if( m_tris(tri).Vertex[vertex] == UniqueVerts(uniq) )
					{
						DeleteTriangle(m_tris(tri));
						tri = 0;
						break;
					}

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );
		unguard;
	}
	void FileSaveAs( HWND hWnd )
	{
		OPENFILENAMEA ofn;
		char File[256] = "\0";
		strcpy( File, TCHAR_TO_ANSI( MapFilename ) );

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(char) * 256;
		ofn.lpstrFilter = "2D Shapes (*.2ds)\0*.2ds\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_2DS]) );
		ofn.lpstrDefExt = "2ds";
		ofn.lpstrTitle = "Save 2D Shape";
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

		// Display the Open dialog box. 
		//
		if( GetSaveFileNameA(&ofn) )
		{
			appStrcpy( MapFilename, appFromAnsi( File ) );
			WriteShape( MapFilename );

			FString S = MapFilename;
			GLastDir[eLASTDIR_2DS] = S.Left( S.InStr( TEXT("\\"), 1 ) );
		}

		SetCaption();
	}
	void FileSave( HWND hWnd )
	{
		if( ::appStrlen( MapFilename ) )
			WriteShape( MapFilename );
		else
			FileSaveAs( hWnd );
	}
	void FileOpen( HWND hWnd )
	{
		OPENFILENAMEA ofn;
		char File[256] = "\0";

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = File;
		ofn.nMaxFile = sizeof(char) * 256;
		ofn.lpstrFilter = "2D Shapes (*.2ds)\0*.2ds\0All Files\0*.*\0\0";
		ofn.lpstrInitialDir = appToAnsi( *(GLastDir[eLASTDIR_2DS]) );
		ofn.lpstrDefExt = "2ds";
		ofn.lpstrTitle = "Open 2D Shape";
		ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

		// Display the Open dialog box. 
		//
		if( GetOpenFileNameA(&ofn) )
		{
			appStrcpy( MapFilename, appFromAnsi( File ) );
			ReadShape( MapFilename );
			SetCaption();

			FString S = MapFilename;
			GLastDir[eLASTDIR_2DS] = S.Left( S.InStr( TEXT("\\"), 1 ) );
		}
	}
	void SetCaption( void )
	{
		TCHAR l_chCaption[256];

		appSprintf( l_chCaption, TEXT("2D Shape Editor - [%s]"), MapFilename );
		SetText( l_chCaption );
	}
	void WriteShape( TCHAR* Filename )
	{
		FArchive* Archive;
		Archive = GFileManager->CreateFileWriter( Filename );

		if( Archive )
		{
			// Header
			//
			int x = m_tris.Num();
			*Archive << x;

			// Origin
			//
			*Archive << m_origin.X << m_origin.Y;
			
			// Vertices
			//
			for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
					*Archive << m_tris(tri).Vertex[vertex].X << m_tris(tri).Vertex[vertex].Y;

			Archive->Close();
		}
	}
	void ReadShape( TCHAR* Filename )
	{
		FArchive* Archive;
		Archive = GFileManager->CreateFileReader( Filename );

		if( Archive )
		{
			m_tris.Empty();

			// Header
			//
			int NumTris;
			Archive->Serialize( &NumTris, sizeof(int) );

			// Origin
			//
			Archive->Serialize( &m_origin.X, sizeof(float) );
			Archive->Serialize( &m_origin.Y, sizeof(float) );
			
			// Vertices
			//
			F2DSEVector vtxs[3];
			for( int tri = 0 ; tri < NumTris ; tri++ )
			{
				for( int vertex = 0 ; vertex < 3 ; vertex++ )
				{
					Archive->Serialize( &vtxs[vertex].X, sizeof(float) );
					Archive->Serialize( &vtxs[vertex].Y, sizeof(float) );
					vtxs[vertex].Z = 0;
				}

				new(m_tris)FTriangle( vtxs[0], vtxs[1], vtxs[2] );
			}

			Archive->Close();
		}

		Compute();
		InvalidateRect( hWnd, NULL, FALSE );
	}
	void SetOrigin( void )
	{
		POINT l_click = m_ContextPos;

		l_click.x += -m_camera.X - (m_rcWnd.right / 2);
		l_click.y += -m_camera.Y - (m_rcWnd.bottom / 2);

		l_click.x -= l_click.x % m_iGridSize;
		l_click.y -= l_click.y % m_iGridSize;

		m_origin.X = l_click.x;
		m_origin.Y = -l_click.y;

		InvalidateRect( hWnd, NULL, FALSE );
	}
	void ProcessSheet()
	{
		guard(W2DShapeEditor::ProcessSheet)
		FString Cmd;

		Cmd += TEXT("BRUSH SET\n\n");
		Cmd += TEXT("Begin PolyList\n");

		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, 0.0\n"),
					m_tris(tri).Vertex[vertex].X, m_tris(tri).Vertex[vertex].Y));
			}

			Cmd += TEXT("End Polygon\n");
		}
	
		Cmd += TEXT("End PolyList");

		GEditor->Exec( *Cmd );
		unguard;
	}
	void ProcessExtrude( int Depth )
	{
		guard(W2DShapeEditor::ProcessExtrude)

		FString Cmd;

		Cmd += TEXT("BRUSH SET\n\n");

		// Create the top.
		Cmd += TEXT("Begin PolyList\n");
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					m_tris(tri).Vertex[vertex].X, m_tris(tri).Vertex[vertex].Y, m_tris(tri).Vertex[vertex].Z));
			}

			Cmd += TEXT("End Polygon\n");
		}
		Cmd += TEXT("End PolyList");

		// Create the bottom.
		Cmd += TEXT("Begin PolyList\n");
		for( tri = 0 ; tri < m_tris.Num() ; tri++ )
		{
			Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

			for( int vertex = 2 ; vertex > -1 ; vertex-- )
			{
				Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
					m_tris(tri).Vertex[vertex].X, m_tris(tri).Vertex[vertex].Y, m_tris(tri).Vertex[vertex].Z + Depth));
			}

			Cmd += TEXT("End Polygon\n");
		}
		Cmd += TEXT("End PolyList");

		// Create an ordered list of vertices which defines the outer edge of the shape.
		TArray<F2DSEVector> Shape;
		MakeShape( &Shape );

		Cmd += TEXT("Begin PolyList\n");

		// Create the sides.
		for( int vertex = 0 ; vertex < Shape.Num() ; vertex++ )
		{
			Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

			Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
				Shape((vertex+1)%Shape.Num()).X, Shape((vertex+1)%Shape.Num()).Y, Shape((vertex+1)%Shape.Num()).Z ));
			Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
				Shape(vertex).X, Shape(vertex).Y, Shape(vertex).Z ));
			Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
				Shape(vertex).X, Shape(vertex).Y, Shape(vertex).Z + Depth ));
			Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
				Shape((vertex+1)%Shape.Num()).X, Shape((vertex+1)%Shape.Num()).Y, Shape((vertex+1)%Shape.Num()).Z + Depth ));

			Cmd += TEXT("End Polygon\n");
		}

		// Finish up and submit it.
		Cmd += TEXT("End PolyList");

		GEditor->Exec( *Cmd );

		unguard;
	}
	void ProcessRevolve( int TotalSides, int Sides )
	{
		guard(W2DShapeEditor::ProcessRevolve)
		BOOL bPositive, bNegative, bBrushSetDone = FALSE;
		BOOL bFromLeftSide;

		// Make sure the origin is totally to the left or right of the shape.
		bPositive = bNegative = FALSE;
		for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			for( int vertex = 0 ; vertex < 3 ; vertex++ )
			{
				if( m_origin.X > m_tris(tri).Vertex[vertex].X )		bPositive = TRUE;
				if( m_origin.X < m_tris(tri).Vertex[vertex].X )		bNegative = TRUE;
			}
		if( bPositive && bNegative )
		{
			appMsgf( TEXT("Origin must be completely to left or right side of the shape.") );
			return;
		}

		// When revolving from the left side, we have to flip the polys around.
		bFromLeftSide = ( bNegative && !bPositive );

		// Create an ordered list of vertices which defines the outer edge of the shape.
		TArray<F2DSEVector> Shape;
		MakeShape( &Shape );

		// Create the sides.
		FString Cmd;
		FVector* l_pvecRotatedShapes;

		int NumEdges = Shape.Num();
		l_pvecRotatedShapes = new FVector[TotalSides * NumEdges];

		// Create a pool of rotated shapes.
		FVector l_vec, l_origin;
		float l_fRotStep = 65536.0f / TotalSides;
		FRotator StepRotation( l_fRotStep, 0, 0 );

		l_origin.X = m_origin.X;
		l_origin.Y = m_origin.Y;
		l_origin.Z = m_origin.Z;

		for( int l_iSide = 0 ; l_iSide < TotalSides ; l_iSide++ )
		{
			for( int vertex = 0 ; vertex < Shape.Num() ; vertex++ )
			{
				l_vec.X = Shape(vertex).X;	l_vec.Y = Shape(vertex).Y;	l_vec.Z = Shape(vertex).Z;
				l_vec = l_origin + ( l_vec - l_origin ).TransformVectorBy( GMath.UnitCoords * (StepRotation * l_iSide) );
				l_pvecRotatedShapes[(l_iSide * NumEdges) + vertex] = l_vec;
			}
		}

		// Create a top if we are doing a partial revolve.
		if( Sides < TotalSides )
		{
			Cmd += TEXT("BRUSH SET\n\n");
			bBrushSetDone = TRUE;
			Cmd += TEXT("Begin PolyList\n");

			for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			{
				Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

				if( bFromLeftSide )
					for( int vertex = 0 ; vertex < 3 ; vertex++ )
					{
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							m_tris(tri).Vertex[vertex].X, m_tris(tri).Vertex[vertex].Y, m_tris(tri).Vertex[vertex].Z));
					}
				else
					for( int vertex = 2 ; vertex > -1 ; vertex-- )
					{
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							m_tris(tri).Vertex[vertex].X, m_tris(tri).Vertex[vertex].Y, m_tris(tri).Vertex[vertex].Z));
					}

				Cmd += TEXT("End Polygon\n");
			}

			Cmd += TEXT("End PolyList");
		}

		// Create a bottom if we are doing a partial revolve.
		if( Sides < TotalSides )
		{
			Cmd += TEXT("BRUSH MORE\n\n");
			Cmd += TEXT("Begin PolyList\n");

			// For each triangle, rotate it around the sape before placing it.
			for( int tri = 0 ; tri < m_tris.Num() ; tri++ )
			{
				Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

				if( bFromLeftSide )
					for( int vertex = 2 ; vertex > -1 ; vertex-- )
					{
						l_vec.X = m_tris(tri).Vertex[vertex].X;
						l_vec.Y = m_tris(tri).Vertex[vertex].Y;
						l_vec.Z = m_tris(tri).Vertex[vertex].Z;
						l_vec = m_origin + ( l_vec - m_origin ).TransformVectorBy( GMath.UnitCoords * (StepRotation * Sides) );
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							l_vec.X, l_vec.Y, l_vec.Z ));
					}
				else
					for( int vertex = 0 ; vertex < 3 ; vertex++ )
					{
						l_vec.X = m_tris(tri).Vertex[vertex].X;
						l_vec.Y = m_tris(tri).Vertex[vertex].Y;
						l_vec.Z = m_tris(tri).Vertex[vertex].Z;
						l_vec = m_origin + ( l_vec - m_origin ).TransformVectorBy( GMath.UnitCoords * (StepRotation * Sides) );
						Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
							l_vec.X, l_vec.Y, l_vec.Z ));
					}

				Cmd += TEXT("End Polygon\n");
			}

			Cmd += TEXT("End PolyList");
		}

		// Create the segments we need.
		FVector l_vec1, l_vec2, l_vec3, l_vec4;
		int IndexVtx1, IndexVtx2, IndexSide1, IndexSide2;

		for( l_iSide = 0 ; l_iSide < Sides ; l_iSide++ )
		{
			IndexVtx1 = 0;
			IndexVtx2 = 1;
			IndexSide1 = l_iSide;
			IndexSide2 = l_iSide + 1;		if( IndexSide2 == TotalSides ) { IndexSide2 = 0; }

			if( !bBrushSetDone )
			{
				Cmd += TEXT("BRUSH SET\n\n");
				bBrushSetDone = TRUE;
			}
			else
				Cmd += TEXT("BRUSH MORE\n\n");

			Cmd += TEXT("Begin PolyList\n");

			for( int vertex = 0 ; vertex < NumEdges ; vertex++ )
			{
				Cmd += *(FString::Printf(TEXT("Begin Polygon Flags=%d\n"), PF_NotSolid));

				l_vec1 = l_pvecRotatedShapes[(IndexSide1 * NumEdges) + IndexVtx1];
				l_vec2 = l_pvecRotatedShapes[(IndexSide1 * NumEdges) + IndexVtx2];
				l_vec3 = l_pvecRotatedShapes[(IndexSide2 * NumEdges) + IndexVtx1];
				l_vec4 = l_pvecRotatedShapes[(IndexSide2 * NumEdges) + IndexVtx2];

				if( bFromLeftSide )
				{
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec3.X, l_vec3.Y, l_vec3.Z));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec4.X, l_vec4.Y, l_vec4.Z));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec2.X, l_vec2.Y, l_vec2.Z));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec1.X, l_vec1.Y, l_vec1.Z));
				}
				else
				{
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec1.X, l_vec1.Y, l_vec1.Z));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec2.X, l_vec2.Y, l_vec2.Z));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec4.X, l_vec4.Y, l_vec4.Z));
					Cmd += *(FString::Printf(TEXT("Vertex   %1.1f, %1.1f, %1.1f\n"),
						l_vec3.X, l_vec3.Y, l_vec3.Z));
				}

				IndexVtx1++;		if( IndexVtx1 == NumEdges ) { IndexVtx1 = 0; }
				IndexVtx2++;		if( IndexVtx2 == NumEdges ) { IndexVtx2 = 0; }

				Cmd += TEXT("End Polygon\n");
			}

			Cmd += TEXT("End PolyList");
		}

		// Finish up and submit it.
		GEditor->Exec( *Cmd );

		delete [] l_pvecRotatedShapes;

		unguard;
	}
};
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif