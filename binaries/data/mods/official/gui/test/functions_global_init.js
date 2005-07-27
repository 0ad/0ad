/*
	DESCRIPTION	: Global initialisation functions, called when scripts first included at startup.
	NOTES		: Use for any initialisation that cannot be mapped to an object.
*/

// ====================================================================

function init ()
{
	// Root point for anything that needs to be globally initialised at startup.

// (Both of these need to be rewritten, so might be removed.)
	initCoord();
	initSession();
}

// ====================================================================