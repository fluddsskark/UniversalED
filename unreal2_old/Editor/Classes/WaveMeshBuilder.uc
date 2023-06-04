//=============================================================================
// WaveMeshBuilder: Builds a wavy mesh object and adds it to the level.
//=============================================================================
class WaveMeshBuilder extends BrushBuilder
	native;

var() float	Width;				// Width of grid in units.
var() float	Height;				// Height of grid in units.
var() float	Magnitude;			// Max height of a vertex from the plane.
var() float	MinRate, MaxRate;	// How fast the verticies move up and down.
var() int	USubDiv;			// Number of columns in grid.
var() int	VSubDiv;			// Number of rows in grid.
var() int	UTexTile;			// Number of columns wide a single texture uses.
var() int	VTexTile;			// Number of rows tall a single texture uses.
var() float	Noise;				// How far vertices are allowed to deviate from their normal grid location.  (0.0...1.0)
var() bool	bFixedEdges;		// Whether or not the edges of the mesh move.
var() bool	bFlushEdges;		// Whether or not to fill in the lateral edges with extra polys to make them smooth.  (MT_Tri only)
var() bool  bShiny;				// Use environmapped multitexturing?
var() bool  bMultiTexture;		// Use multitexturing?  -- not compatible with bShiny.

var() enum EMeshType
{
	MT_Quad,
	MT_Tri
} MeshType;

native function bool CreateWavyMesh
(
	EMeshType		MeshType,
	float			Width,
	float			Height,
	float			Magnitude,
	float			MinRate,
	float			MaxRate,
	int				USubDiv,
	int				VSubDiv,
	optional int	UTexTile,
	optional int	VTexTile,
	optional float	Noise,
	optional bool	bFixedEdges,
	optional bool	bFlushEdges,
	optional bool	bShiny,
	optional bool	bMultiTexture
);

event bool Build()
{
	return CreateWavyMesh( MeshType, Width, Height, Magnitude, MinRate, MaxRate, USubDiv, VSubDiv, UTexTile, VTexTile, Noise, bFixedEdges, bFlushEdges, bShiny, bMultiTexture );
}

defaultproperties
{
     MeshType=MT_Quad
     Width=1024.000000
     Height=1024.000000
     Magnitude=16.000000
     MinRate=2.000000
     MaxRate=4.000000
     USubDiv=10
     VSubDiv=10
     UTexTile=0
     VTexTile=0
	 Noise=0.000000
     bFixedEdges=false
	 bFlushEdges=false
}
