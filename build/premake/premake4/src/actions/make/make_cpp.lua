--
-- make_cpp.lua
-- Generate a C/C++ project makefile.
-- Copyright (c) 2002-2009 Jason Perkins and the Premake project
--

	premake.make.cpp = { }
	local _ = premake.make.cpp
	

	function premake.make_cpp(prj)
		-- create a shortcut to the compiler interface
		local cc = premake.gettool(prj)
		
		-- build a list of supported target platforms that also includes a generic build
		local platforms = premake.filterplatforms(prj.solution, cc.platforms, "Native")
		
		premake.gmake_cpp_header(prj, cc, platforms)

		for _, platform in ipairs(platforms) do
			for cfg in premake.eachconfig(prj, platform) do
				premake.gmake_cpp_config(cfg, cc)
			end
		end
		
		-- list intermediate files
		_p('OBJECTS := \\')
		for _, file in ipairs(prj.files) do
			if path.iscppfile(file) or path.getextension(file) == ".asm" then
				_p('\t$(OBJDIR)/%s.o \\', _MAKE.esc(path.getbasename(file)))
			end
		end
		_p('')
 
		_p('RESOURCES := \\')
		for _, file in ipairs(prj.files) do
			if path.isresourcefile(file) then
				_p('\t$(OBJDIR)/%s.res \\', _MAKE.esc(path.getbasename(file)))
			end
		end
		_p('')
 
		-- identify the shell type
		_p('SHELLTYPE := msdos')
		_p('ifeq (,$(ComSpec)$(COMSPEC))')
		_p('  SHELLTYPE := posix')
		_p('endif')
		_p('ifeq (/bin,$(findstring /bin,$(SHELL)))')
		_p('  SHELLTYPE := posix')
		_p('endif')
		_p('')
		
		-- main build rule(s)
		_p('.PHONY: clean prebuild prelink')
		_p('')

		if os.is("MacOSX") and prj.kind == "WindowedApp" then
			_p('all: $(TARGET) $(dir $(TARGETDIR))PkgInfo $(dir $(TARGETDIR))Info.plist')
		else
			_p('all: $(TARGET)')
		end
		_p('\t@:')
		_p('')

		-- target build rule
		_p('$(TARGET): $(OBJECTS) $(LDDEPS) $(RESOURCES) | prelink')
		_p('\t@echo Linking %s', prj.name)
		_p('\t$(SILENT) $(LINKCMD)')
		_p('\t$(POSTBUILDCMDS)')
		_p('')
		
		-- Create destination directories. Can't use $@ for this because it loses the
		-- escaping, causing issues with spaces and parenthesis
		_p('$(TARGETDIR):')
		premake.make_mkdirrule("$(TARGETDIR)")
		
		_p('$(OBJDIR):')
		premake.make_mkdirrule("$(OBJDIR)")

		-- Mac OS X specific targets
		if os.is("MacOSX") and prj.kind == "WindowedApp" then
			_p('$(dir $(TARGETDIR))PkgInfo:')
			_p('$(dir $(TARGETDIR))Info.plist:')
			_p('')
		end

		-- clean target
		_p('clean:')
		_p('\t@echo Cleaning %s', prj.name)
		_p('ifeq (posix,$(SHELLTYPE))')
		_p('\t$(SILENT) rm -f  $(TARGET)')
		_p('\t$(SILENT) rm -rf $(OBJDIR)')
		_p('else')
		_p('\t$(SILENT) if exist $(subst /,\\\\,$(TARGET)) del $(subst /,\\\\,$(TARGET))')
		_p('\t$(SILENT) if exist $(subst /,\\\\,$(OBJDIR)) rmdir /s /q $(subst /,\\\\,$(OBJDIR))')
		_p('endif')
		_p('')

		-- custom build step targets
		_p('prebuild: $(TARGETDIR) $(OBJDIR)')
		_p('\t$(PREBUILDCMDS)')
		_p('')
		
		_p('prelink:')
		_p('\t$(PRELINKCMDS)')
		_p('')

		-- precompiler header rule
		_.pchrules(prj)
				
		-- per-file rules
		for _, file in ipairs(prj.files) do
			if path.iscppfile(file) then
				-- Don't use PCH for Obj-C/C++ files (we could but we'd have to compile them separately
				-- and there's no advantage to that yet)
				local gchobj = '$(GCH)'
				local pchincludes = '$(PCHINCLUDES)'
				if (path.getextension(file) == ".mm" or path.getextension(file) == ".m") then
					gchobj = ''
					pchincludes = ''
				end

				_p('$(OBJDIR)/%s.o: %s %s | prebuild', _MAKE.esc(path.getbasename(file)), _MAKE.esc(file), gchobj)
				_p('\t@echo $(notdir $<)')
				if (path.iscfile(file)) then
					_p('\t$(SILENT) $(CC) %s $(CFLAGS) -MF $(OBJDIR)/%s.d -MT "$@" -o "$@" -c "$<"', pchincludes, _MAKE.esc(path.getbasename(file)))
				else
					_p('\t$(SILENT) $(CXX) %s $(CXXFLAGS) -MF $(OBJDIR)/%s.d -MT "$@" -o "$@" -c "$<"', pchincludes, _MAKE.esc(path.getbasename(file)))
				end
			elseif (path.getextension(file) == ".rc") then
				_p('$(OBJDIR)/%s.res: %s', _MAKE.esc(path.getbasename(file)), _MAKE.esc(file))
				_p('\t@echo $(notdir $<)')
				_p('\t$(SILENT) windres $< -O coff -o "$@" $(RESFLAGS)')
			elseif (path.getextension(file) == ".asm") then
				_p('$(OBJDIR)/%s.o: %s', _MAKE.esc(path.getbasename(file)), _MAKE.esc(file))
				_p('\t@echo $(notdir $<)')

				local opts = ''
				if os.is('windows') then
					opts = ''
				elseif os.is('macosx') then
					opts = '-D OS_UNIX=1'
				else
					opts = '-D DONT_USE_UNDERLINE=1 -D OS_UNIX=1'
				end

				if not (prj.solution.nasmpath) then
					prj.solution.nasmpath = 'nasm'				
				end

				_p('\t$(SILENT)'.._MAKE.esc(prj.solution.nasmpath)..' '..opts..' -i'.._MAKE.esc(path.getdirectory(file))..'/'..' -f '..
				   _MAKE.esc(prj.solution.nasmformat)..' -o $@ $<\n\t')

				_p('\t$(SILENT)'.._MAKE.esc(prj.solution.nasmpath)..' '..opts..' -i'.._MAKE.esc(path.getdirectory(file))..'/'..
				   ' -M -o $@ $< >$(OBJDIR)/$(<F:%%.asm=%%.d)\n')
			end
			
		end
		_p('')		

		-- output for test-generation
		-- test generation only works if all required parameters are set!
		if(prj.cxxtestpath and prj.cxxtestrootfile and prj.cxxtesthdrfiles and prj.cxxtestsrcfiles) then
			if not(prj.cxxtestrootoptions) then
				prj.cxxtestrootoptions = ''
			end
			if not(prj.cxxtestoptions) then 
				prj.cxxtestoptions = ''
			end

			_p(prj.cxxtestrootfile..': ')
			_p('\t@echo $(notdir $<)')
			_p('\t$(SILENT)'.._MAKE.esc(prj.cxxtestpath)..' --root '..prj.cxxtestrootoptions..' -o '.._MAKE.esc(prj.cxxtestrootfile))	
			_p('')	
	
			for i, file in ipairs(prj.cxxtesthdrfiles) do
				_p('%s: %s', _MAKE.esc(prj.cxxtestsrcfiles[i]), _MAKE.esc(file))
				_p('\t@echo $(notdir $<)')
				_p('\t$(SILENT)'.._MAKE.esc(prj.cxxtestpath)..' --part '..prj.cxxtestoptions..' -o ' .._MAKE.esc(prj.cxxtestsrcfiles[i])..' '.._MAKE.esc(file))
			end
			_p('')
		end
		
		-- include the dependencies, built by GCC (with the -MMD flag)
		_p('-include $(OBJECTS:%%.o=%%.d)')
		_p('-include $(GCH:%%.h.gch=%%.h.d)')
	end



--
-- Write the makefile header
--

	function premake.gmake_cpp_header(prj, cc, platforms)
		_p('# %s project makefile autogenerated by Premake', premake.action.current().shortname)

		-- set up the environment
		_p('ifndef config')
		_p('  config=%s', _MAKE.esc(premake.getconfigname(prj.solution.configurations[1], platforms[1], true)))
		_p('endif')
		_p('')
		
		_p('ifndef verbose')
		_p('  SILENT = @')
		_p('endif')
		_p('')
		
		_p('ifndef CC')
		_p('  CC = %s', cc.cc)
		_p('endif')
		_p('')
		
		_p('ifndef CXX')
		_p('  CXX = %s', cc.cxx)
		_p('endif')
		_p('')
		
		_p('ifndef AR')
		_p('  AR = %s', cc.ar)
		_p('endif')
		_p('')
	end
	
	
--
-- Write a block of configuration settings.
--

	function premake.gmake_cpp_config(cfg, cc)

		_p('ifeq ($(config),%s)', _MAKE.esc(cfg.shortname))

		
		-- if this platform requires a special compiler or linker, list it now
		local platform = cc.platforms[cfg.platform]
		if platform.cc then
			_p('  CC         = %s', platform.cc)
		end
		if platform.cxx then
			_p('  CXX        = %s', platform.cxx)
		end
		if platform.ar then
			_p('  AR         = %s', platform.ar)
		end
		if not(cfg.gnuexternals) then
			cfg.gnuexternal = { }		
		end 

		_p('  OBJDIR     = %s', _MAKE.esc(cfg.objectsdir))		
		_p('  TARGETDIR  = %s', _MAKE.esc(cfg.buildtarget.directory))
		_p('  TARGET     = $(TARGETDIR)/%s', _MAKE.esc(cfg.buildtarget.name))
		_p('  DEFINES   += %s', table.concat(cc.getdefines(cfg.defines), " "))
		_p('  INCLUDES  += %s', table.concat(cc.getincludedirs(cfg.includedirs), " "))
		_p('  CPPFLAGS  += %s $(DEFINES) $(INCLUDES)', table.concat(cc.getcppflags(cfg), " "))

		-- set up precompiled headers
		_.pchconfig(cfg)
				
		_p('  CFLAGS    += $(CPPFLAGS) %s', table.concat(table.join(cc.getcflags(cfg), cfg.buildoptions), " "))
		_p('  CXXFLAGS  += $(CFLAGS) %s', table.concat(cc.getcxxflags(cfg), " "))
		_p('  LDFLAGS   += %s', table.concat(table.join(cc.getldflags(cfg), cfg.linkoptions, cc.getlibdirflags(cfg)), " "))
		_p('  LIBS      += %s %s', table.concat(cc.getlinkflags(cfg), " "), table.concat(cfg.gnuexternals, " "))
		_p('  RESFLAGS  += $(DEFINES) $(INCLUDES) %s', table.concat(table.join(cc.getdefines(cfg.resdefines), cc.getincludedirs(cfg.resincludedirs), cfg.resoptions), " "))
		_p('  LDDEPS    += %s', table.concat(_MAKE.esc(premake.getlinks(cfg, "static", "fullpath")), " "))
		
		if cfg.kind == "StaticLib" then
			if cfg.platform:startswith("Universal") then
				_p('  LINKCMD    = libtool -o $(TARGET) $(OBJECTS)')
			else
				_p('  LINKCMD    = $(AR) -rcs $(TARGET) $(OBJECTS)')
			end
		else
			-- this was $(TARGET) $(LDFLAGS) $(OBJECTS) ... but was having trouble linking to certain 
			-- static libraries so $(OBJECTS) was moved up
			local lddeps = ''

			-- on osx, --start-group and --end-group aren't supported by ld
			if os.is('macosx') then
				lddeps = '$(LDDEPS)'
			else
				lddeps = '-Xlinker --start-group $(LDDEPS) -Xlinker --end-group'
			end
			_p('  LINKCMD    = $(%s) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) %s $(LIBS)', 
			iif(cfg.language == "C", "CC", "CXX"), lddeps)
		end
		
		_p('  define PREBUILDCMDS')
		if #cfg.prebuildcommands > 0 then
			_p('\t@echo Running pre-build commands')
			_p('\t%s', table.implode(cfg.prebuildcommands, "", "", "\n\t"))
		end
		_p('  endef')

		_p('  define PRELINKCMDS')
		if #cfg.prelinkcommands > 0 then
			_p('\t@echo Running pre-link commands')
			_p('\t%s', table.implode(cfg.prelinkcommands, "", "", "\n\t"))
		end
		_p('  endef')

		_p('  define POSTBUILDCMDS')
		if #cfg.postbuildcommands > 0 then
			_p('\t@echo Running post-build commands')
			_p('\t%s', table.implode(cfg.postbuildcommands, "", "", "\n\t"))
		end
		_p('  endef')
		
		_p('endif')
		_p('')
	end
	
	
--
-- Precompiled header support
--

	function _.pchconfig(cfg)			
		if not cfg.flags.NoPCH and cfg.pchheader then
			_p('  PCH        = %s', _MAKE.esc(cfg.pchheader))
			_p('  GCH        = $(OBJDIR)/%s.gch', _MAKE.esc(path.getname(cfg.pchheader))) 
			_p('  PCHINCLUDES = -I$(OBJDIR) -include $(OBJDIR)/%s', _MAKE.esc(path.getname(cfg.pchheader)))
		end
	end

	function _.pchrules(prj)
		_p('ifneq (,$(PCH))')
		_p('$(GCH): $(PCH) | $(OBJDIR)')
		_p('\t@echo $(notdir $<)')
		_p('\t-$(SILENT) cp $< $(OBJDIR)')
		if prj.language == "C" then
			_p('\t$(SILENT) $(CC) $(CFLAGS) -o "$@" -c "$<"')
		else
			_p('\t$(SILENT) $(CXX) $(CXXFLAGS) -x c++-header -o "$@" -c "$<"')
		end
		_p('endif')
		_p('')
	end
