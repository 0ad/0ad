/*
 * Used to know which templates I have, which templates I know I can train, things like that.
 */

var TemplateManager = function(gameState) {
	var self = this;
	
	this.knownTemplatesList = [];
	this.buildingTemplates = [];
	this.unitTemplates = [];
	
	// this will store templates that exist
	this.AcknowledgeTemplates(gameState);
	this.getBuildableSubtemplates(gameState);
	this.getTrainableSubtemplates(gameState);
	this.getBuildableSubtemplates(gameState);
	this.getTrainableSubtemplates(gameState);
	// should be enough in 100% of the cases.
	
};
TemplateManager.prototype.AcknowledgeTemplates = function(gameState)
{
	var self = this;
	var myEntities = gameState.getOwnEntities();
	myEntities.forEach(function(ent) { // }){
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
TemplateManager.prototype.getBuildableSubtemplates = function(gameState)
{
	for each (templateName in this.knownTemplatesList) {
		var template = gameState.getTemplate(templateName);
		if (template !== null) {
			var buildable = template.buildableEntities();
			if (buildable !== undefined)
				for each (subtpname in buildable) {
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
TemplateManager.prototype.getTrainableSubtemplates = function(gameState)
{
	for each (templateName in this.knownTemplatesList) {
		var template = gameState.getTemplate(templateName);
		if (template !== null) {
			var trainables = template.trainableEntities();
			if (trainables !== undefined)
				for each (subtpname in trainables) {
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
