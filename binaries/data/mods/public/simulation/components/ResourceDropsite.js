function ResourceDropsite() {}

ResourceDropsite.prototype.Schema =
	"<element name='Types'>" +
		"<list>" +
			"<oneOrMore>" +
				"<choice>" +
					"<value>food</value>" +
					"<value>wood</value>" +
					"<value>stone</value>" +
					"<value>metal</value>" +
				"</choice>" +
			"</oneOrMore>" +
		"</list>" +
	"</element>";


/**
 * Returns the list of resource types accepted by this dropsite.
 */
ResourceDropsite.prototype.GetTypes = function()
{
	return this.template.Types.split(/\s+/);
};

/**
 * Returns whether this dropsite accepts the given generic type of resource.
 */
ResourceDropsite.prototype.AcceptsType = function(type)
{
	return this.GetTypes().indexOf(type) != -1;
};

Engine.RegisterComponentType(IID_ResourceDropsite, "ResourceDropsite", ResourceDropsite);
