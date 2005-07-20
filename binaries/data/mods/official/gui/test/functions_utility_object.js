function GUIObjectHide(objectName) 
{
        // Hide our GUI object
        var GUIObject = getGUIObjectByName(objectName);
        GUIObject.hidden = true;

}

// ====================================================================

function GUIObjectUnhide(objectName)
{
        // Unhide our GUI object
        var GUIObject = getGUIObjectByName(objectName);
        GUIObject.hidden = false;

}

// ====================================================================

function GUIObjectToggle(objectName)
{
        // Get our GUI object
        var GUIObject = getGUIObjectByName(objectName);

        // Toggle it
        GUIObject.hidden = !GUIObject.hidden;
}

// ====================================================================

function GUIObjectRenameandReveal(objectName, objectCaption)
{
        // Get our GUI object
        var GUIObject = getGUIObjectByName(objectName);

	// Rename it
	GUIObject.caption = objectCaption;

        // Toggle it
        GUIObject.hidden = false;
}

// ====================================================================

function GUIObjectModifyCaption(objectName, objectModifier, objectPlaces)
{
	// Adds a modifier to a GUI object's caption (eg increase a number by 1).
	// Obviously don't use this unless you're sure the caption contains a number.
	// objectPlaces specifies the number of decimal places to use for a floating point number.
	// If not specified, it defaults to zero (whole number).

        // Get our GUI object
        var GUIObject = getGUIObjectByName(objectName);

	if (!objectPlaces)
		objectPlaces = 0;
	objectPlaces = Math.pow(10, objectPlaces);

	GUIObject.caption = (Math.round(objectPlaces * GUIObject.caption) + Math.round(objectPlaces * objectModifier)) / objectPlaces;
}

// ====================================================================

function GUIObjectSetCaption(objectName, objectCaption)
{
	// Sets an object's caption to the specified value.

        // Get our GUI object
        var GUIObject = getGUIObjectByName(objectName);

	GUIObject.caption = objectCaption;
}

// ====================================================================

function SwitchWindow(CloseWindowName, OpenWindowName)
{
	// Helper function that closes a given window (usually the current parent) and opens another one.

	GUIObjectHide	(CloseWindowName);
	GUIObjectUnhide	(OpenWindowName);
}

// ====================================================================