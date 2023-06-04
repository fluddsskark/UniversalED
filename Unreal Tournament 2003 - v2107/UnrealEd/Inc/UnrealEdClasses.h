/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

#ifndef UNREALED_API
#define UNREALED_API DLL_IMPORT
#endif

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern UNREALED_API FName UNREALED_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#ifndef NAMES_ONLY


class UNREALED_API AAnimBrowserMesh : public AActor
{
public:
    DECLARE_CLASS(AAnimBrowserMesh,AActor,0,UnrealEd)
    NO_DEFAULT_CONSTRUCTOR(AAnimBrowserMesh)
};

struct UNREALED_API FFSectionDigest
{
    BYTE MeshSectionMethod;
    INT MaxRigidParts;
    INT MinPartFaces;
    FLOAT MeldSize;
};

struct UNREALED_API FLODLevel
{
    FLOAT DistanceFactor;
    FLOAT ReductionFactor;
    FLOAT Hysteresis;
    INT MaxInfluences;
    BITFIELD RedigestSwitch:1 GCC_PACK(4);
    FFSectionDigest Rigidize GCC_PACK(4);
};

struct UNREALED_API FAttachSocket
{
    FVector A_Translation;
    FRotator A_Rotation;
    FName AttachAlias;
    FName BoneName;
    FLOAT Test_Scale;
    class UMesh* TestMesh;
    class UStaticMesh* TestStaticMesh;
};


class UNREALED_API UMeshEditProps : public UMeshObject
{
public:
    INT WBrowserAnimationPtr;
    FVector Scale;
    FVector Translation;
    FRotator Rotation;
    FVector MinVisBound;
    FVector MaxVisBound;
    FVector VisSphereCenter;
    FLOAT VisSphereRadius;
    INT LODStyle;
    class UMeshAnimation* DefaultAnimation;
    TArrayNoInit<class UMaterial*> Material;
    FLOAT LOD_Strength;
    TArrayNoInit<FLODLevel> LODLevels;
    FLOAT SkinTesselationFactor;
    FLOAT TestCollisionRadius;
    FLOAT TestCollisionHeight;
    TArrayNoInit<FAttachSocket> Sockets;
    BITFIELD ApplyNewSockets:1 GCC_PACK(4);
    BITFIELD ContinuousUpdate:1;
    BITFIELD bImpostorPresent:1;
    class UMaterial* SpriteMaterial GCC_PACK(4);
    FVector Scale3D;
    FRotator RelativeRotation;
    FVector RelativeLocation;
    FColor ImpColor;
    BYTE ImpSpaceMode;
    BYTE ImpDrawMode;
    BYTE ImpLightMode;
    DECLARE_CLASS(UMeshEditProps,UMeshObject,0,UnrealEd)
	void PostEditChange();
};


class UNREALED_API UAnimEditProps : public UObject
{
public:
    INT WBrowserAnimationPtr;
    FLOAT GlobalCompression;
    DECLARE_CLASS(UAnimEditProps,UObject,0,UnrealEd)
	void PostEditChange();
};

struct UNREALED_API FNotifyInfo
{
    FLOAT NotifyFrame;
    class UAnimNotify* Notify;
    INT OldRevisionNum;
};


class UNREALED_API UNotifyProperties : public UObject
{
public:
    INT OldArrayCount;
    INT WBrowserAnimationPtr;
    TArrayNoInit<FNotifyInfo> Notifys;
    DECLARE_CLASS(UNotifyProperties,UObject,0,UnrealEd)
	void PostEditChange();
};


class UNREALED_API USequEditProps : public UObject
{
public:
    INT WBrowserAnimationPtr;
    FVector Translation;
    FRotator Rotation;
    FLOAT Rate;
    FLOAT Compression;
    FName SequenceName;
    TArrayNoInit<FName> Groups;
    DECLARE_CLASS(USequEditProps,UObject,0,UnrealEd)
	void PostEditChange();
};


class UNREALED_API USkelPrefsEditProps : public UObject
{
public:
    INT WBrowserAnimationPtr;
    INT LODStyle;
    INT RootZero;
    DECLARE_CLASS(USkelPrefsEditProps,UObject,0,UnrealEd)
	void PostEditChange();
};

#endif


#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

#if __STATIC_LINK
#ifndef UNREALED_NATIVE_DEFS
#define UNREALED_NATIVE_DEFS

DECLARE_NATIVE_TYPE(UnrealEd,AAnimBrowserMesh);
DECLARE_NATIVE_TYPE(UnrealEd,UAnimEditProps);
DECLARE_NATIVE_TYPE(UnrealEd,UMeshEditProps);
DECLARE_NATIVE_TYPE(UnrealEd,UNotifyProperties);
DECLARE_NATIVE_TYPE(UnrealEd,USequEditProps);
DECLARE_NATIVE_TYPE(UnrealEd,USkelPrefsEditProps);

#define AUTO_INITIALIZE_REGISTRANTS_UNREALED \
	AAnimBrowserMesh::StaticClass(); \
	UAnimEditProps::StaticClass(); \
	UMeshEditProps::StaticClass(); \
	UNotifyProperties::StaticClass(); \
	USequEditProps::StaticClass(); \
	USkelPrefsEditProps::StaticClass(); \
	UUnrealEdEngine::StaticClass(); \

#endif // UnrealEd_NATIVE_DEFS

#ifdef NATIVES_ONLY
#endif // NATIVES_ONLY
#endif // __STATIC_LINK
