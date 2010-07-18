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

// TODO:
//  Engine.RegisterGlobal
//  Engine.PostMessage
//  Engine.BroadcastMessage

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
	return cmp;
};
