#include "data_engine.h"
#include "parser_csv.h"
#include "bptree.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>   // system()

// ✅ Função para verificar existência do arquivo
bool file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

int main(int argc, char *argv[]) {
    // ✅ Verifica parâmetros obrigatórios
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <csv_input> <datafile_path>\n";
        return 1;
    }

    const char *csv_path    = argv[1];
    const char *data_path   = argv[2];
    const char *hash_path   = "/data/hash_index.db";
    const char *bptree_path = "/data/index_primary.idx";

    // ✅ Se o CSV não existir, baixar
    if (!file_exists(csv_path)) {
        std::cout << "📥 Arquivo CSV não encontrado localmente.\n";
        std::cout << "🔽 Baixando automaticamente do Google Drive...\n";

        const char* gz_path = "data/artigo.csv.gz";
        std::string cmd_wget =
            "wget --no-check-certificate --content-disposition "
            "--trust-server-name -O data/artigo.csv.gz "
            "\"https://drive.usercontent.google.com/download?id=1EVoP0d9Wwzj1O6eoFIkel9I3cpe43Gbv&export=download\"";

        if (system(cmd_wget.c_str()) != 0 || !file_exists(gz_path)) {
            std::cerr << "❌ Erro ao baixar o arquivo compactado do Drive.\n";
            return 1;
        }

        std::cout << "📦 Download concluído! Descompactando...\n";

        if (system("gunzip -f data/artigo.csv.gz") != 0 || !file_exists(csv_path)) {
            std::cerr << "❌ Erro ao descompactar o CSV.\n";
            return 1;
        }

        std::cout << "✅ CSV pronto para uso!\n";
    }

    // ✅ Carregar registros
    std::vector<Record> records = parse_csv(csv_path);
    if (records.empty()) {
        std::cerr << "Nenhum registro válido lido do CSV.\n";
        return 1;
    }

    // ✅ Criar índices
    init_hash_file(hash_path);
    BPTree t(bptree_path);
    criarArvore(t);

    // ✅ Criar arquivo de dados
    int fd = open(data_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Erro ao criar arquivo de dados");
        return 1;
    }

    // ✅ Gravar os registros
    size_t ok = 0;
    for (const auto &r : records) {
        off_t offset = write_record(fd, r);
        uint32_t bucket = hash_id(r.id);
        insert_offset_into_bucket(hash_path, bucket, offset);
        inserirChave(t, r.id, offset);
        ok++;
    }

    close(fd);
    fecharArq(t);

    std::cout << "✅ Upload concluído com sucesso!\n";
    std::cout << "📦 Total de registros inseridos: " << ok << "\n";
    std::cout << "📁 Data gravada em: " << data_path << "\n";
    std::cout << "🗂️ Índice hash salvo em: " << hash_path << "\n";
    std::cout << "🌳 Índice primário salvo em: " << bptree_path << "\n";

    return 0;
}
