function initAtlasSectionMapCreator()
{
	// ============================================= MAP CREATOR SECTION MENU ===============================================

	// ============================================= MAP CREATOR: MAP TYPE ===============================================

	// ============================================= MAP CREATOR: MAP TYPE: MAP SIZE ===============================================

	// Height input box for map size.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 58; 
	Crd[Crd.last-1].height	= 14; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION.RMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// "X" between map size input boxes.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_X = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 20; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_X].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y; 

	// Width input box for map size.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_WIDTH_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_X].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y; 

	// "Size:" label.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 32; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_WIDTH_INPUT_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y; 

	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN = 3;

	// "Huge" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= 20; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width-ATLAS_LEFT_PANE_SECTION.RMARGIN+2; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// "Large" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y; 

	// "Medium" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y; 

	// "Small" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SMALL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y; 

	// Horizontal rule at end of Map Size settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_HR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_BKG].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN; 
	Crd[Crd.last-1].height	= 2; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_BKG].x+ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height+ATLAS_LEFT_PANE_SECTION.BMARGIN; 

	// ============================================= MAP CREATOR: MAP TYPE: RANDOM MAP ===============================================

	// ============================================= MAP CREATOR: MAP SETTINGS ===============================================

	// "Players:" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= 45; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].y+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Players "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= 30; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].x+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].y; 

	// Players up button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= ATLAS_COUNTER_BOX.width; 
	Crd[Crd.last-1].height	= (Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height/2)+3; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y-3; 

	// Players down button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= ATLAS_COUNTER_BOX.width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_DN].height; 

	// Settlements "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION.RMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y; 

	// Settlements up button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= ATLAS_COUNTER_BOX.width; 
	Crd[Crd.last-1].height	= (Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].height/2)+3; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].width;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y-3; 

	// Settlements down button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= ATLAS_COUNTER_BOX.width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].width;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].height-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_DN].height; 

	// "Settlements" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= 62; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_LABEL].width;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y; 

	// "Resources:" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= 60; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Resources "drop-down" box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_COMBO_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= 55; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].x+Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].width;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].y; 

	// ============================================= MAP CREATOR: MAP SETTINGS: TERRITORIES ===============================================

	// ============================================= MAP CREATOR: GENERATE ===============================================

	// Terrain Map input box.
	ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN-ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+ATLAS_LEFT_PANE_SECTION.LMARGIN+ATLAS_LEFT_PANE_SECTION.LMARGIN;
	Crd[Crd.last-1].y	= ATLAS_LEFT_PANE_SECTION.BMARGIN; 

	// "Terrain Map:" label.
	ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= 20; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].height; 

	// Height Map input box.
	ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].height; 

	// "Height Map:" label.
	ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].height; 

	// "Generate!" border.
	ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= 40; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].height+Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].height+ATLAS_LEFT_PANE_SECTION.BMARGIN; 

	ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN = 7;

	// "Generate!" button.
	ATLAS_LEFT_PANE_SECTION_GENERATE_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].height-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].x+ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].y+ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN; 
}

// ====================================================================

