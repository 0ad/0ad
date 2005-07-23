function guiHide (objectName) 
{
        // Hide our GUI object
        var guiObject = getGUIObjectByName (objectName);
        guiObject.hidden = true;

}

// ====================================================================

function guiUnHide (objectName)
{
        // Unhide our GUI object
        var guiObject = getGUIObjectByName (objectName);
        guiObject.hidden = false;

}

// ====================================================================

function guiToggle (objectName)
{
        // Get our GUI object
        var guiObject = getGUIObjectByName (objectName);

        // Toggle it
        guiObject.hidden = !guiObject.hidden;
}

// ====================================================================

function guiRenameAndReveal (objectName, objectCaption)
{
        // Get our GUI object
        var guiObject = getGUIObjectByName (objectName);

	// Rename it
	guiObject.caption = objectCaption;

        // Toggle it
        guiObject.hidden = false;
}

// ====================================================================

function guiModifyCaption (objectName, objectModifier, objectPlaces)
{
	// Adds a modifier to a GUI object's caption (eg increase a number by 1).
	// Obviously don't use this unless you're sure the caption contains a number.
	// objectPlaces specifies the number of decimal places to use for a floating point number.
	// If not specified, it defaults to zero (whole number).

        // Get our GUI object
        var guiObject = getGUIObjectByName (objectName);

	if (!objectPlaces)
		objectPlaces = 0;
	objectPlaces = Math.pow (10, objectPlaces);

	guiObject.caption = 	(Math.round (objectPlaces * guiObject.caption)
				+ Math.round (objectPlaces * objectModifier) ) / objectPlaces;
}

// ====================================================================

function guiSetCaption (objectName, objectCaption)
{
	// Sets an object's caption to the specified value.

        // Get our GUI object
        var guiObject = getGUIObjectByName (objectName);

	guiObject.caption = objectCaption;
}

// ====================================================================

function guiSwitch (closeWindowName, openWindowName)
{
	// Helper function that closes a given window (usually the current parent) and opens another one.

	guiHide		(closeWindowName);
	guiUnHide	(openWindowName);
}

// ====================================================================