#include "data_engine.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " <id> <data.db> <hash_index.db>\n";
        return 1;
    }

    int search_id = std::stoi(argv[1]);
    const char *data_path = argv[2];
    const char *hash_path = argv[3];

    int hash_fd = open(hash_path, O_RDONLY);
    int data_fd = open(data_path, O_RDONLY);

    if (hash_fd < 0 || data_fd < 0) {
        std::cerr << "Erro ao abrir arquivos.\n";
        return 1;
    }

    uint32_t bucket_id = hash_id(search_id);
    off_t pos = bucket_id * sizeof(Bucket);
    Bucket b;

    bool found = false;
    do {
        pread(hash_fd, &b, sizeof(Bucket), pos);

        for (int i = 0; i < b.count; i++) {
            if (b.offsets[i] == INVALID_OFFSET) continue;
            Record r = read_record_by_offset(data_fd, b.offsets[i]);
            if (r.id == search_id) {
                std::cout << "✅ Registro encontrado!\n";
                std::cout << "ID: " << r.id << "\n";
                std::cout << "Título: " << r.titulo << "\n";
                std::cout << "Ano: " << r.ano << "\n";
                std::cout << "Autores: " << r.autores << "\n";
                std::cout << "Citações: " << r.citacoes << "\n";
                std::cout << "Atualização: " << r.atualizacao << "\n";
                std::cout << "Snippet: " << r.snippet << "\n";
                found = true;
                break;
            }
        }

        pos = b.next_overflow;
    } while (!found && b.next_overflow != INVALID_OFFSET);

    if (!found)
        std::cout << "❌ Registro com ID " << search_id << " não encontrado.\n";

    close(hash_fd);
    close(data_fd);
    return 0;
}
