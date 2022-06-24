//
// Created by eyalk on 6/22/2022.
//

#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define MMAP_SIZE 128*1024

//need to make sure size of malloc_meta_data multiplication of 8
typedef struct malloc_meta_data{
    size_t size;
    //void* pointer; //dropped
    malloc_meta_data* next;
    malloc_meta_data* prev;
    int is_free;
    malloc_meta_data* next_free;
    malloc_meta_data* prev_free;
}*MallocMetadata;

MallocMetadata first = nullptr;
MallocMetadata last = nullptr;
MallocMetadata first_free = nullptr;
MallocMetadata last_free = nullptr;
int number_of_free_blocks = 0;
int number_of_free_bytes = 0;
int number_of_allocated_blocks = 0;
int number_of_allocated_bytes = 0;

MallocMetadata findMetaData(void* p){
    MallocMetadata current = first;
    MallocMetadata found = nullptr;

    while(current != nullptr){ //finding the right metadata
        void* pointer = (void*) ((size_t)current + sizeof(malloc_meta_data));
        if (pointer == p){
            found = current;
            break;
        }
        current = current->next;
    }

    return found;
}

MallocMetadata findFreeSpace(size_t size){
    MallocMetadata current = first_free;

    while(current != nullptr){ //finding the right metadata
        if (current->size >= size){
            return current;
        }
        current = current->next_free;
    }
    return nullptr;
}

void _insertFree(MallocMetadata metadata)
{
    if(first_free == nullptr){ //update next and prev
        first_free = metadata;
        last_free = metadata;
    } else {
        MallocMetadata temp = first_free;
        while (temp) //checking if we can insert it sorted
        {
            if (temp->size > metadata->size) //we only insert if the next node has a bigger size/
            {
                temp->prev_free->next_free = metadata;
                metadata->next_free = temp;
                metadata->prev_free = temp->prev_free;
                temp->prev_free = metadata;
                break;
            }
        }
        if (!(metadata->next_free)) //if it should be last by size and address.
        {
            metadata->prev_free = last_free;
            last_free->next_free = metadata;
            last_free = metadata;
        }
    }
}

//checking if a free block can be split into 2 blocks. with the other block being at least 128 bytes.
MallocMetadata _splitBlock(MallocMetadata freeBlock, size_t size)
{
    MallocMetadata result = nullptr;
    //128? 128*8?
    if ((freeBlock->size < size + 128 + sizeof(malloc_meta_data))) //if not big enough return null
        return result;

    result = (size_t)freeBlock + sizeof(malloc_meta_data) + size; //getting the start of the other part of the block
    result->size = freeBlock->size - size - sizeof(malloc_meta_data); //result->size MUST be larger than 128!
    result->next = nullptr;
    result->prev = nullptr;
    result->next_free = nullptr;
    result->prev_free = nullptr;
    return result;
}

//simply disconnects a node. does not update global values
void _disconnectFreeMetaNode(MallocMetadata metadata)
{
    if (!metadata)
        return;
    if (metadata == first_free || metadata == last_free)
    {
        if (metadata == first_free)
            first_free = metadata->next_free ? metadata->next_free : nullptr;
        if (metadata == last_free)
            last_free = metadata->prev_free ? metadata->prev_free : nullptr;
    }
    else
    {
        metadata->prev_free->next_free = metadata->next_free;
        metadata->next_free->prev_free = metadata->prev_free;
    }
}

void* smalloc(size_t size) {
    if(size == 0 || size > 100000000){
        return nullptr;
    }
    size = !(size%8) ? size : size + 8 - size%8;//make size a multiplication of 8
    MallocMetadata free_space = findFreeSpace(size);
    if(free_space != nullptr){
        MallocMetadata leftover = _splitBlock(free_space, size);
        if (leftover)
        {
            _disconnectFreeMetaNode(free_space);
            _insertFree(leftover);
            //inserting leftover after free_space in main list
            leftover->next = free_space->next;
            leftover->prev = free_space;
            free_space->next = leftover;

            number_of_free_bytes -= sizeof(malloc_meta_data);
            number_of_allocated_blocks += 1; //pretty sure need this
            number_of_allocated_bytes -= sizeof(malloc_meta_data);
            leftover->is_free = 1;
        }
        else
        {
            number_of_free_blocks--;
        }
        number_of_free_bytes -= free_space->size;
        free_space->is_free = 0;
        return (void*) (size_t(free_space)+ sizeof(malloc_meta_data));
    }

    //check wilderness block case
    //won't check mmap with this case
    if (last && last->is_free)
    {
        void* alloc_space = sbrk(size - last->size);
        if (alloc_space == (void*) -1) {
            return nullptr;
        }
        last_free = last_free->prev_free;
        last->is_free = 0;
        return (void *) ((size_t)last + sizeof(malloc_meta_data));
    }
    void* alloc_space;
    if (size >= MMAP_SIZE)
    {
        alloc_space = mmap(NULL, size + sizeof(malloc_meta_data), PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
        if (alloc_space == (void*) -1) {
            return nullptr;
        }
        return (void*) ((size_t)alloc_space + sizeof(malloc_meta_data));
    }
    else
        alloc_space = sbrk(sizeof(malloc_meta_data) + size);
    if (alloc_space == (void*) -1) {
        return nullptr;
    }
    MallocMetadata metadata = MallocMetadata(alloc_space);
    void* pointer = (void*) ((size_t)alloc_space + sizeof(malloc_meta_data));
    metadata->size = size;
    metadata->is_free = 0;
    //metadata->pointer = pointer;
    metadata->next = nullptr;
    metadata->prev = nullptr;
    number_of_allocated_blocks++;
    number_of_allocated_bytes += size;

    //adding to main list
    if(first == nullptr){ //update next and prev
        first = metadata;
        last = metadata;
    } else {
        metadata->prev = last;
        last->next = metadata;
        last = metadata;
    }

    return pointer;
}

void* scalloc(size_t num, size_t size){
    void* pointer = smalloc(num * size);
    if ( pointer == nullptr) {
        return nullptr;
    }
    memset(pointer,0, size * num); //setting values to 0
    return pointer;
}

//assumes metadata is a free block and its next bock as well.
//combines them into 1 block where metadata is the pointer to it
void _combineFree(MallocMetadata metadata)
{
    //asserting the values
    if (!metadata || !metadata->next || !metadata->next->is_free)
        return;
    //now combine
    metadata->size += metadata->next->size + sizeof(metadata);
    number_of_allocated_blocks--;
    number_of_free_bytes += sizeof(metadata);
    number_of_allocated_bytes += sizeof (metadata);
    number_of_free_blocks--;
    if (last == metadata->next)
        last = metadata;
    if (last_free == metadata->next)
        last_free = metadata;
    metadata->next->prev_free = metadata->next->next_free; //removing next from the free list
    metadata->next->next_free->prev_free = metadata->next->prev_free;
    metadata->next = metadata->next->next; //removing next from the main list
    metadata->next->next->prev = metadata;
}

void sfree(void *p){
    if(p == nullptr){
        return;
    }

    MallocMetadata found = findMetaData(p); //why not p - sizeof(malloc_meta_data)?

    if(found != nullptr && !found->is_free){
        if (found->size >= MMAP_SIZE)
        {
            munmap(found, found->size + sizeof(malloc_meta_data));
            return;
        }
        if (found->next && found->next->is_free)
        {
            _combineFree(found);
        }
        if (found->prev && found->prev->is_free)
        {
            _combineFree(found->prev);
        }
        number_of_free_blocks++;
        number_of_free_bytes += found->size;
    }
}

void* srealloc(void* oldp, size_t size){
    if (size == 0 || size > 100000000){
        return nullptr;
    }

    if(oldp == nullptr){
        return smalloc(size);
    }

    MallocMetadata old = findMetaData(oldp);
    if (old->size >= size){
        //old->size = size; //not sure about this line :: memory loss
        return oldp;
    }
    size = !(size%8) ? size : size + 8 - size%8;
    //if it's a mmap allocated block, then we simply allocate a new block
    if (old->size >= MMAP_SIZE)
    {
        munmap(old, old->size);
        int alloc_space = mmap(NULL, size + sizeof(malloc_meta_data), PROT_READ | PROT_WRITE, MAP_SHARED, -1, 0);
        if (alloc_space == (void*) -1) {
            return nullptr;
        }
        return (void *) (size_t)(old->prev) + sizeof(malloc_meta_data);
    }
    MallocMetadata current = old;
    //checking if I have a free prev
    if (old->prev && old->prev_free->is_free)
    {
        //if with prev we have enough bytes then merge
        if (size <= old->size + sizeof(malloc_meta_data) + old->prev->size) {
            old->prev->size += sizeof(malloc_meta_data) + old->size;
            old->prev->next = old->next;
            old->next->prev = old->prev;
            if (last == old)
                last = old->prev;
            _disconnectFreeMetaNode(old->prev);
            return (void *) (size_t)(old->prev) + sizeof(malloc_meta_data);
        }
        //if we have a free prev and if we are wilderness then merge and add extra
        if (last == old)
        {
            int difference = size - old->size - sizeof(malloc_meta_data) - old->prev->size;
            old->prev->size = size;
            old->prev->next = nullptr;
            _disconnectFreeMetaNode(old->prev);
            int alloc_space = sbrk(difference);
            if (alloc_space == (void*) -1) {
                return nullptr;
            }
            return (void *) (size_t)(old->prev) + sizeof(malloc_meta_data);
        }
    }
    //if it's the wilderness block
    if (current == last)
    {
        if (current->size < size)
        {
            int difference = size - current->size;
            difference = !(difference%8) ? difference : difference + 8 - difference%8;
            int alloc_space = sbrk(difference);
            if (alloc_space == (void*) -1) {
                return nullptr;
            }
            current->size = size;
            return (void *) (size_t)(current) + sizeof(malloc_meta_data);
        }
    }
    //now check if next block is free and merge if so
    if (old->next->is_free)
    {
        //if with next we have enough bytes then merge
        if (size <= old->size + sizeof(malloc_meta_data) + old->next->size) {
            old->size += sizeof(malloc_meta_data) + old->next->size;
            old->next = old->next->next;
            if (old->next->next)
            {
                old->next->next->prev = old;
            }
            _disconnectFreeMetaNode(old->next);

            if (last == old->next)
                last = old;
            return (void *) (size_t)(old->prev) + sizeof(malloc_meta_data);
        }
        //try merging all 3 neighbors
        if (old->prev && old->prev->is_free)
        {
            if (size <= old->size + 2*sizeof(malloc_meta_data) + old->prev->size + old->next->size)
            {
                old->prev->size += 2*sizeof(malloc_meta_data) + old->size + old->next->size;
                old->prev->next = old->next->next;
                if (old->next->next)
                    old->next->next->prev = old->prev;
                _disconnectFreeMetaNode(old->next);
                _disconnectFreeMetaNode(old->prev);
                if (old->next == last)
                    last = old->prev;
                return (void *) (size_t)(old->prev) + sizeof(malloc_meta_data);
            }

            //if we have both neighbors free and next is the wilderness - merge and enlarge it
            if (old->next == last)
            {
                void* alloc_space = sbrk(size - old->size - old->next->size - old->prev->size - 2* sizeof(malloc_meta_data));
                if (alloc_space == (void*) -1) {
                    return nullptr;
                }
                last = old->prev;
                old->prev->next = nullptr;
                old->prev->size = size;
                return (void *) (size_t)(old->prev) + sizeof(malloc_meta_data);
            }
        }

        //now we merge only with next block if it's last
        if (old->next == last)
        {
            void* alloc_space = sbrk(size - old->size - old->next->size - sizeof(malloc_meta_data));
            if (alloc_space == (void*) -1) {
                return nullptr;
            }
            last = old;
            old->size = size;
            old->next = nullptr;
            return (void *) (size_t)(old) + sizeof(malloc_meta_data);
        }
    }
    old->is_free = 1;
    void* new_p = smalloc(size);
    if(new_p != nullptr){
        memcpy(new_p,oldp,old->size);
        sfree(oldp);
    }
    return new_p;
}

size_t _num_free_blocks(){
    return number_of_free_blocks;
}

size_t _num_free_bytes(){
    return number_of_free_bytes;
}

size_t _num_allocated_blocks(){
    return number_of_allocated_blocks;
}

size_t  _num_allocated_bytes(){
    return number_of_allocated_bytes;
}

size_t _num_meta_data_bytes(){
    return number_of_allocated_blocks * sizeof(malloc_meta_data);
}

size_t _size_meta_data(){
    return sizeof(malloc_meta_data);
}

