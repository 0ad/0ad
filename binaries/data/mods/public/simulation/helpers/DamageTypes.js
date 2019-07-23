/**
 * Builds a RelaxRNG schema based on currently valid elements.
 *
 * To prevent validation errors, disabled damage types are included in the schema.
 *
 * @param {string} helptext	- Text displayed as help
 * @return {string}	- RelaxNG schema string
 */
function BuildDamageTypesSchema(helptext = "")
{
	return "<oneOrMore>" +
		"<element a:help='" + helptext + "'>" +
			"<anyName>" +
				// Armour requires Foundation to not be a damage type.
				"<except><name>Foundation</name></except>" +
			"</anyName>" +
			"<ref name='nonNegativeDecimal' />" +
		"</element>" +
	"</oneOrMore>";
}

Engine.RegisterGlobal("BuildDamageTypesSchema", BuildDamageTypesSchema);
