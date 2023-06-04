//=============================================================================
// OptionsSurfBevel: Options for bevelling surfaces
//
//=============================================================================
class OptionsSurfBevel
	extends OptionsProxy
	native;

var() int Depth;
var() int Bevel;

DefaultProperties
{
	Depth=16
	Bevel=16

	DlgCaption="Bevel"
}
