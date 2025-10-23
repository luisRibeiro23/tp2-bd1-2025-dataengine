#include "data_engine.h"
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <algorithm>

// Remove aspas do início e fim de uma string (versão segura)
std::string remove_quotes(const std::string& str) {
    if (str.length() < 2) return str;  // ✅ Proteção contra strings curtas
    
    std::string result = str;
    if (result.front() == '"' && result.back() == '"') {
        result = result.substr(1, result.length() - 2);
    }
    return result;
}

// Copia string com limitação segura de tamanho
void safe_strcpy(char* dest, const std::string& src, size_t max_len) {
    size_t copy_len = std::min(src.length(), max_len);
    std::memcpy(dest, src.c_str(), copy_len);
    dest[copy_len] = '\0';  // Garante terminação
}

// Parse CSV com delimitador ponto e vírgula e campos com aspas
std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current_field;
    bool inside_quotes = false;
    
    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];
        
        if (c == '"') {
            inside_quotes = !inside_quotes;
            current_field += c;  // Mantém as aspas para remoção posterior
        } else if (c == ';' && !inside_quotes) {
            fields.push_back(remove_quotes(current_field));
            current_field.clear();
        } else {
            current_field += c;
        }
    }
    
    // Adiciona último campo
    if (!current_field.empty()) {
        fields.push_back(remove_quotes(current_field));
    }
    
    return fields;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: " << argv[0] << " <csv_input> <datafile_path>\n";
        std::cerr << "Formato esperado: \"id\";\"titulo\";\"ano\";\"autores\";\"citacoes\";\"atualizacao\";\"snippet\"\n";
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

    std::string line;
    int line_number = 0;
    int processed_count = 0;

    while (std::getline(file, line)) {
        line_number++;
        
        // Pula linhas vazias
        if (line.empty()) continue;
        
        // Debug: mostra progresso a cada 10 registros
        if (line_number % 10 == 0) {
            std::cout << "📄 Processando linha " << line_number << "...\n";
        }
        
        try {
            // Parse da linha CSV com delimitador ';'
            std::vector<std::string> fields = parse_csv_line(line);
            
            // Verifica se tem todos os campos necessários
            if (fields.size() < 6) {
                std::cerr << "⚠️  Linha " << line_number << " ignorada - poucos campos (" 
                          << fields.size() << "/6)\n";
                continue;
            }
            
            Record r;
            memset(&r, 0, sizeof(Record));  // ✅ Zera toda estrutura
            
            // Parse dos campos (formato: id;titulo;ano;autores;citacoes;atualizacao;snippet)
            r.id = std::stoi(fields[0]);
            
            // ✅ Copia segura com limitação de tamanho
            safe_strcpy(r.titulo, fields[1], MAX_TITLE_LEN);
            
            r.ano = std::stoi(fields[2]);
            
            safe_strcpy(r.autores, fields[3], MAX_AUTHORS_LEN);
            
            r.citacoes = std::stoi(fields[4]);
            
            safe_strcpy(r.atualizacao, fields[5], 19);
            
            // Snippet (campo 6, se existir)
            if (fields.size() > 6) {
                safe_strcpy(r.snippet, fields[6], MAX_SNIPPET_LEN);
            }
            
            off_t offset = write_record(fd, r);
            uint32_t bucket = hash_id(r.id);
            insert_offset_into_bucket(hash_path, bucket, offset);

            processed_count++;
            std::cout << "ID " << r.id << " → bucket " << bucket
                      << " → offset " << offset << " [" 
                      << std::string(r.titulo).substr(0, 50) << "...]\n";
                      
        } catch (const std::exception& e) {
            std::cerr << "⚠️  Erro ao processar linha " << line_number 
                      << ": " << e.what() << "\n";
            continue;
        }
    }

    close(fd);
    file.close();
    
    std::cout << "✅ Upload com hashing concluído com sucesso!\n";
    std::cout << "📄 Processadas " << line_number << " linhas, " 
              << processed_count << " registros válidos\n";
    return 0;
}