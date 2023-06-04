//=============================================================================
// Options2DShaperBezierDetail: Options for custom detail levels in beziers
//
//=============================================================================
class Options2DShaperBezierDetail
	extends OptionsProxy
	native;

var() int DetailLevel;

DefaultProperties
{
	DetailLevel=10

	DlgCaption="Bezier Detail"
}
