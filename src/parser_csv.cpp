#include "parser_csv.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <algorithm>   // std::count
#include <cctype>      // std::isspace

// ========================= Funções auxiliares =========================

//  Remove aspas externas de campos que começam e terminam com '"'
static inline std::string trim_quotes(const std::string &s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

//  Detecta delimitador (',' ou ';') com base na primeira linha do CSV
static inline char detect_delim(const std::string &header_line) {
    size_t sc = 0, cc = 0;
    for (char c : header_line) {
        if (c == ';') ++sc;
        else if (c == ',') ++cc;
    }
    return (sc > cc ? ';' : ',');
}

//  Divide a linha respeitando campos entre aspas
static std::vector<std::string> split_csv_quoted(const std::string &line, char delim) {
    std::vector<std::string> out;
    std::string cur;
    bool in_quotes = false;

    for (char c : line) {
        if (c == '"') {
            in_quotes = !in_quotes;
            cur.push_back(c);
        } else if (c == delim && !in_quotes) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

// 🔹 Converte string numérica para inteiro com segurança
static bool to_int_safe(const std::string &s, int &out) {
    try {
        std::string t = trim_quotes(s);
        size_t i = 0, j = t.size();
        while (i < j && std::isspace(static_cast<unsigned char>(t[i]))) ++i;
        while (j > i && std::isspace(static_cast<unsigned char>(t[j - 1]))) --j;
        if (i >= j) return false;
        out = std::stoi(t.substr(i, j - i));
        return true;
    } catch (...) {
        return false;
    }
}

// 🔹 Copia string para campo char[] respeitando tamanho do Record
static void copy_cstr(char *dst, size_t dst_sz, const std::string &s) {
    if (dst_sz == 0) return;
    std::string t = trim_quotes(s);
    std::strncpy(dst, t.c_str(), dst_sz - 1);
    dst[dst_sz - 1] = '\0';
}

// ====================== PARSER PRINCIPAL ======================

std::vector<Record> parse_csv(const char *csv_path) {
    std::vector<Record> records;

    //  Abre arquivo CSV
    std::ifstream file(csv_path);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo CSV: " << csv_path << "\n";
        return records;
    }

    //  Lê cabeçalho e detecta delimitador
    std::string header;
    if (!std::getline(file, header)) {
        std::cerr << "CSV vazio.\n";
        return records;
    }
    char delim = detect_delim(header);

    //  Controla linha acumulada e número de linhas lidas
    std::string rawLine, fullLine;
    size_t lineno = 1;

    //  Lê o arquivo linha por linha reconstruindo registros quebrados por aspas
    while (std::getline(file, rawLine)) {
        ++lineno;

        if (!fullLine.empty()) fullLine += '\n';
        fullLine += rawLine;

        // Lê aspas abertas até fechar
        size_t quoteCount = std::count(fullLine.begin(), fullLine.end(), '"');
        if (quoteCount % 2 != 0) continue;

        // Divide quando as aspas estão balanceadas
        std::vector<std::string> fields = split_csv_quoted(fullLine, delim);

        // Se ainda faltar campo tenta ajustar com mais linhas
        while (fields.size() < 7 && std::getline(file, rawLine)) {
            ++lineno;
            fullLine += '\n' + rawLine;
            quoteCount = std::count(fullLine.begin(), fullLine.end(), '"');
            if (quoteCount % 2 != 0) continue;
            fields = split_csv_quoted(fullLine, delim);
        }

        // Se continuar iválido ignora tudo
        if (fields.size() < 7) {
            std::cerr << "⚠️ Linha " << lineno << " ignorada (campos insuficientes): "
                      << fullLine << "\n";
            fullLine.clear();
            continue;
        }

        //  Constrói o Record preenchendo cada campo
        Record r{};
        int v;

        if (!to_int_safe(fields[0], v)) { fullLine.clear(); continue; }
        r.id = v;

        copy_cstr(r.titulo,      sizeof(r.titulo),      fields[1]);
        if (!to_int_safe(fields[2], v)) { fullLine.clear(); continue; }
        r.ano = v;

        copy_cstr(r.autores,     sizeof(r.autores),     fields[3]);
        if (!to_int_safe(fields[4], v)) { fullLine.clear(); continue; }
        r.citacoes = v;

        copy_cstr(r.atualizacao, sizeof(r.atualizacao), fields[5]);
        copy_cstr(r.snippet,     sizeof(r.snippet),     fields[6]);

        records.push_back(r);

        fullLine.clear();
    }

    return records;
}
