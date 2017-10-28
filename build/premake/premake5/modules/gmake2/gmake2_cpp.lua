--
-- gmake2_cpp.lua
-- Generate a C/C++ project makefile.
-- (c) 2016-2017 Jason Perkins, Blizzard Entertainment and the Premake project
--

	local p = premake
	local gmake2 = p.modules.gmake2

	gmake2.cpp       = {}
	local cpp        = gmake2.cpp

	local project    = p.project
	local config     = p.config
	local fileconfig = p.fileconfig


---
-- Add namespace for element definition lists for premake.callarray()
---

	cpp.elements = {}


--
-- Generate a GNU make C++ project makefile, with support for the new platforms API.
--

	cpp.elements.makefile = function(prj)
		return {
			gmake2.header,
			gmake2.phonyRules,
			gmake2.shellType,
			cpp.createRuleTable,
			cpp.outputConfigurationSection,
			cpp.outputPerFileConfigurationSection,
			cpp.createFileTable,
			cpp.outputFilesSection,
			cpp.outputRulesSection,
			cpp.outputFileRuleSection,
			cpp.dependencies,
		}
	end


	function cpp.generate(prj)
		p.eol("\n")
		p.callArray(cpp.elements.makefile, prj)

		-- allow the garbage collector to clean things up.
		for cfg in project.eachconfig(prj) do
			cfg._gmake = nil
		end
		prj._gmake = nil
	end


	function cpp.initialize()
		rule 'cpp'
			fileExtension {".cc", ".cpp", ".cxx", ".mm"}
			buildoutputs  {'$(OBJDIR)/%{file.objname}.o'}
			buildmessage  '$(notdir $<)'
			buildcommands {'$(CXX) $(%{premake.modules.gmake2.cpp.fileFlags(cfg, file)}) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"'}

		rule 'cc'
			fileExtension {".c", ".s", ".m"}
			buildoutputs  {'$(OBJDIR)/%{file.objname}.o'}
			buildmessage  '$(notdir $<)'
			buildcommands {'$(CC) $(%{premake.modules.gmake2.cpp.fileFlags(cfg, file)}) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"'}

		rule 'resource'
			fileExtension ".rc"
			buildoutputs  {'$(OBJDIR)/%{file.objname}.res'}
			buildmessage  '$(notdir $<)'
			buildcommands {'$(RESCOMP) $< -O coff -o "$@" $(ALL_RESFLAGS)'}

		global(nil)
	end


	function cpp.createRuleTable(prj)
		local rules = {}

		local function addRule(extension, rule)
			if type(extension) == 'table' then
				for _, value in ipairs(extension) do
					addRule(value, rule)
				end
			else
				rules[extension] = rule
			end
		end

		-- add all rules.
		local usedRules = table.join({'cpp', 'cc', 'resource'}, prj.rules)
		for _, name in ipairs(usedRules) do
			local rule = p.global.getRule(name)
			addRule(rule.fileExtension, rule)
		end

		-- create fileset categories.
		local filesets = {
			['.o']   = 'OBJECTS',
			['.obj'] = 'OBJECTS',
			['.cc']  = 'SOURCES',
			['.cpp'] = 'SOURCES',
			['.cxx'] = 'SOURCES',
			['.mm']  = 'SOURCES',
			['.c']   = 'SOURCES',
			['.s']   = 'SOURCES',
			['.m']   = 'SOURCES',
			['.rc']  = 'RESOURCES',
		}

		-- cache the result.
		prj._gmake = prj._gmake or {}
		prj._gmake.rules = rules
		prj._gmake.filesets = filesets
	end


	function cpp.createFileTable(prj)
		for cfg in project.eachconfig(prj) do
			cfg._gmake = cfg._gmake or {}
			cfg._gmake.filesets = {}
			cfg._gmake.fileRules = {}

			local files = table.shallowcopy(prj._.files)
			table.foreachi(files, function(node)
				cpp.addFile(cfg, node)
			end)

			for _, f in pairs(cfg._gmake.filesets) do
				table.sort(f)
			end

			cfg._gmake.kinds = table.keys(cfg._gmake.filesets)
			table.sort(cfg._gmake.kinds)

			prj._gmake.kinds = table.join(prj._gmake.kinds or {}, cfg._gmake.kinds)
		end

		-- we need to reassign object sequences if we generated any files.
		if prj.hasGeneratedFiles and p.project.iscpp(prj) then
			p.oven.assignObjectSequences(prj)
		end

		prj._gmake.kinds = table.unique(prj._gmake.kinds)
		table.sort(prj._gmake.kinds)
	end


	function cpp.addFile(cfg, node)
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
				["file.basename"]     = { absolute = false, token = node.basename },
				["file.abspath"]      = { absolute = true,  token = node.abspath },
				["file.relpath"]      = { absolute = false, token = node.relpath },
				["file.name"]         = { absolute = false, token = node.name },
				["file.objname"]      = { absolute = false, token = node.objname },
				["file.path"]         = { absolute = true,  token = node.path },
				["file.directory"]    = { absolute = true,  token = path.getdirectory(node.abspath) },
				["file.reldirectory"] = { absolute = false, token = path.getdirectory(node.relpath) },
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
					cpp.addGeneratedFile(cfg, node, output)
				end
			end
		else
			cpp.addRuleFile(cfg, node)
		end
	end

	function cpp.addGeneratedFile(cfg, source, filename)
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
		local kind     = filesets[path.getextension(filename):lower()] or "CUSTOM"

		-- don't link generated object files automatically if it's explicitly
		-- disabled.
		if path.isobjectfile(filename) and source.linkbuildoutputs == false then
			kind = "CUSTOM"
		end

		local fileset = cfg._gmake.filesets[kind] or {}
		table.insert(fileset, filename)
		cfg._gmake.filesets[kind] = fileset

		-- recursively setup rules.
		cpp.addRuleFile(cfg, node)
	end

	function cpp.addRuleFile(cfg, node)
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
					cpp.addGeneratedFile(cfg, node, output)
				end
			end
		end
	end


--
-- Write out the settings for a particular configuration.
--

	cpp.elements.configuration = function(cfg)
		return {
			cpp.tools,
			gmake2.target,
			gmake2.objdir,
			cpp.pch,
			cpp.defines,
			cpp.includes,
			cpp.forceInclude,
			cpp.cppFlags,
			cpp.cFlags,
			cpp.cxxFlags,
			cpp.resFlags,
			cpp.libs,
			cpp.ldDeps,
			cpp.ldFlags,
			cpp.linkCmd,
			cpp.bindirs,
			cpp.exepaths,
			cpp.ruleProperties,
			gmake2.settings,
			gmake2.preBuildCmds,
			gmake2.preLinkCmds,
			gmake2.postBuildCmds,
		}
	end


	function cpp.outputConfigurationSection(prj)
		_p('# Configurations')
		_p('# #############################################')
		_p('')
		gmake2.outputSection(prj, cpp.elements.configuration)
	end


	function cpp.tools(cfg, toolset)
		local tool = toolset.gettoolname(cfg, "cc")
		if tool then
			_p('ifeq ($(origin CC), default)')
			_p('  CC = %s', tool)
			_p('endif' )
		end

		tool = toolset.gettoolname(cfg, "cxx")
		if tool then
			_p('ifeq ($(origin CXX), default)')
			_p('  CXX = %s', tool)
			_p('endif' )
		end

		tool = toolset.gettoolname(cfg, "ar")
		if tool then
			_p('ifeq ($(origin AR), default)')
			_p('  AR = %s', tool)
			_p('endif' )
		end

		tool = toolset.gettoolname(cfg, "rc")
		if tool then
			_p('RESCOMP = %s', tool)
		end
	end


	function cpp.pch(cfg, toolset)
		-- If there is no header, or if PCH has been disabled, I can early out
		if not cfg.pchheader or cfg.flags.NoPCH then
			return
		end

		-- Visual Studio requires the PCH header to be specified in the same way
		-- it appears in the #include statements used in the source code; the PCH
		-- source actual handles the compilation of the header. GCC compiles the
		-- header file directly, and needs the file's actual file system path in
		-- order to locate it.

		-- To maximize the compatibility between the two approaches, see if I can
		-- locate the specified PCH header on one of the include file search paths
		-- and, if so, adjust the path automatically so the user doesn't have
		-- add a conditional configuration to the project script.

		local pch = cfg.pchheader
		local found = false

		-- test locally in the project folder first (this is the most likely location)
		local testname = path.join(cfg.project.basedir, pch)
		if os.isfile(testname) then
			pch = project.getrelative(cfg.project, testname)
			found = true
		else
			-- else scan in all include dirs.
			for _, incdir in ipairs(cfg.includedirs) do
				testname = path.join(incdir, pch)
				if os.isfile(testname) then
					pch = project.getrelative(cfg.project, testname)
					found = true
					break
				end
			end
		end

		if not found then
			pch = project.getrelative(cfg.project, path.getabsolute(pch))
		end

		p.outln('PCH = ' .. pch)
		p.outln('PCH_PLACEHOLDER = $(OBJDIR)/$(notdir $(PCH))')
		p.outln('GCH = $(PCH_PLACEHOLDER).gch')
	end


	function cpp.defines(cfg, toolset)
		p.outln('DEFINES +=' .. gmake2.list(table.join(toolset.getdefines(cfg.defines, cfg), toolset.getundefines(cfg.undefines))))
	end


	function cpp.includes(cfg, toolset)
		local includes = toolset.getincludedirs(cfg, cfg.includedirs, cfg.sysincludedirs)
		p.outln('INCLUDES +=' .. gmake2.list(includes))
	end


	function cpp.forceInclude(cfg, toolset)
		local includes = toolset.getforceincludes(cfg)
		if not cfg.flags.NoPCH and cfg.pchheader then
			table.insert(includes, "-include $(PCH_PLACEHOLDER)")
		end
		p.outln('FORCE_INCLUDE +=' .. gmake2.list(includes))
	end


	function cpp.cppFlags(cfg, toolset)
		local flags = gmake2.list(toolset.getcppflags(cfg))
		p.outln('ALL_CPPFLAGS += $(CPPFLAGS)' .. flags .. ' $(DEFINES) $(INCLUDES)')
	end


	function cpp.cFlags(cfg, toolset)
		local flags = gmake2.list(table.join(toolset.getcflags(cfg), cfg.buildoptions))
		p.outln('ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS)' .. flags)
	end


	function cpp.cxxFlags(cfg, toolset)
		local flags = gmake2.list(table.join(toolset.getcxxflags(cfg), cfg.buildoptions))
		p.outln('ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS)' .. flags)
	end


	function cpp.resFlags(cfg, toolset)
		local resflags = table.join(toolset.getdefines(cfg.resdefines), toolset.getincludedirs(cfg, cfg.resincludedirs), cfg.resoptions)
		p.outln('ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)' .. gmake2.list(resflags))
	end


	function cpp.libs(cfg, toolset)
		local flags = toolset.getlinks(cfg)
		p.outln('LIBS +=' .. gmake2.list(flags, true))
	end


	function cpp.ldDeps(cfg, toolset)
		local deps = config.getlinks(cfg, "siblings", "fullpath")
		p.outln('LDDEPS +=' .. gmake2.list(p.esc(deps)))
	end


	function cpp.ldFlags(cfg, toolset)
		local flags = table.join(toolset.getLibraryDirectories(cfg), toolset.getrunpathdirs(cfg, cfg.runpathdirs), toolset.getldflags(cfg), cfg.linkoptions)
		p.outln('ALL_LDFLAGS += $(LDFLAGS)' .. gmake2.list(flags))
	end


	function cpp.linkCmd(cfg, toolset)
		if cfg.kind == p.STATICLIB then
			if cfg.architecture == p.UNIVERSAL then
				p.outln('LINKCMD = libtool -o "$@" $(OBJECTS)')
			else
				p.outln('LINKCMD = $(AR) -rcs "$@" $(OBJECTS)')
			end
		elseif cfg.kind == p.UTILITY then
			-- Empty LINKCMD for Utility (only custom build rules)
			p.outln('LINKCMD =')
		else
			-- this was $(TARGET) $(LDFLAGS) $(OBJECTS)
			--   but had trouble linking to certain static libs; $(OBJECTS) moved up
			-- $(LDFLAGS) moved to end (http://sourceforge.net/p/premake/patches/107/)
			-- $(LIBS) moved to end (http://sourceforge.net/p/premake/bugs/279/)

			local cc = iif(p.languages.isc(cfg.language), "CC", "CXX")
			p.outln('LINKCMD = $(' .. cc .. ') -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)')
		end
	end


	function cpp.bindirs(cfg, toolset)
		local dirs = project.getrelative(cfg.project, cfg.bindirs)
		if #dirs > 0 then
			p.outln('EXECUTABLE_PATHS = "' .. table.concat(dirs, ":") .. '"')
		end
	end


	function cpp.exepaths(cfg, toolset)
		local dirs = project.getrelative(cfg.project, cfg.bindirs)
		if #dirs > 0 then
			p.outln('EXE_PATHS = export PATH=$(EXECUTABLE_PATHS):$$PATH;')
		end
	end


	function cpp.ruleProperties(cfg, toolset)
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
-- Write out the per file configurations.
--
	function cpp.outputPerFileConfigurationSection(prj)
		_p('# Per File Configurations')
		_p('# #############################################')
		_p('')
		for cfg in project.eachconfig(prj) do
			table.foreachi(cfg.project._.files, function(node)
				local fcfg = fileconfig.getconfig(node, cfg)
				if fcfg then
					cpp.perFileFlags(cfg, fcfg)
				end
			end)
		end
		_p('')
	end

	local function makeVarName(prj, value, saltValue)
		prj._gmake.varlist = prj._gmake.varlist or {}
		prj._gmake.varlistlength = prj._gmake.varlistlength or 0
		local cache = prj._gmake.varlist
		local length = prj._gmake.varlistlength

		local key = value .. saltValue

		if (cache[key] ~= nil) then
			return cache[key], false
		end

		local var = string.format("PERFILE_FLAGS_%d", length)
		cache[key] = var

		prj._gmake.varlistlength = length + 1

		return var, true
	end

	function cpp.perFileFlags(cfg, fcfg)
		local toolset = gmake2.getToolSet(cfg)

		local value = gmake2.list(table.join(toolset.getcflags(fcfg), fcfg.buildoptions))

		if fcfg.defines or fcfg.undefines then
			local defs = table.join(toolset.getdefines(fcfg.defines, cfg), toolset.getundefines(fcfg.undefines))
			if #defs > 0 then
				value = value .. gmake2.list(defs)
			end
		end

		if fcfg.includedirs or fcfg.sysincludedirs then
			local includes = toolset.getincludedirs(cfg, fcfg.includedirs, fcfg.sysincludedirs)
			if #includes > 0 then
				value = value ..  gmake2.list(includes)
			end
		end

		if #value > 0 then
			local newPerFileFlag = false
			fcfg.flagsVariable, newPerFileFlag = makeVarName(cfg.project, value, iif(path.iscfile(fcfg.name), '_C', '_CPP'))
			if newPerFileFlag then
				if path.iscfile(fcfg.name) then
					_p('%s = $(ALL_CFLAGS)%s', fcfg.flagsVariable, value)
				else
					_p('%s = $(ALL_CXXFLAGS)%s', fcfg.flagsVariable, value)
				end
			end
		end
	end

	function cpp.fileFlags(cfg, file)
		local fcfg = fileconfig.getconfig(file, cfg)
		if fcfg and fcfg.flagsVariable then
			return fcfg.flagsVariable
		end

		if path.iscfile(file.name) then
			return 'ALL_CFLAGS'
		else
			return 'ALL_CXXFLAGS'
		end
	end

--
-- Write out the file sets.
--

	cpp.elements.filesets = function(cfg)
		local result = {}
		for _, kind in ipairs(cfg._gmake.kinds) do
			for _, f in ipairs(cfg._gmake.filesets[kind]) do
				table.insert(result, function(cfg, toolset)
					cpp.outputFileset(cfg, kind, f)
				end)
			end
		end
		return result
	end

	function cpp.outputFilesSection(prj)
		_p('# File sets')
		_p('# #############################################')
		_p('')

		for _, kind in ipairs(prj._gmake.kinds) do
			_x('%s :=', kind)
		end
		_x('')

		gmake2.outputSection(prj, cpp.elements.filesets)
	end

	function cpp.outputFileset(cfg, kind, file)
		_x('%s += %s', kind, file)
	end


--
-- Write out the targets.
--

	cpp.elements.rules = function(cfg)
		return {
			cpp.allRules,
			cpp.targetRules,
			gmake2.targetDirRules,
			gmake2.objDirRules,
			cpp.cleanRules,
			gmake2.preBuildRules,
			gmake2.preLinkRules,
			cpp.pchRules,
		}
	end


	function cpp.outputRulesSection(prj)
		_p('# Rules')
		_p('# #############################################')
		_p('')
		gmake2.outputSection(prj, cpp.elements.rules)
	end


	function cpp.allRules(cfg, toolset)
		if cfg.system == p.MACOSX and cfg.kind == p.WINDOWEDAPP then
			_p('all: prebuild prelink $(TARGET) $(dir $(TARGETDIR))PkgInfo $(dir $(TARGETDIR))Info.plist | $(TARGETDIR) $(OBJDIR)')
			_p('\t@:')
			_p('')
			_p('$(dir $(TARGETDIR))PkgInfo:')
			_p('$(dir $(TARGETDIR))Info.plist:')
		else
			_p('all: prebuild prelink $(TARGET) | $(TARGETDIR) $(OBJDIR)')
			_p('\t@:')
		end
		_p('')
	end


	function cpp.targetRules(cfg, toolset)
		local targets = '$(GCH) '

		for _, kind in ipairs(cfg._gmake.kinds) do
			if kind ~= 'OBJECTS' and kind ~= 'RESOURCES' then
				targets = targets .. '$(' .. kind .. ') '
			end
		end

		targets = targets .. '$(OBJECTS) $(LDDEPS)'
		if cfg._gmake.filesets['RESOURCES'] then
			targets = targets .. ' $(RESOURCES)'
		end

		_p('$(TARGET): %s | $(TARGETDIR)', targets)
		_p('\t@echo Linking %s', cfg.project.name)
		_p('\t$(SILENT) $(LINKCMD)')
		_p('\t$(POSTBUILDCMDS)')
		_p('')
	end


	function cpp.cleanRules(cfg, toolset)
		_p('clean:')
		_p('\t@echo Cleaning %s', cfg.project.name)
		_p('ifeq (posix,$(SHELLTYPE))')
		_p('\t$(SILENT) rm -f  $(TARGET)')
		_p('\t$(SILENT) rm -rf $(OBJDIR)')
		_p('else')
		_p('\t$(SILENT) if exist $(subst /,\\\\,$(TARGET)) del $(subst /,\\\\,$(TARGET))')
		_p('\t$(SILENT) if exist $(subst /,\\\\,$(OBJDIR)) rmdir /s /q $(subst /,\\\\,$(OBJDIR))')
		_p('endif')
		_p('')
	end


	function cpp.pchRules(cfg, toolset)
		_p('ifneq (,$(PCH))')
		_p('$(OBJECTS): $(GCH) $(PCH) | $(OBJDIR) $(PCH_PLACEHOLDER)')
		_p('$(GCH): $(PCH) | $(OBJDIR)')
		_p('\t@echo $(notdir $<)')
		local cmd = iif(p.languages.isc(cfg.language), "$(CC) -x c-header $(ALL_CFLAGS)", "$(CXX) -x c++-header $(ALL_CXXFLAGS)")
		_p('\t$(SILENT) %s -o "$@" -MF "$(@:%%.gch=%%.d)" -c "$<"', cmd)
		_p('$(PCH_PLACEHOLDER): $(GCH) | $(OBJDIR)')
		_p('\t$(SILENT) touch "$@"')
		_p('else')
		_p('$(OBJECTS): | $(OBJDIR)')
		_p('endif')
		_p('')
	end

--
-- Output the file compile targets.
--

	cpp.elements.fileRules = function(cfg)
		local funcs = {}
		for _, fileRule in ipairs(cfg._gmake.fileRules) do
			table.insert(funcs, function(cfg, toolset)
				cpp.outputFileRules(cfg, fileRule)
			end)
		end
		return funcs
	end


	function cpp.outputFileRuleSection(prj)
		_p('# File Rules')
		_p('# #############################################')
		_p('')
		gmake2.outputSection(prj, cpp.elements.fileRules)
	end


	function cpp.outputFileRules(cfg, file)
		local outputs = table.concat(file.buildoutputs, ' ')

		local dependencies = p.esc(file.source)
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
				if cfg.bindirs and #cfg.bindirs > 0 then
					_p('\t$(SILENT) $(EXE_PATHS) %s', cmd)
				else
					_p('\t$(SILENT) %s', cmd)
				end
			end
		end
	end


---------------------------------------------------------------------------
--
-- Handlers for individual makefile elements
--
---------------------------------------------------------------------------


	function cpp.dependencies(prj)
		-- include the dependencies, built by GCC (with the -MMD flag)
		_p('-include $(OBJECTS:%%.o=%%.d)')
		_p('ifneq (,$(PCH))')
			_p('  -include "$(PCH_PLACEHOLDER).d"')
		_p('endif')
	end



