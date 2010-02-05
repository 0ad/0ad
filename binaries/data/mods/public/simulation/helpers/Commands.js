function ProcessCommand(player, cmd)
{
//	print("command: " + player + " " + uneval(cmd) + "\n");
	
	switch (cmd.type)
	{
	case "walk":
		for each (var ent in cmd.entities)
		{
			var ai = Engine.QueryInterface(ent, IID_UnitAI);
			if (!ai)
				continue;
			ai.Walk(cmd.x, cmd.z);
		}
		break;

	case "attack":
		for each (var ent in cmd.entities)
		{
			var ai = Engine.QueryInterface(ent, IID_UnitAI);
			if (!ai)
				continue;
			ai.Attack(cmd.target);
		}
		break;

	case "construct":
		// TODO: this should do all sorts of stuff with foundations and resource costs etc
		var ent = Engine.AddEntity(cmd.template);
		if (ent)
		{
			var pos = Engine.QueryInterface(ent, IID_Position);
			if (pos)
			{
				pos.JumpTo(cmd.x, cmd.z);
				pos.SetYRotation(cmd.angle);
			}
		}
		break;

	default:
		print("Ignoring unrecognised command type '" + cmd.type + "'\n");
	}
}

Engine.RegisterGlobal("ProcessCommand", ProcessCommand);
