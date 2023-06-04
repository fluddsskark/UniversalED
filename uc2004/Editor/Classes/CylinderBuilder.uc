//=============================================================================
// CylinderBuilder: Builds a 3D cylinder brush.
//=============================================================================
class CylinderBuilder
	extends BrushBuilder;

var() float Height, OuterRadius, InnerRadius;
var() int Sides;
var() name GroupName;
var() bool AlignToSide, Hollow;

final function BuildCylinder( int Direction, bool bAlignToSide, int NumSides, float NewHeight, float Radius )
{
	local int n,i,j,Ofs;
	n = GetVertexCount();
	if( bAlignToSide )
	{
		Radius /= cos(pi/NumSides);
		Ofs = 1;
	}

	// Vertices.
	for( i=0; i<NumSides; i++ )
		for( j=-1; j<2; j+=2 )
			Vertex3f( Radius*sin((2*i+Ofs)*pi/NumSides), Radius*cos((2*i+Ofs)*pi/NumSides), j*NewHeight/2 );

	// Polys.
	for( i=0; i<NumSides; i++ )
		Poly4i( Direction, n+i*2, n+i*2+1, n+((i*2+3)%(2*NumSides)), n+((i*2+2)%(2*NumSides)), 'Wall' );
}

function bool Build()
{
	local int i,j;

	if( Sides<3 )
		return BadParameters();
	if( Height<=0 || OuterRadius<=0 )
		return BadParameters();
	if( Hollow && (InnerRadius<=0 || InnerRadius>=OuterRadius) )
		return BadParameters();

	BeginBrush( false, GroupName );
	BuildCylinder( +1, AlignToSide, Sides, Height, OuterRadius );
	if( Hollow )
	{
		BuildCylinder( -1, AlignToSide, Sides, Height, InnerRadius );
		for( j=-1; j<2; j+=2 )
			for( i=0; i<Sides; i++ )
				Poly4i( j, i*2+(1-j)/2, ((i+1)%Sides)*2+(1-j)/2, ((i+1)%Sides)*2+(1-j)/2+Sides*2, i*2+(1-j)/2+Sides*2, 'Cap' );
	}
	else
	{
		for( j=-1; j<2; j+=2 )
		{
			PolyBegin( j, 'Cap' );
			for( i=0; i<Sides; i++ )
				Polyi( i*2+(1-j)/2 );
			PolyEnd();
		}
	}
	return EndBrush();
}

defaultproperties
{
	Height=256
	OuterRadius=512
	InnerRadius=384
	Sides=8
	GroupName=Cylinder
	AlignToSide=true
	Hollow=false
	BitmapFilename="BBCylinder"
	ToolTip="Cylinder"
}
