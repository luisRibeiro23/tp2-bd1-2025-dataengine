#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <unistd.h>

#define MAX_TITLE_LEN   300
#define MAX_AUTHORS_LEN 150
#define MAX_SNIPPET_LEN 1024
#define BUCKET_SIZE     4
#define NUM_BUCKETS     100
#define INVALID_OFFSET  -1

struct Record {
    int32_t id;
    char titulo[MAX_TITLE_LEN + 1];
    int32_t ano;
    char autores[MAX_AUTHORS_LEN + 1];
    int32_t citacoes;
    char atualizacao[20];
    char snippet[MAX_SNIPPET_LEN + 1];
};

struct Bucket {
    int32_t count;
    off_t offsets[BUCKET_SIZE];
    off_t next_overflow;
};

#define BTREE_ORDER 10          
#define BTREE_MIN_KEYS (BTREE_ORDER/2)
#define BTREE_MAX_KEYS (BTREE_ORDER-1)

// ✅ B+Tree com string direta
struct BTreeNode {
    bool is_leaf;                           
    int32_t num_keys;                       
    char keys[BTREE_MAX_KEYS][MAX_TITLE_LEN + 1];  // ✅ String direta!
    off_t data_offsets[BTREE_MAX_KEYS];    
    off_t children[BTREE_ORDER];           
    off_t next_leaf;                       
    off_t parent;                          
};

struct BTreeHeader {
    off_t root_offset;                     
    int32_t num_nodes;                     
    int32_t height;                        
};

// --- Funções de Record/Serialização ---
std::vector<uint8_t> serialize_record(const Record &r);
Record deserialize_record(const uint8_t *buf, size_t len);
off_t write_record(int fd, const Record &r);
Record read_record_by_offset(int fd, off_t offset);

// --- Funções de Hash/Buckets ---
uint32_t hash_id(int32_t id);
void init_hash_file(const char *filename);
void insert_offset_into_bucket(const char *hash_file, uint32_t bucket_id, off_t record_offset);

// --- Funções de B+Tree com string --- ✅ CORRIGIDO!
void init_btree_file(const char *filename);
off_t write_btree_node(int fd, const BTreeNode &node);
BTreeNode read_btree_node(int fd, off_t offset);
int find_key_position(const BTreeNode &node, const char* key);  // ✅ String
bool insert_in_leaf(int btree_fd, off_t leaf_offset, const char* key, off_t data_offset);  // ✅ String
off_t search_btree(const char *btree_filename, const char* key);  // ✅ String
off_t find_leaf_for_key(int btree_fd, off_t root_offset, const char* key);  // ✅ String
off_t split_leaf_node(int btree_fd, off_t leaf_offset, const char* new_key, off_t new_data_offset);  // ✅ String
off_t split_internal_node(int btree_fd, off_t node_offset, const char* promoted_key, off_t new_child);  // ✅ String
bool insert_into_btree(int btree_fd, const char* key, off_t data_offset);  // ✅ String