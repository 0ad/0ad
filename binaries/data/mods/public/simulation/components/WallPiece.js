function WallPiece() {}

WallPiece.prototype.Schema =
	"<a:help></a:help>" +
	"<a:example>" +
	"</a:example>" +
	"<element name='Length' a:help='Measured in Terrain Tiles. Used in rmgen wallbuilder and the in-game wall-placer.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Orientation' a:help='Radians. Default: 1. Used in rmgen wallbuilder. The angle to rotate the wallpiece so it is orientated the same way as every other wallpiece: with the \"line\" of the wall running along a map&apos;s `z` axis and the \"outside face\" towards positive `x`. If the piece bends (see below), the orientation should be that of the start of the wallpiece, not the middle.'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Indent' a:help='Measured in Terrain Tiles. Default: 0. Used in rmgen wallbuilder. Permits piece to be placed in front, behind, or inline with a wall.'>" +
			"<data type='decimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Bend' a:help='Radians. Default: 0. Used in rmgen wallbuilder. Defines the angle that the wallpiece bends or curves across its length.'>" +
			"<data type='decimal'/>" +
		"</element>" +
	"</optional>";


WallPiece.prototype.Init = function()
{
};

WallPiece.prototype.Serialize = null;

Engine.RegisterComponentType(IID_WallPiece, "WallPiece", WallPiece);
