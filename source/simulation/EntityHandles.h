// EntityHandles.h
// 
// Entity smart pointer definitions.
//
// Usage: Use in place of a standard CEntity pointer.
//	      Handles dereference and pointer-to-member as a standard smart pointer.
//		  Reference counted. Handles are freed and entity classes released when refcount hits 0.
//		  For this reason, be careful where and how long you store these.
//        
//		  Entities maintain their own handles via their HEntity me member.
//		  To release an entity, scramble this handle. The memory will be freed when the last reference to it expires.
//
//        Standard CEntity pointers must not be used for a) anything that could be sent over the network.
//														 b) anything that will be kept between simulation steps.
//		  CEntity pointers will be faster for passing to the graphics subsytem, etc. where they won't be stored.
//		  And yes, most of this /is/ obvious. But it should be said anyway.

#ifndef INCLUDED_ENTITYHANDLES
#define INCLUDED_ENTITYHANDLES

#define INVALID_HANDLE 65535

class CEntity;
class CEntityManager;
struct CEntityList;
class CStr8;

class CHandle
{
public:
	CEntity* m_entity;
	i16 m_refcount;
	CHandle();
};

class HEntity
{
	friend class CEntity;
	friend class CEntityManager;
	friend struct CEntityList;
	u16 m_handle;
private:
	void AddRef();
	void DecRef();
	HEntity( size_t index );
public:
	HEntity();
	HEntity( const HEntity& copy );
	~HEntity();

	void operator=( const HEntity& copy );

	CEntity& operator*() const;
	CEntity* operator->() const;

	bool operator==( const HEntity& test ) const;
	bool operator!=( const HEntity& test ) const { return( !operator==( test ) ); }
	
	operator CEntity*() const;

	// Returns true iff we are a valid handle, i.e. one to a non-deleted (but possibly destroyed) entity
	bool IsValid() const;

	// Returns true iff we are a valid handle to an entity that is not destroyed
	bool IsAlive() const;

	// Same as IsValid(); maybe this should be removed altogether to prevent confusion?
	operator bool() const { return IsValid(); }

	// Same as !IsValid()
	bool operator!() const { return !IsValid(); };

	size_t GetSerializedLength() const;
	u8 *Serialize(u8 *buffer) const;
	const u8 *Deserialize(const u8 *buffer, const u8 *end);
	operator CStr8() const;
};

/*
	CEntityList
	
	DESCRIPTION: Represents a group of entities that is the target/subject of
	a network command. Use for easy serialization of one or more entity IDs.
	
	SERIALIZED FORMAT: The entities are stored as a sequence of 16-bit ints, the
	termination of the list marked by an integer with HANDLE_SENTINEL_BIT
	set. The length is thus 2*(number of entities)
*/
struct CEntityList:  public std::vector<HEntity>
{
	// Create an empty list
	inline CEntityList()
	{}
	// Create a list from an existing entity vector
	inline CEntityList(const std::vector<HEntity> &vect):
		std::vector<HEntity>(vect)
	{}
	// Create a list containing one entity
	inline CEntityList(HEntity oneEntity)
	{
		push_back(oneEntity);
	}

	size_t GetSerializedLength() const;
	u8 *Serialize(u8 *buffer) const;
	const u8 *Deserialize(const u8 *buffer, const u8 *end);
	operator CStr8() const;
};

typedef CEntityList::iterator CEntityIt;
typedef CEntityList::const_iterator CEntityCIt;

#endif
