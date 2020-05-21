function WallPiece() {}

WallPiece.prototype.Schema =
	"<a:help></a:help>" +
	"<a:example>" +
	"</a:example>" +
	"<element name='Length' a:help='Meters. Used in rmgen wallbuilder and the in-game wall-placer.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Orientation' a:help='Multiples of Pi, measured anti-clockwise. Default: 1; full revolution: 2. Used in rmgen wallbuilder. How the wallpiece should be rotated so it is orientated the same way as every other wallpiece: with the \"line\" of the wall running along a map&apos;s `z` axis and the \"outside face\" towards positive `x`. If the piece bends (see below), the orientation should be that of the start of the wallpiece, not the middle.'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Indent' a:help='Meters. Default: 0. Used in rmgen wallbuilder. Permits piece to be placed in front (-ve value) or behind (+ve value) a wall.'>" +
			"<data type='decimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Bend' a:help='Multiples of Pi, measured anti-clockwise. Default: 0. Used in rmgen wallbuilder. The difference in orientation between the ends of a wallpiece.'>" +
			"<data type='decimal'/>" +
		"</element>" +
	"</optional>";


WallPiece.prototype.Init = function()
{
};

WallPiece.prototype.Serialize = null;

Engine.RegisterComponentType(IID_WallPiece, "WallPiece", WallPiece);
