#include "data_engine.h"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

void init_hash_file(const char *filename) {
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Erro ao criar hash_index.db");
        return;
    }

    Bucket empty;
    empty.count = 0;
    empty.next_overflow = INVALID_OFFSET;
    for (int i = 0; i < BUCKET_SIZE; i++)
        empty.offsets[i] = INVALID_OFFSET;

    // grava todos os buckets iniciais
    for (int i = 0; i < NUM_BUCKETS; i++)
        write(fd, &empty, sizeof(Bucket));

    close(fd);
}

// Serializa um Record em bytes
std::vector<uint8_t> serialize_record(const Record &r) {
    std::vector<uint8_t> buf;
    buf.insert(buf.end(), (uint8_t*)&r.id, (uint8_t*)&r.id + sizeof(r.id));
    buf.insert(buf.end(), (uint8_t*)&r.titulo, (uint8_t*)&r.titulo + sizeof(r.titulo));
    buf.insert(buf.end(), (uint8_t*)&r.ano, (uint8_t*)&r.ano + sizeof(r.ano));
    buf.insert(buf.end(), (uint8_t*)&r.autores, (uint8_t*)&r.autores + sizeof(r.autores));
    buf.insert(buf.end(), (uint8_t*)&r.citacoes, (uint8_t*)&r.citacoes + sizeof(r.citacoes));
    buf.insert(buf.end(), (uint8_t*)&r.atualizacao, (uint8_t*)&r.atualizacao + sizeof(r.atualizacao));
    buf.insert(buf.end(), (uint8_t*)&r.snippet, (uint8_t*)&r.snippet + sizeof(r.snippet));
    return buf;
}

// Desserializa bytes em Record
Record deserialize_record(const uint8_t *buf, size_t len) {
    Record r;
    size_t off = 0;

    memcpy(&r.id, buf + off, sizeof(r.id)); off += sizeof(r.id);
    memcpy(&r.titulo, buf + off, sizeof(r.titulo)); off += sizeof(r.titulo);
    memcpy(&r.ano, buf + off, sizeof(r.ano)); off += sizeof(r.ano);
    memcpy(&r.autores, buf + off, sizeof(r.autores)); off += sizeof(r.autores);
    memcpy(&r.citacoes, buf + off, sizeof(r.citacoes)); off += sizeof(r.citacoes);
    memcpy(&r.atualizacao, buf + off, sizeof(r.atualizacao)); off += sizeof(r.atualizacao);
    memcpy(&r.snippet, buf + off, sizeof(r.snippet));
    return r;
}

off_t write_record(int fd, const Record &r) {
    off_t offset = lseek(fd, 0, SEEK_END);
    write(fd, &r, sizeof(Record));
    return offset;
}

Record read_record_by_offset(int fd, off_t offset) {
    Record r;
    lseek(fd, offset, SEEK_SET);
    read(fd, &r, sizeof(Record));
    return r;
}

void insert_offset_into_bucket(const char *hash_file, uint32_t bucket_id, off_t record_offset) {
    int fd = open(hash_file, O_RDWR);
    if (fd < 0) {
        perror("Erro ao abrir arquivo de hash");
        return;
    }

    off_t pos = bucket_id * sizeof(Bucket);
    Bucket b;
    pread(fd, &b, sizeof(Bucket), pos);

    // há espaço neste bucket?
    if (b.count < BUCKET_SIZE) {
        b.offsets[b.count++] = record_offset;
        pwrite(fd, &b, sizeof(Bucket), pos);
        close(fd);
        return;
    }

    // bucket cheio → overflow
    Bucket new_b;
    new_b.count = 1;
    new_b.offsets[0] = record_offset;
    for (int i = 1; i < BUCKET_SIZE; i++)
        new_b.offsets[i] = INVALID_OFFSET;
    new_b.next_overflow = INVALID_OFFSET;

    off_t end = lseek(fd, 0, SEEK_END);
    pwrite(fd, &new_b, sizeof(Bucket), end);

    // encadeia o novo bucket
    b.next_overflow = end;
    pwrite(fd, &b, sizeof(Bucket), pos);

    close(fd);
}

uint32_t hash_id(int32_t id) {
    // ✅ Garante resultado positivo mesmo para IDs negativos
    if (id < 0) {
        return (uint32_t)(-id) % NUM_BUCKETS;
    }
    return (uint32_t)id % NUM_BUCKETS;
}

// =============== B+TREE COM TITULO ===============

// Inicializa arquivo B+Tree
void init_btree_file(const char *filename) {
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) return;
    
    BTreeHeader header = {0, 0, 0};
    write(fd, &header, sizeof(BTreeHeader));
    close(fd);
}

// Escreve nó no arquivo B+Tree
off_t write_btree_node(int fd, const BTreeNode &node) {
    off_t offset = lseek(fd, 0, SEEK_END);
    write(fd, &node, sizeof(BTreeNode));
    return offset;
}

// Lê nó do arquivo B+Tree
BTreeNode read_btree_node(int fd, off_t offset) {
    BTreeNode node;
    pread(fd, &node, sizeof(BTreeNode), offset);
    return node;
}

// Busca posição para inserir chave (STRING)
int find_key_position(const BTreeNode &node, const char* key) {
    int pos = 0;
    while (pos < node.num_keys && strcmp(node.keys[pos], key) < 0) {
        pos++;
    }
    return pos;
}

// Insere chave em nó folha (STRING)
bool insert_in_leaf(int btree_fd, off_t leaf_offset, const char* key, off_t data_offset) {
    BTreeNode leaf = read_btree_node(btree_fd, leaf_offset);
    
    if (leaf.num_keys >= BTREE_MAX_KEYS) {
        return false; // Nó cheio, precisa dividir
    }
    
    int pos = find_key_position(leaf, key);
    
    // Desloca elementos para direita
    for (int i = leaf.num_keys; i > pos; i--) {
        strcpy(leaf.keys[i], leaf.keys[i-1]);
        leaf.data_offsets[i] = leaf.data_offsets[i-1];
    }
    
    // Insere nova chave
    strcpy(leaf.keys[pos], key);
    leaf.data_offsets[pos] = data_offset;
    leaf.num_keys++;
    
    // Reescreve nó
    pwrite(btree_fd, &leaf, sizeof(BTreeNode), leaf_offset);
    return true;
}

// Busca chave na B+Tree (STRING)
off_t search_btree(const char *btree_filename, const char* key) {
    int fd = open(btree_filename, O_RDONLY);
    if (fd < 0) return -1;
    
    BTreeHeader header;
    read(fd, &header, sizeof(BTreeHeader));
    
    if (header.root_offset == 0) {
        close(fd);
        return -1; // Árvore vazia
    }
    
    off_t current_offset = header.root_offset;
    
    // Navega até folha
    while (true) {
        BTreeNode node = read_btree_node(fd, current_offset);
        
        if (node.is_leaf) {
            // Busca na folha
            for (int i = 0; i < node.num_keys; i++) {
                if (strcmp(node.keys[i], key) == 0) {
                    close(fd);
                    return node.data_offsets[i];
                }
            }
            close(fd);
            return -1; // Não encontrado
        }
        
        // Nó interno - encontra filho correto
        int pos = find_key_position(node, key);
        current_offset = node.children[pos];
    }
}

// Encontra folha apropriada para inserção (STRING)
off_t find_leaf_for_key(int btree_fd, off_t root_offset, const char* key) {
    off_t current_offset = root_offset;
    
    while (true) {
        BTreeNode node = read_btree_node(btree_fd, current_offset);
        
        if (node.is_leaf) {
            return current_offset;
        }
        
        // Nó interno - encontra filho apropriado
        int pos = 0;
        while (pos < node.num_keys && strcmp(key, node.keys[pos]) > 0) {
            pos++;
        }
        current_offset = node.children[pos];
    }
}

// Divide nó folha quando cheio (STRING)
off_t split_leaf_node(int btree_fd, off_t leaf_offset, const char* new_key, off_t new_data_offset) {
    BTreeNode old_leaf = read_btree_node(btree_fd, leaf_offset);
    BTreeNode new_leaf;
    
    // Prepara novo nó
    memset(&new_leaf, 0, sizeof(BTreeNode));
    new_leaf.is_leaf = true;
    new_leaf.next_leaf = old_leaf.next_leaf;
    new_leaf.parent = old_leaf.parent;
    
    // Arrays temporários para ordenação
    char temp_keys[BTREE_ORDER][MAX_TITLE_LEN + 1];
    off_t temp_offsets[BTREE_ORDER];
    
    // Copia chaves existentes
    for (int i = 0; i < old_leaf.num_keys; i++) {
        strcpy(temp_keys[i], old_leaf.keys[i]);
        temp_offsets[i] = old_leaf.data_offsets[i];
    }
    
    // Insere nova chave na posição correta
    int insert_pos = 0;
    while (insert_pos < old_leaf.num_keys && strcmp(temp_keys[insert_pos], new_key) < 0) {
        insert_pos++;
    }
    
    // Desloca elementos
    for (int i = old_leaf.num_keys; i > insert_pos; i--) {
        strcpy(temp_keys[i], temp_keys[i-1]);
        temp_offsets[i] = temp_offsets[i-1];
    }
    
    strcpy(temp_keys[insert_pos], new_key);
    temp_offsets[insert_pos] = new_data_offset;
    
    // Divide: metade fica no nó antigo, metade vai pro novo
    int split_point = (BTREE_ORDER) / 2;
    
    // Atualiza nó antigo
    old_leaf.num_keys = split_point;
    for (int i = 0; i < split_point; i++) {
        strcpy(old_leaf.keys[i], temp_keys[i]);
        old_leaf.data_offsets[i] = temp_offsets[i];
    }
    
    // Preenche novo nó
    new_leaf.num_keys = BTREE_ORDER - split_point;
    for (int i = 0; i < new_leaf.num_keys; i++) {
        strcpy(new_leaf.keys[i], temp_keys[split_point + i]);
        new_leaf.data_offsets[i] = temp_offsets[split_point + i];
    }
    
    // Atualiza encadeamento
    old_leaf.next_leaf = write_btree_node(btree_fd, new_leaf);
    
    // Reescreve nó antigo
    pwrite(btree_fd, &old_leaf, sizeof(BTreeNode), leaf_offset);
    
    return old_leaf.next_leaf; // Retorna offset do novo nó
}

// Split de nó interno (STRING) - nova função
off_t split_internal_node(int btree_fd, off_t node_offset, const char* promoted_key, off_t new_child) {
    // Implementação simplificada - para projeto acadêmico
    return -1; // Não implementado
}

// Insere na B+Tree com divisão automática (STRING)
bool insert_into_btree(int btree_fd, const char* key, off_t data_offset) {
    // Lê header
    BTreeHeader header;
    pread(btree_fd, &header, sizeof(BTreeHeader), 0);
    
    if (header.root_offset == 0) {
        // Árvore vazia - cria raiz
        BTreeNode root;
        memset(&root, 0, sizeof(BTreeNode));
        root.is_leaf = true;
        strcpy(root.keys[0], key);
        root.data_offsets[0] = data_offset;
        root.num_keys = 1;
        root.parent = -1;
        root.next_leaf = -1;
        
        header.root_offset = write_btree_node(btree_fd, root);
        header.num_nodes = 1;
        header.height = 1;
        
        pwrite(btree_fd, &header, sizeof(BTreeHeader), 0);
        return true;
    }
    
    // Encontra folha apropriada
    off_t leaf_offset = find_leaf_for_key(btree_fd, header.root_offset, key);
    BTreeNode leaf = read_btree_node(btree_fd, leaf_offset);
    
    // Se folha não está cheia, insere diretamente
    if (leaf.num_keys < BTREE_MAX_KEYS) {
        return insert_in_leaf(btree_fd, leaf_offset, key, data_offset);
    }
    
    // Folha cheia - precisa dividir
    off_t new_leaf_offset = split_leaf_node(btree_fd, leaf_offset, key, data_offset);
    BTreeNode new_leaf = read_btree_node(btree_fd, new_leaf_offset);
    
    // Se não tem pai, cria nova raiz
    if (leaf.parent == -1) {
        BTreeNode new_root;
        memset(&new_root, 0, sizeof(BTreeNode));
        new_root.is_leaf = false;
        strcpy(new_root.keys[0], new_leaf.keys[0]); // Promove primeira chave do novo nó
        new_root.children[0] = leaf_offset;
        new_root.children[1] = new_leaf_offset;
        new_root.num_keys = 1;
        new_root.parent = -1;
        new_root.next_leaf = -1;
        
        off_t new_root_offset = write_btree_node(btree_fd, new_root);
        
        // Atualiza pais
        leaf.parent = new_root_offset;
        new_leaf.parent = new_root_offset;
        pwrite(btree_fd, &leaf, sizeof(BTreeNode), leaf_offset);
        pwrite(btree_fd, &new_leaf, sizeof(BTreeNode), new_leaf_offset);
        
        // Atualiza header
        header.root_offset = new_root_offset;
        header.height++;
        header.num_nodes += 2;
        pwrite(btree_fd, &header, sizeof(BTreeHeader), 0);
    }
    
    return true;
}