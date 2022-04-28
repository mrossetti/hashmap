// Usage notes:
// > hm_get(hm, key) returns a void* pointing to the value (or null)
//   make sure to check for null and cast to appropriate V* type (best directly during assignment e.g. V* v = hm_get(hm, key);)
// > hm_keys(hm) and hm_values(hm) are similarly void* so it is very easy to wrongly assume hm_keys(hm)[i] will give the key at the ith position
//   all compilers warn about void* arithmetic but, in any case, it is best to directly cast on assign e.g. K* keys = hm_keys(hm); and then use keys as usual.
// > an example is available at the end of the file wrapped in #ifdef HASHMAP_TEST

// TODO(performance):
// > using index in dense arrays instead of pointers (from sparse items repr) saves memory but ergonomics?

// TODO(ergonomics & safety)
// > dense .keys and .values arrays correctly typed (K_t*, V_t*) accessible directly? requires to wrap hashmap and macro magic (keep pointers in both), worth it?
// > similarly macro to typecast the hm_get to actual V_t* (must store initial K, V types in init)

//**************
// HASHMAP INCL 
//**************
#ifndef HASHMAP_INCL
#define HASHMAP_INCL (1)

#include "common.h"

//**************
// HASHMAP DECL 
//**************
#ifndef HM_API_DECL
#define HM_API_DECL static
#endif

#define HM_MAX_LOAD_FACTOR 0.75f

#define hm_init_default(hm,cap,K,V) hm_init(hm,cap,sizeof(K),sizeof(V),hm_hash_default,hm_comp_default)
#define hm_items(hm) (hm_Item*)((hm)->mem)
#define hm_keys(hm) ((hm_K*)((byte*)((hm)->mem) + (hm)->cap * sizeof(hm_Item)))
#define hm_values(hm) ((hm_V*)((byte*)((hm)->mem) + (hm)->cap * (sizeof(hm_Item) + (hm)->sizeof_K)))

typedef void hm_K;
typedef void hm_V;

typedef struct { hm_K* key; hm_V* value; } hm_Item;

typedef struct {
    void* mem;     // sparse storage of items followed by dense storages of keys and values (owned memory)
    u32 len, cap;  // hashmap length #items (#key-value pairs) and capacity (max #items)
    u32 sizeof_K;  // size of the generic K type (treated opaquely behind a hm_K*, really void*)
    u32 sizeof_V;  // size of the generic V type (treated opaquely behind a hm_V*, really void*)
    size_t (*hash_key)(const hm_K*, size_t);               // hash function (key -> hash)
    bool32 (*comp_key)(const hm_K*, const hm_K*, size_t);  // comp function (key == key')
} HashMap;

HM_API_DECL size_t hm_hash_default(const hm_K* key, size_t sz);
HM_API_DECL bool32 hm_comp_default(const hm_K* key, const hm_K* key1, size_t sz);
HM_API_DECL HashMap* hm_init(HashMap* hm, u32 cap, u32 sizeof_K, u32 sizeof_V, size_t (*hash_key)(const void*, size_t), bool32 (*comp_key)(const void*, const void*, size_t));
HM_API_DECL void hm_free(HashMap* hm);
HM_API_DECL bool32 hm_resize(HashMap* hm, u32 cap);
HM_API_DECL bool32 hm_put(HashMap* hm, const hm_K* key, const hm_V* value);
HM_API_DECL bool32 hm_pop(HashMap* hm, const hm_K* key);
HM_API_DECL hm_V* hm_get(HashMap* hm, const hm_K* key);
HM_API_DECL bool32 _hm_next(HashMap* hm, hm_Item** it);

//**************
// HASHMAP IMPL 
//**************
#ifdef HASHMAP_IMPL

#ifndef HM_API_IMPL
#define HM_API_IMPL static
#endif

HM_API_IMPL size_t hm_hash_default(const hm_K* key, size_t sz) {
    const byte* blob = (const byte*)key;
    size_t hash = 0;

    for (size_t i = 0; i < sz; ++i) {
        hash += *blob++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

HM_API_IMPL bool32 hm_comp_default(const hm_K* key, const hm_K* key1, size_t sz) {
    return (memcmp(key, key1, sz) == 0);
}

HM_API_IMPL HashMap* hm_init(HashMap* hm, u32 cap, u32 sizeof_K, u32 sizeof_V,
                             size_t (*hash_key)(const void*, size_t),
                             bool32 (*comp_key)(const void*, const void*, size_t)) {
    assert(hm);
    
    size_t sz = cap * (sizeof(hm_Item) + sizeof_K + sizeof_V);
    void* mem = calloc(1, sz);

    hm->mem = mem;
    hm->len = 0;
    hm->cap = cap;
    hm->sizeof_K = sizeof_K;
    hm->sizeof_V = sizeof_V;
    hm->hash_key = hash_key;
    hm->comp_key = comp_key;

    if (mem == null) {
        return null;
    }

    return hm;
}

HM_API_IMPL void hm_free(HashMap* hm) {
    assert(hm && hm->mem);
    free(hm->mem);
    hm = null;
}

HM_API_IMPL bool32 hm_resize(HashMap* hm, u32 cap) {
    assert(hm && cap >= hm->len);

    size_t sz = cap * (sizeof(hm_Item) + hm->sizeof_K + hm->sizeof_V);
    void* mem = calloc(1, sz);
    
    if (mem == null) {
        return false;  // allocation failed
    }

    hm_Item* items = (hm_Item*)mem;
    byte* keys = (byte*)mem + cap * sizeof(hm_Item);
    byte* values = keys + cap * hm->sizeof_K;
    memcpy(keys, hm_keys(hm), hm->len * hm->sizeof_K);
    memcpy(values, hm_values(hm), hm->len * hm->sizeof_V);

    hm_Item* item; hm_K* key; hm_V* value;
    u64 hash;
    u32 index;

    for (u32 i = 0; i < hm->len; ++i) {
        key = keys + i * hm->sizeof_K;
        value = values + i * hm->sizeof_V;

        hash  = hm->hash_key(key, hm->sizeof_K);
        index = hash % cap;

        while (true) {
            // linear probing
            item = &items[index];
            if (item->key == null || (item->key == key) || ((hm->hash_key(item->key, hm->sizeof_K) == hash) && hm->comp_key(item->key, key, hm->sizeof_K))) {
                break;
            }
            // wrap-around
            index = (index + 1) % cap;
        }

        item->key   = key;
        item->value = value;
    }

    free(hm->mem);
    hm->mem = mem;
    hm->cap = cap;

    return true;
}

HM_API_IMPL bool32 hm_put(HashMap* hm, const hm_K* key, const hm_V* value) {
    assert(hm);

    if (hm->len > (hm->cap * HM_MAX_LOAD_FACTOR)) {
        bool32 ok = hm_resize(hm, hm->cap * 2);

        if (!ok) {  // out of memory
            return false;
        }
    }

    u64 hash = hm->hash_key(key, hm->sizeof_K);
    u32 cap = hm->cap;
    u32 index = hash % cap;
  
    hm_Item* items = hm->mem;
    hm_Item* item;

    while (true) {
        // linear probing
        item = &items[index];
        if (item->key == null || (item->key == key) || ((hm->hash_key(item->key, hm->sizeof_K) == hash) && hm->comp_key(item->key, key, hm->sizeof_K))) {
            break;
        }
        // wrap-around
        index = (index + 1) % cap;
    }

    byte* keys = hm_keys(hm);
    byte* values = hm_values(hm);

    // if key was not present before, push key and value at the end of the arrays
    if (item->key == null) {
        index = hm->len++;
        item->key = memcpy(keys + index * hm->sizeof_K, key, hm->sizeof_K);
    } else {  // else update the entry at this existing key with this new value
        index = ((byte*)item->key - keys) / hm->sizeof_K;
    }

    if ((uintptr_t)(values + index * hm->sizeof_V) != (uintptr_t)(item->value)) {
        item->value = memcpy(values + index * hm->sizeof_V, value, hm->sizeof_V);
    }

    return true;
}

HM_API_IMPL bool32 hm_pop(HashMap* hm, const hm_K* key) {
    assert(hm);

    u64 hash = hm->hash_key(key, hm->sizeof_K);
    u32 index = hash % hm->cap;
    u32 last;

    hm_Item* items = hm->mem;
    hm_Item* item;

    while (true) {
        // linear probing considering tombstone as part of the probe sequence
        item = &items[index];
        if ((item->key == null && (uintptr_t)item->value != 0xDEAD) || (item->key == key) ||
            ((hm->hash_key(item->key, hm->sizeof_K) == hash) && hm->comp_key(item->key, key, hm->sizeof_K))) {
            break;
        }
        // wrap-around
        index = (index + 1) % hm->cap;
    }

    // if key exists, swap-remove key and value at index and reset the item (pointer to key to null and pointer to value to tombstone)
    if (item->key != null) {
        byte* keys = hm_keys(hm);
        byte* values = hm_values(hm);

        last = --hm->len;
        index = ((byte*)item->key - keys) / hm->sizeof_K;
        memcpy(keys   + index * hm->sizeof_K, keys   + last * hm->sizeof_K, hm->sizeof_K);
        memcpy(values + index * hm->sizeof_V, values + last * hm->sizeof_V, hm->sizeof_V);
        item->key = null;
        item->value = (void*)0xDEAD;

        return true;
    }
    
    return false;
}

HM_API_IMPL hm_V* hm_get(HashMap* hm, const hm_K* key) {
    assert(hm);

    u64 hash = hm->hash_key(key, hm->sizeof_K);
    u32 cap = hm->cap;
    u32 index = hash % cap;
    
    hm_Item* items = hm->mem;
    hm_Item item;

    while (true) {
        // linear probing considering tombstones as part of the probe sequence
        item = items[index];
        if ((item.key == null && (uintptr_t)item.value != 0xDEAD) || (item.key == key) ||
            ((hm->hash_key(item.key, hm->sizeof_K) == hash) && hm->comp_key(item.key, key, hm->sizeof_K))) {
            break;
        }
        // wrap-around
        index = (index + 1) % cap;
    }

    return item.value;
}

HM_API_IMPL bool32 _hm_next(HashMap* hm, hm_Item** it) {
    assert(hm);

    // iteration over the sparse items (less efficient because sparse and ptr indirection), should be marked as internal as it only serves as an inspection tool!
    // (iteration over the keys or values dense storages directly, casted to the actual K* and V* types, should always be preferred for performance reasons!)
    hm_Item* items = hm->mem;
    hm_Item* item = *it;
    
    if (item == null) {
        *it = &items[0];
        return (hm->len > 0);
    }

    hm_Item* end = &items[hm->cap - 1];

    while (item != end) {
        ++item;

        if (item->key != null) {
            *it = item;
            return true;
        }
    }

    return false;
}

//**************
// HASHMAP TEST 
//**************
#ifdef HASHMAP_TEST

typedef struct FakeK { s64 x, y, z; } FakeK;
typedef struct FakeV { char tag; } FakeV;

void hm_print(HashMap* hm) {
    FakeK* k;
    FakeV* v;
    
    printf("HashMap {\n");
    for (hm_Item* it = null; _hm_next(hm, &it);) {
        k = it->key;
        v = it->value;
        printf("  { %lld, %lld, %lld } => { %c },\n", k->x, k->y, k->z, v->tag);
    }
    printf("}\n");
}

int main(int argc, char* argv[]) {
    HashMap hm;

    hm_init_default(&hm, 8, FakeK, FakeV);

    FakeK k; FakeV v;

    // load factor: hashmap will resize when >70% full (at 6/8 items cap will grow from 8 to 16)
    printf("HashMap len/cap = %u/%u\n", hm.len, hm.cap);

    for (u32 i = 0; i < 8; ++i) {
        k.x = i; k.y = i; k.z = i;

        switch (i) {
            case 0: v.tag = 'z'; break;
            case 1: v.tag = 'o'; break;
            case 2: v.tag = 't'; break;
            case 3: v.tag = 'T'; break;
            case 4: v.tag = 'f'; break;
            case 5: v.tag = 'F'; break;
            case 6: v.tag = 's'; break;
            case 7: v.tag = 'S'; break;
            default:
                panic("invalid tag!");
        }

        hm_put(&hm, &k, &v);
    }

    hm_print(&hm);

    printf("HashMap len/cap = %u/%u\nActual Items (sparse) Layout: [", hm.len, hm.cap);
    hm_Item* items = hm_items(&hm);
    FakeK* kp = null;
    for (u32 i = 0; i < hm.cap; ++i) {
        kp = items[i].key;

        if (kp != null) {
            printf("%lld ", kp->x);
        } else {
            printf("_ ");
        }
    }
    printf("]\n");

    FakeK k4 = (FakeK){ 4, 4, 4 }; 
    FakeV* v4 = (FakeV*)hm_get(&hm, &k4);

    printf("Get { %lld, %lld, %lld } => %c\n", k4.x, k4.y, k4.z, v4->tag);
    printf("Pop { %lld, %lld, %lld }\n", k4.x, k4.y, k4.z);
    
    hm_pop(&hm, &k4);
    hm_print(&hm);

    printf("Iterating over dense .keys and .values arrays!\n");
    FakeK* keys = hm_keys(&hm);
    FakeV* vals = hm_values(&hm);

    for (u32 i = 0; i < hm.len; ++i) {
        printf("{ %lld } => { %c }\n", keys[i].x, vals[i].tag);
    }

    hm_free(&hm);

    return 0;
}
#endif  // HM_TEST
#endif  // HM_IMPL
#endif  // HM_INCL
