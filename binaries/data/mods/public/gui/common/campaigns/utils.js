/**
 * Wrap object in a proxy, that calls callback
 * anytime a property is set, passing the property name as parameter.
 * Note that this doesn't modify any variable that pointer towards object,
 * so this is _not_ equivalent to replacing the target object with a proxy.
 */
function _watch(object, callback)
{
	return new Proxy(object, {
		"get": (obj, key) => {
			return obj[key];
		},
		"set": (obj, key, value) => {
			obj[key] = value;
			callback(key);
			return true;
		}
	});
}

/**
 * Inherit from AutoWatcher to make 'this' a proxy object that
 * watches for its own property changes and calls the method given
 * (takes a string because 'this' is unavailable when calling 'super').
 * This can be used to e.g. automatically call a rendering function
 * if a property is changed.
 * Using inheritance is necessary because 'this' is immutable,
 * and isn't defined in the class constructor _unless_ super() is called.
 * (thus you can't do something like this = new Proxy(this) at the end).
 */
class AutoWatcher
{
	constructor(method_name)
	{
		this._ready = false;
		return _watch(this, () => this._ready && this[method_name]());
	}
}
