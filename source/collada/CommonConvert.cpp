#include "precompiled.h"

#include "CommonConvert.h"

#include "FCollada.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSkinController.h"

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

// These don't get exported properly from FCollada (3.02, DLL), so define them
// here instead of fixing it correctly.
const FMVector3 FMVector3::XAxis(1.0f, 0.0f, 0.0f);
static float identity[] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
FMMatrix44 FMMatrix44::Identity(identity);

struct FoundInstance
{
	FCDEntityInstance* instance;
	FMMatrix44 transform;
};

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

		FoundInstance f;
		f.transform = transform * node->ToMatrix();
		f.instance = node->GetInstance(i);
		instances.push_back(f);
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
