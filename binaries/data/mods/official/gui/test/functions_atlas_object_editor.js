function initAtlasSectionObjectEditor()
{
	// ============================================= OBJECT EDITOR: SELECTION ==============================================

	// Object category label.
	ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN-ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].height	= 20; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+ATLAS_LEFT_PANE_SECTION.LMARGIN+ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].height; 

	// Object category input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].width; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].y+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].height; 

	// Custom button.
	ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].x-4; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Custom button.
	ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ENTITY = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].x+4+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ENTITY].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].y; 

	// ============================================= OBJECT EDITOR: LIST ==============================================

	// Background of Object List.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].width+2; 
	Crd[Crd.last-1].height	= 163+2; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].y+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// Player input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x+10; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].y+10; 

	// Up button for Player input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= ATLAS_COUNTER_BOX.width; 
	Crd[Crd.last-1].height	= (Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].height/2)+3; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y-3; 

	// Down button for Player input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_DN = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= ATLAS_COUNTER_BOX.width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].width-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].height-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_DN].height; 

	// Player label.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].x+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y; 

	// Objects heading.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 65; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].width-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y; 

	// Object list sort input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 90; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].width-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].width-10; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING].y+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING].height+10; 

	// Object list sort label.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 35; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].x-Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].y-3; 

	// Object List.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].width; 
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].height; 
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].y+Crd[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN; 

	// ============================================= OBJECT EDITOR: ANIMATION VIEWER ===============================================

	// Animation Viewer combobox label.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 60; 
	Crd[Crd.last-1].height	= 20; 
	Crd[Crd.last-1].x	= ATLAS_LEFT_PANE_SECTION.LMARGIN; 
	Crd[Crd.last-1].y	= 2; 

	// Animation Viewer combobox.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 65; 
	Crd[Crd.last-1].height	= 10; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].x+Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].width; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].y; 

	// Animation Viewer.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 125; 
	Crd[Crd.last-1].height	= 125; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].y+Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].height; 

	// Animation Viewer label.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER_LABEL = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].width; 
	Crd[Crd.last-1].height	= 15; 
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].x; 
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].y+Crd[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].height; 
}

// ====================================================================