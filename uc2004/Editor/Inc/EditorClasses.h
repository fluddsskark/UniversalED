/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
===========================================================================*/
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

#ifndef EDITOR_API
#define EDITOR_API DLL_IMPORT
#endif

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern EDITOR_API FName EDITOR_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif

AUTOGENERATE_NAME(Build)
AUTOGENERATE_NAME(CreateMaterial)

#ifndef NAMES_ONLY


class EDITOR_API UAnimNotifyProps : public UObject
{
public:
    class UAnimNotify* Notify;
    DECLARE_CLASS(UAnimNotifyProps,UObject,0,Editor)
    NO_DEFAULT_CONSTRUCTOR(UAnimNotifyProps)
};


struct UBrushBuilder_eventBuild_Parms
{
    BITFIELD ReturnValue;
};
class EDITOR_API UBrushBuilder : public UObject
{
public:
    FStringNoInit BitmapFilename;
    FStringNoInit ToolTip;
    TArrayNoInit<FVector> Vertices;
    TArrayNoInit<FBuilderPoly> Polys;
    FName Group;
    BITFIELD MergeCoplanars:1 GCC_PACK(4);
    DECLARE_FUNCTION(execPolyEnd);
    DECLARE_FUNCTION(execPolyi);
    DECLARE_FUNCTION(execPolyBegin);
    DECLARE_FUNCTION(execPoly4i);
    DECLARE_FUNCTION(execPoly3i);
    DECLARE_FUNCTION(execVertex3f);
    DECLARE_FUNCTION(execVertexv);
    DECLARE_FUNCTION(execBadParameters);
    DECLARE_FUNCTION(execGetPolyCount);
    DECLARE_FUNCTION(execGetVertex);
    DECLARE_FUNCTION(execGetVertexCount);
    DECLARE_FUNCTION(execEndBrush);
    DECLARE_FUNCTION(execBeginBrush);
    BITFIELD eventBuild()
    {
        UBrushBuilder_eventBuild_Parms Parms;
        Parms.ReturnValue=0;
        ProcessEvent(FindFunctionChecked(EDITOR_Build),&Parms);
        return Parms.ReturnValue;
    }
    DECLARE_CLASS(UBrushBuilder,UObject,0,Editor)
    #include "UBrushBuilder.h"
};

#define UCONST_RF_Standalone 0x00080000

struct UMaterialFactory_eventCreateMaterial_Parms
{
    class UObject* InOuter;
    FString InPackage;
    FString InGroup;
    FString InName;
    class UMaterial* ReturnValue;
};
class EDITOR_API UMaterialFactory : public UObject
{
public:
    FStringNoInit Description;
    DECLARE_FUNCTION(execConsoleCommand);
    class UMaterial* eventCreateMaterial(class UObject* InOuter, const FString& InPackage, const FString& InGroup, const FString& InName)
    {
        UMaterialFactory_eventCreateMaterial_Parms Parms;
        Parms.ReturnValue=0;
        Parms.InOuter=InOuter;
        Parms.InPackage=InPackage;
        Parms.InGroup=InGroup;
        Parms.InName=InName;
        ProcessEvent(FindFunctionChecked(EDITOR_CreateMaterial),&Parms);
        return Parms.ReturnValue;
    }
    DECLARE_CLASS(UMaterialFactory,UObject,0,Editor)
    NO_DEFAULT_CONSTRUCTOR(UMaterialFactory)
};

#endif

AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execPolyEnd);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execPolyi);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execPolyBegin);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execPoly4i);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execPoly3i);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execVertex3f);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execVertexv);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execBadParameters);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execGetPolyCount);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execGetVertex);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execGetVertexCount);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execEndBrush);
AUTOGENERATE_FUNCTION(UBrushBuilder,-1,execBeginBrush);
AUTOGENERATE_FUNCTION(UMaterialFactory,-1,execConsoleCommand);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

#if __STATIC_LINK
#ifndef EDITOR_NATIVE_DEFS
#define EDITOR_NATIVE_DEFS

DECLARE_NATIVE_TYPE(Editor,UAnimNotifyProps);
DECLARE_NATIVE_TYPE(Editor,UBrushBuilder);
DECLARE_NATIVE_TYPE(Editor,UMaterialFactory);

#define AUTO_INITIALIZE_REGISTRANTS_EDITOR \
	UAnalyzeContentCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UAnalyzeContentCommandlet);\
	UAnalyzeMeshesCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UAnalyzeMeshesCommandlet);\
	UAnalyzeScriptCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UAnalyzeScriptCommandlet);\
	UAnimNotifyProps::StaticClass(); \
VERIFY_CLASS_SIZE(UAnimNotifyProps);\
	UBatchExportCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UBatchExportCommandlet);\
	UBitArray::StaticClass(); \
VERIFY_CLASS_SIZE(UBitArray);\
	UBitMatrix::StaticClass(); \
VERIFY_CLASS_SIZE(UBitMatrix);\
	UBrushBuilder::StaticClass(); \
VERIFY_CLASS_SIZE(UBrushBuilder);\
	GNativeLookupFuncs[Lookup++] = &FindEditorUBrushBuilderNative; \
	UBulkDataExporterRDF::StaticClass(); \
VERIFY_CLASS_SIZE(UBulkDataExporterRDF);\
	UCheckLODActorsCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UCheckLODActorsCommandlet);\
	UCheckStaticMeshes::StaticClass(); \
VERIFY_CLASS_SIZE(UCheckStaticMeshes);\
	UCheckUnicodeCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UCheckUnicodeCommandlet);\
	UClassExporterH::StaticClass(); \
VERIFY_CLASS_SIZE(UClassExporterH);\
	UClassExporterUC::StaticClass(); \
VERIFY_CLASS_SIZE(UClassExporterUC);\
	UClassFactoryNew::StaticClass(); \
VERIFY_CLASS_SIZE(UClassFactoryNew);\
	UClassFactoryUC::StaticClass(); \
VERIFY_CLASS_SIZE(UClassFactoryUC);\
	UCompareIntCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UCompareIntCommandlet);\
	UCompileWeaponsCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UCompileWeaponsCommandlet);\
	UConformCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UConformCommandlet);\
	UConvertMaterialCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UConvertMaterialCommandlet);\
	UCutdownContentCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UCutdownContentCommandlet);\
	UDXTConvertCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UDXTConvertCommandlet);\
	UDataRipCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UDataRipCommandlet);\
	UDumpIntCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UDumpIntCommandlet);\
	UEditInfo::StaticClass(); \
VERIFY_CLASS_SIZE(UEditInfo);\
	UEditorEngine::StaticClass(); \
VERIFY_CLASS_SIZE(UEditorEngine);\
	UFindInstancesCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UFindInstancesCommandlet);\
	UFindRefCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UFindRefCommandlet);\
	UFontFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UFontFactory);\
	UFontKerningCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UFontKerningCommandlet);\
	UFontUpdateCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UFontUpdateCommandlet);\
	UInvalidMaterialsCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UInvalidMaterialsCommandlet);\
	ULevelExporterOBJ::StaticClass(); \
VERIFY_CLASS_SIZE(ULevelExporterOBJ);\
	ULevelExporterSTL::StaticClass(); \
VERIFY_CLASS_SIZE(ULevelExporterSTL);\
	ULevelExporterT3D::StaticClass(); \
VERIFY_CLASS_SIZE(ULevelExporterT3D);\
	ULevelFactory::StaticClass(); \
VERIFY_CLASS_SIZE(ULevelFactory);\
	ULevelFactoryNew::StaticClass(); \
VERIFY_CLASS_SIZE(ULevelFactoryNew);\
	UListAmbientSoundsCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UListAmbientSoundsCommandlet);\
	UListBrokenDistortionCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UListBrokenDistortionCommandlet);\
	UListDuplicateEmittersCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UListDuplicateEmittersCommandlet);\
	UListUsedMaterialTypesCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UListUsedMaterialTypesCommandlet);\
	UMapConvertCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UMapConvertCommandlet);\
	UMasterCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UMasterCommandlet);\
	UMaterialExporterRDF::StaticClass(); \
VERIFY_CLASS_SIZE(UMaterialExporterRDF);\
	UMaterialFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UMaterialFactory);\
	GNativeLookupFuncs[Lookup++] = &FindEditorUMaterialFactoryNative; \
	UMergeIntCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UMergeIntCommandlet);\
	UModelExporterT3D::StaticClass(); \
VERIFY_CLASS_SIZE(UModelExporterT3D);\
	UModelFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UModelFactory);\
	UPackageFlagCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UPackageFlagCommandlet);\
	UPkgCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UPkgCommandlet);\
	UPolysExporterOBJ::StaticClass(); \
VERIFY_CLASS_SIZE(UPolysExporterOBJ);\
	UPolysExporterT3D::StaticClass(); \
VERIFY_CLASS_SIZE(UPolysExporterT3D);\
	UPolysFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UPolysFactory);\
	UPrefab::StaticClass(); \
VERIFY_CLASS_SIZE(UPrefab);\
	UPrefabExporterT3D::StaticClass(); \
VERIFY_CLASS_SIZE(UPrefabExporterT3D);\
	UPrefabFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UPrefabFactory);\
	UPushbufferExporterRDF::StaticClass(); \
VERIFY_CLASS_SIZE(UPushbufferExporterRDF);\
	URearrangeIntCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(URearrangeIntCommandlet);\
	URebuildPathsCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(URebuildPathsCommandlet);\
	URedigestAnimationsCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(URedigestAnimationsCommandlet);\
	UReferenceCountCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UReferenceCountCommandlet);\
	URemoveDuplicatesCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(URemoveDuplicatesCommandlet);\
	URemoveTwoSidedCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(URemoveTwoSidedCommandlet);\
	UResavePackageCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UResavePackageCommandlet);\
	USoundExporterWAV::StaticClass(); \
VERIFY_CLASS_SIZE(USoundExporterWAV);\
	USoundFactory::StaticClass(); \
VERIFY_CLASS_SIZE(USoundFactory);\
	UStaticMeshCalcCacheCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UStaticMeshCalcCacheCommandlet);\
	UStaticMeshExporterT3D::StaticClass(); \
VERIFY_CLASS_SIZE(UStaticMeshExporterT3D);\
	UStaticMeshFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UStaticMeshFactory);\
	UStripSourceCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UStripSourceCommandlet);\
	UTextBufferExporterTXT::StaticClass(); \
VERIFY_CLASS_SIZE(UTextBufferExporterTXT);\
	UTextureExporterBMP::StaticClass(); \
VERIFY_CLASS_SIZE(UTextureExporterBMP);\
	UTextureExporterDDS::StaticClass(); \
VERIFY_CLASS_SIZE(UTextureExporterDDS);\
	UTextureExporterPCX::StaticClass(); \
VERIFY_CLASS_SIZE(UTextureExporterPCX);\
	UTextureExporterTGA::StaticClass(); \
VERIFY_CLASS_SIZE(UTextureExporterTGA);\
	UTextureFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UTextureFactory);\
	UTextureFactoryNew::StaticClass(); \
VERIFY_CLASS_SIZE(UTextureFactoryNew);\
	UTransBuffer::StaticClass(); \
VERIFY_CLASS_SIZE(UTransBuffer);\
	UTransactor::StaticClass(); \
VERIFY_CLASS_SIZE(UTransactor);\
	UTrueTypeFontFactory::StaticClass(); \
VERIFY_CLASS_SIZE(UTrueTypeFontFactory);\
	UUpdateUModCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UUpdateUModCommandlet);\
	UmakeCommandlet::StaticClass(); \
VERIFY_CLASS_SIZE(UmakeCommandlet);\

#endif // Editor_NATIVE_DEFS

#ifdef NATIVES_ONLY
NATIVE_INFO(UBrushBuilder) GEditorUBrushBuilderNatives[] = 
{ 
	MAP_NATIVE(UBrushBuilder,execPolyEnd)
	MAP_NATIVE(UBrushBuilder,execPolyi)
	MAP_NATIVE(UBrushBuilder,execPolyBegin)
	MAP_NATIVE(UBrushBuilder,execPoly4i)
	MAP_NATIVE(UBrushBuilder,execPoly3i)
	MAP_NATIVE(UBrushBuilder,execVertex3f)
	MAP_NATIVE(UBrushBuilder,execVertexv)
	MAP_NATIVE(UBrushBuilder,execBadParameters)
	MAP_NATIVE(UBrushBuilder,execGetPolyCount)
	MAP_NATIVE(UBrushBuilder,execGetVertex)
	MAP_NATIVE(UBrushBuilder,execGetVertexCount)
	MAP_NATIVE(UBrushBuilder,execEndBrush)
	MAP_NATIVE(UBrushBuilder,execBeginBrush)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Editor,UBrushBuilder);

NATIVE_INFO(UMaterialFactory) GEditorUMaterialFactoryNatives[] = 
{ 
	MAP_NATIVE(UMaterialFactory,execConsoleCommand)
	{NULL,NULL}
};
IMPLEMENT_NATIVE_HANDLER(Editor,UMaterialFactory);

#endif // NATIVES_ONLY
#endif // __STATIC_LINK

#ifdef VERIFY_CLASS_SIZES
VERIFY_CLASS_SIZE_NODIE(UAnimNotifyProps)
VERIFY_CLASS_SIZE_NODIE(UBrushBuilder)
VERIFY_CLASS_SIZE_NODIE(UEditorEngine)
VERIFY_CLASS_SIZE_NODIE(UMaterialFactory)
#endif // VERIFY_CLASS_SIZES
