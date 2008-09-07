/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterSampler.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectPassShader.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectStandard.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDImage.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDTexture.h"
#include "FUtils/FUDaeSyntax.h"
#include "FCTestExportImport.h"

static const float sampleMatrix[16] = { 0.5f, 0.1f, 0.7f, 2.0f, 1.11f, 0.5e-2f, 111.0f, 0.5f, 0.0f, 0.0f, 0.557f, -10.02f, 0.001f, 12.0f, 1.02e-3f, 0.0f };
static fm::string wantedImageId = "test_image";
static fm::string wantedImage2Id = "test_image";
static fm::string surfaceParameterImageId;

namespace FCTestExportImport
{
	static const char* szTestName = "FCTestExportImportMaterial";

	bool FillMaterialLibrary(FULogFile& fileOut, FCDMaterialLibrary* library)
	{
		// Create an empty material
		FCDMaterial* material = library->AddEntity();
		material->SetNote(FS("EmptyMaterial244"));

		// Create an effect and attach it to a new material
		FCDEffect* effect = library->GetDocument()->GetEffectLibrary()->AddEntity();
		material = library->AddEntity();
		material->SetEffect(effect);
		effect->SetNote(FS("EmptyEffect.. for now!"));

		FillEffectStandard(fileOut, (FCDEffectStandard*) effect->AddProfile(FUDaeProfileType::COMMON));
		FillEffectFX(fileOut, (FCDEffectProfileFX*) effect->AddProfile(FUDaeProfileType::CG));
		return true;
	}

	bool FillEffectStandard(FULogFile& fileOut, FCDEffectStandard* profile)
	{
		FailIf(profile == NULL);
		profile->SetLightingType(FCDEffectStandard::PHONG);
		profile->SetDiffuseColor(FMVector4(1.0f, 0.0f, -2.0f, 1.0f));
		PassIf(IsEquivalent(profile->GetDiffuseColor(), FMVector4(1.0f, 0.0f, -2.0f, 1.0f)));
		profile->SetSpecularColor(FMVector4(0.0f, 1.0f, 0.4f, 1.0f));
		profile->SetIndexOfRefraction(21.0f);
		profile->SetShininess(40.0f);

		// Retrieve two images created earlier
		FCDImage* image1 = profile->GetDocument()->FindImage(wantedImageId);
		FCDImage* image2 = profile->GetDocument()->FindImage(wantedImage2Id);
		PassIf(image1 != NULL && image2 != NULL); // Dependency! Must fill the image library first

		// Two simple bump textures
		FCDTexture* texture1 = profile->AddTexture(FUDaeTextureChannel::BUMP);
		texture1->SetImage(image2);
		FCDETechnique* technique1 = texture1->GetExtra()->GetDefaultType()->AddTechnique(DAEMAYA_MAYA_PROFILE);
		technique1->AddParameter(DAEMAYA_TEXTURE_WRAPU_PARAMETER, 2.0f);
		FCDTexture* texture2 = profile->AddTexture(FUDaeTextureChannel::BUMP);
		texture2->SetImage(image1);
		FCDETechnique* technique2 = texture2->GetExtra()->GetDefaultType()->AddTechnique(DAEMAYA_MAYA_PROFILE);
		technique2->AddParameter(DAEMAYA_TEXTURE_WRAPV_PARAMETER, -1.2f);

		// A bit more extra support.
		FCDEType* extraType = profile->GetExtra()->AddType("Curious");
		PassIf(extraType != NULL);
		FCDETechnique* extraTechnique = extraType->AddTechnique("Kiddy");
		PassIf(extraTechnique != NULL);
		FCDENode* extraNode = extraTechnique->AddParameter("Causes", "trouble");
		PassIf(extraNode != NULL);

		// The third texture is a filter texture and will remain empty.
		UNUSED(FCDTexture* texture3 =)profile->AddTexture(FUDaeTextureChannel::FILTER);
		return true;
	}

	bool FillEffectFX(FULogFile& fileOut, FCDEffectProfileFX* profile)
	{
		FailIf(profile == NULL);

		// Generate some code: both from a code file and using inline code.
		FCDEffectCode* code = profile->AddCode();
		code->SetCode(FS("Some CG code for you to parse!"));
		code->SetSubId("TouchesAKey");
		FCDEffectCode* code2 = profile->AddCode();
		code2->SetFilename(FS("HowDoTheyDoIt.dd"));
		code2->SetSubId("PowerOfWill");

		// Create a technique and dump a few passes in there.
		FCDEffectTechnique* technique = profile->AddTechnique();
		technique->SetName(FC("100 percent reason to remember the name."));
		FCDEffectPass* pass1 = technique->AddPass();
		pass1->SetPassName(FC("First Pass"));
		FCDEffectPass* pass2 = technique->AddPass();
		pass2->SetPassName(FC("Second Pass"));

		// Dump a few parameters around
		FCDEffectParameterBool* booleanParameter = (FCDEffectParameterBool*) profile->AddEffectParameter(FCDEffectParameter::BOOLEAN);
		booleanParameter->SetSemantic("BOOL_P");
		booleanParameter->SetValue(true);
		FCDEffectParameterFloat2* floatParameter = (FCDEffectParameterFloat2*) technique->AddEffectParameter(FCDEffectParameter::FLOAT2);
		floatParameter->SetSemantic("FLOAT2_P");
		floatParameter->SetValue(FMVector2(1.5f, 5.2f));

		// Add some annotations to the float parameter.
		floatParameter->AddAnnotation(FS("UIMin"), FCDEffectParameter::FLOAT, 0.2f);
		floatParameter->AddAnnotation(FS("UIMax"), FCDEffectParameter::INTEGER, 5);
		floatParameter->AddAnnotation(FS("UIWidget"), FCDEffectParameter::STRING, FS("RolloverSlider"));
		floatParameter->AddAnnotation(FS("UIValid"), FCDEffectParameter::BOOLEAN, true);

		// Add a sampler/surface parameter pair
		FCDEffectParameterSurface* surface = (FCDEffectParameterSurface*) profile->AddEffectParameter(FCDEffectParameter::SURFACE);
		FCDImage* surfaceImage = profile->GetDocument()->GetImageLibrary()->GetEntity(0);
		surface->AddImage(surfaceImage);
		surface->SetReference("TestSF");
		surface->SetSurfaceType("RANDOMIZE");
		surface->SetFormat("Plurial");
		surface->SetInitMethod(new FCDEffectParameterSurfaceInitFrom());
		surfaceParameterImageId = surfaceImage->GetDaeId();

		FCDFormatHint* fhint = surface->AddFormatHint();
		fhint->channels = FCDFormatHint::CHANNEL_RGB;
		fhint->precision = FCDFormatHint::PRECISION_MID;
		fhint->range = FCDFormatHint::RANGE_SNORM;
		fhint->options.push_back(FCDFormatHint::OPT_NORMALIZED3);

		FCDEffectParameterSampler* sampler = (FCDEffectParameterSampler*) profile->AddEffectParameter(FCDEffectParameter::SAMPLER);
		sampler->SetSurface(surface);
		sampler->SetSamplerType(FCDEffectParameterSampler::SAMPLER2D);
		sampler->SetReference("Gotit");
		return true;
	}

	bool CheckMaterialLibrary(FULogFile& fileOut, FCDMaterialLibrary* library)
	{
		// There should be two materials within the material library: one is empty, the other is not.
		PassIf(library->GetEntityCount() == 2);
		PassIf(library->GetDocument()->GetEffectLibrary()->GetEntityCount() == 1);

		FCDMaterial* emptyMaterial = NULL,* material = NULL;
		for (size_t i = 0; i < library->GetEntityCount(); ++i)
		{
			FCDMaterial* m = library->GetEntity(i);
			if (m->GetEffect() == NULL) { PassIf(emptyMaterial == NULL); emptyMaterial = m; }
			else { PassIf(material == NULL); material = m; }
		}
		PassIf(emptyMaterial != NULL && material != NULL);

		// Verify the empty material. It should only have a note.
		PassIf(emptyMaterial->GetNote() == FC("EmptyMaterial244"));

		// Verify the other material and its effect.
		FCDEffect* effect = material->GetEffect();
		PassIf(library->GetDocument()->GetEffectLibrary()->GetEntity(0) == effect);
		PassIf(effect->GetNote() == FC("EmptyEffect.. for now!"));
		PassIf(effect->GetProfileCount() == 2);

		CheckEffectStandard(fileOut, (FCDEffectStandard*) effect->FindProfile(FUDaeProfileType::COMMON));
		CheckEffectFX(fileOut, (FCDEffectProfileFX*) effect->FindProfile(FUDaeProfileType::CG));
		return true;
	}

	bool CheckEffectStandard(FULogFile& fileOut, FCDEffectStandard* profile)
	{
		FailIf(profile == NULL);
		PassIf(profile->GetLightingType() == FCDEffectStandard::PHONG);
		PassIf(IsEquivalent(profile->GetDiffuseColor(), FMVector4(1.0f, 0.0f, -2.0f, 1.0f)));
		PassIf(IsEquivalent(profile->GetSpecularColor(), FMVector4(0.0f, 1.0f, 0.4f, 1.0f)));
		PassIf(IsEquivalent(profile->GetShininess(), 40.0f));
		PassIf(IsEquivalent(profile->GetIndexOfRefraction(), 21.0f));

		// There should be two textures in the bump channel.
		PassIf(profile->GetTextureCount(FUDaeTextureChannel::BUMP) == 2);
		FCDTexture* texture1 = NULL,* texture2 = NULL;
		for (size_t i = 0; i < 2; ++i)
		{
			FCDTexture* texture = profile->GetTexture(FUDaeTextureChannel::BUMP, i);
			FailIf(texture == NULL);
			if (texture1 == NULL) texture1 = texture;
			else if (texture2 == NULL) texture2 = texture;
			else Fail;
		}
		PassIf(texture1 != NULL && texture2 != NULL);

		// Verify the texture images
		FCDImage* image1 = profile->GetDocument()->FindImage(wantedImageId);
		FCDImage* image2 = profile->GetDocument()->FindImage(wantedImage2Id);
		PassIf(image1 != NULL && image2 != NULL);
		PassIf(texture1->GetImage() == image2);
		PassIf(texture2->GetImage() == image1);

		// Verify the placement parameters
		FCDETechnique* technique1 = texture1->GetExtra()->GetDefaultType()->FindTechnique(DAEMAYA_MAYA_PROFILE);
		FCDETechnique* technique2 = texture2->GetExtra()->GetDefaultType()->FindTechnique(DAEMAYA_MAYA_PROFILE);
		PassIf(technique1 != NULL && technique2 != NULL);
		FCDENode* node1 = technique1->FindParameter(DAEMAYA_TEXTURE_WRAPU_PARAMETER);
		FCDENode* node2 = technique2->FindParameter(DAEMAYA_TEXTURE_WRAPV_PARAMETER);
		PassIf(node1 != NULL && node2 != NULL);
		PassIf(IsEquivalent(FUStringConversion::ToFloat(node1->GetContent()), 2.0f));
		PassIf(IsEquivalent(FUStringConversion::ToFloat(node2->GetContent()), -1.2f));

		// Profile-level extra support verification.
		FCDEType* extraType = profile->GetExtra()->FindType("Curious");
		PassIf(extraType != NULL);
		FCDETechnique* extraTechnique = extraType->FindTechnique("Kiddy");
		PassIf(extraTechnique != NULL);
		FCDENode* extraNode = extraTechnique->FindParameter("Causes");
		PassIf(extraTechnique != NULL);
		PassIf(IsEquivalent(extraNode->GetContent(), FC("trouble")));

		// There should be an empty texture in the filter channel
		PassIf(profile->GetTextureCount(FUDaeTextureChannel::FILTER) == 1);
		FCDTexture* texture3 = profile->GetTexture(FUDaeTextureChannel::FILTER, 0);
		FailIf(texture3 == NULL || texture3->GetImage() != NULL);
		return true;
	}

	bool CheckEffectFX(FULogFile& fileOut, FCDEffectProfileFX* profile)
	{
		FailIf(profile == NULL);
		PassIf(profile->GetTechniqueCount() == 1);
		PassIf(profile->GetCodeCount() == 2);

		// Verify the code inclusions.
		FCDEffectCode* inlineCode = NULL,* filenameCode = NULL;
		for (size_t i = 0; i < profile->GetCodeCount(); ++i)
		{
			FCDEffectCode* c = profile->GetCode(i);
			switch (c->GetType())
			{
			case FCDEffectCode::INCLUDE: PassIf(filenameCode == NULL); filenameCode = c; break;
			case FCDEffectCode::CODE: PassIf(inlineCode == NULL); inlineCode = c; break;
			default: Fail; break;
			}
		}
		PassIf(inlineCode->GetCode() == FS("Some CG code for you to parse!"));
		PassIf(inlineCode->GetSubId() == "TouchesAKey");
		PassIf(filenameCode->GetFilename().find(FC("HowDoTheyDoIt.dd")) != fstring::npos);
		PassIf(filenameCode->GetSubId() == "PowerOfWill");

		// Grab the technique and check its name and passes, which are ordered.
		FCDEffectTechnique* technique = profile->GetTechnique(0);
		PassIf(technique->GetName() == FC("100 percent reason to remember the name."));
		PassIf(technique->GetPassCount() == 2);
		FCDEffectPass* pass1 = technique->GetPass(0);
		PassIf(pass1 != NULL);
		FCDEffectPass* pass2 = technique->GetPass(1);
		PassIf(pass2 != NULL);
		PassIf(pass1->GetPassName() == FC("First Pass"));
		PassIf(pass2->GetPassName() == FC("Second Pass"));

		// Verify the few parameters littered around.
		PassIf(profile->GetEffectParameterCount() == 3);
		FCDEffectParameterBool* booleanParameter = NULL;
		FCDEffectParameterSampler* samplerParameter = NULL;
		FCDEffectParameterSurface* surfaceParameter = NULL;
		for (size_t i = 0; i < 3; ++i)
		{
			FCDEffectParameter* p = profile->GetEffectParameter(i);
			if (p->GetObjectType() == FCDEffectParameterBool::GetClassType()) { FailIf(booleanParameter != NULL); booleanParameter = (FCDEffectParameterBool*) p; }
			else if (p->GetObjectType() == FCDEffectParameterSampler::GetClassType()) { FailIf(samplerParameter != NULL); samplerParameter = (FCDEffectParameterSampler*) p; }
			else if (p->GetObjectType() == FCDEffectParameterSurface::GetClassType()) { FailIf(surfaceParameter != NULL); surfaceParameter = (FCDEffectParameterSurface*) p; }
			else Fail;
		}
		FailIf(booleanParameter == NULL || samplerParameter == NULL || surfaceParameter == NULL);
		PassIf(booleanParameter->GetType() == FCDEffectParameter::BOOLEAN);
		PassIf(booleanParameter->GetValue() == true);
		PassIf(technique->GetEffectParameterCount() == 1);
		FCDEffectParameter* float2Parameter = technique->GetEffectParameter(0);
		FailIf(float2Parameter == NULL);
		PassIf(float2Parameter->GetType() == FCDEffectParameter::FLOAT2);
		PassIf(IsEquivalent(((FCDEffectParameterFloat2*) float2Parameter)->GetValue()->x, 1.5f));
		PassIf(IsEquivalent(((FCDEffectParameterFloat2*) float2Parameter)->GetValue()->y, 5.2f));

		// Verify the float2 parameter annotations
		FCDEffectParameterAnnotation* floatAnnotation = NULL,* integerAnnotation = NULL,* stringAnnotation = NULL,* booleanAnnotation = NULL;
		for (size_t i = 0; i < float2Parameter->GetAnnotationCount(); ++i)
		{
			FCDEffectParameterAnnotation* a = float2Parameter->GetAnnotation(i);
			if (a->type == FCDEffectParameter::FLOAT) { PassIf(floatAnnotation == NULL); floatAnnotation = a; }
			else if (a->type == FCDEffectParameter::INTEGER) { PassIf(integerAnnotation == NULL); integerAnnotation = a; }
			else if (a->type == FCDEffectParameter::STRING) { PassIf(stringAnnotation == NULL); stringAnnotation = a; }
			else if (a->type == FCDEffectParameter::BOOLEAN) { PassIf(booleanAnnotation == NULL); booleanAnnotation = a; }
			else Fail;
		}
		PassIf(floatAnnotation != NULL && integerAnnotation != NULL && stringAnnotation != NULL && booleanAnnotation != NULL);
		PassIf(*floatAnnotation->name == FC("UIMin") && IsEquivalent(FUStringConversion::ToFloat(*floatAnnotation->value), 0.2f));
		PassIf(*integerAnnotation->name == FC("UIMax") && IsEquivalent(FUStringConversion::ToInt32(*integerAnnotation->value), 5));
		PassIf(*stringAnnotation->name == FC("UIWidget") && IsEquivalent(*stringAnnotation->value, FS("RolloverSlider")));
		PassIf(*booleanAnnotation->name == FC("UIValid") && IsEquivalent(FUStringConversion::ToBoolean(*booleanAnnotation->value), true));

		// Check the sampler/surface parameter pair
		PassIf(surfaceParameter->GetType() == FCDEffectParameter::SURFACE);
		PassIf(surfaceParameter->GetImageCount() == 1);
		PassIf(surfaceParameter->GetImage(0)->GetDaeId() == surfaceParameterImageId);
		PassIf(surfaceParameter->GetReference() == "TestSF");
		PassIf(surfaceParameter->GetFormat() == "Plurial");
		PassIf(IsEquivalent(surfaceParameter->GetSurfaceType(), "RANDOMIZE"));

		FCDFormatHint* fhint = surfaceParameter->GetFormatHint();
		FailIf(fhint == NULL);
		PassIf(fhint->channels == FCDFormatHint::CHANNEL_RGB);
		PassIf(fhint->options.size() == 1 && fhint->options.front() == FCDFormatHint::OPT_NORMALIZED3);
		PassIf(fhint->precision == FCDFormatHint::PRECISION_MID);
		PassIf(fhint->range == FCDFormatHint::RANGE_SNORM);

		PassIf(samplerParameter->GetType() == FCDEffectParameter::SAMPLER);
		PassIf(samplerParameter->GetSurface()->GetReference() == surfaceParameter->GetReference());
		PassIf(samplerParameter->GetSamplerType() == FCDEffectParameterSampler::SAMPLER2D);
		PassIf(samplerParameter->GetReference() == "Gotit");
		return true;
	}

	bool FillImageLibrary(FULogFile& fileOut, FCDImageLibrary* library)
	{
		FailIf(library == NULL || library->GetEntityCount() > 0);
		FCDImage* image1 = library->AddEntity();
		FCDImage* image2 = library->AddEntity();
		FCDImage* image3 = library->AddEntity();

		image1->SetDaeId(wantedImageId);
		image1->SetFilename(FS("./Texture1.jpg"));
		image2->SetDaeId(wantedImage2Id);
		image2->SetFilename(FC("./Texture3D.jpg"));
		image2->SetWidth(256);
		image2->SetHeight(135);
		image3->SetWidth(33);
		image3->SetDepth(521);

		FailIf(image1->GetDaeId() == image2->GetDaeId());
		return true;
	}

	bool CheckImageLibrary(FULogFile& fileOut, FCDImageLibrary* library)
	{
		FailIf(library == NULL || library->GetEntityCount() != 3);

		// Retrieve the three images, verify that they match the id/filenames that we created.
		FCDImage* image1 = NULL,* image2 = NULL,* image3 = NULL;
		for (size_t i = 0; i < 3; ++i)
		{
			FCDImage* image = library->GetEntity(i);
			if (IsEquivalent(image->GetDaeId(), wantedImageId)) { FailIf(image1 != NULL); image1 = image; }
			else if (IsEquivalent(image->GetDaeId(), wantedImage2Id)) { FailIf(image2 != NULL); image2 = image; }
			else { FailIf(image3 != NULL); image3 = image; }
		}
		PassIf(image1 != NULL && image2 != NULL && image3 != NULL);

		// Verify the depth/width/height.
		PassIf(image1->GetWidth() == 0 && image1->GetHeight() == 0 && image1->GetDepth() == 0);
		PassIf(image2->GetWidth() == 256 && image2->GetHeight() == 135 && image2->GetDepth() == 0);
		PassIf(image3->GetWidth() == 33 && image3->GetHeight() == 0 && image3->GetDepth() == 521);

		// Verify the filenames. They should be absolute filenames now, so look for the wanted substrings.
		PassIf(strstr(TO_STRING(image1->GetFilename()).c_str(), "Texture1.jpg") != NULL);
		PassIf(strstr(TO_STRING(image2->GetFilename()).c_str(), "Texture3D.jpg") != NULL);
		return true;
	}
};

