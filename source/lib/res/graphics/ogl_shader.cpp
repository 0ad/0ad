/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * load and link together shaders; provides hotload support.
 */

#include "precompiled.h"
#include "ogl_shader.h"

#include "lib/ogl.h"

#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/XML/Xeromyces.h"

#include "lib/res/h_mgr.h"
#include "lib/file/vfs/vfs.h"
extern PIVFS g_VFS;


#define LOG_CATEGORY "shaders"


ERROR_ASSOCIATE(ERR::SHDR_CREATE, "Shader creation failed", -1);
ERROR_ASSOCIATE(ERR::SHDR_COMPILE, "Shader compile failed", -1);
ERROR_ASSOCIATE(ERR::SHDR_NO_SHADER, "Invalid shader reference", -1);
ERROR_ASSOCIATE(ERR::SHDR_LINK, "Shader linking failed", -1);
ERROR_ASSOCIATE(ERR::SHDR_NO_PROGRAM, "Invalid shader program reference", -1);


// Convert a shader object type into a descriptive string.
// If the type enum is not known, the given buffer is used as scratch space
// to format the type number. If buf is null, a generic string is returned.
static const char* shader_type_to_string(GLenum type, char* buf, size_t buflen)
{
	switch(type)
	{
	case GL_VERTEX_SHADER_ARB: return "VERTEX_SHADER";
	case GL_FRAGMENT_SHADER_ARB: return "FRAGMENT_SHADER";
	}
	
	if (!buf)
		return "unknown type enum";
	
	snprintf(buf, buflen, "%u", type);
	return buf;
}

// Return the OpenGL shader type enum for the given string,
// or 0 if the shader type is not known.
static GLenum string_to_shader_type(const char* name)
{
	if (!strcasecmp(name, "VERTEX_SHADER"))
		return GL_VERTEX_SHADER_ARB;
	if (!strcasecmp(name, "FRAGMENT_SHADER"))
		return GL_FRAGMENT_SHADER_ARB;
	return 0;
}


//----------------------------------------------------------------------------
// Handle type implementation

// Data for an Ogl_Shader object
struct Ogl_Shader {
	// Type of shader (e.g. GL_VERTEX_SHADER_ARB)
	GLenum type;

	// ID of the OpenGL shader object
	GLhandleARB id;
};


H_TYPE_DEFINE(Ogl_Shader);


// One-time initialization, called once by h_alloc, which is
// in turn called by ogl_shader_load
static void Ogl_Shader_init(Ogl_Shader* shdr, va_list args)
{
	shdr->type = va_arg(args, GLenum);
}


// Reload the shader object from the source file.
//
// TODO: The OpenGL specification says that all changes to shader objects
// have absolutely no effect on a program object that contains these shaders
// when the program object is already linked.
// So, how can we inform the "parent object" (i.e. the program object) of our change?
static LibError Ogl_Shader_reload(Ogl_Shader* shdr, const VfsPath& pathname, Handle UNUSED(h))
{
	LibError err  = ERR::FAIL;

	if (shdr->id)
		return INFO::OK;

	shared_ptr<u8> file; size_t file_size;
	RETURN_ERR(g_VFS->LoadFile(pathname, file, file_size));

	ogl_WarnIfError();

	shdr->id = pglCreateShaderObjectARB(shdr->type);
	if (!shdr->id)
	{
		// May be out of memory, but bad shdr->type is also possible.
		// In any case, checking OpenGL error state will help spot
		// bad code.
		ogl_WarnIfError();
		
		WARN_RETURN(ERR::SHDR_CREATE);
	}
	
	{
		const GLchar* strings[] = { (const GLchar*)file.get() };
		const GLint tmp = (GLint)file_size;
		pglShaderSourceARB(shdr->id, 1, strings, &tmp);
		// Some drivers (Mesa i915 on 945GM) give GL_INVALID_ENUM after calling
		// CompileShader on a GL_FRAGMENT_PROGRAM, because the hardware doesn't support
		// fragment programs. I can't find a better way to detect that situation in advance,
		// so detect the error afterwards and return failure.
		ogl_WarnIfError();
		pglCompileShaderARB(shdr->id);
		if(ogl_SquelchError(GL_INVALID_ENUM))
			goto fail_shadercreated;
	}
	
	GLint log_length;
	GLint compile_success;
	pglGetShaderiv(shdr->id, GL_OBJECT_COMPILE_STATUS_ARB, &compile_success);
	pglGetShaderiv(shdr->id, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
	if (log_length > 1)
	{
		char typenamebuf[32];
		char* infolog = new char[log_length];
		
		pglGetShaderInfoLog(shdr->id, log_length, 0, infolog);
	
		debug_printf("Compile log for shader %s (type %s):\n%s",
				pathname.string().c_str(),
				shader_type_to_string(shdr->type, typenamebuf, ARRAY_SIZE(typenamebuf)),
				infolog);
		
		delete[] infolog;
	}

	if (!compile_success)
	{
		// Compilation failure caused by syntax errors and similar
		// errors at the GLSL level does not set OpenGL error state
		// according to the spec, but this might still prove to be
		// useful some time.
		ogl_WarnIfError();
	
		char typenamebuf[32];
		debug_printf("Failed to compile shader %s (type %s)\n",
			     pathname.string().c_str(),
			     shader_type_to_string(shdr->type, typenamebuf, ARRAY_SIZE(typenamebuf)));
		
		err = ERR::SHDR_COMPILE;
		goto fail_shadercreated;
	}

	return INFO::OK;

fail_shadercreated:
	pglDeleteObjectARB(shdr->id);
	shdr->id = 0;
	return err;
}


// Free associated resources
static void Ogl_Shader_dtor(Ogl_Shader* shdr)
{
	// shdr->id is 0 when reload has failed
	if (shdr->id)
	{
		pglDeleteObjectARB(shdr->id);
		shdr->id = 0;
	}
}

static LibError Ogl_Shader_validate(const Ogl_Shader* UNUSED(shdr))
{
	// TODO
	return INFO::OK;
}

static LibError Ogl_Shader_to_string(const Ogl_Shader* UNUSED(shdr), char* buf)
{
	snprintf(buf, H_STRING_LEN, "-");
	return INFO::OK;
}


//----------------------------------------------------------------------------
// Public API

// Create, load and compile a shader object of the given type
// (e.g. GL_VERTEX_SHADER_ARB). The given file will be used as
// source code for the shader.
Handle ogl_shader_load(const VfsPath& pathname, GLenum type)
{
	return h_alloc(H_Ogl_Shader, pathname, 0, type);
}


// Free all resources associated with the given handle (subject
// to refcounting).
void ogl_shader_free(Handle& h)
{
	h_free(h, H_Ogl_Shader);
}

// Attach a shader to the given OpenGL program.
LibError ogl_shader_attach(GLhandleARB program, Handle& h)
{
	H_DEREF(h, Ogl_Shader, shdr);

	if (!shdr->id)
		WARN_RETURN(ERR::SHDR_NO_SHADER);

	pglAttachObjectARB(program, shdr->id);

	return INFO::OK;
}



//----------------------------------------------------------------------------
// Program type implementation

struct Ogl_Program {
	// ID of the OpenGL program object
	GLhandleARB id;
};


H_TYPE_DEFINE(Ogl_Program);


// One-time initialization, called once by h_alloc, which is
// in turn called by ogl_program_load
static void Ogl_Program_init(Ogl_Program* UNUSED(p), va_list UNUSED(args))
{
}


// Load the shader associated with one Shader element,
// and attach it to our program object.
static LibError do_load_shader(
		Ogl_Program* p, const VfsPath& pathname, Handle UNUSED(h),
		const CXeromyces& XeroFile, const XMBElement& Shader)
{
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	AT(type);
#undef AT
	
	CStr Type = Shader.GetAttributes().GetNamedItem(at_type);

	if (Type.empty())
	{
		LOG(CLogger::Error, LOG_CATEGORY, "%s: Missing attribute \"type\" in element \"Shader\".", pathname.string().c_str());
		WARN_RETURN(ERR::CORRUPTED);
	}

	GLenum shadertype = string_to_shader_type(Type.c_str());
	
	if (!shadertype)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "%s: Unknown shader type \"%s\" (valid are: VERTEX_SHADER, FRAGMENT_SHADER).", pathname.string().c_str(), Type.c_str());
		WARN_RETURN(ERR::CORRUPTED);
	}

	CStr Name = Shader.GetText();
	
	if (Name.empty())
	{
		LOG(CLogger::Error, LOG_CATEGORY, "%s: Missing shader name.", pathname.string().c_str());
		WARN_RETURN(ERR::CORRUPTED);
	}
	
	Handle hshader = ogl_shader_load(Name, shadertype);
	RETURN_ERR(hshader);

	ogl_shader_attach(p->id, hshader);

	// According to the OpenGL specification, a shader object's deletion
	// will not be final as long as the shader object is attached to a
	// container object.
	// TODO: How will this work with automatic reload?
	ogl_shader_free(hshader);

	return INFO::OK;
}


// Reload the program object from the source file.
static LibError Ogl_Program_reload(Ogl_Program* p, const VfsPath& pathname_, Handle h)
{
	if (p->id)
		return INFO::OK;

	ogl_WarnIfError();
	
	p->id = pglCreateProgramObjectARB();
	if (!p->id)
	{
		// The spec doesn't mention any error state that can be set
		// here, but it may still help spot bad code.
		ogl_WarnIfError();
		
		WARN_RETURN(ERR::SHDR_CREATE);
	}
	
	const char* pathname = pathname_.string().c_str();
	CXeromyces XeroFile;
	if (XeroFile.Load(pathname) != PSRETURN_OK)
		WARN_RETURN(ERR::CORRUPTED); // more informative error message?

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	EL(program);
	EL(shaders);
	EL(shader);
#undef EL

	XMBElement Root = XeroFile.GetRoot();

	if (Root.GetNodeName() != el_program)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "%s: XML root was not \"Program\".", pathname);
		WARN_RETURN(ERR::CORRUPTED);
	}

	XMBElementList RootChildren = Root.GetChildNodes();

	for(int i = 0; i < RootChildren.Count; ++i)
	{
		XMBElement Child = RootChildren.Item(i);
	
		int ChildName = Child.GetNodeName();
		if (ChildName == el_shaders)
		{
			XMBElementList Shaders = Child.GetChildNodes();
			
			for(int j = 0; j < Shaders.Count; ++j)
			{
				XMBElement Shader = Shaders.Item(j);
				
				if (Shader.GetNodeName() != el_shader)
				{
					LOG(CLogger::Error, LOG_CATEGORY, "%s: Only \"Shader\" may be child of \"Shaders\".", pathname);
					WARN_RETURN(ERR::CORRUPTED);
				}
				
				RETURN_ERR(do_load_shader(p, pathname, h, XeroFile, Shader));
			}
		}
		else
		{
			LOG(CLogger::Warning, LOG_CATEGORY, "%s: Unknown child of \"Program\".", pathname);
		}
	}

	pglLinkProgramARB(p->id);

	GLint log_length;
	GLint linked;
	
	pglGetProgramiv(p->id, GL_OBJECT_LINK_STATUS_ARB, &linked);
	pglGetProgramiv(p->id, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
	if (log_length > 1)
	{
		char* infolog = new char[log_length];
		pglGetProgramInfoLog(p->id, log_length, 0, infolog);

		debug_printf("Linker log for %s:\n%s\n", pathname, infolog);
		delete[] infolog;
	}

	if (!linked)
	{
		debug_printf("Link failed for %s\n", pathname);
		WARN_RETURN(ERR::SHDR_LINK);
	}

	return INFO::OK;
}


// Free associated resources
static void Ogl_Program_dtor(Ogl_Program* p)
{
	if (p->id)
	{
		pglDeleteObjectARB(p->id);
		p->id = 0;
	}
}

static LibError Ogl_Program_validate(const Ogl_Program* UNUSED(p))
{
	// TODO
	return INFO::OK;
}

static LibError Ogl_Program_to_string(const Ogl_Program* UNUSED(p), char* buf)
{
	snprintf(buf, H_STRING_LEN, "-");
	return INFO::OK;
}


//----------------------------------------------------------------------------
// Public API

// Load a program object based on the given XML file description.
// Shader objects are loaded and attached automatically.
Handle ogl_program_load(const char* fn)
{
	return h_alloc(H_Ogl_Program, fn, 0);
}

// Free all resources associated with the given program handle.
void ogl_program_free(Handle& h)
{
	h_free(h, H_Ogl_Program);
}


// Activate the program (glUseProgramObjectARB).
// h may be 0, in which case program objects are disabled.
LibError ogl_program_use(Handle h)
{
	if (!h)
	{
		pglUseProgramObjectARB(0);
		return INFO::OK;
	}

	Ogl_Program* p = H_USER_DATA(h, Ogl_Program);
	if (!p || !p->id)
	{
		pglUseProgramObjectARB(0);
		WARN_RETURN(ERR::INVALID_HANDLE);
	}

	pglUseProgramObjectARB(p->id);
	return INFO::OK;
}


// Query uniform information
GLint ogl_program_get_uniform_location(Handle h, const char* name)
{
	H_DEREF(h, Ogl_Program, p);
	
	return pglGetUniformLocationARB(p->id, name);
}


// Query vertex attribute information
GLint ogl_program_get_attrib_location(Handle h, const char* name)
{
	H_DEREF(h, Ogl_Program, p);
	
	return pglGetAttribLocationARB(p->id, name);
}
