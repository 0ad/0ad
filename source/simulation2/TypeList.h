/* Copyright (C) 2010 Wildfire Games.
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

// MESSAGE: message types
// INTERFACE: component interface types
// COMPONENT: component types

// Components intended only for use in test cases:
// (The tests rely on the enum IDs, so don't change the order of these)
INTERFACE(Test1)
COMPONENT(Test1A)
COMPONENT(Test1B)
COMPONENT(Test1Scripted)
INTERFACE(Test2)
COMPONENT(Test2A)
COMPONENT(Test2Scripted)

// Message types:
MESSAGE(TurnStart)
MESSAGE(Update)
MESSAGE(Update_MotionFormation)
MESSAGE(Update_MotionUnit)
MESSAGE(Update_Final)
MESSAGE(Interpolate) // non-deterministic (use with caution)
MESSAGE(RenderSubmit) // non-deterministic (use with caution)
MESSAGE(Create)
MESSAGE(Destroy)
MESSAGE(OwnershipChanged)
MESSAGE(PositionChanged)
MESSAGE(MotionChanged)
MESSAGE(RangeUpdate)
MESSAGE(TerrainChanged)
MESSAGE(PathResult)

// TemplateManager must come before all other (non-test) components,
// so that it is the first to be (de)serialized
INTERFACE(TemplateManager)
COMPONENT(TemplateManager)

// Special component for script component types with no native interface
INTERFACE(UnknownScript)
COMPONENT(UnknownScript)

// In alphabetical order:

INTERFACE(AIInterface)
COMPONENT(AIInterfaceScripted)

INTERFACE(AIManager)
COMPONENT(AIManager)

INTERFACE(AIProxy)
COMPONENT(AIProxyScripted)

INTERFACE(CommandQueue)
COMPONENT(CommandQueue)

INTERFACE(Decay)
COMPONENT(Decay)

INTERFACE(Footprint)
COMPONENT(Footprint)

INTERFACE(GuiInterface)
COMPONENT(GuiInterfaceScripted)

INTERFACE(Minimap)
COMPONENT(Minimap)

INTERFACE(Motion)
COMPONENT(MotionBall)
COMPONENT(MotionScripted)

INTERFACE(Obstruction)
COMPONENT(Obstruction)

INTERFACE(ObstructionManager)
COMPONENT(ObstructionManager)

INTERFACE(OverlayRenderer)
COMPONENT(OverlayRenderer)

INTERFACE(Ownership)
COMPONENT(Ownership)

INTERFACE(Pathfinder)
COMPONENT(Pathfinder)

INTERFACE(Player)
COMPONENT(PlayerScripted)

INTERFACE(PlayerManager)
COMPONENT(PlayerManagerScripted)

INTERFACE(Position)
COMPONENT(Position) // must be before VisualActor

INTERFACE(ProjectileManager)
COMPONENT(ProjectileManager)

INTERFACE(RangeManager)
COMPONENT(RangeManager)

INTERFACE(Selectable)
COMPONENT(Selectable)

INTERFACE(SoundManager)
COMPONENT(SoundManager)

INTERFACE(Terrain)
COMPONENT(Terrain)

INTERFACE(UnitMotion)
COMPONENT(UnitMotion) // must be after Obstruction

INTERFACE(Vision)
COMPONENT(Vision)

INTERFACE(Visual)
COMPONENT(VisualActor)

INTERFACE(WaterManager)
COMPONENT(WaterManager)
