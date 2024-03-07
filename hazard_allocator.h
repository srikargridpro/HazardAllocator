#include <iostream>
#include <vector>
#include <atomic>
#include <cassert>
#include <memory>
#include <deque>


class memory_allocator_details {

  public :
  memory_allocator_details() = default;
  memory_allocator_details(memory_allocator_details &&memoryPool) = default;
  memory_allocator_details(const memory_allocator_details &memoryPool) = default;
  memory_allocator_details operator =(memory_allocator_details &&memoryPool) = delete;
  memory_allocator_details operator =(const memory_allocator_details &memoryPool) = delete;

};

template <class T, std::size_t bufferSize = 4096>
class HazardMemoryPool :  public memory_allocator_details
{
    public:
    alignas(T) uint8_t buffer[bufferSize*sizeof(T)];
    std::atomic<std::size_t> nextIndex{0};

    struct object_memory_info
    {
        object_memory_info()  :  is_deallocated(false) {}
       ~object_memory_info() {}
        bool is_deallocated = false; 
        size_t start_idx , end_idx;
    };

    std::unordered_map<T*, object_memory_info> memory_block_map;
    
    HazardMemoryPool()  {}
   ~HazardMemoryPool()  {}

    T* allocate(const size_t& obj_count = 1)
    {
        if (nextIndex.load() + sizeof(T) >= sizeof(buffer))
        {
            throw std::runtime_error("Out of memory!");
        }
         
        T* ptr = reinterpret_cast<T* >(&buffer[nextIndex]);
        memory_block_map[ptr].start_idx = nextIndex.load();
        nextIndex.fetch_add(obj_count*sizeof(T));
        memory_block_map[ptr].end_idx = nextIndex.load(); 
        return ptr;
    }

    void deallocate(T*  pointer)
    {
          memory_block_map[pointer].is_deallocated = true;
       // Deallocation is a no-op in this allocator
       // Deferred Reclaimation 
    }  
};


template <class T, std::size_t growSize = 1024>
class HazardAllocator 
{
    public:
        HazardMemoryPool<T, growSize> memory_pool;
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T value_type;

        template <class U>
        struct rebind
        {
            typedef HazardAllocator<U, growSize> other;
        };

        HazardAllocator() throw() = default;

        HazardAllocator(HazardAllocator &allocator) throw() {}

        template <class U>
        HazardAllocator(const HazardAllocator<U, growSize> &other) throw() {}

        ~HazardAllocator() {}

        pointer allocate(size_type n, const void *hint = 0)
        {
           return memory_pool.allocate(n);
        }

        void deallocate(pointer p, size_type n)
        {
            memory_pool.deallocate(p);
        }

        void construct(pointer p, const_reference val)
        {
            new (p) T(val);
        }

        void destroy(pointer p)
        {
            // Optional Since in Multi Threading env it
            p->~T();
        }

        const size_t get_index()
        {
            return memory_pool.nextIndex.load();
        }
};
