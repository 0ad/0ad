function GUIObjectHide(objectName) {

        // Hide our GUI object
        GUIObject = getGUIObjectByName(objectName);
        GUIObject.hidden = true;

}

function GUIObjectUnhide(objectName) {

        // Unhide our GUI object
        GUIObject = getGUIObjectByName(objectName);
        GUIObject.hidden = false;

}

function GUIObjectToggle(objectName) {

        // Get our GUI object
        GUIObject = getGUIObjectByName(objectName);

        // Toggle it
        GUIObject.hidden = !GUIObject.hidden;

}

function GUIUpdateObjectInfo() {

        // Get GUI Objects needed
        ObjectNameText = getGUIObjectByName("selection_name_test");
        ObjectPositionText = getGUIObjectByName("selection_pos_test");
        ObjectSpeedText = getGUIObjectByName("selection_speed_test");

        // Check number of selected entities
        if (selection.length > 1) {

                var MultipleEntitiesSelected = true;
                ObjectNameText.caption = "Selected " + selection.length + " entities.";
                ObjectNameText.hidden = false;
                ObjectPositionText.hidden = true;
                ObjectSpeedText.hidden = true;

        } else {

                if (!selection[0]) {

                        // Reset object name
                        ObjectNameText.caption = "";
                        ObjectNameText.hidden = true;

                        // Reset position
                        ObjectPositionText.caption = "";
                        ObjectPositionText.hidden = true;

                        // Reset Speed
                        ObjectSpeedText.caption = "";
                        ObjectSpeedText.hidden = true;

                } else {

                        // Update object name
                        ObjectNameText.caption = selection[0].name;
                        ObjectNameText.hidden = false;

                        // Update position
                        var strString = "" + selection[0].position;
                        ObjectPositionText.caption = "Position: " + strString.substring(20,strString.length-3);
                        //ObjectPositionText.caption = strString;
                        ObjectPositionText.hidden = false;

                        // Update speed
                        ObjectSpeedText.caption = "Speed: " + selection[0].speed;
                        ObjectSpeedText.hidden = false;
                }
        
        }
        
}
