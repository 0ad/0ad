function initMapOrb()
{
	SN_MINIMAP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 180; 
	Crd[Crd.last-1].height	= Crd[Crd.last-1].width; 
	Crd[Crd.last-1].x	= 0; 
	Crd[Crd.last-1].y	= 0; 

	SN_MAP_ORB_SEGLEFT1 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 32; 
	Crd[Crd.last-1].height	= 46; 
	Crd[Crd.last-1].x	= Crd[SN_MINIMAP].width;
	Crd[Crd.last-1].y	= Crd[SN_MINIMAP].height-Crd[Crd.last-1].height; 

	SN_MAP_ORB_SEGLEFT1_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MINIMAP].width;
	Crd[Crd.last-1].y	= 0; 

	SN_MAP_ORB_SEGLEFT2 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 40; 
	Crd[Crd.last-1].height	= 44; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGLEFT1].x; 
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGLEFT1].y-Crd[Crd.last-1].height;

	SN_MAP_ORB_SEGLEFT2_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGLEFT1].x; 
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGLEFT2].y-Crd[Crd.last-1].height;

	SN_MAP_ORB_SEGLEFT3 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_MAP_ORB_SEGLEFT2].width; 
	Crd[Crd.last-1].height	= Crd[SN_MAP_ORB_SEGLEFT2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGLEFT2].x;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGLEFT2].y-Crd[Crd.last-1].height;

	SN_MAP_ORB_SEGLEFT3_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGLEFT2].x;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGLEFT1].y-Crd[Crd.last-1].height;

	SN_MAP_ORB_SEGLEFT4 = new Object();
	SN_MAP_ORB_SEGLEFT4 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_MAP_ORB_SEGLEFT1].width; 
	Crd[Crd.last-1].height	= Crd[SN_MAP_ORB_SEGLEFT1].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGLEFT2].x;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGLEFT3].y-Crd[Crd.last-1].height;

	SN_MAP_ORB_SEGLEFT4_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGLEFT2].x;
	Crd[Crd.last-1].y	= Crd[SN_MINIMAP].height-Crd[Crd.last-1].height;

	SN_MAP_ORB_SEGBOTTOM1 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 46; 
	Crd[Crd.last-1].height	= 32; 
	Crd[Crd.last-1].x	= Crd[SN_MINIMAP].width-Crd[Crd.last-1].width;
	Crd[Crd.last-1].y	= Crd[SN_MINIMAP].height; 

	SN_MAP_ORB_SEGBOTTOM1_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MINIMAP].width-Crd[Crd.last-1].width;
	Crd[Crd.last-1].y	= Crd[SN_MINIMAP].height; 

	SN_MAP_ORB_SEGBOTTOM2 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 44; 
	Crd[Crd.last-1].height	= 40; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGBOTTOM1].x-Crd[Crd.last-1].width; 
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGBOTTOM1].y;

	SN_MAP_ORB_SEGBOTTOM2_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGBOTTOM1].x-Crd[Crd.last-1].width; 
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGBOTTOM1].y;

	SN_MAP_ORB_SEGBOTTOM3 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_MAP_ORB_SEGBOTTOM2].width; 
	Crd[Crd.last-1].height	= Crd[SN_MAP_ORB_SEGBOTTOM2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGBOTTOM2].x-Crd[Crd.last-1].width;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGBOTTOM2].y;

	SN_MAP_ORB_SEGBOTTOM3_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGBOTTOM2].x-Crd[Crd.last-1].width;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGBOTTOM2].y;

	SN_MAP_ORB_SEGBOTTOM4 = new Object();
	SN_MAP_ORB_SEGBOTTOM4 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_MAP_ORB_SEGBOTTOM1].width; 
	Crd[Crd.last-1].height	= Crd[SN_MAP_ORB_SEGBOTTOM1].height; 
	Crd[Crd.last-1].x	= Crd[SN_MAP_ORB_SEGBOTTOM3].x-Crd[Crd.last-1].width;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGBOTTOM2].y;

	SN_MAP_ORB_SEGBOTTOM4_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= 0;
	Crd[Crd.last-1].y	= Crd[SN_MAP_ORB_SEGBOTTOM2].y;
}
