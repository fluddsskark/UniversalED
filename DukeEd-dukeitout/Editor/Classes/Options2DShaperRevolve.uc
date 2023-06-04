//=============================================================================
// Options2DShaperRevolve: Options for revolving in the 2D shaper.
//
//=============================================================================
class Options2DShaperRevolve
	extends Options2DShaper
	native;

var() int Per360;
var() int Use;

DefaultProperties
{
	Per360=12
	Use=3

	DlgCaption="Revolve"
}
