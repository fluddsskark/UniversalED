#if 1 //NEW: U2Ed
/*=============================================================================
	Extern.h: External declarations that we need to communicate with editor.dll
	Copyright 1999 Legend Entertainment, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall
=============================================================================*/

__declspec(dllimport) void __stdcall NE_EdStarting( void );
__declspec(dllimport) void __stdcall NE_EdInitServer( HWND hInWndMain, HWND hInWndCallback );
__declspec(dllimport) FStringOutputDevice* GetPropResult;

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif