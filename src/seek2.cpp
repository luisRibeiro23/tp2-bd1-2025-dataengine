#include "data_engine.h"
#include <fcntl.h>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " \"<titulo_exato>\" <data_file> <btree_file>\n";
        return 1;
    }
    
    std::string search_titulo = argv[1];
    const char *data_file = argv[2];
    const char *btree_file = argv[3];
    
    std::cout << "🔍 Buscando título: \"" << search_titulo << "\"\n";
    
    // ✅ Busca direta por título (sem hash)
    off_t data_offset = search_btree(btree_file, search_titulo.c_str());
    
    if (data_offset == -1) {
        std::cout << "❌ Título não encontrado na B+Tree\n";
        return 1;
    }
    
    std::cout << "✅ Título encontrado na B+Tree, offset: " << data_offset << "\n";
    
    // Lê registro do arquivo de dados
    int fd = open(data_file, O_RDONLY);
    if (fd < 0) {
        std::cerr << "❌ Erro ao abrir arquivo de dados\n";
        return 1;
    }
    
    Record r;
    if (pread(fd, &r, sizeof(Record), data_offset) != sizeof(Record)) {
        std::cerr << "❌ Erro ao ler registro\n";
        close(fd);
        return 1;
    }
    close(fd);
    
    std::cout << "✅ Registro encontrado!\n";
    std::cout << "ID: " << r.id << "\n";
    std::cout << "Título: " << r.titulo << "\n";
    std::cout << "Ano: " << r.ano << "\n";
    std::cout << "Autores: " << r.autores << "\n";
    std::cout << "Citações: " << r.citacoes << "\n";
    
    return 0;
}