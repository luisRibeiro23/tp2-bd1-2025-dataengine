# TP2 - Sistema de Indexação de Banco de Dados

- Estefany Licinha Mendes da Silva - estefany.silva@icomp.ufam.edu.br
- Luís Henrique de Carvalho Ribeiro - luis.henrique@icomp.ufam.edu.br
- Samuel Davi Silva de Lima Chagas - samuel.chagas@icomp.ufam.edu.br

## Build e Execução

### Build Local

```bash
# Compilar todos os executáveis
make build

# Limpar arquivos compilados
make clean
```

### Build via Docker

```bash
# Criar imagem Docker
make docker-build

# Limpar tudo e recompilar
make clean && make docker-build
```

## Comandos de Execução

### 1. Upload - Carregamento de Dados e Criação de Índices

```bash
# Via Docker (recomendado)
make docker-run-upload

# Local
./bin/upload data/artigos.csv data/data.db
```

**Função**: Processa o arquivo CSV e cria simultaneamente:
- Arquivo de dados sequencial
- Hash Index por ID
- B+Tree Primária por ID  
- B+Tree Secundária por título

### 2. FindRec - Busca por ID usando Hash Index

```bash
# Via Docker
make docker-run-findrec ID=20

# Local
./bin/findrec 20 data/data.db data/hash_index.db
```

**Performance**: O(1) - busca mais rápida quando a chave é encontrada

### 3. Seek1 - Busca por ID usando B+Tree Primária

```bash
# Via Docker
make docker-run-seek1 ID=20

# Local  
./bin/seek1 data/index_primary.idx data/data.db 20
```

### 4. Seek2 - Busca por Título usando B+Tree Secundária

```bash
# Via Docker
make docker-run-seek2

# Local
./bin/seek2 "Poster: 3D sketching and flexible input for surface design: A case study." data/data.db data/titulo_index.btree
```

## Layout dos Arquivos em /data/

```
data/
├── artigos.csv                    # Arquivo CSV de entrada
├── data.db                        # Arquivo de dados principal (binário)
├── hash_index.db                  # Índice hash por ID
├── index_primary.idx              # B+Tree primária por ID
└── titulo_index.btree             # B+Tree secundária por título
```

### Detalhes dos Arquivos:

- **`data.db`**: Registros sequenciais de 1.6KB cada (1M+ registros)
- **`hash_index.db`**: 1024 buckets com encadeamento para colisões
- **`index_primary.idx`**: B+Tree com MAX_KEYS=128, blocos de 4KB
- **`titulo_index.btree`**: B+Tree especializada para strings, blocos de 4KB

## Exemplo de Entrada/Saída

### Entrada (artigos.csv - amostra):
```csv
id;titulo;ano;autores;citacoes;atualizacao;snippet
1;"Poster: 3D sketching and flexible input for surface design: A case study.";2013;"Anamary Leal|Doug A. Bowman";0;"2016-07-28 09:36:29";"Abstract content..."
5;"Design and implementation of an immersive virtual reality system based on a smartphone platform.";2013;"Anthony Steed|Simon Julier";12;"2016-10-03 21:10:56";"Technical content..."
```

### Saída - Upload:
```
📊 Inicializando índices...
✅ Hash index inicializado
✅ B+Tree primária inicializada  
✅ B+Tree secundária inicializada
📊 Processados 1000 registros...
📊 Processados 2000 registros...
...
🎉 Upload concluído com sucesso!
📊 Total de registros processados: 1021435
📁 Arquivo de dados: /data/data.db
🔗 Índice hash: data/hash_index.db
🌳 B+Tree primária (ID): data/index_primary.idx
🌳 B+Tree secundária (Título): data/titulo_index.btree
```

### Saída - FindRec (Hash):
```bash
$ make docker-run-findrec ID=1
Registro encontrado
-----------------------------------------
ID: 1
Título: Poster: 3D sketching and flexible input for surface design: A case study.
Ano: 2013
Autores: Anamary Leal|Doug A. Bowman
Citações: 0
Atualização: 2016-07-28 09:36:29
Snippet: "Poster: 3D sketching and flexible input for surface design..."
-----------------------------------------
Blocos lidos: 1
Blocos totais do arquivo de dados: 378051
```

### Saída - Seek1 (B+Tree Primária):
```bash
$ make docker-run-seek1 ID=5
Registro encontrado via índice primário:
-----------------------------------------
ID: 5
Título: Design and implementation of an immersive virtual reality system based on a smartphone platform.
Ano: 2013
Autores: Anthony Steed|Simon Julier
Citações: 12
Atualização: 2016-10-03 21:10:56
Snippet: "Design and implementation of an immersive virtual reality system..."
-----------------------------------------
Blocos lidos no índice: 4
Blocos totais do índice: 15960
Tempo de busca: 0.009419 ms
```

### Saída - Seek2 (B+Tree Secundária):
```bash
$ make docker-run-seek2
🔍 Buscando título na B+Tree secundária...
📖 Título: "Poster: 3D sketching and flexible input for surface design: A case study."
✅ Título encontrado na B+Tree, offset: 0
Registro encontrado via índice secundário:
-----------------------------------------
ID: 1
Título: Poster: 3D sketching and flexible input for surface design: A case study.
Ano: 2013
Autores: Anamary Leal|Doug A. Bowman
Citações: 0
Atualização: 2016-07-28 09:36:29
Snippet: "Poster: 3D sketching and flexible input for surface design..."
-----------------------------------------
Tempo de busca: 0.029 ms
```

## Testes Automatizados

```bash
# Testar todos os métodos de indexação
make test-all

# Testar apenas Hash Index
make test-hash

# Testar apenas B+Tree Primária  
make test-btree-primary

# Testar apenas B+Tree Secundária
make test-btree-secondary
```

