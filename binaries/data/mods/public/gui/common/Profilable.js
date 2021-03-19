/**
 * This is an auto-proxying class that adds profiling to all method.
 * Can be quite useful to track where time is spent in the GUI.
 * Usage: Just add "extens Profilable" to your class
 * and call super() in the constructor as appropriate.
 * Give your class a name if you want the ouput to be usable.
 *
 * It is recommended to only use this class when actually working on profiling,
 * or the profiler2 graph will be very cluttered.
 */
var ProfilableMixin = (Parent) => class Profilable extends (() => Parent || Object)()
{
	constructor()
	{
		super();
		return new Proxy(this, {
			"get": (target, prop, receiver) => {
				let ret = Reflect.get(target, prop);
				if (typeof ret !== 'function')
					return ret;
				{
					ret = ret.bind(receiver);
					return (...a) => {
						let ret2;
						Engine.ProfileStart(target.constructor.name + ":" + prop);
						ret2 = ret(...a);
						Engine.ProfileStop();
						return ret2;
					};
				}
			}
		});
	}
};
var Profilable = ProfilableMixin();
