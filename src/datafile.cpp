#include "datafile.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <iostream>

DataFile::DataFile(const std::string &path, size_t block_size)
    : path_(path), block_size_(block_size) {}

DataFile::~DataFile() { close(); }

bool DataFile::create_empty(size_t n_buckets) {
    // TODO: criar arquivo com n blocos vazios (buckets)
    n_buckets_ = n_buckets;
    return true;
}

bool DataFile::open() {
    // TODO: abrir arquivo para leitura/escrita
    return true;
}

void DataFile::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

std::optional<uint64_t> DataFile::write_record(const Record &r) {
    // TODO: serializar e gravar no bucket correto (com overflow)
    (void)r;
    return std::nullopt;
}

bool DataFile::read_record_by_offset(uint64_t offset, Record &out, size_t &blocks_read) {
    // TODO: ler bloco e deserializar registro
    (void)offset; (void)out; (void)blocks_read;
    return false;
}

bool DataFile::find_by_id(int32_t id, Record &out, size_t &blocks_read) {
    // TODO: buscar por ID (hash + varredura de bucket/overflow)
    (void)id; (void)out; (void)blocks_read;
    return false;
}

uint32_t DataFile::hash_id(int32_t id) const {
    uint32_t x = static_cast<uint32_t>(id);
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x % n_buckets_;
}

bool DataFile::read_block(uint64_t block_number, std::vector<uint8_t> &buf, size_t &blocks_read) {
    // TODO: usar pread para ler block_size_ bytes do arquivo
    (void)block_number; (void)buf; (void)blocks_read;
    return false;
}

bool DataFile::write_block(uint64_t block_number, const std::vector<uint8_t> &buf) {
    // TODO: usar pwrite para escrever block_size_ bytes
    (void)block_number; (void)buf;
    return false;
}

uint64_t DataFile::allocate_new_block() {
    // TODO: aumentar tamanho do arquivo e retornar novo número de bloco
    return 0;
}
