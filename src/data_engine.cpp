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
    // simples, pode ajustar conforme tamanho de tabela
    return id % 1000;
}
