#include <furi.h>
#include <stdlib.h>

extern void* pvPortMalloc(size_t xSize);
extern void vPortFree(void* pv);

void* malloc(size_t size) {
    return pvPortMalloc(size);
}

void free(void* ptr) {
    vPortFree(ptr);
}

void* realloc(void* ptr, size_t size) {
    if(size == 0) {
        vPortFree(ptr);
        return NULL;
    }

    void* p = pvPortMalloc(size);
    if(ptr != NULL) {
        memcpy(p, ptr, size);
        vPortFree(ptr);
    }

    return p;
}

void* calloc(size_t count, size_t size) {
    return pvPortMalloc(count * size);
}

char* strdup(const char* s) {
    // arg s marked as non-null, so we need hack to check for NULL
    furi_check(((uint32_t)s << 2) != 0);

    size_t siz = strlen(s) + 1;
    char* y = pvPortMalloc(siz);
    memcpy(y, s, siz);

    return y;
}

void* __wrap__malloc_r(struct _reent* r, size_t size) {
    UNUSED(r);
    return pvPortMalloc(size);
}

void __wrap__free_r(struct _reent* r, void* ptr) {
    UNUSED(r);
    vPortFree(ptr);
}

void* __wrap__calloc_r(struct _reent* r, size_t count, size_t size) {
    UNUSED(r);
    return calloc(count, size);
}

void* __wrap__realloc_r(struct _reent* r, void* ptr, size_t size) {
    UNUSED(r);
    return realloc(ptr, size);
}