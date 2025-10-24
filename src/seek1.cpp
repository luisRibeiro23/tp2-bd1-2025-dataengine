#include "bptree.h"
#include "data_engine.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " <index_file> <data_file> <id>\n";
        return 1;
    }

    const char *index_path = argv[1];
    const char *data_path = argv[2];
    int id = std::stoi(argv[3]);

    BPTree t(index_path);
    criarArvore(t);

    // inicia a busca e mede o tempo de execução
    auto inicio = std::chrono::high_resolution_clock::now();
    off_t offset = buscarChave(t, id);
    auto fim = std::chrono::high_resolution_clock::now();
    double tempo = std::chrono::duration<double, std::milli>(fim - inicio).count();

    // caso o ID não esteja no índice
    if (offset == -1) {
        std::cout << "ID " << id << " não encontrado no índice.\n";
        std::cout << "Blocos lidos no índice: " << t.blocosLidos << "\n";
        std::cout << "Blocos totais do índice: " << t.totalBlocos << "\n";
        std::cout << "Tempo de busca: " << tempo << " ms\n";
        fecharArq(t);
        return 0;
    }

    // lê o registro correspondente em data.bin
    int fd = open(data_path, O_RDONLY);
    if (fd < 0) {
        std::cerr << "Erro ao abrir o arquivo de dados: " << data_path << "\n";
        fecharArq(t);
        return 1;
    }

    Record r = read_record_by_offset(fd, offset);
    close(fd);
    fecharArq(t);

    std::cout << "Registro encontrado via índice primário:\n";
    std::cout << "-----------------------------------------\n";
    std::cout << "ID: " << r.id << "\n";
    std::cout << "Título: " << r.titulo << "\n";
    std::cout << "Ano: " << r.ano << "\n";
    std::cout << "Autores: " << r.autores << "\n";
    std::cout << "Citações: " << r.citacoes << "\n";
    std::cout << "Atualização: " << r.atualizacao << "\n";
    std::cout << "Snippet: " << r.snippet << "\n";
    std::cout << "-----------------------------------------\n";
    std::cout << "Blocos lidos no índice: " << t.blocosLidos << "\n";
    std::cout << "Blocos totais do índice: " << t.totalBlocos << "\n";
    std::cout << "Tempo de busca: " << tempo << " ms\n";

    return 0;
}
