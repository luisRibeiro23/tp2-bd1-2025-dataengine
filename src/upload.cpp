#include "data_engine.h"
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <csv_input> <datafile_path>\n";
        return 1;
    }

    const char *csv_path = argv[1];
    const char *data_path = argv[2];
    const char *hash_path = "data/hash_index.db";

    init_hash_file(hash_path);

    std::ifstream file(csv_path);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo CSV: " << csv_path << "\n";
        return 1;
    }

    int fd = open(data_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        std::cerr << "Erro ao criar arquivo de dados.\n";
        return 1;
    }

    std::string line, campo;
    std::getline(file, line); // pula cabeçalho

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        Record r;

        std::getline(ss, campo, ','); r.id = stoi(campo);
        std::getline(ss, campo, ','); strncpy(r.titulo, campo.c_str(), MAX_TITLE_LEN);
        std::getline(ss, campo, ','); r.ano = stoi(campo);
        std::getline(ss, campo, ','); strncpy(r.autores, campo.c_str(), MAX_AUTHORS_LEN);
        std::getline(ss, campo, ','); r.citacoes = stoi(campo);
        std::getline(ss, campo, ','); strncpy(r.atualizacao, campo.c_str(), 19);
        std::getline(ss, campo, ','); strncpy(r.snippet, campo.c_str(), MAX_SNIPPET_LEN);

        off_t offset = write_record(fd, r);
        uint32_t bucket = hash_id(r.id);

        insert_offset_into_bucket(hash_path, bucket, offset);

        std::cout << "ID " << r.id << " → bucket " << bucket
                  << " → offset " << offset << "\n";
    }

    close(fd);
    std::cout << "✅ Upload com hashing concluído com sucesso!\n";
    return 0;
}
