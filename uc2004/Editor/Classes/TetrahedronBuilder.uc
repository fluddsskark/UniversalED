//=============================================================================
// TetrahedronBuilder: Builds an octahedron (not tetrahedron) - experimental.
//=============================================================================
class TetrahedronBuilder
	extends BrushBuilder;

var() float Radius;
var() int SphereExtrapolation;
var() name GroupName;

final function Extrapolate(int a, int b, int c, int Count, float NewRadius)
{
	local int ab,bc,ca;
	if( Count>1 )
	{
		ab=Vertexv( NewRadius*Normal(GetVertex(a)+GetVertex(b)) );
		bc=Vertexv( NewRadius*Normal(GetVertex(b)+GetVertex(c)) );
		ca=Vertexv( NewRadius*Normal(GetVertex(c)+GetVertex(a)) );
		Extrapolate(a,ab,ca,Count-1,NewRadius);
		Extrapolate(b,bc,ab,Count-1,NewRadius);
		Extrapolate(c,ca,bc,Count-1,NewRadius);
		Extrapolate(ab,bc,ca,Count-1,NewRadius);
		//wastes shared vertices
	}
	else Poly3i(+1,a,b,c);
}

final function BuildTetrahedron( float R, int NewSphereExtrapolation )
{
	vertex3f( R,0,0);
	vertex3f(-R,0,0);
	vertex3f(0, R,0);
	vertex3f(0,-R,0);
	vertex3f(0,0, R);
	vertex3f(0,0,-R);

	Extrapolate(2,1,4,NewSphereExtrapolation,Radius);
	Extrapolate(1,3,4,NewSphereExtrapolation,Radius);
	Extrapolate(3,0,4,NewSphereExtrapolation,Radius);
	Extrapolate(0,2,4,NewSphereExtrapolation,Radius);
	Extrapolate(1,2,5,NewSphereExtrapolation,Radius);
	Extrapolate(3,1,5,NewSphereExtrapolation,Radius);
	Extrapolate(0,3,5,NewSphereExtrapolation,Radius);
	Extrapolate(2,0,5,NewSphereExtrapolation,Radius);
}

event bool Build()
{
	if( Radius<=0 || SphereExtrapolation<=0 )
		return BadParameters();

	BeginBrush( false, GroupName );
	BuildTetrahedron( Radius, SphereExtrapolation );
	return EndBrush();
}

defaultproperties
{
	Radius=256
	SphereExtrapolation=1
	GroupName=Tetrahedron
	BitmapFilename="BBSphere"
	ToolTip="Tetrahedron (Sphere)"
}
