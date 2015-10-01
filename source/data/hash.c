/**
  @file hash.c
  @brief See header for details
  $Revision$
  $HeadURL$
  */
#include <data/hash.h>
#include <stdlib.h>

#define NUM_PRIMES (sizeof(primes)/sizeof(unsigned int))

static unsigned int Primes[] = {
    5, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
    49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
    12582917, 25165843, 50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};

static inline unsigned int num_buckets(unsigned int idx) {
    return Primes[idx];
}

static void find_entry(hash_t* hash, hash_entry_t** parent, hash_entry_t** current, hash_entry_t* entry)
{
    while(*current != NULL) {
        if (((*current)->hash == entry->hash) &&
            (0 == hash->cmpfn(*current, entry)))
            break;
        *parent  = *current;
        *current = (*current)->next;
    }
}

static void rehash(hash_t* hash)
{
    unsigned int oldcount = hash->bkt_count++;
    hash_entry_t** oldbuckets = hash->buckets;
    hash->buckets = (hash_entry_t**)calloc(sizeof(hash_entry_t*), num_buckets(hash->bkt_count));
    hash->size = 0;
    /* Iterate over all of the old buckets */
    for (unsigned int i = 0; i < num_buckets(oldcount); i++) {
        hash_entry_t* node = oldbuckets[i];
        /* re-insert all entries in the bucket into the new bucket table */
        while (node != NULL) {
            hash_entry_t* entry = node;
            node = entry->next;
            hash_set(hash, entry);
        }
    }
    /* Free the old bucket table */
    free(oldbuckets);
}

void hash_init(hash_t* hash, hash_hashfn_t hashfn, hash_cmpfn_t cmpfn, hash_freefn_t delfn)
{
    hash->size      = 0;
    hash->bkt_count = 0;
    hash->hashfn    = hashfn;
    hash->cmpfn     = cmpfn;
    hash->delfn     = delfn;
    hash->buckets   = (hash_entry_t**)calloc(sizeof(hash_entry_t*), num_buckets(hash->bkt_count));
}

void hash_deinit(hash_t* hash)
{
    hash_clr(hash);
    free(hash->buckets);
}

size_t hash_size(hash_t* hash)
{
    return hash->size;
}

bool hash_set(hash_t* hash, hash_entry_t* entry)
{
    if (hash->size >= num_buckets(hash->bkt_count))
        rehash(hash);
    entry->hash = hash->hashfn(entry);
    unsigned int index    = (entry->hash % num_buckets(hash->bkt_count));
    hash_entry_t* parent  = NULL;
    hash_entry_t* node    = hash->buckets[index];
    hash_entry_t* deadite = NULL;
    find_entry(hash, &parent, &node, entry);
    if ((parent == NULL) && (node == NULL)) {
        hash->buckets[index] = entry;
        entry->next = NULL;
        hash->size++;
    } else if (node == NULL) {
        parent->next = entry;
        entry->next = NULL;
        hash->size++;
    } else if (parent == NULL) {
        deadite = node;
        entry->next = deadite->next;
        hash->buckets[index] = entry;
    } else {
        deadite = node;
        entry->next = deadite->next;
        parent->next = entry;
    }
    if (deadite != NULL)
        hash->delfn(deadite);
    return true;
}

hash_entry_t* hash_get(hash_t* hash, hash_entry_t* entry)
{
    entry->hash = hash->hashfn(entry);
    unsigned int index   = (entry->hash % num_buckets(hash->bkt_count));
    hash_entry_t* parent = NULL;
    hash_entry_t* node   = hash->buckets[index];
    find_entry(hash, &parent, &node, entry);
    (void)parent;
    return node;
}

bool hash_del(hash_t* hash, hash_entry_t* entry)
{
    bool ret = false;
    entry->hash = hash->hashfn(entry);
    unsigned int index = (entry->hash % num_buckets(hash->bkt_count));
    hash_entry_t* parent = NULL;
    hash_entry_t* node = hash->buckets[index];
    find_entry(hash, &parent, &node, entry);
    if (node != NULL) {
        hash_entry_t* deadite = node;
        node = node->next;
        if (parent != NULL)
            parent->next = node;
        else
            hash->buckets[index] = node;
        hash->delfn(deadite);
        ret = true;
    }
    return ret;
}

void hash_clr(hash_t* hash)
{
    /* Delete all the entries  in the hash */
    for (unsigned int i = 0; i < num_buckets(hash->bkt_count); i++) {
        hash_entry_t* node = hash->buckets[i];
        while (node != NULL) {
            hash_entry_t* deadite = node;
            node = node->next;
            hash->delfn(deadite);
        }
    }
    /* Shrink the buckets array */
    free(hash->buckets);
    hash->bkt_count = 0;
    hash->buckets   = (hash_entry_t**)calloc(sizeof(hash_entry_t*), num_buckets(hash->bkt_count));
}

