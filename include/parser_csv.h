#ifndef PARSER_CSV_H
#define PARSER_CSV_H

#include "data_engine.h"
#include <vector>
#include <string>

/**
 * Lê um CSV e retorna os registros no formato Record (data_engine.h).
 * - Aceita separadores ';' (com aspas) e ','.
 * - Remove aspas automaticamente.
 * - Converte campos numéricos mesmo se vierem entre aspas.
 * - Ignora linhas inválidas (logs via stderr).
 */
std::vector<Record> parse_csv(const char *csv_path);

#endif // PARSER_CSV_H
