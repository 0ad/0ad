function WallSet() {}

WallSet.prototype.Schema =
	"<a:help></a:help>" +
	"<a:example>" +
	"</a:example>" +
	"<element name='Templates'>" +
		"<interleave>" +
			"<element name='Tower' a:help='Template name of the tower piece'>" +
				"<text/>" +
			"</element>" +
			"<element name='Gate' a:help='Template name of the gate piece'>" +
				"<text/>" +
			"</element>" +
			"<element name='WallLong' a:help='Template name of the long wall segment'>" +
				"<text/>" +
			"</element>" +
			"<element name='WallMedium' a:help='Template name of the medium-size wall segment'>" +
				"<text/>" +
			"</element>" +
			"<element name='WallShort' a:help='Template name of the short wall segment'>" +
				"<text/>" +
			"</element>" +
			"<optional>" +
				"<element name='WallCurves' a:help='Space-separated list of template names of curving wall segments.'>" +
					"<text/>" +
				"</element>" +
			"</optional>" +
			"<optional>" +
				"<element name='WallEnd'>" +
					"<text/>" +
				"</element>" +
			"</optional>" +
			"<optional>" +
				"<element name='Fort'>" +
					"<text/>" +
				"</element>" +
			"</optional>" +
		"</interleave>" +
	"</element>" +
	"<element name='MinTowerOverlap' a:help='Maximum fraction that wall segments are allowed to overlap towers, where 0 signifies no overlap and 1 full overlap'>" +
		"<data type='decimal'><param name='minInclusive'>0.0</param><param name='maxInclusive'>1.0</param></data>" +
	"</element>" +
	"<element name='MaxTowerOverlap' a:help='Minimum fraction that wall segments are required to overlap towers, where 0 signifies no overlap and 1 full overlap'>" +
		"<data type='decimal'><param name='minInclusive'>0.0</param><param name='maxInclusive'>1.0</param></data>" +
	"</element>";


WallSet.prototype.Init = function()
{
};

WallSet.prototype.Serialize = null;

Engine.RegisterComponentType(IID_WallSet, "WallSet", WallSet);
