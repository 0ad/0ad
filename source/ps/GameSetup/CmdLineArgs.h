#ifndef INCLUDED_CMDLINEARGS
#define INCLUDED_CMDLINEARGS

#include "ps/CStr.h"

class CmdLineArgs
{
public:
	CmdLineArgs() {}

	/**
	 * Parse the command-line options, for future processing.
	 * All arguments are required to be of the form <tt>-name</tt> or
	 * <tt>-name=value</tt> - anything else is ignored.
	 *
	 * @param argc size of argv array
	 * @param argv array of arguments; argv[0] should be the program's name
	 */
	CmdLineArgs(int argc, const char* argv[]);

	/**
	 * Test whether the given name was specified, as either <tt>-name</tt> or
	 * <tt>-name=value</tt>
	 */
	bool Has(const char* name) const;

	/**
	 * Get the value of the named parameter. If it was not specified, returns
	 * the empty string. If it was specified multiple times, returns the value
	 * from the first occurrence.
	 */
	CStr Get(const char* name) const;

	/**
	 * Get all the values given to the named parameter. Returns values in the
	 * same order as they were given in argv.
	 */
	std::vector<CStr> GetMultiple(const char* name) const;

	/**
	 * Get the value of argv[0], which is typically meant to be the name/path of
	 * the program (but the actual value is up to whoever executed the program).
	 */
	CStr GetArg0() const;

private:
	typedef std::vector<std::pair<CStr, CStr> > ArgsT;
	ArgsT m_Args;
	CStr m_Arg0;
};

#endif // INCLUDED_CMDLINEARGS
