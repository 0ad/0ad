function initAtlasSectionMapCreator()
{
	// ============================================= MAP CREATOR SECTION MENU ===============================================

	// ============================================= MAP CREATOR: MAP TYPE ===============================================

	// ============================================= MAP CREATOR: MAP TYPE: MAP SIZE ===============================================

	// Height input box for map size.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		58,
		14
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// "X" between map size input boxes.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_X				= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		20,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_X].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y
	);

	// Width input box for map size.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_WIDTH_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_X].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y
	);

	// "Size:" label.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		32,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_WIDTH_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y
	);

	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN = 3;

	// "Huge" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width-ATLAS_LEFT_PANE_SECTION.RMARGIN+2,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// "Large" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y
	);

	// "Medium" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y
	);

	// "Small" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SMALL	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y
	);

	// Horizontal rule at end of Map Size settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_HR		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		2
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	// ============================================= MAP CREATOR: MAP TYPE: RANDOM MAP ===============================================

	// ============================================= MAP CREATOR: MAP SETTINGS ===============================================

	// "Players:" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		45,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Players "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		30,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].y
	);

	// Players up button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y-3
	);

	// Players down button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_DN].height
	);

	// Settlements "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y
	);

	// Settlements up button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y-3
	);

	// Settlements down button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_DN].height
	);

	// "Settlements" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		62,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y
	);

	// "Resources:" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		60,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Resources "drop-down" box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_COMBO_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		55,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].y
	);

	// ============================================= MAP CREATOR: MAP SETTINGS: TERRITORIES ===============================================

	// ============================================= MAP CREATOR: GENERATE ===============================================

	// Terrain Map input box.
	ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN-ATLAS_LEFT_PANE_SECTION.LMARGIN,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+ATLAS_LEFT_PANE_SECTION.LMARGIN+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	// "Terrain Map:" label.
	ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].height
	);

	// Height Map input box.
	ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].height
	);

	// "Height Map:" label.
	ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].height
	);

	// "Generate!" border.
	ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER				= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		40
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].height+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN = 7;

	// "Generate!" button.
	ATLAS_LEFT_PANE_SECTION_GENERATE_BUTTON				= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].height-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].x+ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].y+ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN
	);
}

// ====================================================================

