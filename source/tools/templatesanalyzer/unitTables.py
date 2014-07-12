import xml.etree.ElementTree as ET
import os
import glob

# What data to use
AttackTypes = ["Hack","Pierce","Crush"]
Resources = ["food", "wood", "stone", "metal"]

#Generic templates to load
Class = ["_champion",""]
Types = ["infantry","cavalry"];
Subtypes = ["spearman","pikeman","swordsman", "archer","javelinist","slinger"];
Fix = ["melee_","ranged_",""]
AddedTemplates = ["template_unit_support_female_citizen.xml", "template_unit_champion_elephant_melee.xml"]

# Those describe Civs to analyze and buildings to consider.
# That way you can restrict to some buildings specifically.
Civs = ["spart", "athen", "mace", "rome", "cart", "pers", "maur", "gaul", "brit", "iber"]
#Civs = ["spart","mace", "gaul"]
CivBuildings = ["civil_centre", "barracks","gymnasion", "stables", "elephant_stables", "fortress", "embassy_celtic", "embassy_italiote", "embassy_iberian"]
#CivBuildings = ["civil_centre", "barracks"]

# Remote Civ templates with those strings in their name.
FilterOut = ["hero", "mecha", "support"]

# Graphic parameters: affects only how the data is shown
ComparativeSortByCav = True
ComparativeSortByChamp = True
SortTypes = ["Support","Pike","Spear","Sword", "Archer","Javelin","Sling","Elephant"];	# Classes
ShowCivLists = False

# Parameters: affects the data
paramFactorSpeed = True;
paramIncludePopCost = False
paramFactorCounters = True;	# False means attack bonuses are not counted.
paramDPSImp = 5
paramHPImp = 3	# Decrease to make more important
paramRessImp = { "food" : 1.0, "wood" : 1.0, "stone" : 2.0, "metal" : 1.5 }

def htbout(file, balise, value):
	file.write("<" + balise + ">" + value + "</" + balise + ">\n" )
def htout(file, value):
	file.write("<p>" + value + "</p>\n" )

def CalcUnit(UnitName, existingUnit = None):
	unit = { 'HP' : "0", "BuildTime" : "0", "Cost" : { 'food' : "0", "wood" : "0", "stone" : "0", "metal" : "0", "population" : "0"},
	'Attack' : { "Melee" : { "Hack" : 0, "Pierce" : 0, "Crush" : 0 }, "Ranged" : { "Hack" : 0, "Pierce" : 0, "Crush" : 0 } },
	'RepeatRate' : {"Melee" : "0", "Ranged" : "0"},'PrepRate' : {"Melee" : "0", "Ranged" : "0"}, "Armour" : {},
	"Ranged" : "false", "Classes" : [], "AttackBonuses" : {}, "Restricted" : [],
	"Civ" : None };
	
	if (existingUnit != None):
		unit = existingUnit

	Template = ET.parse(UnitName)
	
	# Recursively get data from our parent which we'll override.
	if (Template.getroot().get("parent") != None):
		unit = CalcUnit(Template.getroot().get("parent") + ".xml", unit)
		unit["Parent"] = Template.getroot().get("parent") + ".xml"

	if (Template.find("./Identity/Civ") != None):
		unit['Civ'] = Template.find("./Identity/Civ").text

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
		if (Template.find("./Attack/Melee/Bonuses") != None):
			for Bonus in Template.find("./Attack/Melee/Bonuses"):
				Against = []
				CivAg = []
				if (Bonus.find("Classes") != None and Bonus.find("Classes").text != None):
					Against = Bonus.find("Classes").text.split(" ")
				if (Bonus.find("Civ") != None and Bonus.find("Civ").text != None):
					CivAg = Bonus.find("Civ").text.split(" ")
				Val = float(Bonus.find("Multiplier").text)
				unit["AttackBonuses"][Bonus.tag] = {"Classes" : Against, "Civs" : CivAg, "Multiplier" : Val}
		if (Template.find("./Attack/Melee/RestrictedClasses") != None):
			newClasses = Template.find("./Attack/Melee/RestrictedClasses").text.split(" ")
			for elem in newClasses:
				if (elem.find("-") != -1):
					newClasses.pop(newClasses.index(elem))
					if elem in unit["Restricted"]:
						unit["Restricted"].pop(newClasses.index(elem))
			unit["Restricted"] += newClasses


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
		if (Template.find("./Attack/Ranged/Bonuses") != None):
			for Bonus in Template.find("./Attack/Ranged/Bonuses"):
				Against = []
				CivAg = []
				if (Bonus.find("Classes") != None and Bonus.find("Classes").text != None):
					Against = Bonus.find("Classes").text.split(" ")
				if (Bonus.find("Civ") != None and Bonus.find("Civ").text != None):
					CivAg = Bonus.find("Civ").text.split(" ")
				Val = float(Bonus.find("Multiplier").text)
				unit["AttackBonuses"][Bonus.tag] = {"Classes" : Against, "Civs" : CivAg, "Multiplier" : Val}
		if (Template.find("./Attack/Melee/RestrictedClasses") != None):
			newClasses = Template.find("./Attack/Melee/RestrictedClasses").text.split(" ")
			for elem in newClasses:
				if (elem.find("-") != -1):
					newClasses.pop(newClasses.index(elem))
					if elem in unit["Restricted"]:
						unit["Restricted"].pop(newClasses.index(elem))
			unit["Restricted"] += newClasses

	if (Template.find("./Armour") != None):
		for atttype in AttackTypes:
			if (Template.find("./Armour/"+atttype) != None):
				unit['Armour'][atttype] = Template.find("./Armour/"+atttype).text

	if (Template.find("./UnitMotion") != None):
		if (Template.find("./UnitMotion/WalkSpeed") != None):
				unit['WalkSpeed'] = Template.find("./UnitMotion/WalkSpeed").text

	if (Template.find("./Identity/VisibleClasses") != None):
		newClasses = Template.find("./Identity/VisibleClasses").text.split(" ")
		for elem in newClasses:
			if (elem.find("-") != -1):
				newClasses.pop(newClasses.index(elem))
				if elem in unit["Classes"]:
					unit["Classes"].pop(newClasses.index(elem))
		unit["Classes"] += newClasses
	
	if (Template.find("./Identity/Classes") != None):
		newClasses = Template.find("./Identity/Classes").text.split(" ")
		for elem in newClasses:
			if (elem.find("-") != -1):
				newClasses.pop(newClasses.index(elem))
				if elem in unit["Classes"]:
					unit["Classes"].pop(newClasses.index(elem))
		unit["Classes"] += newClasses


	return unit

def WriteUnit(Name, UnitDict):
	rstr = "<tr>"

	rstr += "<td>" + Name + "</td>\n"
	
	rstr += "<td>HP: " + UnitDict["HP"] + "</td>\n"

	rstr += "<td>BuildTime: " + UnitDict["BuildTime"] + "</td>\n"
	
	rstr += "<td>Costs: " + UnitDict["Cost"]["food"] + "F / " + UnitDict["Cost"]["wood"] + "W / " + UnitDict["Cost"]["stone"] + "S / " + UnitDict["Cost"]["metal"] + "M</td>\n"

	rstr += "<td>Classes: "
	for classe in UnitDict["Classes"]:
		rstr += classe + " "
	rstr += "</td>"
	
	rstr += "<td>Efficient against:"
	for Bonus in UnitDict["AttackBonuses"]:
		rstr += "["
		for classe in UnitDict["AttackBonuses"][Bonus]["Classes"]:
			rstr += classe + " "
		rstr += ': ' + str(UnitDict["AttackBonuses"][Bonus]["Multiplier"]) + "]  "
	rstr += "</td>"

	rstr += "<td>Cannot Attack: "
	for classe in UnitDict["Restricted"]:
		rstr += classe + " "
	rstr += "</td>"

	return "</tr>" + rstr

def CalcValue(UnitDict):
	Worth = {};

	# Try to estimate how "worth it" a unit is.
	# We can't really differentiate between attack types and armor types and counters here, so values might fluctuate
	# But this can be used to quickly estimate if some units are too costly.

	Attack = "Melee"
	if (UnitDict["Ranged"] == "true"):
		Attack = "Ranged"

	DPS = 0
	for att in AttackTypes:
		DPS += float(UnitDict['Attack'][Attack][att])
	if (Attack == "Ranged"):
		DPS += float(UnitDict["Range"]) / 10
	DPS /= (float(UnitDict['RepeatRate'][Attack])+float(UnitDict['PrepRate'][Attack])+1)/1000

	Worth["Attack"] = DPS * paramDPSImp;

	if "Infantry" in UnitDict["Restricted"]:
		Worth["Attack"] /= 5.0;
	if "Cavalry" in UnitDict["Restricted"]:
		Worth["Attack"] /= 5.0;

	armorTotal = float(UnitDict['Armour']["Hack"]) + float(UnitDict['Armour']["Pierce"]) + float(UnitDict['Armour']["Crush"])/2.0

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

def canAttack(Attacker, Defender):
	ARestrictions = Attacker["Restricted"]
	BClasses = Defender["Classes"]

	for Classes in ARestrictions:
		if Classes in BClasses:
			return False
	return True

def GetAttackBonus(Attacker, Defender):
	if not canAttack(Attacker, Defender):
		return 0.0
	Abonuses = Attacker["AttackBonuses"]
	BClasses = Defender["Classes"]

	Bonus = 1.0;

	for BonusTypes in Abonuses:
		found = True
		for classe in Abonuses[BonusTypes]["Classes"]:
			if classe not in BClasses:
				found = False;
		for civi in Abonuses[BonusTypes]["Civs"]:
			if Defender["Civ"] not in civi:
				found = False;
		if found:
			Bonus *= Abonuses[BonusTypes]["Multiplier"]

	return Bonus;

# Returns how much stronger unit A is compared to unit B in a direct fight. Uses counters, speed. Not perfect, but not widely inaccurate either.
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

	ADPS = 0.0
	BDPS = 0.0
	# Exponential armor
	for att in AttackTypes:
		ADPS += float(UnitDictA['Attack'][AAttack][att]) * 0.9**float(UnitDictB['Armour'][att])
		BDPS += float(UnitDictB['Attack'][BAttack][att]) * 0.9**float(UnitDictA['Armour'][att])
		#Check if A is bonused against B and vice versa
		if (paramFactorCounters == True):
			ADPS *= GetAttackBonus(UnitDictA,UnitDictB)
			BDPS *= GetAttackBonus(UnitDictB,UnitDictA)

	if (AAttack == "Ranged"):
		ADPS += float(UnitDictA["Range"]) / 10
	if (BAttack == "Ranged"):
		BDPS += float(UnitDictB["Range"]) / 10

	ADPS /= (float(UnitDictA['RepeatRate'][AAttack])+float(UnitDictA['PrepRate'][AAttack])+1)/1000
	BDPS /= (float(UnitDictB['RepeatRate'][BAttack])+float(UnitDictB['PrepRate'][BAttack])+1)/1000

	AWorth += int(UnitDictA['HP']) / (BDPS+1);
	BWorth += int(UnitDictB['HP']) / (ADPS+1);

	if (paramFactorSpeed):
		SpeedRatio = float(UnitDictA["WalkSpeed"]) / float(UnitDictB["WalkSpeed"])
	else:
		SpeedRatio = 1.0
	return AWorth / BWorth * SpeedRatio


# Output a matrix of each units against one another, where the square is greener if the unit is stronger, redder if weaker.
def WriteComparisonTable(Units, OtherUnits = None):
	f.write("<table class=\"ComparisonTable\">")

	# Sort the templates dictionary.
	def SortFn(A):
		sortVal = 0
		for classe in SortTypes:
			sortVal += 1
			if classe in A[1]["Classes"]:
				break
		if (ComparativeSortByChamp == True and A[0].find("champion") == -1):
			sortVal -= 20
		if (ComparativeSortByCav == True and A[0].find("cavalry") == -1):
			sortVal -= 10
		if (A[1]["Civ"] != None):
			sortVal += 100 * Civs.index(A[1]["Civ"])
		return sortVal

	if OtherUnits == None:
		OtherUnits = Units

	sortedDict = sorted(Units.items(), key=SortFn)
	sortedOtherDict = sorted(OtherUnits.items(), key=SortFn)

	# First column: write them all
	f.write ("<tr class=\"vertical-text\">")
	f.write("<td></td>")
	for unitName in sortedOtherDict:
		f.write("<th>" + unitName[0] + "</th>")
	f.write("<td class=\"Separator\"></td>")
	for unitName in sortedOtherDict:
		f.write("<th>" + unitName[0] + "</th>")	
	f.write("</tr>")

	# Write the comparisons
	for unitName in sortedDict:
		f.write("<tr>\n<th>" + unitName[0] + "</th>")
		
		for otherUnitName in sortedOtherDict:
			outcome = Compare2(unitName[1],otherUnitName[1])
			green = max(0.0, min(4.0,outcome))
			if (green > 1.0):
				f.write("<td style=\"background-color: rgb(" + str(int(85*(4.0-green))) + ",255,0)\"></td>")
			else:
				f.write("<td style=\"background-color: rgb(255," + str(int(255*green)) + ",0" + ")\"></td>")

		f.write("<td class=\"Separator\"></td>")

		for otherUnitName in sortedOtherDict:
			outcome = GetAttackBonus(unitName[1],otherUnitName[1])
			green = max(0.0, min(3.0,outcome))
			if (green > 1.0):
				f.write("<td style=\"background-color: rgb(" + str(int(126*(3.0-green))) + ",255,"  + str(int(126*(3.0-green))) + ")\"></td>")
			else:
				f.write("<td style=\"background-color: rgb(255," + str(int(255*green)) + "," + str(int(255*green)) + ")\"></td>")
		f.write("</tr>")

	f.write("</table>")


def WriteWorthList(Units):

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

#Whilst loading I also write them.
htbout(f,"h2", "Units")
f.write("<table class=\"UnitList\">")
for clss in Class:
	for tp in Types:
		for sbt in Subtypes:
			for x in Fix:
				template = "template_unit" + clss + "_" + tp + "_" + x + sbt + ".xml"
				if (os.path.isfile(template)):
					templates[template] = CalcUnit(template)
					f.write(WriteUnit(template, templates[template]))
for Tmp in AddedTemplates:
	if (os.path.isfile(Tmp)):
		templates[Tmp] = CalcUnit(Tmp)
		f.write(WriteUnit(Tmp, templates[Tmp]))
f.write("</table>")

f.write("<h2>Unit Comparison Table</h2>\n")
f.write("<p class=\"desc\">The first table shows how strong a unit is against another one. Full green means 4 times as strong, Yellow means equal, Full red means infinitely worse.<br/>The second table shows hardcoded attack counters. Full Green is 3.0. White is 1.0. Red is 0.0.</p>")
if (not paramFactorCounters):
	f.write("<p class=\"desc\">The left graph does not account for hardcoded counters here.</p>")
WriteComparisonTable(templates)

f.write("<h2>Worthiness</h2>\n")
f.write("<p class=\"desc\">This gives a representation of how efficient a unit is cost-wise.<br/>Red bars are offensive capabilities, blue bars defensive capabilites.</p>")
f.write("<p class=\"desc\">Costs are estimated as the sum of Food*" + str(paramRessImp["food"]) + " / Wood*" + str(paramRessImp["wood"]) + " / Stone*" + str(paramRessImp["stone"]) + " / Metal*" + str(paramRessImp["metal"]) + "<br/>")
f.write("Population cost is " + ("counted" if paramIncludePopCost else "not counted") + " in the costs.<br/>")
WriteWorthList(templates)

#Load Civ specific templates
# Will be loaded by loading buildings, and templates will be set the Civ building, the Civ and in general.
# Allows to avoid special units.
CivTemplatesByBuilding = {};
CivTemplatesByCiv = {};
CivTemplates = {};

for Civ in Civs:
	if ShowCivLists:
		htbout(f,"h2", Civ)
		f.write("<table class=\"UnitList\">")
	CivTemplatesByCiv[Civ] = {}
	CivTemplatesByBuilding[Civ] = {}
	for building in CivBuildings:
		CivTemplatesByBuilding[Civ][building] = {}
		bdTemplate = "./structures/" + Civ + "_" + building + ".xml"
		TrainableUnits = []
		if (os.path.isfile(bdTemplate)):
			Template = ET.parse(bdTemplate)
			if (Template.find("./ProductionQueue/Entities") != None):
				TrainableUnits = Template.find("./ProductionQueue/Entities").text.replace(" ","").replace("\t","").split("\n")
		for UnitFile in TrainableUnits:
			breakIt = False
			for filter in FilterOut:
				if UnitFile.find(filter) != -1: breakIt = True
			if breakIt: continue
			if (os.path.isfile(UnitFile + ".xml")):
				templates[UnitFile + ".xml"] = CalcUnit(UnitFile + ".xml")
				CivTemplatesByBuilding[Civ][building][UnitFile + ".xml"] = templates[UnitFile + ".xml"]
				if UnitFile + ".xml" not in CivTemplatesByCiv[Civ]:
					CivTemplates[UnitFile + ".xml"] = templates[UnitFile + ".xml"]
					CivTemplatesByCiv[Civ][UnitFile + ".xml"] = templates[UnitFile + ".xml"]
					if ShowCivLists:
						f.write(WriteUnit(UnitFile + ".xml", templates[UnitFile + ".xml"]))
	f.write("</table>")

# Writing Civ Specific Comparisons.
f.write("<h2>Civ Unit Comparisons</h2>\n")
f.write("<p class=\"desc\">The following graphs attempt to do some analysis of civilizations against each other. ")
f.write("They use the data in the comparison tables below. The averages are not extremely useful but can give some indications.<br/>")
f.write("You can deduce some things based on other columns though.<br/><br/>In particular \"Minimal Maximal Efficiency\" is low if there is a unit this civ cannot counter, and \"Maximal Minimal Efficiency\" is high if there is a unit that cannot be countered. If the (minimal) maximal efficiency and the average maximal efficiency are close, this means there are several units for that civ that counter the enemy civ's unit.<br/>The Balance column analyses those two to give some verdict (consider the average effectiveness too though)<br/><br/>Note that \"Counter\" in this data means \"Any unit that is strong against another unit\", so a unit that's simply way stronger than another counts as \"countering it\".<br/>If a civ's average efficiency and average efficiency disregarding costs are very different, this can mean that its costly units are particularly strong (Mauryans fit the bill with elephants).</p>")

for Civ in Civs:
	f.write("<h3>" + Civ + "</h3>")
	f.write("<table class=\"AverageComp\">")
	f.write("<tr><td></td><td>Average Efficiency<br/><span style=\"font-size:10px\">(disregards costs)</span></td><td>Maximal Minimal Efficiency<br/><span style=\"font-size:10px\">(disregards costs)</span></td><td>Minimal Maximal Efficiency<br/><span style=\"font-size:10px\">(disregards costs)</span></td><td>Average Efficiency</td><td>Maximal Minimal Efficiency</td><td>Minimal Maximal Efficiency</td><td>Balance</td></tr>")
	for oCiv in Civs:
		averageEffNoCost = 0
		MaxMinEffNoCost = 0
		MinMaxEffNoCost = 999999
		averageEff = 0
		MaxMinEff = 0
		MinMaxEff = 999999
		if (oCiv == Civ):
			continue
		for theirUnit in CivTemplatesByCiv[oCiv]:
			unitAverageNoCost = 0
			unitMaxNoCost = 0
			unitAverage = 0
			unitMax = 0
			for myUnit in CivTemplatesByCiv[Civ]:
				eff = Compare2(CivTemplatesByCiv[Civ][myUnit],CivTemplatesByCiv[oCiv][theirUnit])
				unitAverageNoCost += eff
				if (eff > unitMaxNoCost): unitMaxNoCost = eff
				eff /= CalcValue(CivTemplatesByCiv[Civ][myUnit])["Cost"] / CalcValue(CivTemplatesByCiv[oCiv][theirUnit])["Cost"]
				unitAverage += eff
				if (eff > unitMax): unitMax = eff
			unitAverageNoCost /= len(CivTemplatesByCiv[oCiv])
			unitAverage /= len(CivTemplatesByCiv[oCiv])
			averageEffNoCost += unitAverageNoCost
			averageEff += unitAverage
			if (unitMax < MinMaxEff): MinMaxEff = unitMax
			if (unitMaxNoCost < MinMaxEffNoCost): MinMaxEffNoCost = unitMaxNoCost

		for myUnit in CivTemplatesByCiv[Civ]:
			unitMinNoCost = 9999999
			unitMin = 9999999
			for theirUnit in CivTemplatesByCiv[oCiv]:
				eff = Compare2(CivTemplatesByCiv[Civ][myUnit],CivTemplatesByCiv[oCiv][theirUnit])
				if (eff < unitMinNoCost): unitMinNoCost = eff
				eff /= CalcValue(CivTemplatesByCiv[Civ][myUnit])["Cost"] / CalcValue(CivTemplatesByCiv[oCiv][theirUnit])["Cost"]
				if (eff < unitMin): unitMin = eff
			if (unitMin > MaxMinEff): MaxMinEff = unitMin
			if (unitMinNoCost > MaxMinEffNoCost): MaxMinEffNoCost = unitMinNoCost

		f.write("<tr><td>" + oCiv + "</td>")
		averageEffNoCost /= len(CivTemplatesByCiv[Civ])
		averageEff /= len(CivTemplatesByCiv[Civ])
		f.write("<td style=\"background-color:rgba(" + str(int(255 - (averageEffNoCost-1) * 90)) + "," + str(int((averageEffNoCost-1) * 90)) 	+ ",0,0.7);\">" + str("%.2f" % averageEffNoCost) + "</td>")
		f.write("<td style=\"background-color:rgba(" + str(int(255 - (MaxMinEffNoCost-0.3) * 360)) + "," + str(int((MaxMinEffNoCost-0.3) * 360))+ ",0,0.7);\">" + str("%.2f" % MaxMinEffNoCost) + "</td>")
		f.write("<td style=\"background-color:rgba(" + str(int(255 - (MinMaxEffNoCost-1) * 130)) + "," + str(int((MinMaxEffNoCost-1) * 130)) 	+ ",0,0.7);\">" + str("%.2f" % MinMaxEffNoCost) + "</td>")
		f.write("<td style=\"background-color:rgba(" + str(int(255 - (averageEff-0.5) * 110)) + "," + str(int((averageEff-0.5) * 110)) 			+ ",0,0.7);\">" + str("%.2f" % averageEff) + "</td>")
		f.write("<td style=\"background-color:rgba(" + str(int(255 - (MaxMinEff-0.3) * 360)) + "," + str(int((MaxMinEff-0.3) * 360)) 			+ ",0,0.7);\">" + str("%.2f" % MaxMinEff) + "</td>")
		f.write("<td style=\"background-color:rgba(" + str(int(255 - (MinMaxEff-1) * 130)) + "," + str(int((MinMaxEff-1) * 130)) 				+ ",0,0.7);\">" + str("%.2f" % MinMaxEff) + "</td>")
		if MaxMinEff <= MinMaxEff and abs(MaxMinEff * MinMaxEff - 1.0) < 0.1:
			f.write("<td style=\"background-color:rgb(0,255,0);\">Surely</td></tr>")
		elif MaxMinEff <= MinMaxEff and abs(MaxMinEff * MinMaxEff - 1.0) < 0.2:
			f.write("<td style=\"background-color:rgb(255,160,0);\">Probably</td></tr>")
		elif MaxMinEff >= MinMaxEff or MaxMinEff * MinMaxEff > 1.0:
			f.write("<td style=\"background-color:rgb(255,0,0);\">No, too strong</td></tr>")
		else:
			f.write("<td style=\"background-color:rgb(255,0,0);\">No, too weak</td></tr>")
	f.write("</table>")


f.write("<h2>Civ Comparison Tables</h2>\n")

for Civ in Civs:
	f.write("<h3>" + Civ + "</h3>")
	z = {}
	for oCiv in Civs:
		if (oCiv != Civ):
			z.update(CivTemplatesByCiv[oCiv])
	WriteComparisonTable(CivTemplatesByCiv[Civ],z)

f.write("<h2>Civ Units Worthiness</h2>\n")
for Civ in Civs:
	f.write("<h3>" + Civ + "</h3>")
	WriteWorthList(CivTemplatesByCiv[Civ])

f.write("</body>\n</html>")