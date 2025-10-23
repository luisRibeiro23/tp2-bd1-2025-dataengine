CXX = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude

# === COMPILAÇÃO ===
all: build

build: bin/upload bin/findrec bin/build_secondary bin/seek2

# Regra para compilar arquivos .o
src/%.o: src/%.cpp include/data_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Executáveis
bin/upload: src/data_engine.o src/upload.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/findrec: src/data_engine.o src/findrec.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/build_secondary: src/data_engine.o src/build_secondary.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/seek2: src/data_engine.o src/seek2.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

# === DOCKER ===
docker-build:
	docker build -t tp2 .

# === OPERAÇÕES PRINCIPAIS ===
docker-run-upload:
	docker run --rm -v $(PWD)/data:/data -v $(PWD)/data:/app/data tp2 /app/bin/upload /data/artigos.csv /data/data.db

docker-run-findrec:
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/findrec $(if $(ID),$(ID),1) /data/data.db /data/hash_index.db

# ✅ Corrigir: garantir que upload seja executado primeiro
docker-run-build-secondary:
	@if [ ! -f data/data.db ]; then echo "📁 Executando upload primeiro..."; make docker-run-upload; fi
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/build_secondary /data/data.db /data/titulo_index.btree

# ✅ Corrigir: garantir que B+Tree seja criado primeiro  
docker-run-seek2:
	@if [ ! -f data/titulo_index.btree ]; then echo "🔄 Criando B+Tree primeiro..."; make docker-run-build-secondary; fi
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/seek2 "Poster: 3D sketching and flexible input for surface design: A case study." /data/data.db /data/titulo_index.btree

# === TESTES ===
test-hash:
	@echo "🧪 Testando busca por ID (hash)..."
	@if [ ! -f data/data.db ]; then make docker-run-upload; fi
	make docker-run-findrec ID=1
	make docker-run-findrec ID=999

test-btree:
	@echo "🧪 Testando busca por título (B+Tree)..."
	make docker-run-seek2

test-all: docker-build test-hash test-btree
	@echo "✅ Todos os testes concluídos!"

# === LIMPEZA ===
clean:
	rm -rf bin src/*.o data/*.db data/*.btree

.PHONY: all build docker-build docker-run-upload docker-run-findrec docker-run-build-secondary docker-run-seek2 test-hash test-btree test-all clean