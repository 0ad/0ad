#include "precompiled.h"

#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"
#include "FUtils/FUDaeSyntax.h"
#include "FUtils/FUFileManager.h"
#include "FUtils/FUXmlParser.h"

#include <cassert>

void require_(int line, bool value, const char* type, const char* message)
{
	if (value) return;
	char linestr[16];
	sprintf(linestr, "%d", line);
	throw ColladaException(std::string(type) + " (line " + linestr + "): " + message);
}

/** Error handler for libxml2 */
void errorHandler(void* ctx, const char* msg, ...)
{
	char buffer[1024];
	va_list ap;
	va_start(ap, msg);
	vsnprintf(buffer, sizeof(buffer), msg, ap);
	buffer[sizeof(buffer)-1] = '\0';
	va_end(ap);

	*((std::string*)ctx) += buffer;
}

FColladaErrorHandler::FColladaErrorHandler(std::string& xmlErrors_)
: xmlErrors(xmlErrors_)
{
	// Grab all the error output from libxml2, for useful error reporting
	xmlSetGenericErrorFunc(&xmlErrors, &errorHandler);

	FUError::SetErrorCallback(FUError::DEBUG, new FUFunctor3<FColladaErrorHandler, FUError::Level, uint32, uint32, void>(this, &FColladaErrorHandler::OnError));
	FUError::SetErrorCallback(FUError::WARNING, new FUFunctor3<FColladaErrorHandler, FUError::Level, uint32, uint32, void>(this, &FColladaErrorHandler::OnError));
	FUError::SetErrorCallback(FUError::ERROR, new FUFunctor3<FColladaErrorHandler, FUError::Level, uint32, uint32, void>(this, &FColladaErrorHandler::OnError));
}

FColladaErrorHandler::~FColladaErrorHandler()
{
	xmlSetGenericErrorFunc(NULL, NULL);

	FUError::SetErrorCallback(FUError::DEBUG, NULL);
	FUError::SetErrorCallback(FUError::WARNING, NULL);
	FUError::SetErrorCallback(FUError::ERROR, NULL);
}

void FColladaErrorHandler::OnError(FUError::Level errorLevel, uint32 errorCode, uint32 UNUSED(lineNumber))
{
	const char* errorString = FUError::GetErrorString((FUError::Code) errorCode);
	if (! errorString)
		errorString = "Unknown error code";

	if (errorLevel == FUError::DEBUG)
		Log(LOG_INFO, "FCollada message %d: %s", errorCode, errorString);
	else if (errorLevel == FUError::WARNING)
		Log(LOG_WARNING, "FCollada error %d: %s", errorCode, errorString);
	else
		throw ColladaException(errorString);
}


//////////////////////////////////////////////////////////////////////////

void FColladaDocument::LoadFromText(const char *text)
{
	document.reset(FCollada::NewTopDocument());

	FUFileManager* fileManager = document->GetFileManager();
	const char* basePath = "";

	// Mostly copied from FCDocument::LoadFromText

	bool status = true;

	// Push the given path unto the file manager's stack
	fileManager->PushRootPath(basePath);

	// Parse the document into a XML tree
	xmlDoc* daeDocument = xmlParseDoc((const xmlChar*)text);
	if (daeDocument != NULL)
	{
		xmlNode *rootNode = xmlDocGetRootElement(daeDocument);

		// Read in the whole document from the root node
		status &= (document->LoadDocumentFromXML(rootNode));

		// HACK (sort of): read in <extra> from the root, because FCollada
		// doesn't let us do that
		ReadExtras(rootNode);

		// Free the XML document
		xmlFreeDoc(daeDocument);
	}
	else
	{
		FUError::Error(FUError::ERROR, FUError::ERROR_MALFORMED_XML);
		status = false;
	}

	// Clean-up the XML reader
	xmlCleanupParser();

	// Restore the original OS current folder
	fileManager->PopRootPath();

	if (status)
		FUError::Error(FUError::DEBUG, FUError::DEBUG_LOAD_SUCCESSFUL);

	REQUIRE_SUCCESS(status);
}

void FColladaDocument::ReadExtras(xmlNode* colladaNode)
{
	if (! IsEquivalent(colladaNode->name, DAE_COLLADA_ELEMENT))
		return;

	extra.reset(new FCDExtra(document.get()));

	xmlNodeList extraNodes;
	FUXmlParser::FindChildrenByType(colladaNode, DAE_EXTRA_ELEMENT, extraNodes);
	for (xmlNodeList::iterator it = extraNodes.begin(); it != extraNodes.end(); ++it)
	{
		xmlNode* extraNode = (*it);
		extra->LoadFromXML(extraNode);
	}
}

//////////////////////////////////////////////////////////////////////////

// HACK: These don't get exported properly from FCollada (3.02, DLL), so define
// them here instead of fixing it correctly.
const FMVector3 FMVector3::XAxis(1.0f, 0.0f, 0.0f);
static float identity[] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
FMMatrix44 FMMatrix44::Identity(identity);

struct FoundInstance
{
	FCDEntityInstance* instance;
	FMMatrix44 transform;
};

static bool IsVisible_XSI(FCDSceneNode* node, bool& visible)
{
	// Look for <extra><technique profile="XSI"><SI_Visibility><xsi_param sid="visibility">

	FCDExtra* extra = node->GetExtra();
	if (! extra) return false;

	FCDEType* type = extra->GetDefaultType();
	if (! type) return false;

	FCDETechnique* technique = type->FindTechnique("XSI");
	if (! technique) return false;

	FCDENode* visibility1 = technique->FindChildNode("SI_Visibility");
	if (! visibility1) return false;

	FCDENode* visibility2 = visibility1->FindChildNode("xsi_param");
	if (! visibility2) return false;

	if (IsEquivalent(visibility2->GetContent(), "TRUE"))
		visible = true;
	else if (IsEquivalent(visibility2->GetContent(), "FALSE"))
		visible = false;
	return true;
}

static bool IsVisible(FCDSceneNode* node)
{
	bool visible;

	// Try the XSI visibility property
	if (IsVisible_XSI(node, visible))
		return visible;

	// Else fall back to the FCollada-specific setting
	visible = (node->GetVisibility() != 0.0);
	return visible;
}

/**
 * Recursively finds all entities under the current node. If onlyMarked is
 * set, only matches entities where the user-defined property was set to
 * "export" in the modelling program.
 *
 * @param node root of subtree to search
 * @param instances output - appends matching entities
 * @param transform transform matrix of current subtree
 * @param onlyMarked only match entities with "export" property
 */
static void FindInstances(FCDSceneNode* node, std::vector<FoundInstance>& instances, const FMMatrix44& transform, bool onlyMarked)
{
	for (size_t i = 0; i < node->GetChildrenCount(); ++i)
	{
		FCDSceneNode* child = node->GetChild(i);
		FindInstances(child, instances, transform * node->ToMatrix(), onlyMarked);
	}

	for (size_t i = 0; i < node->GetInstanceCount(); ++i)
	{
		if (onlyMarked)
		{
			if (node->GetNote() != "export")
				continue;
		}

		// Only accept instances of appropriate types, and not e.g. lights
		FCDEntity::Type type = node->GetInstance(i)->GetEntityType();
		if (! (type == FCDEntity::GEOMETRY || type == FCDEntity::CONTROLLER))
			continue;

		// Ignore invisible objects, because presumably nobody wanted to export them
		if (! IsVisible(node))
			continue;

		FoundInstance f;
		f.transform = transform * node->ToMatrix();
		f.instance = node->GetInstance(i);
		instances.push_back(f);
		Log(LOG_INFO, "Found convertible object '%s'", node->GetName().c_str());
	}
}

bool FindSingleInstance(FCDSceneNode* node, FCDEntityInstance*& instance, FMMatrix44& transform)
{
	std::vector<FoundInstance> instances;

	FindInstances(node, instances, FMMatrix44::Identity, true);
	if (instances.size() > 1)
	{
		Log(LOG_ERROR, "Found too many export-marked objects");
		return false;
	}
	if (instances.empty())
	{
		FindInstances(node, instances, FMMatrix44::Identity, false);
		if (instances.size() > 1)
		{
			Log(LOG_ERROR, "Found too many possible objects to convert - try adding the 'export' property to disambiguate one");
			return false;
		}
		if (instances.empty())
		{
			Log(LOG_ERROR, "Didn't find any objects in the scene");
			return false;
		}
	}

	assert(instances.size() == 1); // if we got this far
	instance = instances[0].instance;
	transform = instances[0].transform;
	return true;
}

//////////////////////////////////////////////////////////////////////////

static bool ReverseSortWeight(const FCDJointWeightPair& a, const FCDJointWeightPair& b)
{
	return (a.weight > b.weight);
}

void SkinReduceInfluences(FCDSkinController* skin, size_t maxInfluenceCount, float minimumWeight)
{
	FCDWeightedMatches& weightedMatches = skin->GetWeightedMatches();
	for (FCDWeightedMatches::iterator itM = weightedMatches.begin(); itM != weightedMatches.end(); ++itM)
	{
		FCDJointWeightPairList& weights = (*itM);

		FCDJointWeightPairList newWeights;
		for (FCDJointWeightPairList::iterator itW = weights.begin(); itW != weights.end(); ++itW)
		{
			// If this joint already has an influence, just add the weight
			// instead of adding a new influence
			bool done = false;
			for (FCDJointWeightPairList::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
			{
				if (itW->jointIndex == itNW->jointIndex)
				{
					itNW->weight += itW->weight;
					done = true;
					break;
				}
			}

			if (done)
				continue;

			// Not had this joint before, so add it
			newWeights.push_back(*itW);
		}

		// Put highest-weighted influences at the front of the list
		sort(newWeights.begin(), newWeights.end(), ReverseSortWeight);

		// Limit the maximum number of influences
		if (newWeights.size() > maxInfluenceCount)
			newWeights.resize(maxInfluenceCount);

		// Enforce the minimum weight per influence
		while (!newWeights.empty() && newWeights.back().weight < minimumWeight)
			newWeights.pop_back();

		// Renormalise, so sum(weights)=1
		float totalWeight = 0;
		for (FCDJointWeightPairList::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
			totalWeight += itNW->weight;
		for (FCDJointWeightPairList::iterator itNW = newWeights.begin(); itNW != newWeights.end(); ++itNW)
			itNW->weight /= totalWeight;

		// Copy new weights into the skin
		weights = newWeights;
	}

	skin->SetDirtyFlag();
}


void FixSkeletonRoots(FCDControllerInstance* controllerInstance)
{
	// HACK: The XSI exporter doesn't do a <skeleton> and FCollada doesn't
	// seem to know where else to look, so just guess that it's somewhere
	// under Scene_Root
	if (controllerInstance->GetSkeletonRoots().empty())
	{
		// HACK (evil): SetSkeletonRoot is declared but not defined, and there's
		// no other proper way to modify the skeleton-roots list, so cheat horribly
		FUUriList& uriList = const_cast<FUUriList&>(controllerInstance->GetSkeletonRoots());
		uriList.push_back(FUUri("Scene_Root"));
		controllerInstance->LinkImport();
	}
}
