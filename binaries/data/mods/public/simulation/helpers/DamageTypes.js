DamageTypes.prototype.BuildSchema = function(helptext = "")
{
	return this.GetTypes().reduce((schema, type) =>
		schema + "<element name='"+type+"' a:help='"+type+" "+helptext+"'><ref name='nonNegativeDecimal'/></element>",
		"");
};

DamageTypes = new DamageTypes();
