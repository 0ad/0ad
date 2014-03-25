var PETRA = function(m)
{

/*
 * Used to know which templates I have, which templates I know I can train, things like that.
 * Mostly unused.
 */

m.TemplateManager = function(gameState) {
	var self = this;
	
	this.knownTemplatesList = [];
	this.buildingTemplates = [];
	this.unitTemplates = [];
	this.templateCounters = {};
	this.templateCounteredBy = {};

	// this will store templates that exist
	this.AcknowledgeTemplates(gameState);
	this.getBuildableSubtemplates(gameState);
	this.getTrainableSubtemplates(gameState);
	this.getBuildableSubtemplates(gameState);
	this.getTrainableSubtemplates(gameState);
	// should be enough in 100% of the cases.
	
	this.getTemplateCounters(gameState);
	
};
m.TemplateManager.prototype.AcknowledgeTemplates = function(gameState)
{
	var self = this;
	var myEntities = gameState.getOwnEntities();
	myEntities.forEach(function(ent)
	{
		var template = ent._templateName;
		if (self.knownTemplatesList.indexOf(template) === -1) {
			self.knownTemplatesList.push(template);
			if (ent.hasClass("Unit") && self.unitTemplates.indexOf(template) === -1)
				self.unitTemplates.push(template);
			else if (self.buildingTemplates.indexOf(template) === -1)
				self.buildingTemplates.push(template);
		}

	});
}
m.TemplateManager.prototype.getBuildableSubtemplates = function(gameState)
{
	for each (var templateName in this.knownTemplatesList) {
		var template = gameState.getTemplate(templateName);
		if (template !== null) {
			var buildable = template.buildableEntities();
			if (buildable !== undefined)
				for each (var subtpname in buildable) {
					if (this.knownTemplatesList.indexOf(subtpname) === -1) {
						this.knownTemplatesList.push(subtpname);
						var subtemplate = gameState.getTemplate(subtpname);
						if (subtemplate.hasClass("Unit") && this.unitTemplates.indexOf(subtpname) === -1)
							this.unitTemplates.push(subtpname);
						else if (this.buildingTemplates.indexOf(subtpname) === -1)
							this.buildingTemplates.push(subtpname);
					}
				}
		}
	}
}
m.TemplateManager.prototype.getTrainableSubtemplates = function(gameState)
{
	for each (var templateName in this.knownTemplatesList) {
		var template = gameState.getTemplate(templateName);
		if (template !== null) {
			var trainables = template.trainableEntities();
			if (trainables !== undefined)
				for each (var subtpname in trainables) {
					if (this.knownTemplatesList.indexOf(subtpname) === -1) {
						this.knownTemplatesList.push(subtpname);
						var subtemplate = gameState.getTemplate(subtpname);
						if (subtemplate.hasClass("Unit") && this.unitTemplates.indexOf(subtpname) === -1)
							this.unitTemplates.push(subtpname);
						else if (this.buildingTemplates.indexOf(subtpname) === -1)
							this.buildingTemplates.push(subtpname);
					}
				}
		}
	}
}
m.TemplateManager.prototype.getTemplateCounters = function(gameState)
{
	for (var i in this.unitTemplates)
	{
		var tp = gameState.getTemplate(this.unitTemplates[i]);
		var tpname = this.unitTemplates[i];
		this.templateCounters[tpname] = tp.getCounteredClasses();
	}
}
// features auto-caching
m.TemplateManager.prototype.getCountersToClasses = function(gameState,classes,templateName)
{
	if (templateName !== undefined && this.templateCounteredBy[templateName])
		return this.templateCounteredBy[templateName];
	
	var templates = [];
	for (var i in this.templateCounters) {
		var okay = false;
		for each (var ticket in this.templateCounters[i]) {
			var okaya = true;
			for (var a in ticket[0]) {
				if (classes.indexOf(ticket[0][a]) === -1)
					okaya = false;
			}
			if (okaya && templates.indexOf(i) === -1)
				templates.push([i, ticket[1]]);
		}
	}
	templates.sort (function (a,b) { return -a[1] + b[1]; });
	
	if (templateName !== undefined)
		this.templateCounteredBy[templateName] = templates;
	return templates;
}


return m;
}(PETRA);
