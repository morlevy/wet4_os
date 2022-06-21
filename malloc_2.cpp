//
// Created by mor levy on 21/06/2022.
//
#include <unistd.h>
#include <string.h>



typedef struct malloc_meta_data{
    size_t size;
    bool is_free;
    void* pointer;
    malloc_meta_data* next;
    malloc_meta_data* prev;
}*MallocMetadata;

MallocMetadata first = nullptr;
MallocMetadata last = nullptr;



void* smalloc(size_t size) {
    if(size == 0 || size > 100000000){
        return nullptr;
    }

    void* alloc_space = sbrk(sizeof(MallocMetadata) + size);
    if ( alloc_space == (void*) -1) {
        return nullptr;
    }
    MallocMetadata metadata = static_cast<MallocMetadata>(alloc_space);
    void* pointer = (void*) ((size_t)alloc_space + sizeof(malloc_meta_data));
    metadata->size = size;
    metadata->is_free = false;
    metadata->pointer = pointer;
    metadata->next = nullptr;
    metadata->prev = nullptr;

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

    MallocMetadata current = first;
    MallocMetadata found = nullptr;
    while(current != nullptr){
        if (current->pointer == p){
            found = current;
            break;
        }
        current = current->next;
    }

    if(found != nullptr){
        found->is_free = true;

    }
}

void* srealloc(void* oldp, size_t size){

}