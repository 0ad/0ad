/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDAnimated.h"
#include "FCDocument/FCDAnimation.h"
#include "FCDocument/FCDAnimationChannel.h"
#include "FCDocument/FCDAnimationCurve.h"
#include "FCDocument/FCDLight.h"
#include "FCTestExportImport.h"

static fm::string animId1 = "GrossAnimation";
static fm::string animId2 = "GrossAnimation";
static fm::string animatedLightId;
static const fstring animSubTreeNote = FS("TestingSTNote");

namespace FCTestExportImport
{
	bool skipAsserts = false; // hack #1
	static const char* szTestName = "FCTestExportImportAnimation"; // hack #2

	bool FillAnimationLibrary(FULogFile& fileOut, FCDAnimationLibrary* library)
	{
		// Create two more tree within the animation library
		FailIf(library == NULL);
		size_t startAnimCount = library->GetEntityCount();
		FCDAnimation* animTree1 = library->AddEntity();
		FCDAnimation* animTree2 = library->AddEntity();
		PassIf(library->GetEntityCount() == startAnimCount + 2);

		// Retrieve the ids of the created entities.
		animTree2->SetDaeId(animId2);
		animTree1->SetDaeId(animId1);
		FailIf(animId1.empty());
		FailIf(animId2.empty());
		FailIf(animId1 == animId2);

		// Add to the first animation tree some sub-trees.
		FCDAnimation* animSubTree1 = animTree1->AddChild();
		FCDAnimation* animSubTree2 = animTree1->AddChild();
		animSubTree1->SetNote(animSubTreeNote);
		animSubTree2->SetNote(animSubTreeNote);
		FailIf(animTree1->GetChildrenCount() != 2);

		// Animate some selected parameters
		FillAnimationLight(fileOut, animSubTree2->GetDocument(), animSubTree2);
		return true;
	}

	bool CheckAnimationLibrary(FULogFile& fileOut, FCDAnimationLibrary* library)
	{
		FailIf(library == NULL);

		// Retrieve the animation trees using the saved ids.
		FCDAnimation* animTree1 = library->FindDaeId(animId1);
		FCDAnimation* animTree2 = library->FindDaeId(animId2);
		FailIf(animTree1 == NULL);
		FailIf(animTree2 == NULL);

		// Verify that the first animation tree has the correct sub-trees.
		// Retrieve the animation sub-tree which contains our channels.
		FCDAnimation* ourChannels = NULL;
		FailIf(animTree1->GetChildrenCount() != 2);
		for (size_t i = 0; i < 2; ++i)
		{
			FCDAnimation* subTree = animTree1->GetChild(i);
			FailIf(subTree == NULL);
			PassIf(subTree->GetNote() == animSubTreeNote);
			if (subTree->GetChannelCount() > 0)
			{
				PassIf(ourChannels == NULL);
				ourChannels = subTree;
			}
		}

		PassIf(ourChannels != NULL);
		CheckAnimationLight(fileOut, ourChannels->GetDocument(), ourChannels);
		return true;
	}

	bool FillAnimationLight(FULogFile& fileOut, FCDocument* document, FCDAnimation* animationTree)
	{
		// Retrieve a light entity and add an animation to its color
		FCDLightLibrary* lightLibrary = document->GetLightLibrary();
		PassIf(lightLibrary != NULL);
		PassIf(lightLibrary->GetEntityCount() > 0);
		FCDLight* light1 = lightLibrary->GetEntity(0);
		animatedLightId = light1->GetDaeId();

		// Create the animated object for the color
		FCDAnimated* animated = light1->GetColor().GetAnimated();
		FailIf(animated == NULL);
		FailIf(animated->GetValueCount() != 3);

		// Create a channel for the animation curves
		FCDAnimationChannel* channel = animationTree->AddChannel();
		FailIf(channel == NULL);

		FCDAnimationCurve* curve = channel->AddCurve();
		animated->AddCurve(0, curve);
		return true;
	}

	bool CheckAnimationLight(FULogFile& fileOut, FCDocument* document, FCDAnimation* UNUSED(animationTree))
	{
		// Retrieve the light whose color is animated.
		FCDLightLibrary* lightLibrary = document->GetLightLibrary();
		PassIf(lightLibrary != NULL);
		PassIf(lightLibrary->GetEntityCount() > 0);
		FCDLight* light1 = lightLibrary->FindDaeId(animatedLightId);
		PassIf(light1 != NULL);
		return true;
	}
};

