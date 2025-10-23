#include "data_engine.h"
#include <fcntl.h>
#include <iostream>
#include <cstring>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <data_file> <btree_output>\n";
        return 1;
    }
    
    const char *data_file = argv[1];
    const char *btree_file = argv[2];
    
    std::cout << "Construindo índice B+Tree para TÍTULO (string direta)\n";
    
    init_btree_file(btree_file);
    
    int data_fd = open(data_file, O_RDONLY);
    int btree_fd = open(btree_file, O_RDWR);
    
    if (data_fd < 0 || btree_fd < 0) {
        std::cerr << "Erro ao abrir arquivos\n";
        return 1;
    }
    
    off_t current_offset = 0;
    Record r;
    int count = 0;
    
    // ✅ Usar título diretamente (sem hash)
    while (read(data_fd, &r, sizeof(Record)) == sizeof(Record)) {
        // ✅ Usar título como chave direta
        if (!insert_into_btree(btree_fd, r.titulo, current_offset)) {
            std::cerr << "Erro ao inserir na B+Tree\n";
            break;
        }
        
        count++;
        if (count % 1000 == 0) {
            std::cout << "Processados " << count << " registros...\n";
        }
        
        current_offset += sizeof(Record);
    }
    
    close(data_fd);
    close(btree_fd);
    
    std::cout << "✅ Índice B+Tree completo criado com " << count 
              << " registros usando títulos diretos!\n";
    return 0;
}