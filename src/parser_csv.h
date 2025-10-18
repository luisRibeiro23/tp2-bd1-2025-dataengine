#pragma once
#include "record.h"
#include <string>
#include <vector>

// Lê um CSV e retorna vetor de Records
std::vector<Record> parse_csv(const std::string &csv_path);
