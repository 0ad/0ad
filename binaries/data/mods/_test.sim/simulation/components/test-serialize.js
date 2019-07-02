function TestScript1_values() {}

TestScript1_values.prototype.Init = function() {
	this.x = +this.template.x;
	this.str = "this is a string";
	this.things = { a: 1, b: "2", c: [3, "4", [5, []]] };
};

TestScript1_values.prototype.GetX = function() {
//	print(uneval(this));
	return this.x;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_values", TestScript1_values);

// -------- //

function TestScript1_entity() {}

TestScript1_entity.prototype.GetX = function() {
	// Test that .entity is readonly
	try {
		delete this.entity;
		Engine.TS_FAIL("Missed exception");
	} catch (e) { }
	try {
		this.entity = -1;
		Engine.TS_FAIL("Missed exception");
	} catch (e) { }

	// and return the value
	return this.entity;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_entity", TestScript1_entity);

// -------- //

function TestScript1_nontree() {}

TestScript1_nontree.prototype.Init = function() {
	var n = [1];
	this.x = [n, n, null, { y: n }];
	this.x[2] = this.x;
};

TestScript1_nontree.prototype.GetX = function() {
//	print(uneval(this)+"\n");
	this.x[0][0] += 1;
	return this.x[0][0] + this.x[1][0] + this.x[2][0][0] + this.x[3].y[0];
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_nontree", TestScript1_nontree);

// -------- //

function TestScript1_custom() {}

TestScript1_custom.prototype.Init = function() {
	this.y = 2;
};

TestScript1_custom.prototype.Serialize = function() {
	return {c:1};
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_custom", TestScript1_custom);

// -------- //

function TestScript1_getter() {}

TestScript1_getter.prototype.Init = function() {
	this.x = 100;
	this.__defineGetter__('x', function () { print("FAIL\n"); die(); return 200; });
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_getter", TestScript1_getter);

// -------- //

function TestScript1_consts() {}

TestScript1_consts.prototype.Schema = "<ref name='anything'/>";

TestScript1_consts.prototype.GetX = function() {
	return (+this.entity) + (+this.template.x);
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_consts", TestScript1_consts);
