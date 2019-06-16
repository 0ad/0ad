DamageTypes.prototype.BuildSchema = function(helptext = "")
{
	return "<interleave>" + this.GetTypes().reduce((schema, type) =>
		schema + "<element name='"+type+"' a:help='"+type+" "+helptext+"'><ref name='nonNegativeDecimal'/></element>",
	"") + "</interleave>";
};

DamageTypes = new DamageTypes();
