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

    // ✅ Validação de entrada
    std::cout << "🔍 Buscando ID: " << search_id << "\n";

    int hash_fd = open(hash_path, O_RDONLY);
    int data_fd = open(data_path, O_RDONLY);

    if (hash_fd < 0 || data_fd < 0) {
        std::cerr << "Erro ao abrir arquivos.\n";
        return 1;
    }

    uint32_t bucket_id = hash_id(search_id);
    std::cout << "📦 Bucket calculado: " << bucket_id << " (NUM_BUCKETS=" << NUM_BUCKETS << ")\n";

    // ✅ Verifica limites
    if (bucket_id >= NUM_BUCKETS) {
        std::cerr << "❌ Erro: Bucket ID " << bucket_id << " >= " << NUM_BUCKETS << "\n";
        close(hash_fd);
        close(data_fd);
        return 1;
    }

    off_t pos = bucket_id * sizeof(Bucket);
    
    // ✅ Verifica se posição está dentro do arquivo
    off_t file_size = lseek(hash_fd, 0, SEEK_END);
    if (pos >= file_size) {
        std::cerr << "⚠️ Posição " << pos << " além do tamanho do arquivo (" << file_size << ")\n";
        close(hash_fd);
        close(data_fd);
        return 1;
    }
    
    std::cout << "🔍 Buscando ID " << search_id << " no bucket " << bucket_id << "\n";
    
    Bucket b;
    bool found = false;
    int iterations = 0;
    const int MAX_ITERATIONS = 10;

    do {
        iterations++;
        if (iterations > MAX_ITERATIONS) {
            std::cerr << "⚠️ Muitas iterações - possível corrupção no índice\n";
            break;
        }
        
        // ✅ Lê bucket com verificação de erro
        ssize_t bytes_read = pread(hash_fd, &b, sizeof(Bucket), pos);
        if (bytes_read != sizeof(Bucket)) {
            std::cerr << "⚠️ Erro ao ler bucket na posição " << pos 
                      << " (lidos: " << bytes_read << "/" << sizeof(Bucket) << ")\n";
            break;
        }
        
        std::cout << "📦 Bucket " << iterations << ": count=" << b.count 
                  << ", next_overflow=" << b.next_overflow << "\n";
        
        // ✅ Valida count do bucket
        if (b.count < 0 || b.count > BUCKET_SIZE) {
            std::cerr << "⚠️ Bucket corrupto - count inválido: " << b.count << "\n";
            break;
        }
        
        // ✅ Se bucket vazio, não há nada para buscar
        if (b.count == 0) {
            std::cout << "📦 Bucket vazio\n";
            break;
        }

        for (int i = 0; i < b.count; i++) {
            // ✅ Verifica offset válido
            if (b.offsets[i] == INVALID_OFFSET || b.offsets[i] < 0) {
                std::cout << "⚠️ Offset inválido no slot " << i << ": " << b.offsets[i] << "\n";
                continue;
            }
            
            std::cout << "🔎 Verificando offset " << b.offsets[i] << "\n";
            
            // ✅ Lê registro com proteção
            Record r;
            ssize_t record_bytes = pread(data_fd, &r, sizeof(Record), b.offsets[i]);
            if (record_bytes != sizeof(Record)) {
                std::cerr << "⚠️ Erro ao ler registro no offset " << b.offsets[i] 
                          << " (lidos: " << record_bytes << "/" << sizeof(Record) << ")\n";
                continue;
            }
            
            std::cout << "📄 Lido ID: " << r.id << "\n";
            
            if (r.id == search_id) {
                std::cout << "✅ Registro encontrado!\n";
                std::cout << "ID: " << r.id << "\n";
                std::cout << "Título: " << r.titulo << "\n";
                std::cout << "Ano: " << r.ano << "\n";
                std::cout << "Autores: " << r.autores << "\n";
                std::cout << "Citações: " << r.citacoes << "\n";
                std::cout << "Atualização: " << r.atualizacao << "\n";
                std::cout << "Snippet: " << std::string(r.snippet).substr(0, 100) << "...\n";
                found = true;
                break;
            }
        }

        pos = b.next_overflow;
        
    } while (!found && b.next_overflow != INVALID_OFFSET && b.next_overflow > 0);

    if (!found) {
        std::cout << "❌ Registro com ID " << search_id << " não encontrado.\n";
    }

    close(hash_fd);
    close(data_fd);
    return found ? 0 : 1;
}