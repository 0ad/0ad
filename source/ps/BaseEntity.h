// base entity for all the Units and Buildings in the game 0AD

// written by Jacob Ricketts

#if !defined( __0AD_BASE_ENTITY_H_ )
#define __0AD_BASE_ENTITY_H_

#include "ai_common.h"
#include <vector>
#include <iostream>

class DLLExport CBaseEntity
{
public:
	CBaseEntity( void ) { Clear(); };
	virtual ~CBaseEntity( void ) { };

	virtual void Clear( void ) { ClearBaseEntity(); };
	//virtual bool Update( void ) = 0;
	//virtual bool HandleMessage( const Message &msg ) = 0;

	// === ID FUNCTIONS ===
	inline
	USHORT	GetID( void ) { return( m_nID ); };

	inline
	USHORT	GetTeamID( void ) { return( m_byteTeamID ); };

	inline
	BYTE	AddAllies( BYTE bitflagTeams ) { return( m_bitflagAllies |= bitflagTeams ); };

	inline
	BYTE	RemoveAllies( BYTE bitflagTeams ) { return( m_bitflagAllies &= ~bitflagTeams ); };

			
	// === HIT POINT FUNCTIONS ===
	inline
	virtual USHORT	GetHitPoints( void ) { std::cout << "CBaseEntity.GetHitPoints() called..." << endl; return( m_nHitPoints ); };

	inline
	virtual USHORT	GetBaseHitPoints( void ) { return( m_nBaseHitPoints ); };

	inline
	virtual USHORT	GetMaxHitPoints( void ) { return( m_nMaxHitPoints ); };

	inline
	virtual short	GetRegenRate( void ) { return( m_nBaseRegenRate ); };


	// === ATTACK STAT FUNCTIONS ===
	inline
	virtual USHORT	GetAttackStrength( void ) { return( m_nBaseAttackStrength ); };

	inline
	virtual USHORT	GetAttackRate( void ) { return( m_nBaseAttackRate ); };

	inline
	virtual USHORT	GetAreaOfEffect( void ) { return( m_nBaseAreaOfEffect ); };

	inline
	virtual USHORT	GetMaxRange( void ) { return( m_nBaseMaxRange ); };

	inline
	virtual USHORT	GetMinRange( void ) { return( m_nBaseMinRange ); };

	inline
	virtual USHORT	GetMeleeArmor( void ) { return( m_nBaseMeleeArmor ); } ;

	inline
	virtual USHORT	GetPierceArmor( void ) { return( m_nBasePierceArmor ); };


	// === MISC ATTRIBUTES ===
	inline
	virtual const std::vector<USHORT>&	AvailableToCivs( void ) { return( m_vAvailableToCivs ); };

	inline
	virtual const Vector3D& GetPosition( void ) { return( m_vPos ); };

	inline
	virtual bool	BoostsMorale( void ) { return( m_bBoostsMorale ); };

	inline
	virtual bool	DemoralizesEnemies( void ) { return( m_bDemoralizesEnemies ); };

	inline
	virtual USHORT	GetActionRate( void ) { return( m_nBaseActionRate ); };

	inline
	virtual USHORT	GetAreaOfInfluence( void ) { return( m_nBaseAreaOfInfluence ); };

	inline
	virtual USHORT	GetLOS( void ) { return( m_nBaseLOS ); };	


	// === BUILD STATS ===
	inline
	virtual USHORT	GetBuildTime( void ) { return( m_nBuildTime ); };

	inline
	virtual USHORT	GetReqFoodToBuild( void ) { return( m_nFoodReqToBuild ); };

	inline
	virtual USHORT	GetReqGoldToBuild( void ) { return( m_nGoldReqToBuild ); };

	inline
	virtual USHORT	GetReqMetalToBuild( void ) { return( m_nMetalReqToBuild ); };

	inline
	virtual USHORT	GetReqStoneToBuild( void ) { return( m_nStoneReqToBuild ); };

	inline
	virtual USHORT	GetReqWoodToBuild( void ) { return( m_nWoodReqToBuild ); };

	inline
	virtual const std::vector<USHORT>&	GetBuildingsReqToBuild( void ) { return( m_vBuildingsReqToBuild ); };

	inline
	virtual const std::vector<USHORT>&	GetTechsReqToBuild( void ) { return( m_vTechsReqToBuild ); };
	

	// === UPGRADE STATS ===
	inline
	virtual USHORT	UpgradesTo( void ) { return( m_nUpgradesTo ); };

	inline
	virtual USHORT	GetUpgradeTime( void ) { return( m_nUpgradeTime ); };

	inline
	virtual USHORT	GetReqFoodToUpgrade( void ) { return( m_nFoodReqToUpgrade ); };

	inline
	virtual USHORT	GetReqGoldToUpgrade( void ) { return( m_nGoldReqToUpgrade ); };

	inline
	virtual USHORT	GetReqMetalToUpgrade( void ) { return( m_nMetalReqToUpgrade ); };

	inline
	virtual USHORT	GetReqStoneToUpgrade( void ) { return( m_nStoneReqToUpgrade ); };

	inline
	virtual USHORT	GetReqWoodToUpgrade( void ) { return( m_nWoodReqToUpgrade ); };

	inline
	virtual const std::vector<USHORT>&	GetBuildingsReqToUpgrade( void ) { return( m_vBuildingsReqToUpgrade ); };

	inline
	virtual const std::vector<USHORT>&	GetTechsReqToUpgrade( void ) { return( m_vTechsReqToUpgrade ); };

	BYTE	m_nCurrentFrame;

protected:
	void ClearBaseEntity( void );

	USHORT	m_nID;				// ID for this entity
	BYTE	m_byteTeamID;		// ID of the team this entity is on
	BYTE	m_bitflagAllies;	// bitflag to contain the list of allies
	

	std::vector<USHORT>	m_vAvailableToCivs;		// a list of Civs that can create this entity

	// === ATTRIBUTES ===
	USHORT	m_nHitPoints;			// current amount of hit points
	USHORT	m_nBaseHitPoints;		// initial hit points
	USHORT	m_nMaxHitPoints;		// maximum hit points
	short	m_nBaseRegenRate;		// rate of health regeneation

	USHORT	m_nBaseAttackStrength;	// base strength
	USHORT	m_nBaseAttackRate;		// rate of attack
	USHORT	m_nBaseMinRange;		// minimum range of attack
	USHORT	m_nBaseMaxRange;		// maximum range of attack
	USHORT	m_nBaseActionRate;		// rate of actions
	USHORT	m_nBaseAreaOfEffect;	// area of effect

	USHORT	m_nBaseAreaOfInfluence;	// area of influence

	USHORT	m_nBaseMeleeArmor;		// armor rating against melee attacks
	USHORT	m_nBasePierceArmor;		// armor rating against projectile attacks
	
	USHORT	m_nBaseLOS;				// line of sight distance

	USHORT	m_nBuildTime;			// time required to build
	USHORT	m_nUpgradeTime;			// time required to upgrade

	USHORT	m_nFoodReqToBuild;		// amount of food required to build
	USHORT	m_nGoldReqToBuild;		// amount of gold required to build
	USHORT	m_nMetalReqToBuild;		// amount of metal required to build
	USHORT	m_nStoneReqToBuild;		// amount of stone required to build
	USHORT	m_nWoodReqToBuild;		// amount of wood required to build
	std::vector<USHORT>	m_vBuildingsReqToBuild;		// buildings required to build this entity
	std::vector<USHORT>	m_vTechsReqToBuild;			// techs required to build this entity

	USHORT	m_nFoodReqToUpgrade;	// cost to upgrade (food)
	USHORT	m_nGoldReqToUpgrade;	// cost to upgrade (gold)
	USHORT	m_nMetalReqToUpgrade;	// cost to upgrade (metal)
	USHORT	m_nStoneReqToUpgrade;	// cost to upgrade (stone)
	USHORT	m_nWoodReqToUpgrade;	// cost to upgrade (wood)
	std::vector<USHORT>	m_vBuildingsReqToUpgrade;	// buildings required to upgrade this entity
	std::vector<USHORT>	m_vTechsReqToUpgrade;		// techs required to upgrade this entity

	USHORT	m_nUpgradesTo;			// ID of entity this unit will upgrade to if possible

	bool	m_bBoostsMorale;		// does this entity boost Morale?
	bool	m_bDemoralizesEnemies;	// does this entity demoralize enemy units?

	Vector3D	m_vPos;				// position on the map

private:
};

#endif