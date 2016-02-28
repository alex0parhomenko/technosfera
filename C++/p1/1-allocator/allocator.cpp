#include "allocator.h"
#include <stdlib.h>
#include <vector>

using namespace std;

/*enum class AllocErrorType
{
    InvalidFree,
    NoMemory,
};*/

/*class AllocError: std::runtime_error 
{
	private:
	    AllocErrorType type;

	public:
	    AllocError(AllocErrorType _type, std::string message):
	            runtime_error(message),
	            type(_type)
	    {}

	    AllocErrorType getType() const { return type; }
};*/

class Pointer 
{
	public:
	    void *get() const
	    {	

	    	return 0; 
	    } 
};

class Allocator
{
	public:
		memory_size = 0
		void * memory_ptr = nullptr
	    Allocator(void *base, size_t size)
	    {
	    	memory_size = size
	    	memory_ptr = base
	    }
	    
	    Pointer alloc(size_t N) 
	    {

	    	return Pointer(); 
	 	}
	    void realloc(Pointer &p, size_t N)
	    {
	    	return 0;
	    }
	    void free(Pointer &p)
	    {
	    	return 0;
	    }

	    void defrag()
	    {

	    }
	    std::string dump() {return ""; }

};

/*int main(int argc, char * argv[])
{
	return 0;
}*/

