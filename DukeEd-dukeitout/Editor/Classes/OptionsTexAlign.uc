//=============================================================================
// OptionsTexAlign: Basic options for texture alignment
//
//=============================================================================
class OptionsTexAlign
	extends OptionsProxy
	native;

var() enum ETAxis
{
	TAXIS_X,
	TAXIS_Y,
	TAXIS_Z,
	TAXIS_AUTO,
	TAXIS_WALLS,
} TAxis;

DefaultProperties
{
	TAxis=TAXIS_AUTO
}
