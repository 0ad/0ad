function initMapOrb()
{
	// Mini Map.
	crd_map_orb_minimap_width = 176;
	crd_map_orb_minimap_height = crd_map_orb_minimap_width;
	crd_map_orb_minimap_x = -crd_map_orb_minimap_width;
	crd_map_orb_minimap_y = 0;

	// Map Button Seg Left 1.
	crd_map_orb_segleft1_span_x = 2;
	crd_map_orb_segleft1_span_y = -1;
	crd_map_orb_segleft1_width = 32;
	crd_map_orb_segleft1_height = 46;
	crd_map_orb_segleft1_x = -crd_map_orb_minimap_width-crd_map_orb_segleft1_width+crd_map_orb_segleft1_span_x;
	crd_map_orb_segleft1_y = 0;

	// Map Button Seg Left 2.
	crd_map_orb_segleft2_span_y = -1;
	crd_map_orb_segleft2_width = 40;
	crd_map_orb_segleft2_height = 44;
	crd_map_orb_segleft2_x = -crd_map_orb_minimap_width-crd_map_orb_segleft2_width+crd_map_orb_segleft1_span_x;
	crd_map_orb_segleft2_y = crd_map_orb_segleft1_y+crd_map_orb_segleft1_height+crd_map_orb_segleft1_span_y;

	// Map Button Seg Left 3.
	crd_map_orb_segleft3_span_y = -1;
	crd_map_orb_segleft3_width = crd_map_orb_segleft2_width;
	crd_map_orb_segleft3_height = crd_map_orb_segleft2_height;
	crd_map_orb_segleft3_x = -crd_map_orb_minimap_width-crd_map_orb_segleft3_width+crd_map_orb_segleft1_span_x;
	crd_map_orb_segleft3_y = crd_map_orb_segleft2_y+crd_map_orb_segleft2_height+crd_map_orb_segleft2_span_y;

	// Map Button Seg Left 4.
	crd_map_orb_segleft4_width = crd_map_orb_segleft1_width;
	crd_map_orb_segleft4_height = crd_map_orb_segleft1_height;
	crd_map_orb_segleft4_x = -crd_map_orb_minimap_width-crd_map_orb_segleft4_width+crd_map_orb_segleft1_span_x;
	crd_map_orb_segleft4_y = crd_map_orb_segleft3_y+crd_map_orb_segleft3_height+crd_map_orb_segleft3_span_y;

	// Map Button Seg Bottom 1.
	crd_map_orb_segbottom1_span_x = 0;
	crd_map_orb_segbottom1_span_y = -3;
	crd_map_orb_segbottom1_width = 46;
	crd_map_orb_segbottom1_height = 32;
	crd_map_orb_segbottom1_x = -crd_map_orb_minimap_width+crd_map_orb_segbottom1_span_x;
	crd_map_orb_segbottom1_y = crd_map_orb_minimap_y+crd_map_orb_minimap_height+crd_map_orb_segbottom1_span_y;

	// Map Button Seg Bottom 2.
	crd_map_orb_segbottom2_span_x = -1;
	crd_map_orb_segbottom2_span_y = -3;
	crd_map_orb_segbottom2_width = 44;
	crd_map_orb_segbottom2_height = 40;
	crd_map_orb_segbottom2_x = crd_map_orb_segbottom1_x+crd_map_orb_segbottom1_width+crd_map_orb_segbottom2_span_x;
	crd_map_orb_segbottom2_y = crd_map_orb_minimap_y+crd_map_orb_minimap_height+crd_map_orb_segbottom2_span_y;

	// Map Button Seg Bottom 3.
	crd_map_orb_segbottom3_span_x = -1;
	crd_map_orb_segbottom3_span_y = -3;
	crd_map_orb_segbottom3_width = crd_map_orb_segbottom2_width;
	crd_map_orb_segbottom3_height = crd_map_orb_segbottom2_height;
	crd_map_orb_segbottom3_x = crd_map_orb_segbottom2_x+crd_map_orb_segbottom2_width+crd_map_orb_segbottom3_span_x;
	crd_map_orb_segbottom3_y = crd_map_orb_minimap_y+crd_map_orb_minimap_height+crd_map_orb_segbottom3_span_y;

	// Map Button Seg Bottom 4.
	crd_map_orb_segbottom4_span_x = -1;
	crd_map_orb_segbottom4_span_y = -3;
	crd_map_orb_segbottom4_width = crd_map_orb_segbottom1_width;
	crd_map_orb_segbottom4_height = crd_map_orb_segbottom1_height;
	crd_map_orb_segbottom4_x = crd_map_orb_segbottom3_x+crd_map_orb_segbottom3_width+crd_map_orb_segbottom4_span_x;
	crd_map_orb_segbottom4_y = crd_map_orb_minimap_y+crd_map_orb_minimap_height+crd_map_orb_segbottom4_span_y;
}
