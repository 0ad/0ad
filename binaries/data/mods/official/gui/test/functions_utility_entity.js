function GetAttrib(getAttribName)
{
	// Simple helper function that elegantly checks if an entity attribute exists.
	// Returns blank if it doesn't, or it's value if it does.

	if (getAttribName)
		return getAttribName;
	else
		return "";
}

