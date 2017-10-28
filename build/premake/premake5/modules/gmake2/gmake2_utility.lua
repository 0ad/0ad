--
-- gmake2_utility.lua
-- Generate a C/C++ project makefile.
-- (c) 2016-2017 Jason Perkins, Blizzard Entertainment and the Premake project
--

	local p = premake
	local gmake2 = p.modules.gmake2

	gmake2.utility   = {}
	local utility    = gmake2.utility

	local project    = p.project
	local config     = p.config
	local fileconfig = p.fileconfig

---
-- Add namespace for element definition lists for premake.callarray()
---

	utility.elements = {}


--
-- Generate a GNU make utility project makefile
--

	utility.elements.makefile = function(prj)
		return {
			gmake2.header,
			gmake2.phonyRules,
			gmake2.shellType,
			utility.initialize,
			utility.createFileTable,
			utility.outputFilesSection,
			utility.outputRulesSection,
			utility.outputFileRuleSection,
		}
	end


	function utility.generate(prj)
		p.eol("\n")
		p.callArray(utility.elements.makefile, prj)

		-- allow the garbage collector to clean things up.
		for cfg in project.eachconfig(prj) do
			cfg._gmake = nil
		end
		prj._gmake = nil

	end


	function utility.initialize(prj)
		prj._gmake = prj._gmake or {}
		prj._gmake.rules = prj.rules
		prj._gmake.filesets = { }
	end


	function utility.createFileTable(prj)
		for cfg in project.eachconfig(prj) do
			cfg._gmake = cfg._gmake or {}
			cfg._gmake.filesets = {}
			cfg._gmake.fileRules = {}

			local files = table.shallowcopy(prj._.files)
			table.foreachi(files, function(node)
				utility.addFile(cfg, node, prj)
			end)

			for _, f in pairs(cfg._gmake.filesets) do
				table.sort(f)
			end

			cfg._gmake.kinds = table.keys(cfg._gmake.filesets)
			table.sort(cfg._gmake.kinds)

			prj._gmake.kinds = table.join(prj._gmake.kinds or {}, cfg._gmake.kinds)
		end

		prj._gmake.kinds = table.unique(prj._gmake.kinds)
		table.sort(prj._gmake.kinds)
	end


	function utility.addFile(cfg, node, prj)
		local filecfg = fileconfig.getconfig(node, cfg)
		if not filecfg or filecfg.flags.ExcludeFromBuild then
			return
		end

		-- skip generated files, since we try to figure it out manually below.
		if node.generated then
			return
		end

		-- process custom build commands.
		if fileconfig.hasCustomBuildRule(filecfg) then
			local env = table.shallowcopy(filecfg.environ)
			env.PathVars = {
				["file.basename"] = { absolute = false, token = node.basename },
				["file.abspath"]  = { absolute = true,  token = node.abspath },
				["file.relpath"]  = { absolute = false, token = node.relpath },
				["file.name"]     = { absolute = false, token = node.name },
				["file.path"]     = { absolute = true,  token = node.path },
			}

			local shadowContext = p.context.extent(filecfg, env)

			local buildoutputs = p.project.getrelative(cfg.project, shadowContext.buildoutputs)
			if buildoutputs and #buildoutputs > 0 then
				local file = {
					buildoutputs  = buildoutputs,
					source        = node.relpath,
					buildmessage  = shadowContext.buildmessage,
					buildcommands = shadowContext.buildcommands,
					buildinputs   = p.project.getrelative(cfg.project, shadowContext.buildinputs)
				}
				table.insert(cfg._gmake.fileRules, file)

				for _, output in ipairs(buildoutputs) do
					utility.addGeneratedFile(cfg, node, output)
				end
			end
		else
			utility.addRuleFile(cfg, node)
		end
	end


	function utility.addGeneratedFile(cfg, source, filename)
		-- mark that we have generated files.
		cfg.project.hasGeneratedFiles = true

		-- add generated file to the project.
		local files = cfg.project._.files
		local node = files[filename]
		if not node then
			node = fileconfig.new(filename, cfg.project)
			files[filename] = node
			table.insert(files, node)
		end

		-- always overwrite the dependency information.
		node.dependsOn = source
		node.generated = true

		-- add to config if not already added.
		if not fileconfig.getconfig(node, cfg) then
			fileconfig.addconfig(node, cfg)
		end

		-- add file to the fileset.
		local filesets = cfg.project._gmake.filesets
		local kind = "CUSTOM"

		local fileset = cfg._gmake.filesets[kind] or {}
		table.insert(fileset, filename)
		cfg._gmake.filesets[kind] = fileset

		-- recursively setup rules.
		utility.addRuleFile(cfg, node)
	end


	function utility.addRuleFile(cfg, node)
		local rules = cfg.project._gmake.rules
		local rule = rules[path.getextension(node.abspath):lower()]
		if rule then

			local filecfg = fileconfig.getconfig(node, cfg)
			local environ = table.shallowcopy(filecfg.environ)

			if rule.propertydefinition then
				p.rule.prepareEnvironment(rule, environ, "$(%s)")
			end

			local shadowContext = p.context.extent(rule, environ)

			local buildoutputs  = shadowContext.buildoutputs
			local buildmessage  = shadowContext.buildmessage
			local buildcommands = shadowContext.buildcommands
			local buildinputs   = shadowContext.buildinputs

			buildoutputs = p.project.getrelative(cfg.project, buildoutputs)
			if buildoutputs and #buildoutputs > 0 then
				local file = {
					buildoutputs  = buildoutputs,
					source        = node.relpath,
					buildmessage  = buildmessage,
					buildcommands = buildcommands,
					buildinputs   = buildinputs
				}
				table.insert(cfg._gmake.fileRules, file)

				for _, output in ipairs(buildoutputs) do
					utility.addGeneratedFile(cfg, node, output)
				end
			end
		end
	end


--
-- Write out the settings for a particular configuration.
--

	utility.elements.configuration = function(cfg)
		return {
			utility.ruleProperties,
			gmake2.settings,
			gmake2.preBuildCmds,
			gmake2.preLinkCmds,
			gmake2.postBuildCmds,
		}
	end


	function utility.ruleProperties(cfg, toolset)
		for i = 1, #cfg.rules do
			local rule = p.global.getRule(cfg.rules[i])

			for prop in p.rule.eachProperty(rule) do
				local fld = p.rule.getPropertyField(rule, prop)
				local value = cfg[fld.name]
				if value ~= nil then
					if fld.kind == "path" then
						value = gmake2.path(cfg, value)
					elseif fld.kind == "list:path" then
						value = gmake2.path(cfg, value)
					end

					value = p.rule.expandString(rule, prop, value)
					if value ~= nil and #value > 0 then
						p.outln(prop.name .. ' = ' .. p.esc(value))
					end
				end
			end
		end
	end



--
-- Write out the file sets.
--

	utility.elements.filesets = function(cfg)
		local result = {}
		for _, kind in ipairs(cfg._gmake.kinds) do
			for _, f in ipairs(cfg._gmake.filesets[kind]) do
				table.insert(result, function(cfg, toolset)
					utility.outputFileset(cfg, kind, f)
				end)
			end
		end
		return result
	end


	function utility.outputFilesSection(prj)
		_p('# File sets')
		_p('# #############################################')
		_p('')

		for _, kind in ipairs(prj._gmake.kinds) do
			_x('%s :=', kind)
		end
		_x('')

		gmake2.outputSection(prj, utility.elements.filesets)
	end


	function utility.outputFileset(cfg, kind, file)
		_x('%s += %s', kind, file)
	end


--
-- Write out the targets.
--

	utility.elements.rules = function(cfg)
		return {
			utility.allRules,
			utility.targetRules,
			gmake2.targetDirRules,
			utility.cleanRules,
			gmake2.preBuildRules,
			gmake2.preLinkRules,
		}
	end


	function utility.outputRulesSection(prj)
		_p('# Rules')
		_p('# #############################################')
		_p('')
		gmake2.outputSection(prj, utility.elements.rules)
	end


	function utility.allRules(cfg, toolset)
		local allTargets = 'all: $(TARGETDIR) prebuild prelink $(TARGET)'
		for _, kind in ipairs(cfg._gmake.kinds) do
			allTargets = allTargets .. ' $(' .. kind .. ')'
		end
		_p(allTargets)
		_p('\t@:')
		_p('')
	end


	function utility.targetRules(cfg, toolset)
		local targets = ''

		for _, kind in ipairs(cfg._gmake.kinds) do
			targets = targets .. '$(' .. kind .. ') '
		end

		_p('$(TARGET): %s', targets)
		_p('\t$(POSTBUILDCMDS)')
		_p('')
	end


	function utility.cleanRules(cfg, toolset)
		_p('clean:')
		_p('\t@echo Cleaning %s', cfg.project.name)
		_p('')
	end


--
-- Output the file compile targets.
--

	utility.elements.fileRules = function(cfg)
		local funcs = {}
		for _, fileRule in ipairs(cfg._gmake.fileRules) do
			table.insert(funcs, function(cfg, toolset)
				utility.outputFileRules(cfg, fileRule)
			end)
		end
		return funcs
	end


	function utility.outputFileRuleSection(prj)
		_p('# File Rules')
		_p('# #############################################')
		_p('')
		gmake2.outputSection(prj, utility.elements.fileRules)
	end


	function utility.outputFileRules(cfg, file)
		local outputs = table.concat(file.buildoutputs, ' ')

		local dependencies = file.source
		if file.buildinputs and #file.buildinputs > 0 then
			dependencies = dependencies .. " " .. table.concat(p.esc(file.buildinputs), " ")
		end

		_p('%s: %s', outputs, dependencies)

		if file.buildmessage then
			_p('\t@echo %s', file.buildmessage)
		end

		if file.buildcommands then
			local cmds = os.translateCommandsAndPaths(file.buildcommands, cfg.project.basedir, cfg.project.location)
			for _, cmd in ipairs(cmds) do
				_p('\t$(SILENT) %s', cmd)
			end
		end
	end
