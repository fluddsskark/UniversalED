//=============================================================================
// Object to facilitate properties editing
//=============================================================================
//  Preferences tab for the animation browser...
//  
 
class SkelPrefsEditProps extends MeshObject
	hidecategories(Object)
	native;
		
cpptext
{
	void PostEditChange();
}

var const int WBrowserAnimationPtr;

var(Interface) int         RootZero;
var(Interface) float       AnimSlomo;

defaultproperties
{	
	RootZero = 0;
	AnimSlomo = 1.0;
}
