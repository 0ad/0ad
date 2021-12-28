var g_NewIID = 1000; // some arbitrary not-yet-used number
var g_NewMTID = 1000; // some arbitrary not-yet-used number
var g_ComponentTypes = {};
var g_Components = {};

// Emulate some engine functions:

Engine.RegisterComponentType = function(iid, name, ctor)
{
	TS_ASSERT(!g_ComponentTypes[name]);
	g_ComponentTypes[name] = { "iid": iid, "ctor": ctor };
};

Engine.RegisterSystemComponentType = function(iid, name, ctor)
{
	TS_ASSERT(!g_ComponentTypes[name]);
	g_ComponentTypes[name] = { "iid": iid, "ctor": ctor };
};

Engine.RegisterInterface = function(name)
{
	global["IID_" + name] = g_NewIID++;
};

Engine.RegisterMessageType = function(name)
{
	global["MT_" + name] = g_NewMTID++;
};

Engine.QueryInterface = function(ent, iid)
{
	if (g_Components[ent] && g_Components[ent][iid])
		return g_Components[ent][iid];
	return null;
};

Engine.RegisterGlobal = function(name, value)
{
	global[name] = value;
};

Engine.DestroyEntity = function(ent)
{
	for (let cid in g_Components[ent])
	{
		let cmp = g_Components[ent][cid];
		if (cmp && cmp.Deinit)
			cmp.Deinit();
	}

	delete g_Components[ent];

	// TODO: should send Destroy message
};

Engine.PostMessage = function(ent, iid, message)
{
	// TODO: make this send a message if necessary
};


Engine.BroadcastMessage = function(iid, message)
{
	// TODO: make this send a message if necessary
};

global.ResetState = function()
{
	g_Components = {};
};

global.AddMock = function(ent, iid, mock)
{
	if (!g_Components[ent])
		g_Components[ent] = {};
	g_Components[ent][iid] = mock;
	return g_Components[ent][iid];
};

global.DeleteMock = function(ent, iid)
{
	if (!g_Components[ent])
		g_Components[ent] = {};
	delete g_Components[ent][iid];
};

global.ConstructComponent = function(ent, name, template)
{
	let cmp = new g_ComponentTypes[name].ctor();

	Object.defineProperties(cmp, {
		"entity": {
			"value": ent,
			"configurable": false,
			"enumerable": false,
			"writable": false
		},
		"template": {
			"value": template && deepfreeze(clone(template)),
			"configurable": false,
			"enumerable": false,
			"writable": false
		},
		"_cmpName": {
			"value": name,
			"configurable": false,
			"enumerable": false,
			"writable": false
		}
	});

	cmp.Init();

	if (!g_Components[ent])
		g_Components[ent] = {};
	g_Components[ent][g_ComponentTypes[name].iid] = cmp;

	return cmp;
};

/**
 * A simple Spy proxy that tracks and forward function calls.
 * NB: this immediately replaces obj's func.
 */
global.Spy = function(obj, func)
{
	this._called = 0;
	this._callargs = [];
	let og_func = obj[func];
	let spy = (...args) => {
		++this._called;
		this._callargs.push(args);
		return og_func.apply(obj, args);
	};
	obj[func] = spy;

	this._reset = () => {
		this._called = 0;
		this._callargs = [];
	};

	return this;
};

global.SerializationCycle = function(cmp)
{
	let data;
	if (typeof cmp.Serialize === "function")
		data = cmp.Serialize();
	else
	{
		data = {};
		for (const att of cmp)
			if (cmp.hasOwnProperty(att))
				data[att] = cmp[att];
	}

	const newCmp = ConstructComponent(cmp.entity, cmp._cmpName, cmp.template);
	if (typeof newCmp.Deserialize === "function")
		newCmp.Deserialize(data);
	else
		for (const att in data)
			newCmp[att] = data[att];

	return newCmp;
};
