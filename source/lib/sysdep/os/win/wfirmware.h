#ifndef INCLUDED_WFIRMWARE
#define INCLUDED_WFIRMWARE

namespace wfirmware {

typedef u32 Provider;

typedef u32 TableId;
typedef std::vector<TableId> TableIds;

extern TableIds GetTableIDs(Provider provider);

typedef std::vector<u8> Table;

extern Table GetTable(Provider provider, TableId tableId);

}	// namespace wfirmware

#endif	// #ifndef INCLUDED_WFIRMWARE
