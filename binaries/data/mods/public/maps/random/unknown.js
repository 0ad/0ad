Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmbiome");
Engine.LoadLibrary("the_unknown");

g_PlayerBases = true;
g_AllowNaval = true;

createUnknownMap();
createUnknownObjects();

ExportMap();
