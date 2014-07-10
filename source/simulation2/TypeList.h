/* Copyright (C) 2014 Wildfire Games.
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
MESSAGE(ProgressiveLoad) // non-deterministic (use with caution)
MESSAGE(Deserialized) // non-deterministic (use with caution)
MESSAGE(Create)
MESSAGE(Destroy)
MESSAGE(OwnershipChanged)
MESSAGE(PositionChanged)
MESSAGE(InterpolatedPositionChanged)
MESSAGE(TerritoryPositionChanged)
MESSAGE(MotionChanged)
MESSAGE(RangeUpdate)
MESSAGE(TerrainChanged)
MESSAGE(VisibilityChanged)
MESSAGE(WaterChanged)
MESSAGE(ObstructionMapShapeChanged)
MESSAGE(TerritoriesChanged)
MESSAGE(PathResult)
MESSAGE(ValueModification)
MESSAGE(TemplateModification)
MESSAGE(VisionRangeChanged)
MESSAGE(MinimapPing)

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

INTERFACE(CommandQueue)
COMPONENT(CommandQueue)

INTERFACE(Decay)
COMPONENT(Decay)

// Note: The VisualActor component relies on this component being initialized before itself, in order to support using
// an entity's footprint shape for the selection boxes. This dependency is not strictly necessary, but it does avoid
// some extra plumbing code to set up on-demand initialization. If you find yourself forced to break this dependency, 
// see VisualActor's Init method for a description of how you can avoid it.
INTERFACE(Footprint)
COMPONENT(Footprint)

INTERFACE(GuiInterface)
COMPONENT(GuiInterfaceScripted)

INTERFACE(Identity)
COMPONENT(IdentityScripted)

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

INTERFACE(ParticleManager)
COMPONENT(ParticleManager)

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

INTERFACE(RallyPointRenderer)
COMPONENT(RallyPointRenderer)

INTERFACE(RangeManager)
COMPONENT(RangeManager)

INTERFACE(Selectable)
COMPONENT(Selectable)

INTERFACE(Settlement)
COMPONENT(SettlementScripted)

INTERFACE(SoundManager)
COMPONENT(SoundManager)

INTERFACE(ValueModificationManager)
COMPONENT(ValueModificationManagerScripted)

INTERFACE(TechnologyTemplateManager)
COMPONENT(TechnologyTemplateManagerScripted)

INTERFACE(Terrain)
COMPONENT(Terrain)

INTERFACE(TerritoryInfluence)
COMPONENT(TerritoryInfluence)

INTERFACE(TerritoryManager)
COMPONENT(TerritoryManager)

INTERFACE(UnitMotion)
COMPONENT(UnitMotion) // must be after Obstruction
COMPONENT(UnitMotionScripted)

INTERFACE(UnitRenderer)
COMPONENT(UnitRenderer)

INTERFACE(Vision)
COMPONENT(Vision)

// Note: this component relies on the Footprint component being initialized before itself. See the comments above for
// the Footprint component to find out why.
INTERFACE(Visual)
COMPONENT(VisualActor) // must be after Ownership (dependency in Deserialize) and Vision (dependency in Init)

INTERFACE(WaterManager)
COMPONENT(WaterManager)
