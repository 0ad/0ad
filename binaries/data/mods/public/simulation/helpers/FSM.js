// Hierarchical finite state machine implementation.
//
// FSMs are specified as a JS data structure;
// see e.g. UnitAI.js for an example of the syntax.
//
// FSMs are implicitly linked with an external object.
// That object stores all FSM-related state.
// (This means we can serialise FSM-based components as
// plain old JS objects, with no need to serialise the complex
// FSM structure itself or to add custom serialisation code.)

/**

FSM API:

Users define the FSM behaviour like:

var FsmSpec = {

	// Define some default message handlers:

	"MessageName1": function(msg) {
		// This function will be called in response to calls to
		//   Fsm.ProcessMessage(this, { "type": "MessageName1", "data": msg });
		//
		// In this function, 'this' is the component object passed into
		// ProcessMessage, so you can access 'this.propertyName'
		// and 'this.methodName()' etc.
	},

	"MessageName2": function(msg) {
		// Another message handler.
	},

	// Define the behaviour for the 'STATENAME' state:
	// Names of states may only contain the characters A-Z
	"STATENAME": {

		"MessageName1": function(msg) {
			// This overrides the previous MessageName1 that was
			// defined earlier, and will be called instead of it
			// in response to ProcessMessage.
		},

		// We don't override MessageName2, so the default one
		// will be called instead.

		// Define the 'STATENAME.SUBSTATENAME' state:
		// (we support arbitrarily-nested hierarchies of states)
		"SUBSTATENAME": {

			"MessageName2": function(msg) {
				// Override the default MessageName2.
				// But we don't override MessageName1, so the one from
				// STATENAME will be used instead.
			},

			"enter": function() {
				// This is a special function called when transitioning
				// into this state, or into a substate of this state.
				//
				// If it returns true, the transition will be aborted:
				// do this if you've called SetNextState inside this enter
				// handler, because otherwise the new state transition
				// will get mixed up with the previous ongoing one.
				// In normal cases, you can return false or nothing.
			},

			"leave": function() {
				// Called when transitioning out of this state.
			},
		},

		// Define a new state which is an exact copy of another
		// state that is defined elsewhere in this FSM:
		"OTHERSUBSTATENAME": "STATENAME.SUBSTATENAME",
	}

}


Objects can then make themselves act as an instance of the FSM by running
	FsmSpec.Init(this, "STATENAME");
which will define a few properties on 'this' (with names prefixed "fsm"),
and then they can call the FSM functions on the object like
	FsmSpec.SetNextState(this, "STATENAME.SUBSTATENAME");

These objects must also define a function property that can be called as
	this.FsmStateNameChanged(name);

(This design aims to avoid storing any per-instance state that cannot be
easily serialized - it only stores state-name strings.)

 */

function FSM(spec)
{
	// The (relatively) human-readable FSM specification needs to get
	// compiled into a more-efficient-to-execute version.
	//
	// In particular, message handling should require minimal
	// property lookups in the common case (even when the FSM has
	// a deeply nested hierarchy), and there should never be any
	// string manipulation at run-time.

	this.decompose = { "": [] };
	/* 'decompose' will store:
		{
			"": [],
			"A": ["A"],
			"A.B": ["A", "A.B"],
			"A.B.C": ["A", "A.B", "A.B.C"],
			"A.B.D": ["A", "A.B", "A.B.D"],
			...
		};
		This is used when switching between states in different branches
		of the hierarchy, to determine the list of sub-states to leave/enter
	*/

	this.states = { };
	/* 'states' will store:
		{
			...
			"A": {
				"_name": "A",
				"_parent": "",
				"_refs": { // local -> global name lookups (for SetNextState)
					"B": "A.B",
					"B.C": "A.B.C",
					"B.D": "A.B.D",
				},
			},
			"A.B": {
				"_name": "A.B",
				"_parent": "A",
				"_refs": {
					"C": "A.B.C",
					"D": "A.B.D",
				},
				"MessageType": function(msg) { ... },
			},
			"A.B.C": {
				"_name": "A.B.C",
				"_parent": "A.B",
				"_refs": {},
				"enter": function() { ... },
				"MessageType": function(msg) { ... },
			},
			"A.B.D": {
				"_name": "A.B.D",
				"_parent": "A.B",
				"_refs": {},
				"enter": function() { ... },
				"leave": function() { ... },
				"MessageType": function(msg) { ... },
			},
			...
		}
	*/

	function process(fsm, node, path, handlers)
	{
		// Handle string references to nodes defined elsewhere in the FSM spec
		if (typeof node === "string")
		{
			var refpath = node.split(".");
			var refd = spec;
			for (var p of refpath)
			{
				refd = refd[p];
				if (!refd)
				{
					error("FSM node "+path.join(".")+" referred to non-defined node "+node);
					return {};
				}
			}
			node = refd;
		}

		var state = {};
		fsm.states[path.join(".")] = state;

		var newhandlers = {};
		for (var e in handlers)
			newhandlers[e] = handlers[e];

		state._name = path.join(".");
		state._parent = path.slice(0, -1).join(".");
		state._refs = {};

		for (var key in node)
		{
			if (key === "enter" || key === "leave")
			{
				state[key] = node[key];
			}
			else if (key.match(/^[A-Z]+$/))
			{
				state._refs[key] = (state._name ? state._name + "." : "") + key;

				// (the rest of this will be handled later once we've grabbed
				// all the event handlers)
			}
			else
			{
				newhandlers[key] = node[key];
			}
		}

		for (var e in newhandlers)
			state[e] = newhandlers[e];

		for (var key in node)
		{
			if (key.match(/^[A-Z]+$/))
			{
				var newpath = path.concat([key]);

				var decomposed = [newpath[0]];
				for (var i = 1; i < newpath.length; ++i)
					decomposed.push(decomposed[i-1] + "." + newpath[i]);
				fsm.decompose[newpath.join(".")] = decomposed;

				var childstate = process(fsm, node[key], newpath, newhandlers);

				for (var r in childstate._refs)
				{
					var cname = key + "." + r;
					state._refs[cname] = childstate._refs[r];
				}
			}
		}

		return state;
	}

	process(this, spec, [], {});
}

FSM.prototype.Init = function(obj, initialState)
{
	this.deferFromState = undefined;

	obj.fsmStateName = "";
	obj.fsmNextState = undefined;
	this.SwitchToNextState(obj, initialState);
};

FSM.prototype.SetNextState = function(obj, state)
{
	obj.fsmNextState = state;
	obj.fsmReenter = false;
};


FSM.prototype.SetNextStateAlwaysEntering = function(obj, state)
{
	obj.fsmNextState = state;
	// If reenter is true then the state will always be entered even if this means exiting it to re-enter
	obj.fsmReenter = true;
};

FSM.prototype.ProcessMessage = function(obj, msg)
{
//	warn("ProcessMessage(obj, "+uneval(msg)+")");

	var func = this.states[obj.fsmStateName][msg.type];
	if (!func)
	{
		error("Tried to process unhandled event '" + msg.type + "' in state '" + obj.fsmStateName + "'");
		return undefined;
	}

	var ret = func.apply(obj, [msg]);

	// If func called SetNextState then switch into the new state,
	// and continue switching if the new state's 'enter' called SetNextState again
	while (obj.fsmNextState)
	{
		var nextStateName = this.LookupState(obj.fsmStateName, obj.fsmNextState);
		obj.fsmNextState = undefined;

		if (nextStateName != obj.fsmStateName || obj.fsmReenter)
			this.SwitchToNextState(obj, nextStateName);
	}

	return ret;
};

FSM.prototype.DeferMessage = function(obj, msg)
{
	// We need to work out which sub-state we were running the message handler from,
	// and then try again in its parent state.
	var old = this.deferFromState;
	var from;
	if (old) // if we're recursively deferring and saved the last used state, use that
		from = old;
	else // if this is the first defer then we must have last processed the message in the current FSM state
		from = obj.fsmStateName;

	// Find and save the parent, for use in recursive defers
	this.deferFromState = this.states[from]._parent;

	// Run the function from the parent state
	var state = this.states[this.deferFromState];
	var func = state[msg.type];
	if (!func)
		error("Failed to defer event '" + msg.type + "' from state '" + obj.fsmStateName + "'");
	func.apply(obj, [msg]);

	// Restore the changes we made
	this.deferFromState = old;

	// TODO: if an inherited handler defers, it calls exactly the same handler
	// on the parent state, which is probably useless and inefficient

	// NOTE: this will break if two units try to execute AI at the same time;
	// as long as AI messages are queue and processed asynchronously it should be fine
};

FSM.prototype.LookupState = function(currentStateName, stateName)
{
//	print("LookupState("+currentStateName+", "+stateName+")\n");
	for (var s = currentStateName; s; s = this.states[s]._parent)
		if (stateName in this.states[s]._refs)
			return this.states[s]._refs[stateName];
	return stateName;
};

FSM.prototype.GetCurrentState = function(obj)
{
	return obj.fsmStateName;
};

FSM.prototype.SwitchToNextState = function(obj, nextStateName)
{
	var fromState = this.decompose[obj.fsmStateName];
	var toState = this.decompose[nextStateName];

	if (!toState)
		error("Tried to change to non-existent state '" + nextStateName + "'");

	// Find the set of states in the hierarchy tree to leave then enter,
	// to traverse from the old state to the new one.
	// If any enter/leave function returns true then abort the process
	// (this lets them intercept the transition and start a new transition)

	for (var equalPrefix = 0; fromState[equalPrefix] && fromState[equalPrefix] === toState[equalPrefix]; ++equalPrefix)
	{
	}

	// Check if we should exit and enter the current state due to the reenter parameter. If so we go up 1 level
	if (obj.fsmReenter && equalPrefix > 0 && equalPrefix === toState.length)
		--equalPrefix;

	for (var i = fromState.length-1; i >= equalPrefix; --i)
	{
		var leave = this.states[fromState[i]].leave;
		if (leave)
		{
			obj.fsmStateName = fromState[i];
			if (leave.apply(obj))
			{
				obj.FsmStateNameChanged(obj.fsmStateName);
				return;
			}
		}
	}

	for (var i = equalPrefix; i < toState.length; ++i)
	{
		var enter = this.states[toState[i]].enter;
		if (enter)
		{
			obj.fsmStateName = toState[i];
			if (enter.apply(obj))
			{
				obj.FsmStateNameChanged(obj.fsmStateName);
				return;
			}
		}
	}

	obj.fsmStateName = nextStateName;
	obj.FsmStateNameChanged(obj.fsmStateName);
};

Engine.RegisterGlobal("FSM", FSM);
