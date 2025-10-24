#pragma once
#include "data_engine.h"
#include <string>
#include <cstdint>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <cstring>

#define BTREE_ORDER 10          
#define BTREE_MIN_KEYS (BTREE_ORDER/2)
#define BTREE_MAX_KEYS (BTREE_ORDER-1)

struct BTreeNode {
    bool is_leaf;                           
    int32_t num_keys;                       
    char keys[BTREE_MAX_KEYS][MAX_TITLE_LEN + 1];  
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

// funções da B+Tree secundária
void init_btree_file(const char *filename);
bool insert_into_btree(int btree_fd, const char* key, off_t data_offset);
off_t search_btree(const char *btree_filename, const char* key);
void close_btree(); 