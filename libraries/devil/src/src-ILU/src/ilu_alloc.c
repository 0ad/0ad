#include <stdio.h>


#if defined(_WIN32) && defined(_MEM_DEBUG)
	#include <windows.h>

	int bAtexit = 0;


	typedef struct ALLOC_INFO
	{
		unsigned long	address;
		unsigned long	size;
		char			file[64];
		unsigned long	line;
		struct ALLOC_INFO *Next;
	} ALLOC_INFO;
	ALLOC_INFO *AllocList;


	void AddTrack(unsigned long addr, unsigned long size, const char *file, unsigned long line)
	{
		ALLOC_INFO *Temp;

		if (AllocList == NULL) {
			AllocList = (ALLOC_INFO*)malloc(sizeof(ALLOC_INFO));  // Just assume it succeeds.
			AllocList->address = addr;
			AllocList->size = size;
			AllocList->line = line;
			strncpy(AllocList->file, file, 63);
			AllocList->Next = NULL;
		}
		else {
			Temp = AllocList;
			AllocList = (ALLOC_INFO*)malloc(sizeof(ALLOC_INFO));  // Just assume it succeeds.
			AllocList->address = addr;
			AllocList->size = size;
			AllocList->line = line;
			strncpy(AllocList->file, file, 63);
			AllocList->Next = Temp;
		}

		return;
	}


	void RemoveTrack(unsigned long addr)
	{
		ALLOC_INFO *Temp, *Prev;
		
		Temp = AllocList;
		Prev = NULL;

		if (Temp == NULL)
			return;

		while (Temp != NULL) {
			if (Temp->address == addr) {
				if (Prev == NULL) {
					AllocList = Temp->Next;
					free(Temp);
				}
				else {
					Prev->Next = Temp->Next;
					free(Temp);
				}
				break;
			}
			Prev = Temp;
			Temp = Temp->Next;
		}

		return;
	}


	void DumpUnfreed(void)
	{
		unsigned long TotalSize = 0;
		char buf[1024];
		ALLOC_INFO *i = AllocList;

		OutputDebugString("ILU Unfreed Information:\n");
		while (i != NULL) {
			sprintf(buf, "%s(%d) : %d bytes unfreed at %d\n", i->file, i->line, i->size, i->address);
			OutputDebugString(buf);
			TotalSize += i->size;

			AllocList = i->Next;
			free(i);
			i = AllocList;
		}

		sprintf(buf, "-----------------------------------------------------------\n");
		OutputDebugString(buf);
		sprintf(buf, "Total Unfreed: %d bytes\n\n\n", TotalSize);
		OutputDebugString(buf);
	}

	void AddToAtexit()
	{
		if (bAtexit)
			return;
		atexit(DumpUnfreed);
		bAtexit = 1;
	}

	void *c_alloc(unsigned long size, unsigned long num, const char *file, unsigned long line)
	{
		ILvoid *ptr;
		ptr = calloc(size, num);
		if (!ptr)
			return NULL;
		AddToAtexit();
		AddTrack((unsigned long)ptr, size * num, file, line);
		return ptr;
	}


	void *m_alloc(unsigned long size, const char *file, unsigned long line)
	{
		ILvoid *ptr;
		ptr = malloc(size);
		if (!ptr)
			return NULL;
		AddToAtexit();
		AddTrack((unsigned long)ptr, size, file, line);
		return ptr;
	}


	void f_ree(void *ptr)
	{
		RemoveTrack((unsigned long)ptr);
		free(ptr);
		return;
	}

#endif//defined(_WIN32) && defined(_MEM_DEBUG)
