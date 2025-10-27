#include "btree_sec.h"
#include <fcntl.h>
#include <iostream>
#include <string>
#include <chrono>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " \"<titulo_exato>\" <data_file> <btree_file>\n";
        return 1;
    }
    
    std::string search_titulo = argv[1];
    const char *data_file = argv[2];
    const char *btree_file = argv[3];
    
    std::cout << "Buscando título na B+Tree secundária...\n";
    std::cout << "Título: \"" << search_titulo << "\"\n";
    
    auto inicio = std::chrono::high_resolution_clock::now();
    
    // Buscar na B+Tree
    off_t data_offset = search_btree(btree_file, search_titulo.c_str());
    
    auto fim = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::microseconds>(fim - inicio);
    
    if (data_offset == -1) {
        std::cout << "Título não encontrado na B+Tree secundária\n";
        return 1;
    }
    
    std::cout << "Título encontrado na B+Tree, offset: " << data_offset << "\n";
    
    // Ler registro do arquivo de dados
    int fd = open(data_file, O_RDONLY);
    if (fd < 0) {
        std::cerr << "Erro ao abrir arquivo de dados\n";
        return 1;
    }
    
    Record r;
    if (pread(fd, &r, sizeof(Record), data_offset) != sizeof(Record)) {
        std::cerr << "Erro ao ler registro\n";
        close(fd);
        return 1;
    }
    close(fd);
    
    std::cout << "Registro encontrado via índice secundário:\n";
    std::cout << "-----------------------------------------\n";
    std::cout << "ID: " << r.id << "\n";
    std::cout << "Título: " << r.titulo << "\n";
    std::cout << "Ano: " << r.ano << "\n";
    std::cout << "Autores: " << r.autores << "\n";
    std::cout << "Citações: " << r.citacoes << "\n";
    std::cout << "Atualização: " << r.atualizacao << "\n";
    std::cout << "Snippet: " << r.snippet << "\n";
    std::cout << "-----------------------------------------\n";
    std::cout << "Tempo de busca: " << duracao.count() / 1000.0 << " ms\n";
    
    return 0;
}