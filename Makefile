CXX = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude

# === COMPILAÇÃO ===
all: build

build: bin/upload bin/findrec bin/seek1 bin/seek2

# Regra genérica para arquivos .o
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Regras específicas
src/bptree.o: src/bptree.cpp include/bptree.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/btree_sec.o: src/btree_sec.cpp include/btree_sec.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Binários finais
bin/upload: src/data_engine.o src/upload.o src/bptree.o src/btree_sec.o src/parser_csv.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/findrec: src/data_engine.o src/findrec.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/seek1: src/data_engine.o src/seek1.o src/bptree.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/seek2: src/data_engine.o src/seek2.o src/btree_sec.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

# === DOCKER ===
docker-build:
	docker build -t tp2 .

# === EXECUÇÃO COM DOCKER ===
docker-run-upload:
	@if [ ! -f data/artigo.csv ]; then echo "❌ CSV não encontrado em data/artigo.csv"; exit 1; fi
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/upload /data/artigo.csv /data/data.db

docker-run-findrec:
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/findrec $(if $(ID),$(ID),1) /data/data.db /data/hash_index.db

docker-run-seek1:
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/seek1 /data/index_primary.idx /data/data.db $(if $(ID),$(ID),1)

docker-run-seek2:
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/seek2 "Poster: 3D sketching and flexible input for surface design: A case study." /data/data.db /data/titulo_index.btree

# === TESTES ===
test-hash:
	@echo "🧪 Testando busca por ID (hash)..."
	@if [ ! -f data/data.db ]; then make docker-run-upload; fi
	make docker-run-findrec ID=1
	make docker-run-findrec ID=999

test-btree-primary:
	@echo "🧪 Testando busca por ID (B+Tree primária)..."
	@if [ ! -f data/data.db ]; then make docker-run-upload; fi
	make docker-run-seek1 ID=1
	make docker-run-seek1 ID=5
	make docker-run-seek1 ID=999

test-btree-secondary:
	@echo "🧪 Testando busca por título (B+Tree secundária)..."
	@if [ ! -f data/data.db ]; then make docker-run-upload; fi
	make docker-run-seek2

test-all: docker-build test-hash test-btree-primary test-btree-secondary
	@echo "✅ Todos os testes concluídos!"

# === LIMPEZA ===
clean:
	rm -rf bin src/*.o data/*.db data/*.btree data/*.idx

.PHONY: all build docker-build docker-run-upload docker-run-findrec docker-run-seek1 docker-run-seek2 test-hash test-btree-primary test-btree-secondary test-all clean
