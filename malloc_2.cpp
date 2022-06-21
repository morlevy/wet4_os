//
// Created by mor levy on 21/06/2022.
//
#include <unistd.h>
#include <string.h>



typedef struct malloc_meta_data{
    size_t size;
    bool is_free;
    //void* pointer; //dropped
    malloc_meta_data* next;
    malloc_meta_data* prev;
}*MallocMetadata;

MallocMetadata first = nullptr;
MallocMetadata last = nullptr;
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
    MallocMetadata current = first;

    while(current != nullptr){ //finding the right metadata
        if (current->is_free && current->size >= size){
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

void* smalloc(size_t size) {
    if(size == 0 || size > 100000000){
        return nullptr;
    }
    MallocMetadata free_space = findFreeSpace(size);
    if(free_space != nullptr){
        free_space->is_free = false;
        number_of_free_blocks--;
        number_of_free_bytes -= free_space->size;
        return (void*) (size_t(free_space)+ sizeof(malloc_meta_data));
    }

    void* alloc_space = sbrk(sizeof(malloc_meta_data) + size);
    if ( alloc_space == (void*) -1) {
        return nullptr;
    }
    MallocMetadata metadata = MallocMetadata(alloc_space);
    void* pointer = (void*) ((size_t)alloc_space + sizeof(malloc_meta_data));
    metadata->size = size;
    metadata->is_free = false;
    //metadata->pointer = pointer;
    metadata->next = nullptr;
    metadata->prev = nullptr;
    number_of_allocated_blocks++;
    number_of_allocated_bytes += size;

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

void sfree(void *p){
    if(p == nullptr){
        return;
    }

    MallocMetadata found = findMetaData(p);

    if(found != nullptr){
        found->is_free = true;
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

