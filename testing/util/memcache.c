#include <assert.h>
#include <stdlib.h>

#include "util/memcache.h"



static inline void add_element(struct memcache *cache, void *element)
{
    spin_lock(&cache->lock);
    if (cache->curr < cache->cache_size) {
        cache->elements[cache->curr++] = element;
        spin_unlock(&cache->lock);
        return;
    } else {
    spin_unlock(&cache->lock);
    free(element);
    }
    
}

static inline void *remove_element(struct memcache *cache)
{   
    spin_lock(&cache->lock);
    if (cache->curr > 0) {
        void *removed_elem = cache->elements[--cache->curr];
        spin_unlock(&cache->lock);
        return removed_elem;
    } else {
        spin_unlock(&cache->lock);
        return NULL;
    }

}

static void free_pool(struct memcache *cache)
{
    while (cache->curr) {
        void *element = remove_element(cache);
        free(element);
    }

    free(cache);
}

struct memcache *memcache_create(size_t obj_size, int max_cache_size)
{
    assert(max_cache_size >= 2);

    size_t size = sizeof(struct memcache) + max_cache_size * sizeof(void *);
    assert(size > 0);

    struct memcache *cache = malloc(size);
    if (!cache)
        return NULL;

    cache->elements = (void **) ((char *) cache + sizeof(struct memcache));
    cache->obj_size = obj_size, cache->cache_size = max_cache_size;
    cache->curr = 0;

    spin_lock_init(&cache->lock);
    max_cache_size >>= 2;
    // printf("obj: %d cache: %d", obj_size, max_cache_size)
    assert(obj_size > 0 && max_cache_size >= 2);
    while (cache->curr < max_cache_size) {
        void *element = malloc(cache->obj_size);
        if (!element) {
            free_pool(cache); /* 若空間不夠 */
            return NULL;
        }

        add_element(cache, element);
    }

    return cache;
}

void memcache_destroy(struct memcache *cache)
{
    free_pool(cache);
}


void *memcache_alloc(struct memcache *cache)
{
    return (cache->curr) ? remove_element(cache) : malloc(cache->obj_size);
}


void memcache_free(struct memcache *cache, void *element)
{
    add_element(cache, element);
}
