import xml.etree.ElementTree as ET
import os

# This script creates the RM demo map "Balance Test" with units defined as such:

Civs = ["rome","pers"]

if len (Civs) != 2:
	sys.exit("You should only input two civilizations to compare")

CivBuildings = ["civil_centre", "barracks","gymnasion", "stables", "elephant_stables", "fortress", "embassy_celtic", "embassy_italiote", "embassy_iberian"]
#CivBuildings = ["civil_centre", "barracks"]

# Remote Civ templates with those strings in their name.
FilterOut = ["hero", "mecha", "support"]


############################################################
# Load Civ specific templates
# Will be loaded by loading buildings, and templates will be set the Civ building, the Civ and in general.
# Allows to avoid special units.
CivData = {};

os.chdir(os.path.realpath(__file__).replace("CreateRMTest.py","") + "../../../binaries/data/mods/public/simulation/templates/")

for Civ in Civs:
	CivData[Civ] = {}
	for building in CivBuildings:
		bdTemplate = "./structures/" + Civ + "_" + building + ".xml"
		TrainableUnits = []
		if (os.path.isfile(bdTemplate)):
			Template = ET.parse(bdTemplate)
			if (Template.find("./ProductionQueue/Entities") != None):
				TrainableUnits = Template.find("./ProductionQueue/Entities").text.replace(" ","").replace("\t","").split("\n")
		# We have the templates this building can train.
		for UnitFile in TrainableUnits:
			breakIt = False
			for filter in FilterOut:
				if UnitFile.find(filter) != -1: breakIt = True
			if breakIt: continue

			if (os.path.isfile(UnitFile + ".xml")):
				if UnitFile not in CivData[Civ]:
					CivData[Civ][UnitFile] = UnitFile

print (CivData[Civs[0]])

basePath = os.path.realpath(__file__).replace("CreateRMTest.py","") + "../../maps/random/"

f = open(basePath + 'demo_testBalance.js', 'w')

f.write("RMS.LoadLibrary(\"rmgen\");\n\nlog(\"Initializing map...\");\n\nInitMap();\n\nvar fx = fractionToTiles(0.5);\nvar fz = fractionToTiles(0.5);\n\nplaceObject(fractionToTiles(0.4), fz, \"special/trigger_point_A\", 0, 0);\n\nplaceObject(fractionToTiles(0.6), fz, \"special/trigger_point_B\", 0, 0);\n\nplaceObject(fx, fz, \"special/trigger_point_C\", 0, 0);\n\n// Export map data\nExportMap();\n")

f = open(basePath + 'demo_testBalance.json', 'w')

f.write("{\n	\"settings\" : {\n		\"Name\" : \"Balance Demo\",\n		\"Script\" : \"demo_testBalance.js\",\n		\"Description\" : \"Test the unit Balance in a trigger map. Change the triggers to change the units created.\",\n		\"BaseTerrain\" : [\"medit_sea_depths\"],\n		\"BaseHeight\" : 30,\n		\"CircularMap\" : true,\n		\"Keywords\": [\"demo\"],\n		\"TriggerScripts\": [\n			\"scripts/TriggerHelper.js\",\n			\"random/demo_testBalance_triggers.js\"\n		],\n		\"XXXXXX\" : \"Optionally define other things here, like we would for a scenario\"\n	}\n}\n")

f = open(basePath + 'demo_testBalance_triggers.js', 'w')

f.write("Trigger.prototype.StartAWave = function()\n{\n	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);\n	var attackerEntity = [")

start = True
for Unit in CivData[Civs[0]]:
	if start == False:
		f.write( ",")
	if start == True:
		start = False
	f.write("\"" + Unit + "\"")

f.write("];\n	var defenderEntity = [")

start = True
for Unit in CivData[Civs[1]]:
	if start == False:
		f.write( ",")
	if start == True:
		start = False
	f.write("\"" + Unit + "\"")

f.write("];\n	var count = 75;\n\n	var pos = Engine.QueryInterface(this.GetTriggerPoints(\"C\")[0], IID_Position).GetPosition();\n\n	var cmd = {\"x\" : pos.x, \"z\" : pos.z};\n	cmd.type = \"attack-walk\";\n	cmd.queued = true;\n	cmd.entities = [];\n\n	// spawn attackers\n	for (var o = 0; o < count; ++o)\n	{\n		var rand = Math.floor(Math.random() * attackerEntity.length);\n		if (rand == attackerEntity.length) rand = attackerEntity - 1;\n		var attackers = TriggerHelper.SpawnUnitsFromTriggerPoints(\"A\", attackerEntity[rand], 1, 1);\n		for each (var i in attackers)\n			cmd.entities.push(i[0]);\n		ProcessCommand(1, cmd);\n	}\n\n	cmd.entities = [];\n	for (var o = 0; o < count; ++o)\n	{\n		var rand = Math.floor(Math.random() * defenderEntity.length);\n		if (rand == defenderEntity.length) rand = attackerEntity - 1;\n		var defenders = TriggerHelper.SpawnUnitsFromTriggerPoints(\"B\", defenderEntity[rand], 1, 2);\n		for each (var i in defenders)\n			cmd.entities.push(i[0]);\n		ProcessCommand(2, cmd);\n	}\n\n	cmpTrigger.DoAfterDelay(180000, \"StartAnEnemyWave\", {}); // The next wave will come in 2 minutes\n}\n\nTrigger.prototype.InitGame = function()\n{\n}\n\nvar cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);\ncmpTrigger.DoAfterDelay(0, \"StartAWave\", {});\n")

