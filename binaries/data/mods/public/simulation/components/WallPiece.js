function WallPiece() {}

WallPiece.prototype.Schema =
	"<a:help></a:help>" +
	"<a:example>" +
	"</a:example>" +
	"<element name='Length'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Orientation'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Indent'>" +
			"<data type='decimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Bend'>" +
			"<data type='decimal'/>" +
		"</element>" +
	"</optional>";


WallPiece.prototype.Init = function()
{
};

WallPiece.prototype.Serialize = null;

Engine.RegisterComponentType(IID_WallPiece, "WallPiece", WallPiece);
