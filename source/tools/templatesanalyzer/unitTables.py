import xml.etree.ElementTree as ET
import os


AttackTypes = ["Hack","Pierce","Crush"]
Resources = ["food", "wood", "stone", "metal"]
# Todo: support siege.
Class = ["_champion",""]#["_hero","_champion",""]
Types = ["infantry","cavalry"];
Subtypes = ["spearman","pikeman","swordsman", "archer","javelinist","slinger"];
Fix = ["melee_","ranged_",""]

ComparativeSortByCav = True
ComparativeSortByChamp = True

paramFactorSpeed = True;
paramIncludePopCost = False
paramDPSImp = 5
paramHPImp = 4	# Decrease to make more important
paramRessImp = { "food" : 1.0, "wood" : 1.0, "stone" : 1.5, "metal" : 2.0 }

def htbout(file, balise, value):
	file.write("<" + balise + ">" + value + "</" + balise + ">\n" )
def htout(file, value):
	file.write("<p>" + value + "</p>\n" )

def CalcUnit(UnitName, existingUnit = None):
	unit = { 'HP' : "0", "BuildTime" : "0", "Cost" : { 'food' : "0", "wood" : "0", "stone" : "0", "metal" : "0", "population" : "0"},
	'Attack' : { "Melee" : {}, "Ranged" : {} }, 'RepeatRate' : {"Melee" : "0", "Ranged" : "0"},'PrepRate' : {"Melee" : "0", "Ranged" : "0"}, "Armour" : {},
	"Ranged" : "false"  };
	
	if (existingUnit != None):
		unit = existingUnit

	Template = ET.parse(UnitName)
	
	# Recursively get data from our parent which we'll override.
	if (Template.getroot().get("parent") != None):
		unit = CalcUnit(Template.getroot().get("parent") + ".xml", unit)

	if (Template.find("./Health/Max") != None):
		unit['HP'] = Template.find("./Health/Max").text

	if (Template.find("./Cost/BuildTime") != None):
		unit['BuildTime'] = Template.find("./Cost/BuildTime").text
	
	if (Template.find("./Cost/Resources") != None):
		for type in list(Template.find("./Cost/Resources")):
			unit['Cost'][type.tag] = type.text

	if (Template.find("./Attack/Melee") != None):
		if (Template.find("./Attack/Melee/RepeatTime") != None):
			unit['RepeatRate']["Melee"] = Template.find("./Attack/Melee/RepeatTime").text
		if (Template.find("./Attack/Melee/PrepareTime") != None):
			unit['PrepRate']["Melee"] = Template.find("./Attack/Melee/PrepareTime").text
		for atttype in AttackTypes:
			if (Template.find("./Attack/Melee/"+atttype) != None):
				unit['Attack']['Melee'][atttype] = Template.find("./Attack/Melee/"+atttype).text

	if (Template.find("./Attack/Ranged") != None):
		unit['Ranged'] = "true"
		if (Template.find("./Attack/Ranged/MaxRange") != None):
			unit['Range'] = Template.find("./Attack/Ranged/MaxRange").text
		if (Template.find("./Attack/Ranged/RepeatTime") != None):
			unit['RepeatRate']["Ranged"] = Template.find("./Attack/Ranged/RepeatTime").text
		if (Template.find("./Attack/Ranged/PrepareTime") != None):
			unit['PrepRate']["Ranged"] = Template.find("./Attack/Ranged/PrepareTime").text
		for atttype in AttackTypes:
			if (Template.find("./Attack/Ranged/"+atttype) != None):
				unit['Attack']['Ranged'][atttype] = Template.find("./Attack/Ranged/"+atttype).text

	if (Template.find("./Armour") != None):
		for atttype in AttackTypes:
			if (Template.find("./Armour/"+atttype) != None):
				unit['Armour'][atttype] = Template.find("./Armour/"+atttype).text

	if (Template.find("./UnitMotion") != None):
		if (Template.find("./UnitMotion/WalkSpeed") != None):
				unit['WalkSpeed'] = Template.find("./UnitMotion/WalkSpeed").text


	return unit

def WriteUnit(Name, UnitDict):
	str = "<tr>"

	str += "<td>" + Name + "</td>\n"
	
	str += "<td>HP: " + UnitDict["HP"] + "</td>\n"

	str += "<td>BuildTime: " + UnitDict["BuildTime"] + "</td>\n"
	
	str += "<td>Costs: " + UnitDict["Cost"]["food"] + "F / " + UnitDict["Cost"]["wood"] + "W / " + UnitDict["Cost"]["stone"] + "S / " + UnitDict["Cost"]["metal"] + "M</td>\n"

	return "</tr>" + str

def CalcValue(UnitDict):
	Worth = {};

	# Try to estimate how "worth it" a unit is.

	# Get some DPS up in here. Use only melee or Ranged if available.
	Attack = "Melee"
	if (UnitDict["Ranged"] == "true"):
		Attack = "Ranged"

	DPS = 0
	for att in AttackTypes:
		DPS += float(UnitDict['Attack'][Attack][att])
	if (Attack == "Ranged"):
		DPS += float(UnitDict["Range"]) / 10
	DPS /= (float(UnitDict['RepeatRate'][Attack])+float(UnitDict['PrepRate'][Attack]))/1000

	Worth["Attack"] = DPS * paramDPSImp;

	armorTotal = int(UnitDict['Armour']["Hack"]) + int(UnitDict['Armour']["Pierce"]) + int(UnitDict['Armour']["Crush"])/2.0

	Worth["Defence"] = int(UnitDict['HP']) / (0.9**armorTotal)/paramHPImp;

	TotalCost = 0
	for ress in paramRessImp:
		TotalCost += int(UnitDict['Cost'][ress]) * paramRessImp[ress]
	
	if (paramIncludePopCost):
		TotalCost *= (int(UnitDict['Cost']["population"])+1)/2.0

	Worth["Cost"] = TotalCost/400.0;

	Worth["Cost"] *= int(UnitDict['BuildTime'])/12.0;

	# Speed makes you way less evasive, and you can chase less.
	Worth["Cost"] /= float(UnitDict["WalkSpeed"])/10.0;

	Worth["Total"] = (Worth["Attack"] + Worth["Defence"])/Worth["Cost"]

	return Worth

# Returns How many of "B" a "A" could kill. Theoretically. With some error.
def Compare2(UnitDictA,UnitDictB):
	AWorth = 1.0;
	BWorth = 1.0;

	output = 1.0;

	# Get some DPS up in here. Use only melee or Ranged if available.
	AAttack = "Melee"
	if (UnitDictA["Ranged"] == "true"):
		AAttack = "Ranged"
	BAttack = "Melee"
	if (UnitDictB["Ranged"] == "true"):
		BAttack = "Ranged"

	ADPS = 0
	BDPS = 0
	# Exponential armor
	for att in AttackTypes:
		ADPS += float(UnitDictA['Attack'][AAttack][att]) * 0.9**float(UnitDictB['Armour'][att])
		BDPS += float(UnitDictB['Attack'][BAttack][att]) * 0.9**float(UnitDictA['Armour'][att])

	if (AAttack == "Ranged"):
		ADPS += float(UnitDictA["Range"]) / 10
	if (BAttack == "Ranged"):
		BDPS += float(UnitDictB["Range"]) / 10

	ADPS /= (float(UnitDictA['RepeatRate'][AAttack])+float(UnitDictA['PrepRate'][AAttack]))/1000
	BDPS /= (float(UnitDictB['RepeatRate'][BAttack])+float(UnitDictB['PrepRate'][BAttack]))/1000

	AWorth += int(UnitDictA['HP']) / BDPS;
	BWorth += int(UnitDictB['HP']) / ADPS;

	if (paramFactorSpeed):
		SpeedRatio = float(UnitDictA["WalkSpeed"]) / float(UnitDictB["WalkSpeed"])
	else:
		SpeedRatio = 1.0
	return AWorth / BWorth * SpeedRatio


# Output a matrix of each units against one another, where the square is greener if the unit is stronger, redder if weaker.
def WriteComparisonTable(Units):
	f.write("<table class=\"ComparisonTable\">")

	# Sort the templates dictionary.
	def SortFn(A):
		sortVal = Subtypes.index( A[0].rsplit("_",1)[1].rsplit(".",1)[0] ) + 1
		if (ComparativeSortByChamp == True and A[0].find("champion") == -1):
			sortVal -= 20
		if (ComparativeSortByCav == True and A[0].find("cavalry") == -1):
			sortVal -= 10
		return sortVal

	sortedDict = sorted(Units.items(), key=SortFn)

	# First column: write them all
	f.write ("<tr class=\"vertical-text\">")
	f.write("<td></td>")
	for unitName in sortedDict:
		f.write("<th>" + unitName[0] + "</th>")
	f.write("</tr>")

	# Write the comparisons
	for unitName in sortedDict:
		f.write("<tr>\n<th>" + unitName[0] + "</th>")
		for otherUnitName in sortedDict:
			outcome = Compare2(unitName[1],otherUnitName[1])
			green = max(0.0, min(4.0,outcome))
			if (green > 1.0):
				f.write("<td style=\"background-color: rgb(" + str(int(85*(4.0-green))) + ",255,0)\"></td>")
			else:
				f.write("<td style=\"background-color: rgb(255," + str(int(255*green)) + ",0" + ")\"></td>")

		f.write("</tr>")
	f.write("</table>")

def WriteWorthList(Units):
	f.write("<h2>Worthiness</h2>")

	def SortFn(A):
		return CalcValue(A[1])["Total"]

	sortedDict = sorted(Units.items(), key=SortFn, reverse=True)

	f.write("<table class=\"WorthList\">")
	for unitName in sortedDict:
		value = CalcValue(unitName[1])
		valueAtt = int(2*value["Attack"]/value["Cost"])
		valueDef = int(2*value["Defence"]/value["Cost"])
		f.write("<tr><th>" + unitName[0] + ": </th><td><span style=\"text-align:right; float:right; background-color:#DA0000; width:" + str(30 + valueAtt/5) + "px;\">" + str(valueAtt) + "</span></td>")
		f.write("<td><span style=\"background-color:#0000DD; width:" + str(30 + valueDef/5) + "px;\">" + str(valueDef) + "</span></td>")
		f.write("</tr>")

	f.write("</table>")


basePath = os.path.realpath(__file__).replace("unitTables.py","") + "../../../binaries/data/mods/public/simulation/templates/"

f = open(os.path.realpath(__file__).replace("unitTables.py","") + 'unit_summary_table.html', 'w')

f.write("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n<html>\n<head>\n	<title>Unit Tables</title>\n	<link rel=\"stylesheet\" href=\"style.css\">\n</head>\n<body>")
htbout(f,"h1","Unit Summary Table")
f.write("\n")

os.chdir(basePath)

#Load values up.
templates = {};	# Values
templatesBySubtype = {};
for sbt in Subtypes:
	templatesBySubtype[sbt] = [];

#Whilst loading I also write them.
htbout(f,"h2", "Units")
f.write("<table>")
for clss in Class:
	for tp in Types:
		for sbt in Subtypes:
			for x in Fix:
				template = "template_unit" + clss + "_" + tp + "_" + x + sbt + ".xml"
				if (os.path.isfile(template)):
					templatesBySubtype[sbt].append(template)
					templates[template] = CalcUnit(template)
					f.write(WriteUnit(template, templates[template]))
f.write("</table>")

WriteComparisonTable(templates)
WriteWorthList(templates)

f.write("</body>\n</html>")