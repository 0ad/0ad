#ifndef __ASTAR_ENGINE_H__
#define __ASTAR_ENGINE_H__

#include "ps/Vector2D.h"
#include "ps/Player.h"
#include "lib/types.h"
#include <queue>

class AStarNode
{
	public:
	float f, g, h;
	AStarNode* parent;
	CVector2D coord;
	bool operator <(const AStarNode& rhs) const { return f<rhs.f; }
	bool equals(const AStarNode& rhs) const 
	{ 
		return ( coord.x==rhs.coord.x ) && ( coord.y==rhs.coord.y ); 
	}
};
class AStarGoalBase;
class AStarGoalLowLevel;
struct AStarNodeComp
{
  bool operator()(const AStarNode* n1, const AStarNode* n2) const
  {
    return (*n2) < (*n1);
  }
};

class CVector2D_hash_compare
{
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 16;
	size_t operator() (const CVector2D& Key) const
	{
		return Key.x + Key.y*1024;
	}
	bool operator() (const CVector2D& _Key1, const CVector2D& _Key2) const
	{
		return (_Key1.x < _Key2.x) || (_Key1.x==_Key2.x && _Key1.y < _Key2.y);
	}
};
typedef STL_HASH_MAP<CVector2D, AStarNode*, CVector2D_hash_compare> ASNodeHashMap;

class PriQueue 
	: public std::priority_queue<AStarNode*, std::vector<AStarNode*>, AStarNodeComp>
{
public:
	// Promote a node in the PQ, or if it doesn't exist, add it
	void promote(AStarNode* node);
	void clear();
};

typedef unsigned char AStarNodeFlag;

class CAStarEngine
{
public:
	CAStarEngine();
	CAStarEngine(AStarGoalBase* goal);
	virtual ~CAStarEngine();

	void setGoal(AStarGoalBase* goal) { mGoal = goal; }

	bool findPath( const CVector2D& src, const CVector2D& dest, CPlayer* player=0, float radius=0.0f );
	std::vector<CVector2D> getLastPath();

	// The maximum number of nodes that will be expanded before failure is declared
	void setSearchLimit( int limit );

protected:
	AStarGoalBase* mGoal;
private:
	int mSearchLimit;
	bool mSolved;
	std::vector<CVector2D> mPath;
	AStarNodeFlag *mFlags;
	long mFlagArraySize;

	ASNodeHashMap mClosed;
	PriQueue mOpen;

	std::vector<AStarNode*> freeNodes;
	std::vector<AStarNode*> usedNodes;

	// addToOpen will promote if the node already is in Open
	void addToOpen( AStarNode* );
	AStarNode* removeBestOpenNode();

	void addToClosed( AStarNode* );
	void removeFromClosed( AStarNode* );
	AStarNode* getFromClosed( const CVector2D& );

	void clearOpen();
	void clearClosed();

	void constructPath( AStarNode* );

	AStarNode* getFreeASNode();
	void cleanup();

	inline AStarNodeFlag* GetFlag(const CVector2D&);
	inline bool GetIsClear(AStarNodeFlag*);
	inline bool GetIsClosed(AStarNodeFlag*);
	inline bool GetIsOpen(AStarNodeFlag*);
	inline void SetClosedFlag(AStarNodeFlag*);
	inline void ClearClosedFlag(AStarNodeFlag*);
	inline void SetOpenFlag(AStarNodeFlag*);
	inline void ClearOpenFlag(AStarNodeFlag*);
	inline bool GetIsPassable(AStarNodeFlag*);
	inline bool GetIsBlocked(AStarNodeFlag*);
	inline void SetPassableFlag(AStarNodeFlag*);
	inline void SetBlockedFlag(AStarNodeFlag*);

};

/**
 * An A* goal consists of a destination tile and a radius within which a
 * unit must get to the destination. The radius is necessary because for
 * actions on a target unit, like attacking or building, the destination
 * tile is obstructed by that unit and what we really want is not to get
 * to that tile but to get close enough to perform our action.
 **/
class AStarGoalBase
{
public:
	AStarGoalBase() {}
	virtual void setDestination( const CVector2D& ) = 0;
	virtual void setRadius( float r ) = 0;
	virtual float distanceToGoal( const CVector2D& ) = 0;
	virtual bool atGoal( const CVector2D& ) = 0;
	virtual float getTileCost( const CVector2D&, const CVector2D& ) = 0;
	virtual bool isPassable( const CVector2D&, CPlayer* player=0 ) = 0;
	virtual std::vector<CVector2D> getNeighbors( const CVector2D&, CPlayer* player=0 ) = 0;
	virtual CVector2D getCoord( const CVector2D& ) = 0;
	virtual CVector2D getTile( const CVector2D& ) = 0;
	virtual float getRadius() = 0;
};

class AStarGoalLowLevel : public AStarGoalBase
{
public:
	AStarGoalLowLevel(): radius(0.0f) {}
	void setDestination( const CVector2D& dest );
	void setRadius( float r );
	float distanceToGoal( const CVector2D& loc );
	bool atGoal( const CVector2D& loc );
	float getTileCost( const CVector2D& loc1, const CVector2D& loc2 );
	bool isPassable( const CVector2D& loc, CPlayer* player=0 );
	std::vector<CVector2D> getNeighbors( const CVector2D& loc, CPlayer* player=0 );
	CVector2D getCoord( const CVector2D& loc);
	CVector2D getTile( const CVector2D& loc);
	float getRadius();
private:
	CVector2D coord;
	float radius;
};

class CAStarEngineLowLevel : public CAStarEngine
{
public:
	CAStarEngineLowLevel()
	{
		mGoal = new AStarGoalLowLevel;
	}
	virtual ~CAStarEngineLowLevel()
	{
		delete mGoal;
	}
};


#endif
