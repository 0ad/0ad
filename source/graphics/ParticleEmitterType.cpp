/* Copyright (C) 2011 Wildfire Games.
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

#include "precompiled.h"

#include "ParticleEmitterType.h"

#include "graphics/Color.h"
#include "graphics/ParticleEmitter.h"
#include "graphics/ParticleManager.h"
#include "graphics/TextureManager.h"
#include "lib/rand.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"

#include <boost/random/uniform_real.hpp>

/**
 * Interface for particle state variables, which get evaluated for each newly
 * constructed particle.
 */
class IParticleVar
{
public:
	IParticleVar() : m_LastValue(0) { }
	virtual ~IParticleVar() {}

	/// Computes and returns a new value.
	float Evaluate(CParticleEmitterType& type)
	{
		m_LastValue = Compute(type);
		return m_LastValue;
	}

	/**
	 * Returns the last value that Evaluate returned.
	 * This is used for variables that depend on other variables (which is kind
	 * of fragile since it's very order-dependent), so they don't get re-randomised
	 * and don't have a danger of infinite recursion.
	 */
	float LastValue() { return m_LastValue; }

	/**
	 * Returns the maximum value that Evaluate might ever return,
	 * for computing bounds.
	 */
	virtual float Max(CParticleEmitterType& type) = 0;

protected:
	virtual float Compute(CParticleEmitterType& type) = 0;

private:
	float m_LastValue;
};

/**
 * Particle variable that returns a constant value.
 */
class CParticleVarConstant : public IParticleVar
{
public:
	CParticleVarConstant(float val) :
		m_Value(val)
	{
	}

	virtual float Compute(CParticleEmitterType& UNUSED(type))
	{
		return m_Value;
	}

	virtual float Max(CParticleEmitterType& UNUSED(type))
	{
		return m_Value;
	}

private:
	float m_Value;
};

/**
 * Particle variable that returns a uniformly-distributed random value.
 */
class CParticleVarUniform : public IParticleVar
{
public:
	CParticleVarUniform(float min, float max) :
		m_Min(min), m_Max(max)
	{
	}

	virtual float Compute(CParticleEmitterType& type)
	{
		return boost::uniform_real<>(m_Min, m_Max)(type.m_Manager.m_RNG);
	}

	virtual float Max(CParticleEmitterType& UNUSED(type))
	{
		return m_Max;
	}

private:
	float m_Min;
	float m_Max;
};

/**
 * Particle variable that returns the same value as some other variable
 * (assuming that variable was evaluated before this one).
 */
class CParticleVarCopy : public IParticleVar
{
public:
	CParticleVarCopy(int from) :
		m_From(from)
	{
	}

	virtual float Compute(CParticleEmitterType& type)
	{
		return type.m_Variables[m_From]->LastValue();
	}

	virtual float Max(CParticleEmitterType& type)
	{
		return type.m_Variables[m_From]->Max(type);
	}

private:
	int m_From;
};


CParticleEmitterType::CParticleEmitterType(const VfsPath& path, CParticleManager& manager) :
	m_Manager(manager)
{
	LoadXML(path);
	// TODO: handle load failure

	// Upper bound on number of particles depends on maximum rate and lifetime
	m_MaxParticles = ceil(m_Variables[VAR_EMISSIONRATE]->Max(*this) * m_Variables[VAR_LIFETIME]->Max(*this));
}

int CParticleEmitterType::GetVariableID(const std::string& name)
{
	if (name == "emissionrate")	return VAR_EMISSIONRATE;
	if (name == "lifetime")		return VAR_LIFETIME;
	if (name == "angle")		return VAR_ANGLE;
	if (name == "velocity.x")	return VAR_VELOCITY_X;
	if (name == "velocity.y")	return VAR_VELOCITY_Y;
	if (name == "velocity.z")	return VAR_VELOCITY_Z;
	if (name == "velocity.angle")	return VAR_VELOCITY_ANGLE;
	if (name == "size")			return VAR_SIZE;
	if (name == "color.r")		return VAR_COLOR_R;
	if (name == "color.g")		return VAR_COLOR_G;
	if (name == "color.b")		return VAR_COLOR_B;
	LOGWARNING(L"Particle sets unknown variable '%hs'", name.c_str());
	return -1;
}

bool CParticleEmitterType::LoadXML(const VfsPath& path)
{
	// Initialise with sane defaults
	m_Variables.clear();
	m_Variables.resize(VAR__MAX);
	m_Variables[VAR_EMISSIONRATE] = IParticleVarPtr(new CParticleVarConstant(10.f));
	m_Variables[VAR_LIFETIME] = IParticleVarPtr(new CParticleVarConstant(3.f));
	m_Variables[VAR_ANGLE] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_VELOCITY_X] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_VELOCITY_Y] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_VELOCITY_Z] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_VELOCITY_ANGLE] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_SIZE] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_COLOR_R] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_COLOR_G] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_COLOR_B] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_BlendEquation = GL_FUNC_ADD;
	m_BlendFuncSrc = GL_SRC_ALPHA;
	m_BlendFuncDst = GL_ONE_MINUS_SRC_ALPHA;

	CXeromyces XeroFile;
	PSRETURN ret = XeroFile.Load(g_VFS, path);
	if (ret != PSRETURN_OK)
		return false;

	// TODO: should do some RNG schema validation

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(texture);
	EL(blend);
	EL(constant);
	EL(uniform);
	EL(copy);
	AT(mode);
	AT(name);
	AT(value);
	AT(min);
	AT(max);
	AT(from);
#undef AT
#undef EL

	XMBElement Root = XeroFile.GetRoot();

	XERO_ITER_EL(Root, Child)
	{
		if (Child.GetNodeName() == el_texture)
		{
			CTextureProperties textureProps(Child.GetText().FromUTF8());
			textureProps.SetWrap(GL_CLAMP_TO_EDGE);
			m_Texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		}
		else if (Child.GetNodeName() == el_blend)
		{
			CStr mode = Child.GetAttributes().GetNamedItem(at_mode);
			if (mode == "add")
			{
				m_BlendEquation = GL_FUNC_ADD;
				m_BlendFuncSrc = GL_SRC_ALPHA;
				m_BlendFuncDst = GL_ONE;
			}
			else if (mode == "subtract")
			{
				m_BlendEquation = GL_FUNC_REVERSE_SUBTRACT;
				m_BlendFuncSrc = GL_SRC_ALPHA;
				m_BlendFuncDst = GL_ONE;
			}
			else if (mode == "over")
			{
				m_BlendEquation = GL_FUNC_ADD;
				m_BlendFuncSrc = GL_SRC_ALPHA;
				m_BlendFuncDst = GL_ONE_MINUS_SRC_ALPHA;
			}
			else if (mode == "multiply")
			{
				m_BlendEquation = GL_FUNC_ADD;
				m_BlendFuncSrc = GL_ZERO;
				m_BlendFuncDst = GL_ONE_MINUS_SRC_COLOR;
			}
		}
		else if (Child.GetNodeName() == el_constant)
		{
			int id = GetVariableID(Child.GetAttributes().GetNamedItem(at_name));
			if (id != -1)
			{
				m_Variables[id] = IParticleVarPtr(new CParticleVarConstant(
					Child.GetAttributes().GetNamedItem(at_value).ToFloat()
				));
			}
		}
		else if (Child.GetNodeName() == el_uniform)
		{
			int id = GetVariableID(Child.GetAttributes().GetNamedItem(at_name));
			if (id != -1)
			{
				m_Variables[id] = IParticleVarPtr(new CParticleVarUniform(
					Child.GetAttributes().GetNamedItem(at_min).ToFloat(),
					Child.GetAttributes().GetNamedItem(at_max).ToFloat()
				));
			}
		}
		else if (Child.GetNodeName() == el_copy)
		{
			int id = GetVariableID(Child.GetAttributes().GetNamedItem(at_name));
			int from = GetVariableID(Child.GetAttributes().GetNamedItem(at_from));
			if (id != -1 && from != -1)
				m_Variables[id] = IParticleVarPtr(new CParticleVarCopy(from));
		}
	}

	return true;
}

void CParticleEmitterType::UpdateEmitter(CParticleEmitter& emitter, float dt)
{
	debug_assert(emitter.m_Type.get() == this);

	// TODO: if dt is very large, maybe we should do the update in multiple small
	// steps to prevent all the particles getting clumped together at low framerates

	if (emitter.m_Active)
	{
		float emissionRate = m_Variables[VAR_EMISSIONRATE]->Evaluate(*this);

		// Find how many new particles to spawn, and accumulate any rounding errors into EmissionTimer
		emitter.m_EmissionTimer += dt;
		int newParticles = floor(emitter.m_EmissionTimer * emissionRate);
		emitter.m_EmissionTimer -= newParticles / emissionRate;

		// If dt was very large, there's no point spawning new particles that
		// we'll immediately overwrite, so clamp it
		newParticles = std::min(newParticles, (int)m_MaxParticles);

		for (int i = 0; i < newParticles; ++i)
		{
			// Compute new particle state based on variables
			SParticle particle;
			particle.pos = emitter.m_Pos;
			particle.velocity.X = m_Variables[VAR_VELOCITY_X]->Evaluate(*this);
			particle.velocity.Y = m_Variables[VAR_VELOCITY_Y]->Evaluate(*this);
			particle.velocity.Z = m_Variables[VAR_VELOCITY_Z]->Evaluate(*this);
			particle.angle = m_Variables[VAR_ANGLE]->Evaluate(*this);
			particle.angleSpeed = m_Variables[VAR_VELOCITY_ANGLE]->Evaluate(*this);
			particle.size = m_Variables[VAR_SIZE]->Evaluate(*this);
			RGBColor color;
			color.X = m_Variables[VAR_COLOR_R]->Evaluate(*this);
			color.Y = m_Variables[VAR_COLOR_G]->Evaluate(*this);
			color.Z = m_Variables[VAR_COLOR_B]->Evaluate(*this);
			particle.color = ConvertRGBColorTo4ub(color);
			particle.age = 0.f;
			particle.maxAge = m_Variables[VAR_LIFETIME]->Evaluate(*this);

			emitter.AddParticle(particle);
		}
	}

	// Update particle states
	for (size_t i = 0; i < emitter.m_Particles.size(); ++i)
	{
		SParticle& p = emitter.m_Particles[i];
		p.pos += p.velocity * dt;
		p.angle += p.angleSpeed * dt;
		p.age += dt;

		// Make alpha fade in/out nicely
		// TODO: this should probably be done as a variable or something,
		// instead of hardcoding
		float ageFrac = p.age / p.maxAge;
		float a = std::min(1.f-ageFrac, 5.f*ageFrac);
		p.color.A = clamp((int)(a*255.f), 0, 255);
	}

	// TODO: maybe we want to add forces like gravity or wind
}
