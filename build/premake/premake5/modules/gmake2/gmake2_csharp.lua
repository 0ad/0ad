--
-- gmake2_csharp.lua
-- Generate a C# project makefile.
-- (c) 2016-2017 Jason Perkins, Blizzard Entertainment and the Premake project
--

	local p = premake
	local gmake2 = p.modules.gmake2

	gmake2.cs        = {}
	local cs         = gmake2.cs

	local project    = p.project
	local config     = p.config
	local fileconfig = p.fileconfig


--
-- Add namespace for element definition lists for p.callarray()
--

	cs.elements = {}


--
-- Generate a GNU make C++ project makefile, with support for the new platforms API.
--

	cs.elements.makefile = function(prj)
		return {
			gmake2.header,
			gmake2.phonyRules,
			gmake2.csConfigs,
			gmake2.csProjectConfig,
			gmake2.csSources,
			gmake2.csEmbedFiles,
			gmake2.csCopyFiles,
			gmake2.csResponseFile,
			gmake2.shellType,
			gmake2.csAllRules,
			gmake2.csTargetRules,
			gmake2.targetDirRules,
			gmake2.csResponseRules,
			gmake2.objDirRules,
			gmake2.csCleanRules,
			gmake2.preBuildRules,
			gmake2.csFileRules,
		}
	end


--
-- Generate a GNU make C# project makefile, with support for the new platforms API.
--

	function cs.generate(prj)
		p.eol("\n")
		local toolset = p.tools.dotnet
		p.callArray(cs.elements.makefile, prj, toolset)
	end


--
-- Write out the settings for a particular configuration.
--

	cs.elements.configuration = function(cfg)
		return {
			gmake2.csTools,
			gmake2.target,
			gmake2.objdir,
			gmake2.csFlags,
			gmake2.csLinkCmd,
			gmake2.preBuildCmds,
			gmake2.preLinkCmds,
			gmake2.postBuildCmds,
			gmake2.settings,
		}
	end

	function gmake2.csConfigs(prj, toolset)
		for cfg in project.eachconfig(prj) do
			_x('ifeq ($(config),%s)', cfg.shortname)
			p.callArray(cs.elements.configuration, cfg, toolset)
			_p('endif')
			_p('')
		end
	end


--
-- Given a .resx resource file, builds the path to corresponding .resource
-- file, matching the behavior and naming of Visual Studio.
--

	function cs.getresourcefilename(cfg, fname)
		if path.getextension(fname) == ".resx" then
			local name = cfg.buildtarget.basename .. "."
			local dir = path.getdirectory(fname)
			if dir ~= "." then
				name = name .. path.translate(dir, ".") .. "."
			end
			return "$(OBJDIR)/" .. p.esc(name .. path.getbasename(fname)) .. ".resources"
		else
			return fname
		end
	end


--
-- Iterate and output some selection of the source code files.
--

	function cs.listsources(prj, selector)
		local tr = project.getsourcetree(prj)
		p.tree.traverse(tr, {
			onleaf = function(node, depth)
				local value = selector(node)
				if value then
					_x('\t%s \\', value)
				end
			end
		})
	end





---------------------------------------------------------------------------
--
-- Handlers for individual makefile elements
--
---------------------------------------------------------------------------

	function gmake2.csAllRules(prj, toolset)
		_p('all: prebuild $(EMBEDFILES) $(COPYFILES) $(TARGET)')
		_p('')
	end


	function gmake2.csCleanRules(prj, toolset)
		--[[
		-- porting from 4.x
		_p('clean:')
		_p('\t@echo Cleaning %s', prj.name)
		_p('ifeq (posix,$(SHELLTYPE))')
		_p('\t$(SILENT) rm -f $(TARGETDIR)/%s.* $(COPYFILES)', target.basename)
		_p('\t$(SILENT) rm -rf $(OBJDIR)')
		_p('else')
		_p('\t$(SILENT) if exist $(subst /,\\\\,$(TARGETDIR)/%s) del $(subst /,\\\\,$(TARGETDIR)/%s.*)', target.name, target.basename)
		for target, source in pairs(cfgpairs[anycfg]) do
			_p('\t$(SILENT) if exist $(subst /,\\\\,%s) del $(subst /,\\\\,%s)', target, target)
		end
		for target, source in pairs(copypairs) do
			_p('\t$(SILENT) if exist $(subst /,\\\\,%s) del $(subst /,\\\\,%s)', target, target)
		end
		_p('\t$(SILENT) if exist $(subst /,\\\\,$(OBJDIR)) rmdir /s /q $(subst /,\\\\,$(OBJDIR))')
		_p('endif')
		_p('')
		--]]
	end


	function gmake2.csCopyFiles(prj, toolset)
		--[[
		-- copied from 4.x; needs more porting
		_p('COPYFILES += \\')
		for target, source in pairs(cfgpairs[anycfg]) do
			_p('\t%s \\', target)
		end
		for target, source in pairs(copypairs) do
			_p('\t%s \\', target)
		end
		_p('')
		--]]
	end


	function cs.getresponsefilename(prj)
		return '$(OBJDIR)/' .. prj.filename .. '.rsp'
	end


	function gmake2.csResponseFile(prj, toolset)
		_x('RESPONSE += ' .. gmake2.cs.getresponsefilename(prj))
	end


	function gmake2.csResponseRules(prj)
		local toolset = p.tools.dotnet
		local ext = gmake2.getmakefilename(prj, true)
		local makefile = path.getname(p.filename(prj, ext))
		local response = gmake2.cs.getresponsefilename(prj)

		_p('$(RESPONSE): %s', makefile)
		_p('\t@echo Generating response file', prj.name)

		_p('ifeq (posix,$(SHELLTYPE))')
			_x('\t$(SILENT) rm -f $(RESPONSE)')
		_p('else')
			_x('\t$(SILENT) if exist $(RESPONSE) del %s', path.translate(response, '\\'))
		_p('endif')

		local sep = os.istarget("windows") and "\\" or "/"
		local tr = project.getsourcetree(prj)
		p.tree.traverse(tr, {
			onleaf = function(node, depth)
				if toolset.fileinfo(node).action == "Compile" then
					_x('\t@echo %s >> $(RESPONSE)', path.translate(node.relpath, sep))
				end
			end
		})
		_p('')
	end


	function gmake2.csEmbedFiles(prj, toolset)
		local cfg = project.getfirstconfig(prj)

		_p('EMBEDFILES += \\')
		cs.listsources(prj, function(node)
			local fcfg = fileconfig.getconfig(node, cfg)
			local info = toolset.fileinfo(fcfg)
			if info.action == "EmbeddedResource" then
				return cs.getresourcefilename(cfg, node.relpath)
			end
		end)
		_p('')
	end


	function gmake2.csFileRules(prj, toolset)
		--[[
		-- porting from 4.x
		_p('# Per-configuration copied file rules')
		for cfg in p.eachconfig(prj) do
			_x('ifneq (,$(findstring %s,$(config)))', cfg.name:lower())
			for target, source in pairs(cfgpairs[cfg]) do
				p.make_copyrule(source, target)
			end
			_p('endif')
			_p('')
		end

		_p('# Copied file rules')
		for target, source in pairs(copypairs) do
			p.make_copyrule(source, target)
		end

		_p('# Embedded file rules')
		for _, fname in ipairs(embedded) do
			if path.getextension(fname) == ".resx" then
				_x('%s: %s', getresourcefilename(prj, fname), fname)
				_p('\t$(SILENT) $(RESGEN) $^ $@')
			end
			_p('')
		end
		--]]
	end


	function gmake2.csFlags(cfg, toolset)
		_p('  FLAGS =%s', gmake2.list(toolset.getflags(cfg)))
	end


	function gmake2.csLinkCmd(cfg, toolset)
		local deps = p.esc(config.getlinks(cfg, "dependencies", "fullpath"))
		_p('  DEPENDS =%s', gmake2.list(deps))
		_p('  REFERENCES = %s', table.implode(deps, "/r:", "", " "))
	end


	function gmake2.csProjectConfig(prj, toolset)
		-- To maintain compatibility with Visual Studio, these values must
		-- be set on the project level, and not per-configuration.
		local cfg = project.getfirstconfig(prj)

		local kindflag = "/t:" .. toolset.getkind(cfg):lower()
		local libdirs = table.implode(p.esc(cfg.libdirs), "/lib:", "", " ")
		_p('FLAGS += %s', table.concat(table.join(kindflag, libdirs), " "))

		local refs = p.esc(config.getlinks(cfg, "system", "fullpath"))
		_p('REFERENCES += %s', table.implode(refs, "/r:", "", " "))
		_p('')
	end


	function gmake2.csSources(prj, toolset)
		local cfg = project.getfirstconfig(prj)

		_p('SOURCES += \\')
		cs.listsources(prj, function(node)
			local fcfg = fileconfig.getconfig(node, cfg)
			local info = toolset.fileinfo(fcfg)
			if info.action == "Compile" then
				return node.relpath
			end
		end)
		_p('')
	end


	function gmake2.csTargetRules(prj, toolset)
		_p('$(TARGET): $(SOURCES) $(EMBEDFILES) $(DEPENDS) $(RESPONSE) | $(TARGETDIR)')
		_p('\t$(PRELINKCMDS)')
		_p('\t$(SILENT) $(CSC) /nologo /out:$@ $(FLAGS) $(REFERENCES) @$(RESPONSE) $(patsubst %%,/resource:%%,$(EMBEDFILES))')
		_p('\t$(POSTBUILDCMDS)')
		_p('')
	end


	function gmake2.csTools(cfg, toolset)
		_p('  CSC = %s', toolset.gettoolname(cfg, "csc"))
		_p('  RESGEN = %s', toolset.gettoolname(cfg, "resgen"))
	end
