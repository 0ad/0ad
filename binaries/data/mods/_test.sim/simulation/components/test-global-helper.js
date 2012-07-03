function TestScript1_GlobalHelper() {}

TestScript1_GlobalHelper.prototype.GetX = function()
{
	return GlobalSubtractionHelper(1, -1);
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_GlobalHelper", TestScript1_GlobalHelper);
