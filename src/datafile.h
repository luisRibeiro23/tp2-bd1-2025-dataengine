#pragma once
#include "record.h"
#include <string>
#include <cstdint>
#include <optional>

class DataFile {
public:
    DataFile(const std::string &path, size_t block_size = 4096);
    ~DataFile();

    bool create_empty(size_t n_buckets);          // Cria arquivo com n buckets
    bool open();                                 // Abre arquivo existente
    void close();                                // Fecha o arquivo

    std::optional<uint64_t> write_record(const Record &r);  // Escreve registro
    bool read_record_by_offset(uint64_t offset, Record &out, size_t &blocks_read);
    bool find_by_id(int32_t id, Record &out, size_t &blocks_read); // Testes

private:
    std::string path_;
    size_t block_size_;
    int fd_ = -1;
    size_t n_buckets_ = 0;

    uint32_t hash_id(int32_t id) const;
    bool read_block(uint64_t block_number, std::vector<uint8_t> &buf, size_t &blocks_read);
    bool write_block(uint64_t block_number, const std::vector<uint8_t> &buf);
    uint64_t allocate_new_block();
};
