function GUIObjectHide(objectName) 
{
        // Hide our GUI object
        GUIObject = getGUIObjectByName(objectName);
        GUIObject.hidden = true;

}

// ====================================================================

function GUIObjectUnhide(objectName)
{
        // Unhide our GUI object
        GUIObject = getGUIObjectByName(objectName);
        GUIObject.hidden = false;

}

// ====================================================================

function GUIObjectToggle(objectName)
{
        // Get our GUI object
        GUIObject = getGUIObjectByName(objectName);

        // Toggle it
        GUIObject.hidden = !GUIObject.hidden;
}

// ====================================================================

