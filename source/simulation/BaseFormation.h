//Andrew aka pyrolink
//ajdecker1022@msn.com
//This class loads all the formation data needs to create an instance of a particular formation.

#ifndef BASEFORMATION_INCLUDED
#define BASEFORMATION_INCLUDED

#include <vector>
#include <map>
#include "XML/Xeromyces.h"

class CStr;

class CBaseFormation
{
friend class CFormationManager;
friend class CBaseFormationCollection;
friend class CEntityFormation;

struct FormationSlot
{
	float fileOff;	//Distance between files of this slot and the formation's center
	float rankOff;
	std::vector<CStr> category;
};

public:
	CBaseFormation();
	~CBaseFormation(){}

	CStr GetBonus(){ return m_bonus; }
	CStr GetBonusBase(){ return m_bonusBase; }
	CStr GetBonusType(){ return m_bonusType; }
	float GetBonusVal(){ return m_bonusVal; }

	CStr GetPenalty(){ return m_penalty; }
	CStr GetPenaltyBase(){ return m_penaltyBase; }
	CStr GetPenaltyType(){ return m_penaltyType; }
	float GetPenaltyVal(){ return m_penaltyVal; }

	CStr GetAnglePenalty(){ return m_anglePenalty; }
	CStr GetAnglePenaltyType(){ return m_anglePenaltyType; }
	int GetAnglePenaltyDivs(){ return m_anglePenaltyDivs; }
	float GetAnglePenaltyVal(){ return m_anglePenaltyVal; }


private:

	CStr m_tag;
	CStr m_bonus;
	CStr m_bonusBase;
	CStr m_bonusType;
	float m_bonusVal;

	CStr m_penalty;
	CStr m_penaltyBase;
	CStr m_penaltyType;
	float m_penaltyVal;

	CStr m_anglePenalty;
	int m_anglePenaltyDivs;
	CStr m_anglePenaltyType;
	float m_anglePenaltyVal;

	int m_required;
	CStr m_next;
	CStr m_prior;
	CStr m_movement;
	float m_fileSpacing;
	float m_rankSpacing;

	int m_numSlots;	//how many possible slots in this formation
	int m_numRanks;
	int m_numFiles;

	//The key is the "order" of the slot
	std::map<int, FormationSlot> m_slots;

	bool loadXML(CStr filename);
	void AssignCategory(int order, CStr category);	//takes care of formatting strings
};
#endif

