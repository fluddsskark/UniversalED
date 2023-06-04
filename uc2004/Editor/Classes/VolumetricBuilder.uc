//=============================================================================
// VolumetricBuilder: Builds a volumetric brush (criss-crossed sheets).
//=============================================================================
class VolumetricBuilder
	extends BrushBuilder;

var() float Height, Radius;
var() int NumSheets;
var() name GroupName;

final function BuildVolumetric(int Direction, int Sheets, float NewHeight, float NewRadius)
{
	local int n,x,y;
	local rotator RotStep;
	local vector vtx, NewVtx;

	n = GetVertexCount();
	RotStep.Yaw = 65536.0f / (Sheets * 2);

	// Vertices.
	vtx.x = NewRadius;
	vtx.z = NewHeight / 2;
	for( x = 0 ; x < (Sheets * 2) ; x++ )
	{
		NewVtx = vtx >> (RotStep * x);
		Vertex3f( NewVtx.x, NewVtx.y, NewVtx.z );
		Vertex3f( NewVtx.x, NewVtx.y, NewVtx.z - NewHeight );
	}

	// Polys.
	for( x = 0 ; x < Sheets ; x++ )
	{
		y = (x*2) + 1;
		if( y >= (Sheets * 2) ) y -= (Sheets * 2);
		Poly4i( Direction, n+(x*2), n+y, n+y+(Sheets*2), n+(x*2)+(Sheets*2), 'Sheets', 0x00000108); // PF_TwoSided|PF_NotSolid.
	}
}

function bool Build()
{
	if( NumSheets<2 )
		return BadParameters();
	if( Height<=0 || Radius<=0 )
		return BadParameters();

	BeginBrush( true, GroupName );
	BuildVolumetric( +1, NumSheets, Height, Radius );
	return EndBrush();
}

defaultproperties
{
	Height=128
	Radius=64
	NumSheets=2
	GroupName=Volumetric
	BitmapFilename="BBVolumetric"
	ToolTip="Volumetric (Torches, Chains, etc)"
}
