--
-- tests/actions/vstudio/cs2005/test_output_props.lua
-- Test the target output settings of a Visual Studio 2005+ C# project.
-- Copyright (c) 2012-2013 Jason Perkins and the Premake project
--

local p = premake
local suite = test.declare("vstudio_cs2005_netcore_prj")
local dn2005 = p.vstudio.dotnetbase
local project = p.project


--
-- Setup and teardown
--

local wks, prj

function suite.setup()
    p.action.set("vs2005")
    wks, prj = test.createWorkspace()
    language "C#"
end

local function targetFrameworkPrepare()
    local cfg = test.getconfig(prj, "Debug")
    dn2005.netcore.targetFramework(cfg)
end

local function targetFrameworkVersionPrepare()
    local cfg = test.getconfig(prj, "Debug")
    dn2005.targetFrameworkVersion(cfg)
end

local function prepareNetcore()
    dn2005.projectElement(prj)
end

function suite.targetFrameworkProperty_framework()
    dotnetframework "4.7.2"
    targetFrameworkPrepare()
    test.isemptycapture()
end

function suite.targetFrameworkProperty_core()
    dotnetframework "netcoreapp2.2"
    targetFrameworkPrepare()
    test.capture [[
		<TargetFramework>netcoreapp2.2</TargetFramework>
	]]
end

function suite.targetFrameworkProperty_standard()
    dotnetframework "netstandard1.2"
    targetFrameworkPrepare()
    test.capture [[
		<TargetFramework>netstandard1.2</TargetFramework>
    ]]
end

function suite.targetFrameworkVersionProperty_framework()
    dotnetframework "4.7.2"
    targetFrameworkVersionPrepare()
    test.capture [[
		<TargetFrameworkVersion>v4.7.2</TargetFrameworkVersion>
    ]]
end

function suite.targetFrameworkVersionProperty_core()
    dotnetframework "netcoreapp2.2"
    targetFrameworkVersionPrepare()
    test.isemptycapture()
end

function suite.targetFrameworkVersionProperty_standard()
    dotnetframework "netstandard1.2"
    targetFrameworkVersionPrepare()
    test.isemptycapture()
end

function suite.project_element_standard()
    dotnetframework "netstandard1.2"
    prepareNetcore()
    test.capture [[
<Project Sdk="Microsoft.NET.Sdk">
    ]]
end

function suite.project_element_core()
    dotnetframework "netcoreapp1.2"
    prepareNetcore()
    test.capture [[
<Project Sdk="Microsoft.NET.Sdk">
    ]]
end

function suite.project_element_framework()
    dotnetframework "4.7.2"
    prepareNetcore()
    test.capture [[
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
    ]]
end
