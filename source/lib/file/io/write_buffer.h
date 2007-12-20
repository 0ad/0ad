#ifndef INCLUDED_WRITE_BUFFER
#define INCLUDED_WRITE_BUFFER

class LIB_API WriteBuffer
{
public:
	WriteBuffer()
	: m_capacity(4096), m_data(io_Allocate(m_capacity)), m_size(0)
	{
	}

	void Append(const void* data, size_t size)
	{
		while(m_size + size > m_capacity)
			m_capacity *= 2;
		shared_ptr<u8> newData = io_Allocate(m_capacity);
		cpu_memcpy(newData.get(), m_data.get(), m_size);
		m_data = newData;

		cpu_memcpy(m_data.get() + m_size, data, size);
		m_size += size;
	}

	void Overwrite(const void* data, size_t size, size_t offset)
	{
		debug_assert(offset+size < m_size);
		cpu_memcpy(m_data.get()+offset, data, size);
	}

	shared_ptr<u8> Data() const
	{
		return m_data;
	}

	size_t Size() const
	{
		return m_size;
	}

private:
	size_t m_capacity;	// must come first (init order)

	shared_ptr<u8> m_data;
	size_t m_size;
};

#endif	// #ifndef INCLUDED_WRITE_BUFFER
