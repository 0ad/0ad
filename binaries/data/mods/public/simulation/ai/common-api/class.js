var API3 = function(m)
{
/**
 * Provides a nicer syntax for defining classes,
 * with support for OO-style inheritance.
 */
m.Class = function(data)
{
	let ctor;
	if (data._init)
		ctor = data._init;
	else
		ctor = function() { };

	if (data._super)
		ctor.prototype = { "__proto__": data._super.prototype };

	for (let key in data)
		ctor.prototype[key] = data[key];

	return ctor;
};

return m;

}(API3);
