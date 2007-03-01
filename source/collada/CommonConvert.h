#ifndef COMMONCONVERT_H__
#define COMMONCONVERT_H__

#include <exception>
#include <string>

class FUStatus;
class FCDSceneNode;
class FCDEntityInstance;
class FMMatrix44;
class FCDSkinController;

class ColladaException : public std::exception
{
public:
	ColladaException(const std::string& msg)
		: msg(msg)
	{
	}

	~ColladaException() throw()
	{
	}

	virtual const char* what() const throw()
	{
		return msg.c_str();
	}
private:
	std::string msg;
};

struct OutputCB
{
	virtual void operator() (const char* data, unsigned int length)=0;
};

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

/** Throws a ColladaException unless the value is true. */
#define REQUIRE(value, message) require_(__LINE__, value, "Assertion not satisfied", message)

/** Throws a ColladaException unless the status is successful. */
#define REQUIRE_SUCCESS(status) require_(__LINE__, status, "FCollada error", "Line " STRINGIFY(__LINE__))
#define STRINGIFY(x) #x

void require_(int line, bool value, const char* type, const char* message);

/** Outputs a structure, using sizeof to get the size. */
template<typename T> void write(OutputCB& output, const T& data)
{
	output((char*)&data, sizeof(T));
}

/** Error handler for libxml2 */
void errorHandler(void* ctx, const char* msg, ...);


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

#endif // COMMONCONVERT_H__
