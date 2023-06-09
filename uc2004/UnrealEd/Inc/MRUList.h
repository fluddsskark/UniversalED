/*=============================================================================
	MRUList : Helper class for handling MRU lists
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Warren Marshall

    Work-in-progress todo's:

=============================================================================*/

#define MRU_MAX_ITEMS 8

class MRUList
{
public:
	MRUList()
	{
		check(0);	// wrong constructor
	}
	MRUList( FString InINISection )
	{
		NumItems = 0;
		INISection = InINISection;
	}
	~MRUList()
	{}

	FString Items[MRU_MAX_ITEMS], INISection;
	INT NumItems;

	void ReadINI()
	{
		guard(MRUList::ReadINI);
		NumItems = 0;
		for( INT mru = 0 ; mru < MRU_MAX_ITEMS ; ++mru )
		{
			FString Item;
			GConfig->GetString( *INISection, *FString::Printf(TEXT("MRUItem%d"),mru), Item, TEXT("UnrealEd.ini") );

			// If we get a item, add it to the top of the MRU list.
			if( Item.Len() )
			{
				Items[mru] = Item;
				NumItems++;
			}
		}
		unguard;
	}
	void WriteINI()
	{
		guard(MRUList::WriteINI);
		for( INT mru = 0 ; mru < NumItems ; ++mru )
			GConfig->SetString( *INISection, *FString::Printf(TEXT("MRUItem%d"),mru), *Items[mru], TEXT("UnrealEd.ini") );
		unguard;
	}

	// Moves the specified item to the top of the list.
	void MoveToTop( INT InItem )
	{
		guard(MRUList::MoveToTop);

		check( InItem < NumItems );

		FString Save = Items[InItem];
		for( INT x = InItem - 1 ; x > -1  ; --x )
			Items[x+1] = Items[x];
		Items[0] = Save;

		unguard;
	}
	// Adds a item to the MRU list.  New itemss are added to the bottom of the list.
	void AddItem( FString Item )
	{
		guard(MRUList::AddItem);
		// See if the item already exists in the list...
		for( INT mru = 0 ; mru < NumItems ; ++mru )
			if( Items[mru] == Item )
				return;

		if( Item.Len() )
		{
			NumItems++;
			NumItems = min(NumItems, MRU_MAX_ITEMS);

			for( INT mru = MRU_MAX_ITEMS - 1 ; mru > 0 ; --mru )
				Items[mru] = Items[mru - 1];
			Items[0] = Item;
		}

		unguard;
	}
	// Removes the specified item from the list
	void RemoveItem( FString InItem )
	{
		guard(MRUList::RemoveItem);

		// Find the item and remove it
		for( INT mru = 0 ; mru < NumItems ; ++mru )
			if( Items[mru] == InItem )
			{
				for( INT x = mru ; x < MRU_MAX_ITEMS-1 ; ++x )
					Items[x] = Items[x+1];
				NumItems--;
				return;
			}

		unguard;
	}

	// Adds all currently known MRU items to the "File" menu.  This
	// replaces the current menu with a new one.
	void AddToMenu( HWND hWnd, HMENU Menu, UBOOL bHasExit = 0 )
	{
		guard(MRUList::AddToMenu);

		// Get the file menu - this is assumed to be the first submenu.
		HMENU FileMenu = GetSubMenu( Menu, 0 );
		if( !FileMenu )
			return;

		// Destroy all MRU items on the menu, as well as the seperate and "Exit".
		DeleteMenu( FileMenu, ID_FileExit, MF_BYCOMMAND );
		DeleteMenu( FileMenu, IDMN_MRU_SEP, MF_BYCOMMAND );
		for( INT x = 0 ; x < MRU_MAX_ITEMS ; ++x )
			DeleteMenu( FileMenu, IDMN_MRU1 + x, MF_BYCOMMAND );

		// Add any MRU items we have to the menu, and add an "Exit" option if
		// requested.
		MENUITEMINFOA mif;
		TCHAR tchItem[256] = TEXT("\0");

		mif.cbSize = sizeof(MENUITEMINFO);
		mif.fMask = MIIM_TYPE | MIIM_ID;
		mif.fType = MFT_STRING;

		for( INT x = 0 ; x < NumItems ; ++x )
		{
			appSprintf(tchItem, TEXT("&%d %s"), x + 1, *Items[x] );
			mif.dwTypeData = TCHAR_TO_ANSI( tchItem );
			mif.wID = IDMN_MRU1 + x;

			InsertMenuItemA( FileMenu, 99999, FALSE, &mif );
		}

		// Only add this seperator if we actually have MRU items... looks weird otherwise.
		if( NumItems && bHasExit )
		{
			mif.fType = MFT_SEPARATOR;
			mif.wID = IDMN_MRU_SEP;
			InsertMenuItemA( FileMenu, 99999, FALSE, &mif );
		}

		if( bHasExit )
		{
			mif.fType = MFT_STRING;
			mif.dwTypeData = "Exit";
			mif.wID = ID_FileExit;
			InsertMenuItemA( FileMenu, 99999, FALSE, &mif );
		}

		unguard;
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
