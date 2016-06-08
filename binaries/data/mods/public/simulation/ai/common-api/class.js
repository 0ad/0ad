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

/* Test inheritance:
var A = Class({foo:1, bar:10});
print((new A).foo+" "+(new A).bar+"\n");
var B = Class({foo:2, bar:20});
print((new A).foo+" "+(new A).bar+"\n");
print((new B).foo+" "+(new B).bar+"\n");
var C = Class({_super:A, foo:3});
print((new A).foo+" "+(new A).bar+"\n");
print((new B).foo+" "+(new B).bar+"\n");
print((new C).foo+" "+(new C).bar+"\n");
//*/

return m;

}(API3);
