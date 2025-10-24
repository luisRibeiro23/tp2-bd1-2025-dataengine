#include "data_engine.h"
#include "bptree.h"
#include "btree_sec.h"
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

std::string remove_quotes(const std::string& str) {
    if (str.length() < 2) return str;
    
    std::string result = str;
    if (result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.length() - 2);
    }
    return result;
}

void safe_strcpy(char* dest, const std::string& src, size_t max_len) {
    size_t copy_len = std::min(src.length(), max_len);
    std::memcpy(dest, src.c_str(), copy_len);
    dest[copy_len] = '\0';
}

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current_field;
    bool inside_quotes = false;
    
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        
        if (c == '"') {
            inside_quotes = !inside_quotes;
            current_field += c;
        } else if (c == ';' && !inside_quotes) {
            fields.push_back(remove_quotes(current_field));
            current_field.clear();
        } else {
            current_field += c;
        }
    }
    
    if (!current_field.empty()) {
        fields.push_back(remove_quotes(current_field));
    }
    
    return fields;
}

int main(int argc, char *argv[]) {
    // ✅ Verifica os argumentos na entrada
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <csv_input> <datafile_path>\n";
        return 1;
    }

    const char *csv_path = argv[1];
    const char *data_path = argv[2];
    const char *hash_path = "data/hash_index.db";
    const char *bptree_primary_path = "data/index_primary.idx";
    const char *bptree_secondary_path = "data/titulo_index.btree";  // ✅ ADICIONAR

    // ✅ Inicializar 3 índices
    std::cout << "📊 Inicializando índices...\n";
    
    // 1. Hash index
    init_hash_file(hash_path);
    std::cout << "✅ Hash index inicializado\n";

    // 2. B+Tree primária (por ID)
    BPTree primary_tree(bptree_primary_path);
    criarArvore(primary_tree);
    std::cout << "✅ B+Tree primária inicializada\n";

    // 3. B+Tree secundária (por título)
    init_btree_file(bptree_secondary_path);
    std::cout << "✅ B+Tree secundária inicializada\n";
    
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        std::cerr << "❌ Erro ao abrir o arquivo CSV: " << csv_path << "\n";
        return 1;
    }
    //
    int fd = open(data_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        std::cerr << "❌ Erro ao criar arquivo de dados.\n";
        return 1;
    }

    std::string line;
    int line_number = 0;
    int processed_count = 0;

    std::cout << "📄 Iniciando processamento do CSV...\n";

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        line_number++;
        
        if (processed_count % 1000 == 0 && processed_count > 0) {
            std::cout << "📊 Processados " << processed_count << " registros...\n";
        }
        
        try {
            std::vector<std::string> fields = parse_csv_line(line);
            
            if (fields.size() < 6) {
                std::cerr << "⚠️  Linha " << line_number << " ignorada - poucos campos\n";
                continue;
            }
            
            Record r;
            memset(&r, 0, sizeof(Record));
            
            r.id = std::stoi(fields[0]);
            safe_strcpy(r.titulo, fields[1], MAX_TITLE_LEN);
            r.ano = std::stoi(fields[2]);
            safe_strcpy(r.autores, fields[3], MAX_AUTHORS_LEN);
            r.citacoes = std::stoi(fields[4]);
            safe_strcpy(r.atualizacao, fields[5], 19);
            
            if (fields.size() > 6) {
                safe_strcpy(r.snippet, fields[6], MAX_SNIPPET_LEN);
            }
            
            // ✅ Escrever registro no arquivo
            off_t offset = write_record(fd, r);
            
            // ✅ Indexar nos 3 índices
            
            // 1. Hash index (por ID)
            uint32_t bucket = hash_id(r.id);
            insert_offset_into_bucket(hash_path, bucket, offset);
            
            // 2. B+Tree primária (por ID)
            inserirChave(primary_tree, r.id, offset);
            
            // 3. B+Tree secundária (por título)
            insert_into_btree(0, r.titulo, offset);
            
            processed_count++;
            
            // Debug detalhado para primeiros registros
            if (processed_count <= 10) {
                std::cout << "📝 ID " << r.id 
                          << " → Hash bucket " << bucket
                          << " → Offset " << offset 
                          << " → Título: \"" << std::string(r.titulo).substr(0, 40) << "...\"\n";
            }
                      
        } catch (const std::exception& e) {
            std::cerr << "⚠️  Erro ao processar linha " << line_number 
                      << ": " << e.what() << "\n";
            continue;
        }
    }

    // ✅ Fechar estruturas
    close(fd);
    file.close();
    fecharArq(primary_tree);
    close_btree();

    std::cout << "\n🎉 Upload concluído com sucesso!\n";
    std::cout << "📊 Total de registros processados: " << processed_count << "\n";
    std::cout << "📁 Arquivo de dados: " << data_path << "\n";
    std::cout << "🔗 Índice hash: " << hash_path << "\n";
    std::cout << "🌳 B+Tree primária (ID): " << bptree_primary_path << "\n";
    std::cout << "🌳 B+Tree secundária (Título): " << bptree_secondary_path << "\n";
    std::cout << "\n✅ Sistema com 3 índices pronto para uso!\n";
    
    return 0;
}