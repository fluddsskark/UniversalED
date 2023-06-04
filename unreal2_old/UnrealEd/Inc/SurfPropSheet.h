#if 1 //NEW: U2Ed
#include <windows.h>
#include <commctrl.h>

enum eSPS {
	eSPS_FLAGS1		= 0,
	eSPS_FLAGS2		= 1,
	eSPS_ALIGNMENT	= 2,
	eSPS_STATS		= 3,
	eSPS_MAX		= 4
};

class TSurfPropSheet
{
public:

	TSurfPropSheet();
	~TSurfPropSheet();

	void OpenWindow( HINSTANCE hInst, HWND hWndOwner );
	void Show( BOOL bShow );
	void GetDataFromSurfs1(void);
	void GetDataFromSurfs2(void);
	void RefreshStats(void);

	PROPSHEETPAGE m_pages[eSPS_MAX];
    PROPSHEETHEADER m_psh;
	HWND m_hwndSheet;
	BOOL m_bShow;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif