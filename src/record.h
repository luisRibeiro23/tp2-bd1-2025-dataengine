#pragma once
#include <string>
#include <cstdint>
#include <vector>

struct Record {
    int32_t id;
    std::string titulo;
    int32_t ano;
    std::string autores;
    int32_t citacoes;
    std::string atualizacao; 
    std::string snippet;
};

// Serialização / Desserialização (stubs)
std::vector<uint8_t> serialize_record(const Record &r);
Record deserialize_record(const uint8_t *buf, size_t len);
