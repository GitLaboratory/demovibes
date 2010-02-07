#ifndef _H_MISC_
#define _H_MISC_

void* _Realloc(void* buffer, size_t size);
void _Free(void* buffer);

template <typename T>
class AlignedBuffer
{
public:
	AlignedBuffer() : buffer(0), size(0) {}
	
	AlignedBuffer(size_t const size) : buffer(0), size(0) 
	{
		Resize(size);
	}
	
	~AlignedBuffer() { _Free(buffer); }
	
	T* Resize(size_t const size) 
	{ 
		buffer = _Realloc(buffer, size * sizeof(T));
		this->size = size;
		return reinterpret_cast<T*>(buffer);
	}
	
	T* Get() const { return reinterpret_cast<T*>(buffer); }
	
	size_t Size() const { return size; }
	
private:
	void* buffer;
	size_t size;	
};

#endif
