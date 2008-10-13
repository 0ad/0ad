//This class loads all the formation data needs to create an instance of a particular formation.

#ifndef INCLUDED_FORMATION
#define INCLUDED_FORMATION

#include <vector>
#include <map>
#include "ps/XML/Xeromyces.h"

class CStr;

class CFormation
{
	friend class CFormationManager;
	friend class CFormationCollection;
	friend class CEntityFormation;
	
	struct FormationSlot
	{
		float fileOff;	//Distance between files of this slot and the formation's center
		float rankOff;
		std::vector<CStr> category;
	};

public:
	CFormation();
	~CFormation(){}

	const CStr& GetBonus(){ return m_bonus; }
	const CStr& GetBonusBase(){ return m_bonusBase; }
	const CStr& GetBonusType(){ return m_bonusType; }
	float GetBonusVal(){ return m_bonusVal; }

	const CStr& GetPenalty(){ return m_penalty; }
	const CStr& GetPenaltyBase(){ return m_penaltyBase; }
	const CStr& GetPenaltyType(){ return m_penaltyType; }
	float GetPenaltyVal(){ return m_penaltyVal; }

	const CStr& GetAnglePenalty(){ return m_anglePenalty; }
	const CStr& GetAnglePenaltyType(){ return m_anglePenaltyType; }
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

	size_t m_required;
	CStr m_next;
	CStr m_prior;
	CStr m_movement;
	float m_fileSpacing;
	float m_rankSpacing;

	size_t m_numSlots;	//how many possible slots in this formation
	size_t m_numRanks;
	size_t m_numFiles;

	//The key is the "order" of the slot
	std::map<size_t, FormationSlot> m_slots;

	bool LoadXml(const CStr& filename);
	void AssignCategory(size_t order, const CStr& category);	//takes care of formatting strings
};
#endif
