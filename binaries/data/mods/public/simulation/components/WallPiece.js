function WallPiece() {}

WallPiece.prototype.Schema =
	"<a:help></a:help>" +
	"<a:example>" +
	"</a:example>" +
	"<element name='Length'>" +
		"<ref name='nonNegativeDecimal'/>" +
    "</element>";


WallPiece.prototype.Init = function()
{
};

WallPiece.prototype.Serialize = null;

Engine.RegisterComponentType(IID_WallPiece, "WallPiece", WallPiece);
