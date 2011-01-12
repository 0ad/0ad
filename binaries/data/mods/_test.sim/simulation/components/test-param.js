function TestScript1_Init() {}

TestScript1_Init.prototype.Init = function() {
	var param = this.template;
//	print("# ",uneval(param),"\n");
	if (param)
		this.x = (+param.x) + (+param.y._string) + (+param.y.z['@w']) + (+param.y.z.a);
	else
		this.x = 100;
};

TestScript1_Init.prototype.GetX = function() {
	return this.x;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_Init", TestScript1_Init);

// -------- //

function TestScript1_readonly() {}

TestScript1_readonly.prototype.GetX = function() {
	try { this.template = null; } catch(e) { }
	try { delete this.template; } catch(e) { }
	try { this.template.x += 1000; } catch(e) { }
	try { delete this.template.x; } catch(e) { }
	try { this.template.y = 2000; } catch(e) { }
	return +(this.template.x || 1) + +(this.template.y || 2);
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_readonly", TestScript1_readonly);
