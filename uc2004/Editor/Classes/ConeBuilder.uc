//=============================================================================
// ConeBuilder: Builds a 3D cone brush, compatible with cylinder of same size.
//=============================================================================
class ConeBuilder
	extends BrushBuilder;

var() float Height, CapHeight, OuterRadius, InnerRadius;
var() int Sides;
var() name GroupName;
var() bool AlignToSide, Hollow;

final function BuildCone( int Direction, bool bAlignToSide, int NumSides, float NewHeight, float Radius, name Item )
{
	local int n,i,Ofs;
	n = GetVertexCount();
	if (bAlignToSide)
	{
		Radius /= cos(pi/NumSides);
		Ofs = 1;
	}

	// Vertices.
	for( i=0; i<NumSides; i++ )
		Vertex3f( Radius*sin((2*i+Ofs)*pi/NumSides), Radius*cos((2*i+Ofs)*pi/NumSides), 0 );
	Vertex3f( 0, 0, NewHeight );

	// Polys.
	for( i=0; i<NumSides; i++ )
		Poly3i( Direction, n+i, n+NumSides, n+((i+1)%NumSides), Item );
}

function bool Build()
{
	local int i;

	if( Sides<3 )
		return BadParameters();
	if( Height<=0 || OuterRadius<=0 )
		return BadParameters();
	if( Hollow && (InnerRadius<=0 || InnerRadius>=OuterRadius) )
		return BadParameters();
	if( Hollow && CapHeight>Height )
		return BadParameters();
	if( Hollow && (CapHeight==Height && InnerRadius==OuterRadius) )
		return BadParameters();

	BeginBrush( false, GroupName );
	BuildCone( +1, AlignToSide, Sides, Height, OuterRadius, 'Top' );
	if( Hollow )
	{
		BuildCone( -1, AlignToSide, Sides, CapHeight, InnerRadius, 'Cap' );
		if( OuterRadius!=InnerRadius )
			for( i=0; i<Sides; i++ )
				Poly4i( 1, i, ((i+1)%Sides), Sides+1+((i+1)%Sides), Sides+1+i, 'Bottom' );
	}
	else
	{
		PolyBegin( 1, 'Bottom' );
		for( i=0; i<Sides; i++ )
			Polyi( i );
		PolyEnd();
	}
	return EndBrush();
}

defaultproperties
{
	Height=256
	CapHeight=256
	OuterRadius=512
	InnerRadius=384
	Sides=8
	GroupName=Cone
	AlignToSide=true
	Hollow=false
	BitmapFilename="BBCone"
	ToolTip="Cone"
}
