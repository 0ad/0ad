/*
	DESCRIPTION	: GUI animation functions (sliding in/out, fading, etc).
	NOTES	: 
*/

// ====================================================================

function GUISlideIn(GUIObject, SlideSpeed)
{

	// Just me testing some stuff :)
	
	var SlideObject = getGUIObjectByName(GUIObject);
	var SizeVar = SlideObject.size;
	//var Sizes = SizeVar.split(" ");

	//var x_s = Sizes[0];
	//var y_s = Sizes[1];
	//var x_e = Sizes[2];
	//var y_e = Sizes[3];
	//var height = x_e - x_s;

	//var Time = new Date();
	//var StartTime = Time.getTime();
	
	var i = 0;
	while(i <= 2) {
		i++;
		SlideObject.size = new GUISize(x_s, 0 + i, x_e, 0 + i + height);
	}

}

// ====================================================================
