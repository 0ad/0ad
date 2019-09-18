/* Copyright (C) 2019 Wildfire Games.
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

#include <boost/random/uniform_real_distribution.hpp>


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
	float Evaluate(CParticleEmitter& emitter)
	{
		m_LastValue = Compute(*emitter.m_Type, emitter);
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
	 * Returns the minimum value that Evaluate might ever return,
	 * for computing bounds.
	 */
	virtual float Min(CParticleEmitterType& type) = 0;

	/**
	 * Returns the maximum value that Evaluate might ever return,
	 * for computing bounds.
	 */
	virtual float Max(CParticleEmitterType& type) = 0;

protected:
	virtual float Compute(CParticleEmitterType& type, CParticleEmitter& emitter) = 0;

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

	virtual float Compute(CParticleEmitterType& UNUSED(type), CParticleEmitter& UNUSED(emitter))
	{
		return m_Value;
	}

	virtual float Min(CParticleEmitterType& UNUSED(type))
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

	virtual float Compute(CParticleEmitterType& type, CParticleEmitter& UNUSED(emitter))
	{
		return boost::random::uniform_real_distribution<float>(m_Min, m_Max)(type.m_Manager.m_RNG);
	}

	virtual float Min(CParticleEmitterType& UNUSED(type))
	{
		return m_Min;
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

	virtual float Compute(CParticleEmitterType& type, CParticleEmitter& UNUSED(emitter))
	{
		return type.m_Variables[m_From]->LastValue();
	}

	virtual float Min(CParticleEmitterType& type)
	{
		return type.m_Variables[m_From]->Min(type);
	}

	virtual float Max(CParticleEmitterType& type)
	{
		return type.m_Variables[m_From]->Max(type);
	}

private:
	int m_From;
};

/**
 * A terrible ad-hoc attempt at handling some particular variable calculation,
 * which really needs to be cleaned up and generalised.
 */
class CParticleVarExpr : public IParticleVar
{
public:
	CParticleVarExpr(const CStr& from, float mul, float max) :
		m_From(from), m_Mul(mul), m_Max(max)
	{
	}

	virtual float Compute(CParticleEmitterType& UNUSED(type), CParticleEmitter& emitter)
	{
		return std::min(m_Max, emitter.m_EntityVariables[m_From] * m_Mul);
	}

	virtual float Min(CParticleEmitterType& UNUSED(type))
	{
		return 0.f;
	}

	virtual float Max(CParticleEmitterType& UNUSED(type))
	{
		return m_Max;
	}

private:
	CStr m_From;
	float m_Mul;
	float m_Max;
};



/**
 * Interface for particle effectors, which get evaluated every frame to
 * update particles.
 */
class IParticleEffector
{
public:
	IParticleEffector() { }
	virtual ~IParticleEffector() {}

	/// Updates all particles.
	virtual void Evaluate(std::vector<SParticle>& particles, float dt) = 0;

	/// Returns maximum acceleration caused by this effector.
	virtual CVector3D Max() = 0;

};

/**
 * Particle effector that applies a constant acceleration.
 */
class CParticleEffectorForce : public IParticleEffector
{
public:
	CParticleEffectorForce(float x, float y, float z) :
		m_Accel(x, y, z)
	{
	}

	virtual void Evaluate(std::vector<SParticle>& particles, float dt)
	{
		CVector3D dv = m_Accel * dt;

		for (size_t i = 0; i < particles.size(); ++i)
			particles[i].velocity += dv;
	}

	virtual CVector3D Max()
	{
		return m_Accel;
	}

private:
	CVector3D m_Accel;
};




CParticleEmitterType::CParticleEmitterType(const VfsPath& path, CParticleManager& manager) :
	m_Manager(manager)
{
	LoadXML(path);
	// TODO: handle load failure

	// Upper bound on number of particles depends on maximum rate and lifetime
	m_MaxLifetime = m_Variables[VAR_LIFETIME]->Max(*this);
	m_MaxParticles = ceil(m_Variables[VAR_EMISSIONRATE]->Max(*this) * m_MaxLifetime);


	// Compute the worst-case bounds of all possible particles,
	// based on the min/max values of positions and velocities and accelerations
	// and sizes. (This isn't a guaranteed bound but it should be sufficient for
	// culling.)

	// Assuming constant acceleration,
	//        p = p0 + v0*t + 1/2 a*t^2
	// => dp/dt = v0 + a*t
	//          = 0 at t = -v0/a
	// max(p) is at t=0, or t=tmax, or t=-v0/a if that's between 0 and tmax
	// => max(p) = max(p0, p0 + v0*tmax + 1/2 a*tmax, p0 - 1/2 v0^2/a)

	// Compute combined acceleration (assume constant)
	CVector3D accel;
	for (size_t i = 0; i < m_Effectors.size(); ++i)
		accel += m_Effectors[i]->Max();

	CVector3D vmin(m_Variables[VAR_VELOCITY_X]->Min(*this), m_Variables[VAR_VELOCITY_Y]->Min(*this), m_Variables[VAR_VELOCITY_Z]->Min(*this));
	CVector3D vmax(m_Variables[VAR_VELOCITY_X]->Max(*this), m_Variables[VAR_VELOCITY_Y]->Max(*this), m_Variables[VAR_VELOCITY_Z]->Max(*this));

	// Start by assuming p0 = 0; compute each XYZ component individually
	m_MaxBounds.SetEmpty();
	// Lower/upper bounds at t=0, t=tmax
	m_MaxBounds[0].X = std::min(0.f, vmin.X*m_MaxLifetime + 0.5f*accel.X*m_MaxLifetime*m_MaxLifetime);
	m_MaxBounds[0].Y = std::min(0.f, vmin.Y*m_MaxLifetime + 0.5f*accel.Y*m_MaxLifetime*m_MaxLifetime);
	m_MaxBounds[0].Z = std::min(0.f, vmin.Z*m_MaxLifetime + 0.5f*accel.Z*m_MaxLifetime*m_MaxLifetime);
	m_MaxBounds[1].X = std::max(0.f, vmax.X*m_MaxLifetime + 0.5f*accel.X*m_MaxLifetime*m_MaxLifetime);
	m_MaxBounds[1].Y = std::max(0.f, vmax.Y*m_MaxLifetime + 0.5f*accel.Y*m_MaxLifetime*m_MaxLifetime);
	m_MaxBounds[1].Z = std::max(0.f, vmax.Z*m_MaxLifetime + 0.5f*accel.Z*m_MaxLifetime*m_MaxLifetime);
	// Extend bounds to include position at t where dp/dt=0, if 0 < t < tmax
	if (accel.X && 0 < -vmin.X/accel.X && -vmin.X/accel.X < m_MaxLifetime)
		m_MaxBounds[0].X = std::min(m_MaxBounds[0].X, -0.5f*vmin.X*vmin.X / accel.X);
	if (accel.Y && 0 < -vmin.Y/accel.Y && -vmin.Y/accel.Y < m_MaxLifetime)
		m_MaxBounds[0].Y = std::min(m_MaxBounds[0].Y, -0.5f*vmin.Y*vmin.Y / accel.Y);
	if (accel.Z && 0 < -vmin.Z/accel.Z && -vmin.Z/accel.Z < m_MaxLifetime)
		m_MaxBounds[0].Z = std::min(m_MaxBounds[0].Z, -0.5f*vmin.Z*vmin.Z / accel.Z);
	if (accel.X && 0 < -vmax.X/accel.X && -vmax.X/accel.X < m_MaxLifetime)
		m_MaxBounds[1].X = std::max(m_MaxBounds[1].X, -0.5f*vmax.X*vmax.X / accel.X);
	if (accel.Y && 0 < -vmax.Y/accel.Y && -vmax.Y/accel.Y < m_MaxLifetime)
		m_MaxBounds[1].Y = std::max(m_MaxBounds[1].Y, -0.5f*vmax.Y*vmax.Y / accel.Y);
	if (accel.Z && 0 < -vmax.Z/accel.Z && -vmax.Z/accel.Z < m_MaxLifetime)
		m_MaxBounds[1].Z = std::max(m_MaxBounds[1].Z, -0.5f*vmax.Z*vmax.Z / accel.Z);

	// Offset by the initial positions
	m_MaxBounds[0] += CVector3D(m_Variables[VAR_POSITION_X]->Min(*this), m_Variables[VAR_POSITION_Y]->Min(*this), m_Variables[VAR_POSITION_Z]->Min(*this));
	m_MaxBounds[1] += CVector3D(m_Variables[VAR_POSITION_X]->Max(*this), m_Variables[VAR_POSITION_Y]->Max(*this), m_Variables[VAR_POSITION_Z]->Max(*this));
}

int CParticleEmitterType::GetVariableID(const std::string& name)
{
	if (name == "emissionrate")		return VAR_EMISSIONRATE;
	if (name == "lifetime")			return VAR_LIFETIME;
	if (name == "position.x")		return VAR_POSITION_X;
	if (name == "position.y")		return VAR_POSITION_Y;
	if (name == "position.z")		return VAR_POSITION_Z;
	if (name == "angle")			return VAR_ANGLE;
	if (name == "velocity.x")		return VAR_VELOCITY_X;
	if (name == "velocity.y")		return VAR_VELOCITY_Y;
	if (name == "velocity.z")		return VAR_VELOCITY_Z;
	if (name == "velocity.angle")	return VAR_VELOCITY_ANGLE;
	if (name == "size")				return VAR_SIZE;
	if (name == "size.growthRate")	return VAR_SIZE_GROWTHRATE;
	if (name == "color.r")			return VAR_COLOR_R;
	if (name == "color.g")			return VAR_COLOR_G;
	if (name == "color.b")			return VAR_COLOR_B;
	LOGWARNING("Particle sets unknown variable '%s'", name.c_str());
	return -1;
}

bool CParticleEmitterType::LoadXML(const VfsPath& path)
{
	// Initialise with sane defaults
	m_Variables.clear();
	m_Variables.resize(VAR__MAX);
	m_Variables[VAR_EMISSIONRATE] = IParticleVarPtr(new CParticleVarConstant(10.f));
	m_Variables[VAR_LIFETIME] = IParticleVarPtr(new CParticleVarConstant(3.f));
	m_Variables[VAR_POSITION_X] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_POSITION_Y] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_POSITION_Z] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_ANGLE] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_VELOCITY_X] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_VELOCITY_Y] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_VELOCITY_Z] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_VELOCITY_ANGLE] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_SIZE] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_SIZE_GROWTHRATE] = IParticleVarPtr(new CParticleVarConstant(0.f));
	m_Variables[VAR_COLOR_R] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_COLOR_G] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_Variables[VAR_COLOR_B] = IParticleVarPtr(new CParticleVarConstant(1.f));
	m_BlendEquation = GL_FUNC_ADD;
	m_BlendFuncSrc = GL_SRC_ALPHA;
	m_BlendFuncDst = GL_ONE_MINUS_SRC_ALPHA;
	m_StartFull = false;
	m_UseRelativeVelocity = false;
	m_Texture = g_Renderer.GetTextureManager().GetErrorTexture();

	CXeromyces XeroFile;
	PSRETURN ret = XeroFile.Load(g_VFS, path, "particle");
	if (ret != PSRETURN_OK)
		return false;

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(texture);
	EL(blend);
	EL(start_full);
	EL(use_relative_velocity);
	EL(constant);
	EL(uniform);
	EL(copy);
	EL(expr);
	EL(force);
	AT(mode);
	AT(name);
	AT(value);
	AT(min);
	AT(max);
	AT(mul);
	AT(from);
	AT(x);
	AT(y);
	AT(z);
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
		else if (Child.GetNodeName() == el_start_full)
		{
			m_StartFull = true;
		}
		else if (Child.GetNodeName() == el_use_relative_velocity)
		{
			m_UseRelativeVelocity = true;
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
				float min = Child.GetAttributes().GetNamedItem(at_min).ToFloat();
				float max = Child.GetAttributes().GetNamedItem(at_max).ToFloat();
				// To avoid hangs in the RNG, only use it if [min, max) is non-empty
				if (min < max)
					m_Variables[id] = IParticleVarPtr(new CParticleVarUniform(min, max));
				else
					m_Variables[id] = IParticleVarPtr(new CParticleVarConstant(min));
			}
		}
		else if (Child.GetNodeName() == el_copy)
		{
			int id = GetVariableID(Child.GetAttributes().GetNamedItem(at_name));
			int from = GetVariableID(Child.GetAttributes().GetNamedItem(at_from));
			if (id != -1 && from != -1)
				m_Variables[id] = IParticleVarPtr(new CParticleVarCopy(from));
		}
		else if (Child.GetNodeName() == el_expr)
		{
			int id = GetVariableID(Child.GetAttributes().GetNamedItem(at_name));
			CStr from = Child.GetAttributes().GetNamedItem(at_from);
			float mul = Child.GetAttributes().GetNamedItem(at_mul).ToFloat();
			float max = Child.GetAttributes().GetNamedItem(at_max).ToFloat();
			if (id != -1)
				m_Variables[id] = IParticleVarPtr(new CParticleVarExpr(from, mul, max));
		}
		else if (Child.GetNodeName() == el_force)
		{
			float x = Child.GetAttributes().GetNamedItem(at_x).ToFloat();
			float y = Child.GetAttributes().GetNamedItem(at_y).ToFloat();
			float z = Child.GetAttributes().GetNamedItem(at_z).ToFloat();
			m_Effectors.push_back(IParticleEffectorPtr(new CParticleEffectorForce(x, y, z)));
		}
	}

	return true;
}

void CParticleEmitterType::UpdateEmitter(CParticleEmitter& emitter, float dt)
{
	// If dt is very large, we should do the update in multiple small
	// steps to prevent all the particles getting clumped together at
	// low framerates

	const float maxStepLength = 0.2f;

	// Avoid wasting time by computing periods longer than the lifetime
	// period of the particles
	dt = std::min(dt, m_MaxLifetime);

	while (dt > maxStepLength)
	{
		UpdateEmitterStep(emitter, maxStepLength);
		dt -= maxStepLength;
	}

	UpdateEmitterStep(emitter, dt);
}

void CParticleEmitterType::UpdateEmitterStep(CParticleEmitter& emitter, float dt)
{
	ENSURE(emitter.m_Type.get() == this);

	if (emitter.m_Active)
	{
		float emissionRate = m_Variables[VAR_EMISSIONRATE]->Evaluate(emitter);

		// Find how many new particles to spawn, and accumulate any rounding errors
		// (to maintain a constant emission rate even if dt is very small)
		int newParticles = floor(emitter.m_EmissionRoundingError + dt*emissionRate);
		emitter.m_EmissionRoundingError += dt*emissionRate - newParticles;

		// If dt was very large, there's no point spawning new particles that
		// we'll immediately overwrite, so clamp it
		newParticles = std::min(newParticles, (int)m_MaxParticles);

		for (int i = 0; i < newParticles; ++i)
		{
			// Compute new particle state based on variables
			SParticle particle;

			particle.pos.X = m_Variables[VAR_POSITION_X]->Evaluate(emitter);
			particle.pos.Y = m_Variables[VAR_POSITION_Y]->Evaluate(emitter);
			particle.pos.Z = m_Variables[VAR_POSITION_Z]->Evaluate(emitter);
			particle.pos += emitter.m_Pos;

			if (m_UseRelativeVelocity)
			{
				float xVel = m_Variables[VAR_VELOCITY_X]->Evaluate(emitter);
				float yVel = m_Variables[VAR_VELOCITY_Y]->Evaluate(emitter);
				float zVel = m_Variables[VAR_VELOCITY_Z]->Evaluate(emitter);
				CVector3D EmitterAngle = emitter.GetRotation().ToMatrix().Transform(CVector3D(xVel,yVel,zVel));
				particle.velocity.X = EmitterAngle.X;
				particle.velocity.Y = EmitterAngle.Y;
				particle.velocity.Z = EmitterAngle.Z;
			} else {
				particle.velocity.X = m_Variables[VAR_VELOCITY_X]->Evaluate(emitter);
				particle.velocity.Y = m_Variables[VAR_VELOCITY_Y]->Evaluate(emitter);
				particle.velocity.Z = m_Variables[VAR_VELOCITY_Z]->Evaluate(emitter);
			}
			particle.angle = m_Variables[VAR_ANGLE]->Evaluate(emitter);
			particle.angleSpeed = m_Variables[VAR_VELOCITY_ANGLE]->Evaluate(emitter);

			particle.size = m_Variables[VAR_SIZE]->Evaluate(emitter);
			particle.sizeGrowthRate = m_Variables[VAR_SIZE_GROWTHRATE]->Evaluate(emitter);

			RGBColor color;
			color.X = m_Variables[VAR_COLOR_R]->Evaluate(emitter);
			color.Y = m_Variables[VAR_COLOR_G]->Evaluate(emitter);
			color.Z = m_Variables[VAR_COLOR_B]->Evaluate(emitter);
			particle.color = ConvertRGBColorTo4ub(color);

			particle.age = 0.f;
			particle.maxAge = m_Variables[VAR_LIFETIME]->Evaluate(emitter);

			emitter.AddParticle(particle);
		}
	}

	// Update particle states
	for (size_t i = 0; i < emitter.m_Particles.size(); ++i)
	{
		SParticle& p = emitter.m_Particles[i];

		// Don't bother updating particles already at the end of their life
		if (p.age > p.maxAge)
			continue;

		p.pos += p.velocity * dt;
		p.angle += p.angleSpeed * dt;
		p.age += dt;
		p.size += p.sizeGrowthRate * dt;

		// Make alpha fade in/out nicely
		// TODO: this should probably be done as a variable or something,
		// instead of hardcoding
		float ageFrac = p.age / p.maxAge;
		float a = std::min(1.f - ageFrac, 5.f * ageFrac);
		p.color.A = Clamp(static_cast<int>(a * 255.f), 0, 255);
	}

	for (size_t i = 0; i < m_Effectors.size(); ++i)
	{
		m_Effectors[i]->Evaluate(emitter.m_Particles, dt);
	}
}

CBoundingBoxAligned CParticleEmitterType::CalculateBounds(CVector3D emitterPos, CBoundingBoxAligned emittedBounds)
{
	CBoundingBoxAligned bounds = m_MaxBounds;
	bounds[0] += emitterPos;
	bounds[1] += emitterPos;

	bounds += emittedBounds;

	// The current bounds is for the particles' centers, so expand by
	// sqrt(2) * max_size/2 to ensure any rotated billboards fit in
	bounds.Expand(m_Variables[VAR_SIZE]->Max(*this)/2.f * sqrt(2.f));

	return bounds;
}
