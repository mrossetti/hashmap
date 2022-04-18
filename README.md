# hashmap
A simple and versatile implementation of an hashmap in plain C.

```c
typedef struct FakeK { s64 x, y, z; } FakeK;
typedef struct FakeV { char tag[2]; } FakeV;

void hm_print(HashMap* hm) {
    FakeK* k;
    FakeV* v;
    
    printf("HashMap {\n");
    for (hm_Item* it = null; _hm_next(hm, &it);) {
        k = it->key;
        v = it->value;
        printf("  { %lld, %lld, %lld } => { %s },\n", k->x, k->y, k->z, v->tag);
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
            case 0: v.tag[0] = 'z'; v.tag[1] = '\0'; break;
            case 1: v.tag[0] = 'o'; v.tag[1] = '\0'; break;
            case 2: v.tag[0] = 't'; v.tag[1] = '\0'; break;
            case 3: v.tag[0] = 'T'; v.tag[1] = '\0'; break;
            case 4: v.tag[0] = 'f'; v.tag[1] = '\0'; break;
            case 5: v.tag[0] = 'F'; v.tag[1] = '\0'; break;
            case 6: v.tag[0] = 's'; v.tag[1] = '\0'; break;
            case 7: v.tag[0] = 'S'; v.tag[1] = '\0'; break;
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

    printf("Get { %lld, %lld, %lld } => %s\n", k4.x, k4.y, k4.z, v4->tag);
    printf("Pop { %lld, %lld, %lld }\n", k4.x, k4.y, k4.z);
    
    hm_pop(&hm, &k4);
    hm_print(&hm);

    printf("Iterating over dense .keys and .values arrays!\n");
    FakeK* keys = hm_keys(&hm);
    FakeV* vals = hm_values(&hm);

    for (u32 i = 0; i < hm.len; ++i) {
        printf("{ %lld } => { %c }\n", keys[i].x, vals[i].tag[0]);
    }

    hm_free(&hm);

    return 0;
}
```
