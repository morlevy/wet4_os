

#include <unistd.h>

void* smalloc(size_t size) {
    if(size == 0 || size > 100000000){
        return nullptr;
    }
    void* alloc_space = sbrk(size);
    if ( alloc_space == (void*) -1) {
        return nullptr;
    }
    return alloc_space;
}