#ifndef INCLUDED_WACPI
#define INCLUDED_WACPI

extern std::vector<u32> wacpi_TableIDs();

extern std::vector<u8> wacpi_GetTable(u32 id);

#endif	// #ifndef INCLUDED_WACPI
