function initAtlasSectionObjectEditor()
{
	// ============================================= OBJECT EDITOR: SELECTION ==============================================

	// Object category label.
	ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN-ATLAS_LEFT_PANE_SECTION.LMARGIN,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+ATLAS_LEFT_PANE_SECTION.LMARGIN+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].height
	);

	// Object category input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].width,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_LABEL].height
	);

	// Custom button.
	ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_BUTTON_CUSTOM].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].x-4,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Custom button.
	ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ENTITY	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].x+4+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ENTITY].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].y
	);

	// ============================================= OBJECT EDITOR: LIST ==============================================

	// Background of Object List.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].width+2,
		163+2
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_CATEGORY_COMBO_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_BUTTON_CUSTOM_ACTOR].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Player input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x+10,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].y+10
	);

	// Up button for Player input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y-3
	);

	// Down button for Player input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX_DN].height
	);

	// Player label.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y
	);

	// Objects heading.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		65,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_DEPTH_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_PLAYER_INPUT_BOX].y
	);

	// Object list sort input box.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		90,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].width-10,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_HEADING].height+10
	);

	// Object list sort label.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		35,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].y-3
	);

	// Object List.
	ATLAS_LEFT_PANE_SECTION_OBJECT_LIST		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_BKG].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_OBJECT_LIST_SORT_COMBO_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);


	// ============================================= OBJECT EDITOR: ANIMATION VIEWER ===============================================

	// Animation Viewer combobox label.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		60,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		ATLAS_LEFT_PANE_SECTION.LMARGIN,
		2
	);

	// Animation Viewer combobox.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		65,
		10
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].x+atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].width,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].y
	);

	// Animation Viewer.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		125,
		125
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].x,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].y+atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_COMBO_BOX_LABEL].height
	);

	// Animation Viewer label.
	ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER_LABEL	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].width,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].x,
		atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].y+atlasCoord[ATLAS_BOTTOM_PANE_SECTION_OBJECT_ANIM_VIEWER].height
	);
}

// ====================================================================