function initAtlasSectionTerrainEditor()
{
	// ============================================= TERRAIN EDITOR SECTION MENU ===============================================

	// ============================================= TERRAIN EDITOR: EDIT ELEVATION ===============================================

	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_BUTTON_SPAN = 10;

	// Modify button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 50; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Smooth button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width+ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_BUTTON_SPAN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].y; 

	// Sample button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].width+ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_BUTTON_SPAN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].y; 

	// Intensity slider bar.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 138; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Intensity slider marker.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_MARKER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 8; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].x+40; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].y+(Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].height/3); 

	// Intensity label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 50; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].x-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].y; 

	// Roughen button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Roughen style combo box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_COMBO_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 75; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].width-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_COMBO_BOX].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].y+4; 

	// Roughen style label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_COMBO_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].y; 

	// Power input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].width-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].y+4+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Power label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y; 

	// Scale label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y; 

	// Scale input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_LABEL].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Increment button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Amount input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].width-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].y; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Amount label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 45; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].y; 

	// Horizontal rule at end of Edit Elevation.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_BKG].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN; 
	Crd[Crd.last-1].height	= 2; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_BKG].x+ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].height+ATLAS_LEFT_PANE_SECTION.BMARGIN; 

	// ============================================= TERRAIN EDITOR: CLIFF ===============================================

	// List of cliff portraits.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 75; 
	Crd[Crd.last-1].height	= 162; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].x+3; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].x+3; 

	// "Cliff" heading.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].x-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].width; 
	Crd[Crd.last-1].height	= 22; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].height; 

	// Horizontal rule under heading.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].x+ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].height; 

	// Place button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 84; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR].height+(ATLAS_LEFT_PANE_SECTION.TMARGIN/2); 

	// Height label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 48; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].x+2; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].height+ATLAS_LEFT_PANE_SECTION.TMARGIN*1.5; 

	// Height input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].y; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Angle label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].height+ATLAS_LEFT_PANE_SECTION.TMARGIN*1.5; 

	// Angle input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN*1.5; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Smooth label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_SMOOTH_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_LABEL].height+ATLAS_LEFT_PANE_SECTION.TMARGIN*1.5; 

	// Smooth input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_SMOOTH_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_ANGLE_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN*1.5; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_SMOOTH_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_SMOOTH_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_SMOOTH_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Custom button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].height-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].height; 

	// Horizontal rule at end of Paint Cliff.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].height+ATLAS_LEFT_PANE_SECTION.BMARGIN; 

	// ============================================= TERRAIN EDITOR: WATER ===============================================

	// List of water portraits.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].x+3; 

	// "Water" heading.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HEADING = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].y; 

	// Horizontal rule under heading.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HEADING_HR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEADING_HR].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HEADING].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HEADING].height; 

	// Place button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_PLACE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].width-15; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_PLACE].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HEADING_HR].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HEADING_HR].height+(ATLAS_LEFT_PANE_SECTION.TMARGIN/2); 

	// Beautify Water button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_BEAUTIFY = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 15; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_PLACE].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_PLACE].width+1; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_PLACE].y; 

	// Depth label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_PLACE].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_PLACE].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Depth input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HEIGHT_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].y; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Colour label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height+ATLAS_LEFT_PANE_SECTION.TMARGIN*1.5; 

	// Colour input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_R_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height/1.5; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN/1.2; 

	// Colour input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_G_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height/1.5; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_R_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_R_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN/1.2; 

	// Colour input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_B_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height/1.5; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_G_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_G_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN/1.2; 

	// Colour tint box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_TINT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL].width/3; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_TINT_BOX].width; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL].x+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL].width/5; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL].height+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_LABEL].height/3; 

	// Smooth input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_SMOOTH_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_B_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_COLOUR_B_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_SMOOTH_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_SMOOTH_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 	
	setCoordInputUpDown(Crd[Crd.last-2], Crd[Crd.last-1], ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_SMOOTH_INPUT_BOX,
		left_screen, top_screen, left_screen, top_screen);

	// Smooth label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_SMOOTH_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_SMOOTH_INPUT_BOX].y-2; 

	// Custom button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_CUSTOM = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST].height-Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_BUTTON_CUSTOM].height; 

	// Horizontal rule at end of Paint Water.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST].y+Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST].height+ATLAS_LEFT_PANE_SECTION.BMARGIN; 

	// ============================================= TERRAIN EDITOR: BRUSH SETTINGS ==============================================

	// Terrain Preview.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 125; 
	Crd[Crd.last-1].height	= 125; 
	Crd[Crd.last-1].x	= ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= ATLAS_LEFT_PANE_SECTION.LMARGIN; 

	// Terrain Preview label.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].width; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].y+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].height; 

	// Brush border.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_RB_CORNER].x-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].x+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].width+(Crd[ATLAS_RB_CORNER].width/4); 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PREVIEW].y; 

	// Brush border label.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].width; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].y+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].height; 

	// Brush size slider bar.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_SIZE_SLIDER_BAR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 15; 
	Crd[Crd.last-1].height	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].height+11; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].x+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].width-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_SIZE_SLIDER_BAR].width-5; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].y-22; 

	// Brush size slider marker.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_SIZE_SLIDER_MARKER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_SIZE_SLIDER_BAR].width; 
	Crd[Crd.last-1].height	= 8; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_SIZE_SLIDER_BAR].x-3; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_SIZE_SLIDER_BAR].y+70; 

	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_SPAN = 5;

	// Brush button.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 15; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].x+10; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].y+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].height-15-10; 

	// Brush button.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_2 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].y-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].height-ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_SPAN; 

	// Brush button.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_3 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_2].y-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_2].height-ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_SPAN; 

	// Brush button.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_4 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_3].y-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_3].height-ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_SPAN; 

	// Beautify Terrain button.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_BEAUTIFY = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BUTTON_1].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_BRUSH_BORDER].y+10; 

	// ============================================= TERRAIN EDITOR: PAINT TERRAIN ===============================================

	// Terrain Palette Background.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 603; 
	Crd[Crd.last-1].height	= 122; 
	Crd[Crd.last-1].x	= Crd[ATLAS_RB_CORNER].x+Crd[ATLAS_RB_CORNER].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_BKG].y+5; 

	// Terrain Palette Tabs.	
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB = new Object();
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.span = 9;
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.max = 9;
	for (ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last = 1; ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last <= ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.max; ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last++)
	{
		ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last] = Crd.last;
		Crd[Crd.last] = new Object();
		Crd[Crd.last].width = 63;
		Crd[Crd.last].height = 17;
		Crd[Crd.last].rleft	= right_screen;	Crd[Crd.last].rtop	= bottom_screen; 
		Crd[Crd.last].rright	= right_screen;	Crd[Crd.last].rbottom	= bottom_screen; 

		if (ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last == 1)
			Crd[Crd.last].x = Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].x+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].width-Crd[Crd.last].width+2;
		else
			Crd[Crd.last].x = Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last]-1].x-Crd[Crd.last].width+ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.span;

		Crd[Crd.last].y = Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].y+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].height;

		Crd.last++;
	}

	// Custom button.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BUTTON_CUSTOM = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].x-1; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].y+Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BKG].height+3; 

	// Right arrow on Terrain Palette.
	ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_RT_ARROW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 6; 
	Crd[Crd.last-1].height	= 12; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_TAB.last-1]].x-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_RT_ARROW].width-Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_RT_ARROW].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_TERRAIN_PALETTE_BUTTON_CUSTOM].y-2; 
}

// ====================================================================