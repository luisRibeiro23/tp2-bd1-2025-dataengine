#include "parser_csv.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<Record> parse_csv(const std::string &csv_path) {
    std::vector<Record> result;
    std::ifstream ifs(csv_path);
    if (!ifs.is_open()) {
        std::cerr << "Erro abrindo CSV: " << csv_path << "\\n";
        return result;
    }

    std::string line;
    // Pular header (se houver)
    if (!std::getline(ifs, line)) return result;

    while (std::getline(ifs, line)) {
        std::istringstream ss(line);
        std::string token;
        Record r{};
        // Campos: id,titulo,ano,autores,citacoes,atualizacao,snippet
        if (!std::getline(ss, token, ',')) continue;
        r.id = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;
        r.titulo = token;
        if (!std::getline(ss, token, ',')) continue;
        r.ano = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;
        r.autores = token;
        if (!std::getline(ss, token, ',')) continue;
        r.citacoes = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;
        r.atualizacao = token;
        if (!std::getline(ss, token, ',')) token = "";
        r.snippet = token;
        result.push_back(std::move(r));
    }
    return result;
}
