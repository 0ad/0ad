OS_ARCH := $(shell uname -s)

ifeq ($(OS_ARCH),Darwin)
PIC_FLAGS ?= -fPIC
else
OS_DEFINE ?= -DLINUX
PIC_FLAGS ?= -fpic
endif

CXX := g++
CXXFLAGS := -fvisibility=hidden -W -Wall -Wno-unused-parameter -Wno-unused-function $(OS_DEFINE) $(PIC_FLAGS)
CXXFLAGS_DEBUG := -O0 -g -D_DEBUG -DRETAIL
CXXFLAGS_RELEASE := -O2 -DNDEBUG -DRETAIL
CXXFLAGS_TEST := -O0 -g -D_DEBUG
LIBS := `pkg-config libxml-2.0 --libs`
INCLUDES := -IFCollada `pkg-config libxml-2.0 --cflags`
INCLUDES_TEST := -IFCollada/FColladaTest $(INCLUDES)

# FCollada is not aliasing-safe, so disallow dangerous optimisations
# (TODO: It'd be nice to fix FCollada, but that looks hard)
CXXFLAGS_RELEASE += -fno-strict-aliasing

SOURCE = \
	FCollada/FCollada.cpp \
	FCollada/FColladaPlugin.cpp \
	FCollada/FCDocument/FCDAnimated.cpp \
	FCollada/FCDocument/FCDAnimationChannel.cpp \
	FCollada/FCDocument/FCDAnimationClip.cpp \
	FCollada/FCDocument/FCDAnimationClipTools.cpp \
	FCollada/FCDocument/FCDAnimation.cpp \
	FCollada/FCDocument/FCDAnimationCurve.cpp \
	FCollada/FCDocument/FCDAnimationCurveTools.cpp \
	FCollada/FCDocument/FCDAnimationKey.cpp \
	FCollada/FCDocument/FCDAnimationMultiCurve.cpp \
	FCollada/FCDocument/FCDAsset.cpp \
	FCollada/FCDocument/FCDCamera.cpp \
	FCollada/FCDocument/FCDController.cpp \
	FCollada/FCDocument/FCDControllerInstance.cpp \
	FCollada/FCDocument/FCDControllerTools.cpp \
	FCollada/FCDocument/FCDEffectCode.cpp \
	FCollada/FCDocument/FCDEffect.cpp \
	FCollada/FCDocument/FCDEffectParameter.cpp \
	FCollada/FCDocument/FCDEffectParameterFactory.cpp \
	FCollada/FCDocument/FCDEffectParameterSampler.cpp \
	FCollada/FCDocument/FCDEffectParameterSurface.cpp \
	FCollada/FCDocument/FCDEffectPass.cpp \
	FCollada/FCDocument/FCDEffectPassShader.cpp \
	FCollada/FCDocument/FCDEffectPassState.cpp \
	FCollada/FCDocument/FCDEffectProfile.cpp \
	FCollada/FCDocument/FCDEffectProfileFX.cpp \
	FCollada/FCDocument/FCDEffectStandard.cpp \
	FCollada/FCDocument/FCDEffectTechnique.cpp \
	FCollada/FCDocument/FCDEffectTools.cpp \
	FCollada/FCDocument/FCDEmitter.cpp \
	FCollada/FCDocument/FCDEmitterInstance.cpp \
	FCollada/FCDocument/FCDEmitterObject.cpp \
	FCollada/FCDocument/FCDEmitterParticle.cpp \
	FCollada/FCDocument/FCDEntity.cpp \
	FCollada/FCDocument/FCDEntityInstance.cpp \
	FCollada/FCDocument/FCDEntityReference.cpp \
	FCollada/FCDocument/FCDExternalReferenceManager.cpp \
	FCollada/FCDocument/FCDExtra.cpp \
	FCollada/FCDocument/FCDForceDeflector.cpp \
	FCollada/FCDocument/FCDForceDrag.cpp \
	FCollada/FCDocument/FCDForceField.cpp \
	FCollada/FCDocument/FCDForceGravity.cpp \
	FCollada/FCDocument/FCDForcePBomb.cpp \
	FCollada/FCDocument/FCDForceWind.cpp \
	FCollada/FCDocument/FCDGeometry.cpp \
	FCollada/FCDocument/FCDGeometryInstance.cpp \
	FCollada/FCDocument/FCDGeometryMesh.cpp \
	FCollada/FCDocument/FCDGeometryNURBSSurface.cpp \
	FCollada/FCDocument/FCDGeometryPolygons.cpp \
	FCollada/FCDocument/FCDGeometryPolygonsInput.cpp \
	FCollada/FCDocument/FCDGeometryPolygonsTools.cpp \
	FCollada/FCDocument/FCDGeometrySource.cpp \
	FCollada/FCDocument/FCDGeometrySpline.cpp \
	FCollada/FCDocument/FCDImage.cpp \
	FCollada/FCDocument/FCDLibrary.cpp \
	FCollada/FCDocument/FCDLight.cpp \
	FCollada/FCDocument/FCDLightTools.cpp \
	FCollada/FCDocument/FCDMaterial.cpp \
	FCollada/FCDocument/FCDMaterialInstance.cpp \
	FCollada/FCDocument/FCDMorphController.cpp \
	FCollada/FCDocument/FCDObject.cpp \
	FCollada/FCDocument/FCDObjectWithId.cpp \
	FCollada/FCDocument/FCDocument.cpp \
	FCollada/FCDocument/FCDocumentTools.cpp \
	FCollada/FCDocument/FCDParameterAnimatable.cpp \
	FCollada/FCDocument/FCDParticleModifier.cpp \
	FCollada/FCDocument/FCDPhysicsAnalyticalGeometry.cpp \
	FCollada/FCDocument/FCDPhysicsForceFieldInstance.cpp \
	FCollada/FCDocument/FCDPhysicsMaterial.cpp \
	FCollada/FCDocument/FCDPhysicsModel.cpp \
	FCollada/FCDocument/FCDPhysicsModelInstance.cpp \
	FCollada/FCDocument/FCDPhysicsRigidBody.cpp \
	FCollada/FCDocument/FCDPhysicsRigidBodyInstance.cpp \
	FCollada/FCDocument/FCDPhysicsRigidBodyParameters.cpp \
	FCollada/FCDocument/FCDPhysicsRigidConstraint.cpp \
	FCollada/FCDocument/FCDPhysicsRigidConstraintInstance.cpp \
	FCollada/FCDocument/FCDPhysicsScene.cpp \
	FCollada/FCDocument/FCDPhysicsShape.cpp \
	FCollada/FCDocument/FCDPlaceHolder.cpp \
	FCollada/FCDocument/FCDSceneNode.cpp \
	FCollada/FCDocument/FCDSceneNodeIterator.cpp \
	FCollada/FCDocument/FCDSceneNodeTools.cpp \
	FCollada/FCDocument/FCDSkinController.cpp \
	FCollada/FCDocument/FCDTargetedEntity.cpp \
	FCollada/FCDocument/FCDTexture.cpp \
	FCollada/FCDocument/FCDTransform.cpp \
	FCollada/FCDocument/FCDVersion.cpp \
	FCollada/FMath/FMAllocator.cpp \
	FCollada/FMath/FMAngleAxis.cpp \
	FCollada/FMath/FMColor.cpp \
	FCollada/FMath/FMInterpolation.cpp \
	FCollada/FMath/FMLookAt.cpp \
	FCollada/FMath/FMMatrix33.cpp \
	FCollada/FMath/FMMatrix44.cpp \
	FCollada/FMath/FMQuaternion.cpp \
	FCollada/FMath/FMRandom.cpp \
	FCollada/FMath/FMSkew.cpp \
	FCollada/FMath/FMVector3.cpp \
	FCollada/FMath/FMVolume.cpp \
	FCollada/FUtils/FUAssert.cpp \
	FCollada/FUtils/FUBase64.cpp \
	FCollada/FUtils/FUBoundingBox.cpp \
	FCollada/FUtils/FUBoundingSphere.cpp \
	FCollada/FUtils/FUCrc32.cpp \
	FCollada/FUtils/FUCriticalSection.cpp \
	FCollada/FUtils/FUDaeEnum.cpp \
	FCollada/FUtils/FUDateTime.cpp \
	FCollada/FUtils/FUDebug.cpp \
	FCollada/FUtils/FUError.cpp \
	FCollada/FUtils/FUErrorLog.cpp \
	FCollada/FUtils/FUFile.cpp \
	FCollada/FUtils/FUFileManager.cpp \
	FCollada/FUtils/FULogFile.cpp \
	FCollada/FUtils/FUObject.cpp \
	FCollada/FUtils/FUObjectType.cpp \
	FCollada/FUtils/FUParameter.cpp \
	FCollada/FUtils/FUParameterizable.cpp \
	FCollada/FUtils/FUPluginManager.cpp \
	FCollada/FUtils/FUSemaphore.cpp \
	FCollada/FUtils/FUStringBuilder.cpp \
	FCollada/FUtils/FUStringConversion.cpp \
	FCollada/FUtils/FUSynchronizableObject.cpp \
	FCollada/FUtils/FUThread.cpp \
	FCollada/FUtils/FUTracker.cpp \
	FCollada/FUtils/FUUniqueStringMap.cpp \
	FCollada/FUtils/FUUri.cpp \
	FCollada/FUtils/FUXmlDocument.cpp \
	FCollada/FUtils/FUXmlParser.cpp \
	FCollada/FUtils/FUXmlWriter.cpp \
	FColladaPlugins/FArchiveXML/FArchiveXML.cpp \
	FColladaPlugins/FArchiveXML/FAXAnimationExport.cpp \
	FColladaPlugins/FArchiveXML/FAXAnimationImport.cpp \
	FColladaPlugins/FArchiveXML/FAXCameraExport.cpp \
	FColladaPlugins/FArchiveXML/FAXCameraImport.cpp \
	FColladaPlugins/FArchiveXML/FAXColladaParser.cpp \
	FColladaPlugins/FArchiveXML/FAXColladaWriter.cpp \
	FColladaPlugins/FArchiveXML/FAXControllerExport.cpp \
	FColladaPlugins/FArchiveXML/FAXControllerImport.cpp \
	FColladaPlugins/FArchiveXML/FAXEmitterExport.cpp \
	FColladaPlugins/FArchiveXML/FAXEmitterImport.cpp \
	FColladaPlugins/FArchiveXML/FAXEntityExport.cpp \
	FColladaPlugins/FArchiveXML/FAXEntityImport.cpp \
	FColladaPlugins/FArchiveXML/FAXForceFieldExport.cpp \
	FColladaPlugins/FArchiveXML/FAXForceFieldImport.cpp \
	FColladaPlugins/FArchiveXML/FAXGeometryExport.cpp \
	FColladaPlugins/FArchiveXML/FAXGeometryImport.cpp \
	FColladaPlugins/FArchiveXML/FAXImportLinking.cpp \
	FColladaPlugins/FArchiveXML/FAXInstanceExport.cpp \
	FColladaPlugins/FArchiveXML/FAXInstanceImport.cpp \
	FColladaPlugins/FArchiveXML/FAXLightExport.cpp \
	FColladaPlugins/FArchiveXML/FAXLightImport.cpp \
	FColladaPlugins/FArchiveXML/FAXMaterialExport.cpp \
	FColladaPlugins/FArchiveXML/FAXMaterialImport.cpp \
	FColladaPlugins/FArchiveXML/FAXPhysicsExport.cpp \
	FColladaPlugins/FArchiveXML/FAXPhysicsImport.cpp \
	FColladaPlugins/FArchiveXML/FAXSceneExport.cpp \
	FColladaPlugins/FArchiveXML/FAXSceneImport.cpp \

TEST_SOURCE = \
	FCollada/FMath/FMArrayTest.cpp \
	FCollada/FMath/FMQuaternionTest.cpp \
	FCollada/FMath/FMTreeTest.cpp \
	FCollada/FUtils/FUBoundingTest.cpp \
	FCollada/FUtils/FUCrc32Test.cpp \
	FCollada/FUtils/FUEventTest.cpp \
	FCollada/FUtils/FUFileManagerTest.cpp \
	FCollada/FUtils/FUFunctorTest.cpp \
	FCollada/FUtils/FUObjectTest.cpp \
	FCollada/FUtils/FUStringBuilderTest.cpp \
	FCollada/FUtils/FUStringConversionTest.cpp \
	FCollada/FUtils/FUStringTest.cpp \
	FCollada/FUtils/FUTestBed.cpp \
	FCollada/FUtils/FUUniqueStringMapTest.cpp \
	FCollada/FColladaTest/FCTest.cpp \
	FCollada/FColladaTest/FCTestAnimation.cpp \
	FCollada/FColladaTest/FCTestArchiving.cpp \
	FCollada/FColladaTest/FCTestController.cpp \
	FCollada/FColladaTest/FCTestGeometryPolygonsTools.cpp \
	FCollada/FColladaTest/FCTestParameters.cpp \
	FCollada/FColladaTest/FCTestSceneGraph.cpp \
	FCollada/FColladaTest/FCTestAssetManagement/FCTAMCrossCloning.cpp \
	FCollada/FColladaTest/FCTestAssetManagement/FCTAssetManagement.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIAnimation.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEICamera.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIEmitter.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIExtra.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIGeometry.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEILight.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIMaterial.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIPhysics.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTEIVisualScene.cpp \
	FCollada/FColladaTest/FCTestExportImport/FCTestExportImport.cpp \
	FCollada/FColladaTest/FCTestXRef/FCTestXRef.cpp \
	FCollada/FColladaTest/FCTestXRef/FCTestXRefAcyclic.cpp \
	FCollada/FColladaTest/FCTestXRef/FCTestXRefSimple.cpp \
	FCollada/FColladaTest/FCTestXRef/FCTestXRefTree.cpp \

OBJECTS_DEBUG = $(addprefix output/debug/,$(SOURCE:.cpp=.o))
OBJECTS_RELEASE = $(addprefix output/release/,$(SOURCE:.cpp=.o))
OBJECTS_TEST = $(addprefix output/test/,$(SOURCE:.cpp=.o) $(TEST_SOURCE:.cpp=.o))
OBJECTS_ALL = $(OBJECTS_DEBUG) $(OBJECTS_RELEASE) $(OBJECTS_TEST)

all: output_dirs output/libFColladaSD.a output/libFColladaSR.a install

output_dirs:
	bash -c 'mkdir -p output/{debug,release,test}/{FCollada/{FCDocument,FMath,FUtils,FColladaTest/{FCTestAssetManagement,FCTestExportImport,FCTestXRef}},FColladaPlugins/FArchiveXML}'

test: output/FColladaTest
	( cd FCollada/FColladaTest/ ; ../../output/FColladaTest )
	cat FCollada/FColladaTest/FColladaTestLog.txt

output/libFColladaSD.a: $(OBJECTS_DEBUG)
	@echo "$@"
	@ar -cr $@ $(OBJECTS_DEBUG); ranlib $@

output/libFColladaSR.a: $(OBJECTS_RELEASE)
	@echo "$@"
	@ar -cr $@ $(OBJECTS_RELEASE); ranlib $@

output/FColladaTest: $(OBJECTS_TEST)
	@$(CXX) -o $@ $(LDFLAGS) $(OBJECTS_TEST) $(LIBS)

install: output/libFColladaSD.a output/libFColladaSR.a
	cp output/libFColladaSD.a ../lib/libFColladaSD.a
	cp output/libFColladaSR.a ../lib/libFColladaSR.a

dfile = $(@:.o=.d)
Pfile = $(@:.o=.P)
gendep = \
	@cp $(dfile) $(Pfile) && \
		sed -e 's/\#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		    -e '/^$$/ d' -e 's/$$/ :/' < $(dfile) >> $(Pfile) && \
		rm -f $(dfile)
output/debug/%.o: %.cpp
	@echo "$<"
	@$(CXX) $(CXXFLAGS) $(CXXFLAGS_DEBUG) $(INCLUDES) -MD -MF $(dfile) -c $< -o $@
	$(gendep)
output/release/%.o: %.cpp
	@echo "$<"
	@$(CXX) $(CXXFLAGS) $(CXXFLAGS_RELEASE) $(INCLUDES) -MD -MF $(dfile) -c $< -o $@
	$(gendep)
output/test/%.o: %.cpp
	@echo "$<"
	@$(CXX) $(CXXFLAGS) $(CXXFLAGS_TEST) $(INCLUDES_TEST) -MD -MF $(dfile) -c $< -o $@
	$(gendep)

clean:
	rm -rf output

-include $(OBJECTS_ALL:.o=.P)
