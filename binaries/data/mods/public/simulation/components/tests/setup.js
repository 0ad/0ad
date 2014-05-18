var g_NewIID = 1000; // some arbitrary not-yet-used number
var g_NewMTID = 1000; // some arbitrary not-yet-used number
var g_ComponentTypes = {};
var g_Components = {};

// Emulate some engine functions:

Engine.RegisterComponentType = function(iid, name, ctor)
{
	TS_ASSERT(!g_ComponentTypes[name]);
	g_ComponentTypes[name] = { iid: iid, ctor: ctor };
};

Engine.RegisterSystemComponentType = function(iid, name, ctor)
{
	TS_ASSERT(!g_ComponentTypes[name]);
	g_ComponentTypes[name] = { iid: iid, ctor: ctor };
};

Engine.RegisterInterface = function(name)
{
	global["IID_"+name] = g_NewIID++;
};

Engine.RegisterMessageType = function(name)
{
	global["MT_"+name] = g_NewMTID++;
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
	for (var cid in g_Components[ent])
	{
		var cmp = g_Components[ent][cid];
		if (cmp.Deinit)
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
};

global.ConstructComponent = function(ent, name, template)
{
	var cmp = new g_ComponentTypes[name].ctor();
	cmp.entity = ent;
	cmp.template = template;
	cmp.Init();

	if (!g_Components[ent])
		g_Components[ent] = {};
	g_Components[ent][g_ComponentTypes[name].iid] = cmp;

	return cmp;
};
