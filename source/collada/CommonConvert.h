#ifndef INCLUDED_COMMONCONVERT
#define INCLUDED_COMMONCONVERT

#include <exception>
#include <string>
#include <memory>
#include <vector>

class FCDEntityInstance;
class FCDSceneNode;
class FCDSkinController;
class FMMatrix44;
class FUStatus;

class Skeleton;

class ColladaException : public std::exception
{
public:
	ColladaException(const std::string& msg) : msg(msg) { }
	~ColladaException() throw() { }
	virtual const char* what() const throw() { return msg.c_str(); }
private:
	std::string msg;
};

struct OutputCB
{
	virtual void operator() (const char* data, unsigned int length)=0;
};

/**
 * Standard error handler - logs FCollada messages using Log(), and also
 * maintains a list of XML parser errors.
 */
class FColladaErrorHandler
{
public:
	FColladaErrorHandler(std::string& xmlErrors);
	~FColladaErrorHandler();

private:
	void OnError(FUError::Level errorLevel, uint32 errorCode, uint32 lineNumber);
	std::string& xmlErrors;

	void operator=(FColladaErrorHandler);
};

/**
 * Standard document loader. Based on FCDocument::LoadFromText, but allows
 * access to <extra> nodes at the document level (i.e. directly in <COLLADA>).
 */
class FColladaDocument
{
public:
	/**
	 * Loads the document from the given XML string. Should be the first function
	 * called on this object, and should only be called once.
	 * @throws ColladaException if unable to load.
	 */
	void LoadFromText(const char* text);

	/** Returns the FCDocument that was loaded. */
	FCDocument* GetDocument() const { return document.get(); }

	/** Returns the <extra> data from the <COLLADA> element. */
	FCDExtra* GetExtra() const { return extra.get(); }

private:
	void ReadExtras(xmlNode* colladaNode);
	std::auto_ptr<FCDocument> document;
	std::auto_ptr<FCDExtra> extra;
};

/**
 * Wrapper for code shared between the PMD and PSA converters. Loads the document
 * and provides access to the relevant objects and values.
 */
class CommonConvert
{
public:
	CommonConvert(const char* text, std::string& xmlErrors);
	~CommonConvert();
	const FColladaDocument& GetDocument() const { return m_Doc; }
	FCDSceneNode& GetRoot() { return *m_Doc.GetDocument()->GetVisualSceneRoot(); }
	FCDEntityInstance& GetInstance() { return *m_Instance; }
	const FMMatrix44& GetEntityTransform() const { return m_EntityTransform; }
	bool IsYUp() const { return m_YUp; }
	bool IsXSI() const { return m_IsXSI; }

private:
	FColladaErrorHandler m_Err;
	FColladaDocument m_Doc;
	FCDEntityInstance* m_Instance;
	FMMatrix44 m_EntityTransform;
	bool m_YUp;
	bool m_IsXSI;
};

/** Throws a ColladaException unless the value is true. */
#define REQUIRE(value, message) require_(__LINE__, value, "Assertion not satisfied", "failed requirement \"" message "\"")

/** Throws a ColladaException unless the status is successful. */
#define REQUIRE_SUCCESS(status) require_(__LINE__, status, "FCollada error", "Line " STRINGIFY(__LINE__))
#define STRINGIFY(x) #x

void require_(int line, bool value, const char* type, const char* message);

/** Outputs a structure, using sizeof to get the size. */
template<typename T> void write(OutputCB& output, const T& data)
{
	output((char*)&data, sizeof(T));
}

/**
 * Tries to find a single suitable entity instance in the scene. Fails if there
 * are none, or if there are too many and it's not clear which one should
 * be converted.
 *
 * @param node root scene node to search under
 * @param instance output - the found entity instance (if any)
 * @param transform - the world-space transform of the found entity
 *
 * @return true if one was found
 */
bool FindSingleInstance(FCDSceneNode* node, FCDEntityInstance*& instance, FMMatrix44& transform);

/**
 * Like FCDSkinController::ReduceInfluences but works correctly.
 * Additionally, multiple influences for the same joint-vertex pair are
 * collapsed into a single influence.
 */
void SkinReduceInfluences(FCDSkinController* skin, size_t maxInfluenceCount, float minimumWeight);

/**
 * Fixes some occasional problems with the skeleton root definitions in a
 * controller. (In particular, it's needed for models exported from XSI.)
 * Should be called before extracting any joint information from the controller.
 */
void FixSkeletonRoots(FCDControllerInstance& controllerInstance);

/**
 * Finds the skeleton definition which best matches the given controller.
 * @throws ColladaException if none is found.
 */
const Skeleton& FindSkeleton(const FCDControllerInstance& controllerInstance);

/** Bone pose data */
struct BoneTransform
{
	float translation[3];
	float orientation[4];
};

/**
 * Performs the standard transformations on bones, applying a scale matrix and
 * moving them into the game's coordinate space.
 */
void TransformBones(std::vector<BoneTransform>& bones, const FMMatrix44& scaleTransform, bool yUp);

extern FMMatrix44 FMMatrix44_Identity;

#endif // INCLUDED_COMMONCONVERT
