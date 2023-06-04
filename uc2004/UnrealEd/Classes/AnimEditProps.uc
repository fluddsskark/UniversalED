//=============================================================================
// Object to facilitate properties editing
//=============================================================================
//  Animation / Mesh editor object to expose/shuttle only selected editable 
//  parameters from UMeshAnim/ UMesh objects back and forth in the editor.
//  

class AnimEditProps extends MeshObject
	hidecategories(Object)
	native;	

// scion sz
// list of bones and their translation method to aid
// in deciding how to treat translation keys when importing animations
//
struct native AnimSequenceRefBone
{
	var() editconst Name BoneName;
	var() EBoneTranslationMethod TranslationMethod;
};

cpptext
{
	void PostEditChange();
}

var const int WBrowserAnimationPtr;
var(Compression) float   GlobalCompression;
var(Compression) EAnimCompressMethod CompressionMethod;
// scion sz
var(RefBones) editconstarray array<AnimSequenceRefBone> AnimRefBones;

defaultproperties
{	
	GlobalCompression=1.0
}
