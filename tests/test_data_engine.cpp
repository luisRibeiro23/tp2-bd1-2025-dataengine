#include "data_engine.h"
#include <cassert>
#include <fcntl.h>
#include <unistd.h>

int main() {
    Record r;
    r.id = 1;
    strcpy(r.titulo, "Teste");
    r.ano = 2024;
    strcpy(r.autores, "Autor Exemplo");
    r.citacoes = 42;
    strcpy(r.atualizacao, "2025-10-11");
    strcpy(r.snippet, "Resumo de teste");

    int fd = open("data/test.db", O_CREAT | O_RDWR | O_TRUNC, 0666);
    off_t offset = write_record(fd, r);

    Record r2 = read_record_by_offset(fd, offset);
    assert(r2.id == 1);
    assert(std::string(r2.titulo) == "Teste");

    close(fd);
    printf("✅ Teste de gravação/leitura passou!\n");
}
