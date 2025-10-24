#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <cstring>

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

// --- Funções de Record/Serialização ---
std::vector<uint8_t> serialize_record(const Record &r);
Record deserialize_record(const uint8_t *buf, size_t len);
off_t write_record(int fd, const Record &r);
Record read_record_by_offset(int fd, off_t offset);

// --- Funções de Hash/Buckets ---
uint32_t hash_id(int32_t id);
void init_hash_file(const char *filename);
void insert_offset_into_bucket(const char *hash_file, uint32_t bucket_id, off_t record_offset);