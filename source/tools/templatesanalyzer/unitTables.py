import xml.etree.ElementTree as ET
import os
import glob

# What data to use
AttackTypes = ["Hack","Pierce","Crush"]
Resources = ["food", "wood", "stone", "metal"]

# Generic templates to load
# The way this works is it tries all generic templates
# But only loads those who have one of the following parents
# EG adding "template_unit.xml" will load all units.
LoadTemplatesIfParent = ["template_unit_infantry.xml", "template_unit_cavalry.xml", "template_unit_champion.xml"];

# Those describe Civs to analyze.
# The script will load all entities that derive (to the nth degree) from one of the above templates.
Civs = ["athen", "mace", "spart", "sele", "cart", "rome", "pers", "maur", "brit", "gaul", "iber"]

# Remote Civ templates with those strings in their name.
FilterOut = ["marian"]

# Sorting parameters for the "roster variety" table
ComparativeSortByCav = True
ComparativeSortByChamp = True
SortTypes = ["Support","Pike","Spear","Sword", "Archer","Javelin","Sling","Elephant"]	# Classes

# Disable if you want the more compact basic data. Enable to allow filtering and sorting in-place.
AddSortingOverlay = True

# This is the path to the /templates/ folder to consider. Change this for mod support.
basePath = os.path.realpath(__file__).replace("unitTables.py","") + "../../../binaries/data/mods/public/simulation/templates/"

# For performance purposes, cache opened templates files.
globalTemplatesList = {};

def htbout(file, balise, value):
	file.write("<" + balise + ">" + value + "</" + balise + ">\n" )
def htout(file, value):
	file.write("<p>" + value + "</p>\n" )

def fastParse(templateName):
	if templateName in globalTemplatesList:
		return globalTemplatesList[templateName]
	globalTemplatesList[templateName] = ET.parse(templateName)
	return globalTemplatesList[templateName]

# This function checks that a template has the given parent.
def hasParentTemplate(UnitName, parentName):
	Template = fastParse(UnitName)

	found = False
	Name = UnitName;
	while found != True and Template.getroot().get("parent") != None:
		Name = Template.getroot().get("parent") + ".xml"
		if Name == parentName:
			return True
		Template = ET.parse(Name)

	return False

# This function parses the entity values manually.
def CalcUnit(UnitName, existingUnit = None):
	unit = { 'HP' : "0", "BuildTime" : "0", "Cost" : { 'food' : "0", "wood" : "0", "stone" : "0", "metal" : "0", "population" : "0"},
	'Attack' : { "Melee" : { "Hack" : 0, "Pierce" : 0, "Crush" : 0 }, "Ranged" : { "Hack" : 0, "Pierce" : 0, "Crush" : 0 } },
	'RepeatRate' : {"Melee" : "0", "Ranged" : "0"},'PrepRate' : {"Melee" : "0", "Ranged" : "0"}, "Armour" : {},
	"Ranged" : False, "Classes" : [], "AttackBonuses" : {}, "Restricted" : [],
	"Civ" : None }
	
	if (existingUnit != None):
		unit = existingUnit

	Template = fastParse(UnitName)
	
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

	if (Template.find("./Cost/Population") != None):
		unit['Cost']["population"] = Template.find("./Cost/Population").text

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
		unit['Ranged'] = True
		if (Template.find("./Attack/Ranged/MaxRange") != None):
			unit['Range'] = Template.find("./Attack/Ranged/MaxRange").text
		if (Template.find("./Attack/Ranged/Spread") != None):
			unit['Spread'] = Template.find("./Attack/Ranged/Spread").text
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
	ret = "<tr>"

	ret += "<td class=\"Sub\">" + Name + "</td>"

	ret += "<td>" + str(int(UnitDict["HP"])) + "</td>"
	
	ret += "<td>" +str("%.0f" % float(UnitDict["BuildTime"])) + "</td>"

	ret += "<td>" + str("%.1f" % float(UnitDict["WalkSpeed"])) + "</td>"

	for atype in AttackTypes:
		PercentValue = 1.0 - (0.9 ** float(UnitDict["Armour"][atype]))
		ret += "<td>" + str("%.0f" % float(UnitDict["Armour"][atype])) + " / " + str("%.0f" % (PercentValue*100.0)) + "%</td>"

	attType = ("Ranged" if UnitDict["Ranged"] == True else "Melee")
	if UnitDict["RepeatRate"][attType] != "0":
		for atype in AttackTypes:
			repeatTime = float(UnitDict["RepeatRate"][attType])/1000.0
			ret += "<td>" + str("%.1f" % (float(UnitDict["Attack"][attType][atype])/repeatTime)) + "</td>"

		ret += "<td>" + str("%.1f" % (float(UnitDict["RepeatRate"][attType])/1000.0)) + "</td>"
	else:
		for atype in AttackTypes:
			ret += "<td> - </td>"
		ret += "<td> - </td>"

	if UnitDict["Ranged"] == True:
		ret += "<td>" + str("%.1f" % float(UnitDict["Range"])) + "</td>"
		spread = (float(UnitDict["Spread"]) / float(UnitDict["Range"]))*100.0
		ret += "<td>" + str("%.1f" % spread) + "</td>"
	else:
		ret += "<td> - </td><td> - </td>"

	for rtype in Resources:
		ret += "<td>" + str("%.0f" % float(UnitDict["Cost"][rtype])) + "</td>"
	
	ret += "<td>" + str("%.0f" % float(UnitDict["Cost"]["population"])) + "</td>"

	ret += "<td style=\"text-align:left;\">"
	for Bonus in UnitDict["AttackBonuses"]:
		ret += "["
		for classe in UnitDict["AttackBonuses"][Bonus]["Classes"]:
			ret += classe + " "
		ret += ': ' + str(UnitDict["AttackBonuses"][Bonus]["Multiplier"]) + "]  "
	ret += "</td>"

	ret += "</tr>\n"
	return ret

# Sort the templates dictionary.
def SortFn(A):
	sortVal = 0
	for classe in SortTypes:
		sortVal += 1
		if classe in A[1]["Classes"]:
			break
	if ComparativeSortByChamp == True and A[0].find("champion") == -1:
		sortVal -= 20
	if ComparativeSortByCav == True and A[0].find("cavalry") == -1:
		sortVal -= 10
	if A[1]["Civ"] != None and A[1]["Civ"] in Civs:
		sortVal += 100 * Civs.index(A[1]["Civ"])
	return sortVal

# helper to write coloured text.
def WriteColouredDiff(file, diff, PositOrNegat):

	def cleverParse(diff):
		if float(diff) - int(diff) < 0.001:
			return str(int(diff))
		else:
			return str("%.1f" % float(diff))

	if (PositOrNegat == "positive"):
		file.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff > 0 else "0,150,0")) + ");\">" + cleverParse(diff) + "</span></td>")
	elif (PositOrNegat == "negative"):
		file.write("<td><span style=\"color:rgb(" +("200,200,200" if diff == 0 else ("180,0,0" if diff < 0 else "0,150,0")) + ");\">" + cleverParse(diff) + "</span></td>")
	else:
		complain


############################################################
############################################################
# Create the HTML file

f = open(os.path.realpath(__file__).replace("unitTables.py","") + 'unit_summary_table.html', 'w')

f.write("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n<html>\n<head>\n	<title>Unit Tables</title>\n	<link rel=\"stylesheet\" href=\"style.css\">\n</head>\n<body>")
htbout(f,"h1","Unit Summary Table")
f.write("\n")

os.chdir(basePath)

############################################################
# Load generic templates

templates = {}

htbout(f,"h2", "Units")

f.write("<table id=\"genericTemplates\">\n")
f.write("<thead><tr>")
f.write("<th></th><th>HP</th>	<th>BuildTime</th>	<th>Speed(walk)</th>	<th colspan=\"3\">Armour</th>	<th colspan=\"6\">Attack (DPS)</th>													<th colspan=\"5\">Costs</th>						<th>Efficient Against</th> 	</tr>\n")
f.write("<tr class=\"Label\" style=\"border-bottom:1px black solid;\">")
f.write("<th></th><th></th>		<th></th>			<th></th>				<th>H</th><th>P</th><th>C</th>	<th>H</th><th>P</th><th>C</th><th>Rate</th><th>Range</th><th>Spread\n(/100m)</th>	<th>F</th><th>W</th><th>S</th><th>M</th><th>P</th>	<th></th>		</tr>\n</thead>\n")

for template in list(glob.glob('template_*.xml')):
	if os.path.isfile(template):
		found = False
		for possParent in LoadTemplatesIfParent:
			if hasParentTemplate(template, possParent):
				found = True
				break
		if found == True:
			templates[template] = CalcUnit(template)
			f.write(WriteUnit(template, templates[template]))

f.write("</table>")

############################################################
# Load Civ specific templates

CivTemplates = {}

for Civ in Civs:
	CivTemplates[Civ] = {};
	# Load all templates that start with that civ indicator
	for template in list(glob.glob('units/' + Civ + '_*.xml')):
		if os.path.isfile(template):

			# filter based on FilterOut
			breakIt = False
			for filter in FilterOut:
				if template.find(filter) != -1: breakIt = True
			if breakIt: continue

			# filter based on loaded generic templates
			breakIt = True
			for possParent in LoadTemplatesIfParent:
				if hasParentTemplate(template, possParent):
					breakIt = False
					break
			if breakIt: continue

			unit = CalcUnit(template)

			# Remove variants for now
			if unit["Parent"].find("template_") == -1:
				continue

			# load template
			CivTemplates[Civ][template] = unit

############################################################
f.write("\n\n<h2>Units Specializations</h2>\n")
f.write("<p class=\"desc\">This table compares each template to its parent, showing the differences between the two.<br/>Note that like any table, you can copy/paste this in Excel (or Numbers or ...) and sort it.</p>")

TemplatesByParent = {}

#Get them in the array
for Civ in Civs:
	for CivUnitTemplate in CivTemplates[Civ]:
		parent = CivTemplates[Civ][CivUnitTemplate]["Parent"]
		if parent in templates and templates[parent]["Civ"] == None:
			if parent not in TemplatesByParent:
				TemplatesByParent[parent] = []
			TemplatesByParent[parent].append( (CivUnitTemplate,CivTemplates[Civ][CivUnitTemplate]))

#Sort them by civ and write them in a table.
f.write("<table id=\"TemplateParentComp\">\n")
f.write("<thead><tr>")
f.write("<th></th><th></th><th>HP</th>	<th>BuildTime</th>	<th>Speed</th>	<th colspan=\"3\">Armour</th>	<th colspan=\"6\">Attack</th>												<th colspan=\"5\">Costs</th>						<th>Civ</th>	</tr>\n")
f.write("<tr class=\"Label\" style=\"border-bottom:1px black solid;\">")
f.write("<th></th><th></th><th></th>	<th></th>			<th></th>		<th>H</th><th>P</th><th>C</th>	<th>H</th><th>P</th><th>C</th><th>Rate</th><th>Range</th><th>Spread</th>	<th>F</th><th>W</th><th>S</th><th>M</th><th>P</th>	<th></th>		</tr>\n<tr></thead>")
for parent in TemplatesByParent:
	TemplatesByParent[parent].sort(key=lambda x : Civs.index(x[1]["Civ"]))
	for tp in TemplatesByParent[parent]:
		f.write("<th style='font-size:10px'>" + parent.replace(".xml","").replace("template_","") + "</th>")

		f.write("<td class=\"Sub\">" + tp[0].replace(".xml","").replace("units/","") + "</td>")
		
		# HP
		diff = int(tp[1]["HP"]) - int(templates[parent]["HP"])
		WriteColouredDiff(f, diff, "negative")
		
		# Build Time
		diff = int(tp[1]["BuildTime"]) - int(templates[parent]["BuildTime"])
		WriteColouredDiff(f, diff, "positive")
		
		# walk speed
		diff = float(tp[1]["WalkSpeed"]) - float(templates[parent]["WalkSpeed"])
		WriteColouredDiff(f, diff, "negative")
		
		# Armor
		for atype in AttackTypes:
			diff = float(tp[1]["Armour"][atype]) - float(templates[parent]["Armour"][atype])
			WriteColouredDiff(f, diff, "negative")

		# Attack types (DPS) and rate.
		attType = ("Ranged" if tp[1]["Ranged"] == True else "Melee")
		if tp[1]["RepeatRate"][attType] != "0":
			for atype in AttackTypes:
				myDPS = float(tp[1]["Attack"][attType][atype]) / (float(tp[1]["RepeatRate"][attType])/1000.0)
				parentDPS = float(templates[parent]["Attack"][attType][atype]) / (float(templates[parent]["RepeatRate"][attType])/1000.0)
				WriteColouredDiff(f, myDPS - parentDPS, "negative")
			WriteColouredDiff(f, float(tp[1]["RepeatRate"][attType])/1000.0 - float(templates[parent]["RepeatRate"][attType])/1000.0, "negative")
			# range and spread
			if tp[1]["Ranged"] == True:
				WriteColouredDiff(f, float(tp[1]["Range"]) - float(templates[parent]["Range"]), "negative")
				mySpread = (float(tp[1]["Spread"]) / (float(tp[1]["Range"]))*100.0)
				parentSpread = (float(templates[parent]["Spread"]) / (float(templates[parent]["Range"]))*100.0)
				WriteColouredDiff(f,  mySpread - parentSpread, "positive")
			else:
				f.write("<td></td><td></td>")
		else:
				f.write("<td></td><td></td><td></td><td></td><td></td><td></td>")

		for rtype in Resources:
			WriteColouredDiff(f, float(tp[1]["Cost"][rtype]) - float(templates[parent]["Cost"][rtype]), "positive")
		
		WriteColouredDiff(f, float(tp[1]["Cost"]["population"]) - float(templates[parent]["Cost"]["population"]), "positive")

		f.write("<td>" + tp[1]["Civ"] + "</td>")

		f.write("</tr>\n<tr>")
f.write("<table/>")

# Table of unit having or not having some units.
f.write("\n\n<h2>Roster Variety</h2>\n")
f.write("<p class=\"desc\">This table show which civilizations have units who derive from each loaded generic template.<br/>Green means 1 deriving unit, blue means 2, black means 3 or more.<br/>The total is the total number of loaded units for this civ, which may be more than the total of units inheriting from loaded templates.</p>")
f.write("<table class=\"CivRosterVariety\">\n")
f.write("<tr><th>Template</th>\n")
for civ in Civs:
	f.write("<td class=\"vertical-text\">" + civ + "</td>\n")
f.write("</tr>\n")

sortedDict = sorted(templates.items(), key=SortFn)

for tp in sortedDict:
	if tp[0] not in TemplatesByParent:
		continue
	f.write("<tr><td>" + tp[0] +"</td>\n")
	for civ in Civs:
		found = 0
		for temp in TemplatesByParent[tp[0]]:
			if temp[1]["Civ"] == civ:
				found += 1
		if found == 1:
			f.write("<td style=\"background-color:rgb(0,230,0);\"></td>")
		elif found == 2:
			f.write("<td style=\"background-color:rgb(0,0,200);\"></td>")
		elif found >= 3:
			f.write("<td style=\"background-color:rgb(0,0,0);\"></td>")
		else:
			f.write("<td style=\"background-color:rgb(235,0,0);\"></td>")
	f.write("</tr>\n")
f.write("<tr style=\"margin-top:2px;border-top:2px #aaa solid;\"><th style=\"text-align:right; padding-right:10px;\">Total:</th>\n")
for civ in Civs:
	count = 0
	for units in CivTemplates[civ]: count += 1
	f.write("<td style=\"text-align:center;\">" + str(count) + "</td>\n")

f.write("</tr>\n")

f.write("<table/>")

# Add a simple script to allow filtering on sorting directly in the HTML page.
if AddSortingOverlay:
	f.write("<script src=\"tablefilter/tablefilter.js\"></script>\n\
	\n\
	<script data-config>\n\
	\
		var cast = function (val) {\n\
		console.log(val);\
			if (+val != val)\n\
				return -999999999999;\n\
			return +val;\n\
		}\n\
	\n\n\
		var filtersConfig = {\n\
			base_path: 'tablefilter/',\n\
			col_0: 'checklist',\n\
			alternate_rows: true,\n\
			rows_counter: true,\n\
			btn_reset: true,\n\
			loader: false,\n\
			status_bar: false,\n\
			mark_active_columns: true,\n\
			highlight_keywords: true,\n\
			col_number_format: [\n\
				'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US'\n\
			],\n\
			filters_row_index: 2,\n\
			headers_row_index: 1,\n\
			extensions:[{\
				name: 'sort',\
				types: [\
					'string', 'us', 'us', 'us', 'us', 'us', 'us', 'mytype', 'mytype', 'mytype', 'mytype', 'mytype', 'mytype', 'us', 'us', 'us', 'us', 'us', 'string'\
				],\
				on_sort_loaded: function(o, sort) {\
					sort.addSortType('mytype',cast);\
				},\
			}],\n\
			col_widths: [\n\
				null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,'120px'\n\
			],\n\
		};\n\
		var tf = new TableFilter('genericTemplates', filtersConfig,2);\n\
		tf.init();\n\
	\n\
	   \
		var secondFiltersConfig = {\n\
			base_path: 'tablefilter/',\n\
			col_0: 'checklist',\n\
			col_19: 'checklist',\n\
			alternate_rows: true,\n\
			rows_counter: true,\n\
			btn_reset: true,\n\
			loader: false,\n\
			status_bar: false,\n\
			mark_active_columns: true,\n\
			highlight_keywords: true,\n\
			col_number_format: [\n\
				null, null, 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', 'US', null\n\
			],\n\
			filters_row_index: 2,\n\
			headers_row_index: 1,\n\
			extensions:[{\
				name: 'sort',\
				types: [\
					'string', 'string', 'us', 'us', 'us', 'us', 'us', 'us', 'typetwo', 'typetwo', 'typetwo', 'typetwo', 'typetwo', 'typetwo', 'us', 'us', 'us', 'us', 'us', 'string'\
				],\
				on_sort_loaded: function(o, sort) {\
					sort.addSortType('typetwo',cast);\
				},\
			}],\n\
			col_widths: [\n\
				null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null\n\
			],\n\
		};\n\
		\
		var tf2 = new TableFilter('TemplateParentComp', secondFiltersConfig,2);\n\
		tf2.init();\n\
	\n\
	</script>\n")


f.write("</body>\n</html>")