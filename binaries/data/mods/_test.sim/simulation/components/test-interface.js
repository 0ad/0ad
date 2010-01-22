function TestScript1_Interface() {}

TestScript1_Interface.prototype.GetX = function()
{
	return IID_TestScriptIfc + Engine.QueryInterface(this.entity, IID_TestScriptIfc).Method();
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_Interface", TestScript1_Interface);


function TestScript2_Interface() {}

TestScript2_Interface.prototype.Method = function()
{
	return 1000;
};

Engine.RegisterComponentType(IID_TestScriptIfc, "TestScript2_Interface", TestScript2_Interface);
