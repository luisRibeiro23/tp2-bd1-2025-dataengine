#include "data_engine.h"
#include "parser_csv.h"
#include "bptree.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // ✅ Verifica parâmetros obrigatórios: CSV de entrada e arquivo de dados.
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <csv_input> <datafile_path>\n";
        return 1;
    }

    // ✅ Caminhos de arquivos utilizados.
    const char *csv_path    = argv[1];
    const char *data_path   = argv[2];
    const char *hash_path   = "data/hash_index.db";     // índice hash
    const char *bptree_path = "data/index_primary.idx"; // B+ tree (chave primária)

    // ✅ Lê todos os registros do CSV (parser trata quebras e inconsistências).
    std::vector<Record> records = parse_csv(csv_path);
    if (records.empty()) {
        std::cerr << "Nenhum registro válido lido do CSV.\n";
        return 1;
    }

    // ✅ Inicializa índice hash e B+ tree vazios.
    init_hash_file(hash_path);
    BPTree t(bptree_path);
    criarArvore(t);

    // ✅ Cria arquivo principal de dados.
    int fd = open(data_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Erro ao criar arquivo de dados");
        return 1;
    }

    // ✅ Para cada registro: grava no arquivo, insere hash e chave primária.
    size_t ok = 0;
    for (const auto &r : records) {
        off_t offset = write_record(fd, r);           // gravação do dado
        uint32_t bucket = hash_id(r.id);              // hash por ID
        insert_offset_into_bucket(hash_path, bucket, offset);

        // (Opcional) Mostra inserções especiais dos registros quebrados.
        if (r.id == 368004 || r.id == 424931 || r.id == 500462 || r.id == 738289) {
            std::cout << "📦 Hash inserido para especial ID " << r.id
                      << " no bucket " << bucket
                      << " com offset " << offset << std::endl;
        }

        inserirChave(t, r.id, offset);                // insere na B+ tree
        ok++;
    }

    // ✅ Fecha arquivos e confirma.
    close(fd);
    fecharArq(t);

    std::cout << "✅ Upload concluído com sucesso!\n";
    std::cout << "📦 Total de registros inseridos: " << ok << "\n";
    std::cout << "📁 Data gravada em: " << data_path << "\n";
    std::cout << "🗂️ Índice hash salvo em: " << hash_path << "\n";
    std::cout << "🌳 Índice primário salvo em: " << bptree_path << "\n";
    return 0;
}
