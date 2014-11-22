function ResourceDropsite() {}

ResourceDropsite.prototype.Schema =
	"<element name='Types'>" +
		"<list>" +
			"<zeroOrMore>" +
				"<choice>" +
					"<value>food</value>" +
					"<value>wood</value>" +
					"<value>stone</value>" +
					"<value>metal</value>" +
				"</choice>" +
			"</zeroOrMore>" +
		"</list>" +
	"</element>";


/**
 * Returns the list of resource types accepted by this dropsite.
 */
ResourceDropsite.prototype.GetTypes = function()
{
	let types = ApplyValueModificationsToEntity("ResourceDropsite/Types", this.template.Types, this.entity);
	return types ? types.split(/\s+/) : [];
};

/**
 * Returns whether this dropsite accepts the given generic type of resource.
 */
ResourceDropsite.prototype.AcceptsType = function(type)
{
	return this.GetTypes().indexOf(type) != -1;
};

Engine.RegisterComponentType(IID_ResourceDropsite, "ResourceDropsite", ResourceDropsite);
