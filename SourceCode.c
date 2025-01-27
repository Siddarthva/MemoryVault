#ifndef SIMPLE_MEMORY_H
#define SIMPLE_MEMORY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

// Enable logging (set to 1 for debug logs)
#define ENABLE_LOGGING 1

// Macro for logging
#define LOG(msg) if (ENABLE_LOGGING) { printf("[MEMORY LOG]: %s\n", msg); }

// Error handling macros
#define HANDLE_ERROR(msg) { fprintf(stderr, "Error: %s\n", msg); exit(EXIT_FAILURE); }

// Simplified malloc with logging and error handling
void* allocate_memory(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        HANDLE_ERROR("Memory allocation failed!");
    }
    LOG("Memory allocated.");
    return ptr;
}

// Simplified calloc with logging and error handling
void* allocate_zeroed(size_t num, size_t size) {
    void* ptr = calloc(num, size);
    if (!ptr) {
        HANDLE_ERROR("Memory allocation failed!");
    }
    LOG("Zero-initialized memory allocated.");
    return ptr;
}

// Simplified realloc with logging and error handling
void* reallocate_memory(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        HANDLE_ERROR("Memory reallocation failed!");
    }
    LOG("Memory reallocated.");
    return new_ptr;
}

// Simplified free with null-pointer check and logging
void release_memory(void** ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL; // Nullify pointer to prevent double-free
        LOG("Memory released.");
    }
}

// Clears memory block with zeroes
void clear_memory(void* ptr, size_t size) {
    if (ptr) {
        memset(ptr, 0, size);
        LOG("Memory cleared.");
    }
}

// Duplicates a string safely
char* duplicate_string(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1; // +1 for null-terminator
    char* new_str = (char*) allocate_memory(len);
    strcpy(new_str, str);
    LOG("String duplicated.");
    return new_str;
}

// Custom memory pool (for fixed-size allocations)
typedef struct MemoryPool {
    void* pool;           // The memory pool
    size_t block_size;    // Size of each block
    size_t pool_size;     // Total size of the pool
    size_t used_blocks;   // Number of blocks in use
    unsigned char* bitmap; // Bitmap for tracking allocations
    pthread_mutex_t lock; // Mutex for thread safety
} MemoryPool;

// Initializes a memory pool
MemoryPool* create_memory_pool(size_t block_size, size_t pool_size) {
    MemoryPool* mp = (MemoryPool*) allocate_memory(sizeof(MemoryPool));
    mp->block_size = block_size;
    mp->pool_size = pool_size;
    mp->used_blocks = 0;
    mp->pool = allocate_memory(block_size * pool_size);
    mp->bitmap = (unsigned char*) allocate_zeroed(pool_size, sizeof(unsigned char));
    pthread_mutex_init(&mp->lock, NULL); // Initialize lock
    LOG("Memory pool created.");
    return mp;
}

// Allocates a block from the memory pool
void* pool_allocate(MemoryPool* mp) {
    pthread_mutex_lock(&mp->lock);
    for (size_t i = 0; i < mp->pool_size; i++) {
        if (mp->bitmap[i] == 0) { // Find an available block
            mp->bitmap[i] = 1;
            mp->used_blocks++;
            pthread_mutex_unlock(&mp->lock);
            LOG("Memory allocated from pool.");
            return (void*)((char*)mp->pool + (i * mp->block_size));
        }
    }
    pthread_mutex_unlock(&mp->lock);
    HANDLE_ERROR("Memory pool is full!");
    return NULL;
}

// Frees a block back to the memory pool
void pool_free(MemoryPool* mp, void* ptr) {
    pthread_mutex_lock(&mp->lock);
    size_t index = ((char*)ptr - (char*)mp->pool) / mp->block_size;
    if (index < mp->pool_size && mp->bitmap[index] == 1) {
        mp->bitmap[index] = 0;
        mp->used_blocks--;
        LOG("Memory freed back to pool.");
    }
    pthread_mutex_unlock(&mp->lock);
}

// Destroys the memory pool
void destroy_memory_pool(MemoryPool* mp) {
    if (mp) {
        pthread_mutex_destroy(&mp->lock); // Destroy lock
        release_memory((void**)&mp->pool);
        release_memory((void**)&mp->bitmap);
        release_memory((void**)&mp);
        LOG("Memory pool destroyed.");
    }
}

// Debugging: Prints memory pool usage
void print_memory_pool_status(MemoryPool* mp) {
    if (!mp) return;
    printf("Memory Pool Status:\n");
    printf("  Block Size: %zu bytes\n", mp->block_size);
    printf("  Pool Size: %zu blocks\n", mp->pool_size);
    printf("  Used Blocks: %zu\n", mp->used_blocks);
    printf("  Free Blocks: %zu\n", mp->pool_size - mp->used_blocks);
}

#endif // SIMPLE_MEMORY_H
