#include "btree_sec.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <algorithm>

// ---------------- Operações de Bloco ----------------

static void zerarBloco(std::vector<char>& buffer) {
    std::fill(buffer.begin(), buffer.end(), 0);
}

static void escreverBloco(std::fstream& arq, const BTreeNode& no, off_t posicao) {
    std::vector<char> buffer(4096);
    zerarBloco(buffer);
    std::memcpy(buffer.data(), &no, sizeof(BTreeNode));
    arq.seekp(posicao, std::ios::beg);
    arq.write(buffer.data(), 4096);
    arq.flush();
}

static void lerBloco(std::fstream& arq, BTreeNode& no, off_t posicao) {
    std::vector<char> buffer(4096);
    arq.seekg(posicao, std::ios::beg);
    arq.read(buffer.data(), 4096);
    std::memcpy(&no, buffer.data(), sizeof(BTreeNode));
}

static off_t tamanhoArquivo(std::fstream& arq) {
    auto atual = arq.tellg();
    arq.seekg(0, std::ios::end);
    auto fim = arq.tellg();
    arq.seekg(atual, std::ios::beg);
    return static_cast<off_t>(fim);
}

// ---------------- Cabeçalho ----------------

static void escreverCabecalho(std::fstream& arq, const BTreeHeader& header) {
    std::vector<char> buffer(4096);
    zerarBloco(buffer);
    std::memcpy(buffer.data(), &header, sizeof(BTreeHeader));
    arq.seekp(0, std::ios::beg);
    arq.write(buffer.data(), 4096);
    arq.flush();
}

static void lerCabecalho(std::fstream& arq, BTreeHeader& header) {
    std::vector<char> buffer(4096);
    arq.seekg(0, std::ios::beg);
    arq.read(buffer.data(), 4096);
    std::memcpy(&header, buffer.data(), sizeof(BTreeHeader));
}

// ---------------- Classe B+Tree Secundária ----------------

class BTreeSecondary {
private:
    std::fstream arquivo;
    std::string nomeArquivo;
    BTreeHeader header;
    int blocosLidos;

public:
    BTreeSecondary(const std::string& nome) : nomeArquivo(nome), blocosLidos(0) {}

    bool criarArvore() {
        arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
        
        if (!arquivo.is_open()) {
            arquivo.open(nomeArquivo, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!arquivo.is_open()) {
                std::cerr << "❌ Erro ao criar B+Tree secundária: " << nomeArquivo << "\n";
                return false;
            }

            header.root_offset = 4096;
            header.num_nodes = 1;
            header.height = 1;
            escreverCabecalho(arquivo, header);

            BTreeNode raiz{};
            raiz.is_leaf = true;
            raiz.num_keys = 0;
            raiz.next_leaf = -1;
            raiz.parent = -1;
            for (int i = 0; i < BTREE_ORDER; i++) raiz.children[i] = -1;
            for (int i = 0; i < BTREE_MAX_KEYS; i++) raiz.data_offsets[i] = -1;

            escreverBloco(arquivo, raiz, header.root_offset);
            arquivo.close();
            arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
        }

        if (!arquivo.is_open()) return false;

        lerCabecalho(arquivo, header);
        std::cout << "✅ B+Tree secundária criada/carregada: " << nomeArquivo << "\n";
        return true;
    }

    void fecharArvore() {
        if (arquivo.is_open()) {
            arquivo.close();
            std::cout << "✅ B+Tree secundária fechada. Blocos lidos: " << blocosLidos << "\n";
        }
    }

    int findKeyPosition(const BTreeNode& node, const char* key) {
        int pos = 0;
        while (pos < node.num_keys && strcmp(node.keys[pos], key) < 0) {
            pos++;
        }
        return pos;
    }

    bool inserirChave(const char* key, off_t data_offset) {
        if (!arquivo.is_open()) return false;

        BTreeNode raiz{};
        lerBloco(arquivo, raiz, header.root_offset);

        // Verifica se chave já existe
        for (int i = 0; i < raiz.num_keys; i++) {
            if (strcmp(raiz.keys[i], key) == 0) {
                raiz.data_offsets[i] = data_offset;
                escreverBloco(arquivo, raiz, header.root_offset);
                return true;
            }
        }

        // Se há espaço, insere diretamente
        if (raiz.num_keys < BTREE_MAX_KEYS) {
            int pos = findKeyPosition(raiz, key);
            
            for (int i = raiz.num_keys; i > pos; i--) {
                strcpy(raiz.keys[i], raiz.keys[i-1]);
                raiz.data_offsets[i] = raiz.data_offsets[i-1];
            }
            
            strcpy(raiz.keys[pos], key);
            raiz.data_offsets[pos] = data_offset;
            raiz.num_keys++;
            
            escreverBloco(arquivo, raiz, header.root_offset);
            return true;
        } else {
            // Nó cheio - implementação simplificada ignora
            static int overflow_count = 0;
            if (++overflow_count <= 5) {
                std::cout << "⚠️ Nó raiz cheio (" << raiz.num_keys << " chaves) - ignorando inserção\n";
            }
            return false;
        }
    }

    off_t buscarChave(const char* key) {
        if (!arquivo.is_open()) return -1;

        blocosLidos = 0;
        BTreeNode raiz{};
        lerBloco(arquivo, raiz, header.root_offset);
        blocosLidos++;

        for (int i = 0; i < raiz.num_keys; i++) {
            if (strcmp(raiz.keys[i], key) == 0) {
                return raiz.data_offsets[i];
            }
        }
        return -1;
    }

    int getBlocosLidos() const { return blocosLidos; }
};

// ---------------- Interface Global ----------------

static BTreeSecondary* btreeInstance = nullptr;

void init_btree_file(const char *filename) {
    if (btreeInstance) {
        btreeInstance->fecharArvore();
        delete btreeInstance;
    }
    btreeInstance = new BTreeSecondary(filename);
    btreeInstance->criarArvore();
}

bool insert_into_btree(int btree_fd, const char* key, off_t data_offset) {
    if (!btreeInstance) return false;
    return btreeInstance->inserirChave(key, data_offset);
}

off_t search_btree(const char *btree_filename, const char* key) {
    if (!btreeInstance) {
        btreeInstance = new BTreeSecondary(btree_filename);
        if (!btreeInstance->criarArvore()) return -1;
    }
    return btreeInstance->buscarChave(key);
}

// ✅ IMPLEMENTAR close_btree()
void close_btree() {
    if (btreeInstance) {
        btreeInstance->fecharArvore();
        delete btreeInstance;
        btreeInstance = nullptr;
    }
}