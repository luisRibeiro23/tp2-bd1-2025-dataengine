#pragma once
#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

static constexpr int BLOCK_SIZE = 4096;
static constexpr int MAX_KEYS = 128;
using key_t = int32_t;
using ptr_t = int64_t;
using count_t = int16_t;

struct BPTreeNode {
    uint8_t folha;
    count_t nChaves;
    key_t chaves[MAX_KEYS];
    ptr_t filhos[MAX_KEYS + 1];
    ptr_t valores[MAX_KEYS];
    ptr_t proxFolha;
};

static_assert(sizeof(BPTreeNode) <= BLOCK_SIZE, "BPTreeNode exceeds 4096 bytes — reduce MAX_KEYS");

struct BPTree {
    std::fstream arquivo;
    std::string  nomeArquivo;
    ptr_t rootOffset;
    int totalBlocos;
    int blocosLidos;

    BPTree(const std::string& fname)
      : arquivo(),
        nomeArquivo(fname),
        rootOffset(-1),
        totalBlocos(0),
        blocosLidos(0) {}
};


void criarArvore(BPTree& t);
void fecharArq(BPTree& t);
ptr_t buscarChave(BPTree& t, key_t chave);
void inserirChave(BPTree& t, key_t chave, ptr_t posDado);
