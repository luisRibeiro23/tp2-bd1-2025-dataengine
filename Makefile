CXX = g++
CXXFLAGS = -Wall -std=c++17 -Iinclude
SRC = src/data_engine.cpp src/upload.cpp
OBJ = $(SRC:.cpp=.o)

# Targets principais
all: build

build: bin/upload bin/findrec

# Regra para compilar arquivos .o
src/%.o: src/%.cpp include/data_engine.h src/record.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Executáveis
bin/upload: src/data_engine.o src/upload.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/findrec: src/data_engine.o src/findrec.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^

# Docker targets
docker-build:
	docker build -t tp2 .

docker-run-upload:
	docker run --rm -v $(PWD)/data:/data -v $(PWD)/data:/app/data tp2 /app/bin/upload /data/artigos.csv /data/data.db

docker-run-findrec:
	docker run --rm -v $(PWD)/data:/data tp2 /app/bin/findrec 1 /data/data.db /data/hash_index.db

docker-run-findrec-debug:
	docker run --rm -v $(PWD)/data:/data tp2 sh -c "ls -la /data && /app/bin/findrec 1 /data/data.db /data/hash_index.db"

docker-run-upload-debug:
	docker run --rm -v $(PWD)/data:/data -v $(PWD)/data:/app/data tp2 sh -c "/app/bin/upload /data/artigos.csv /data/data.db && ls -la /data && ls -la /app/data"


# Limpeza
clean:
	rm -rf bin src/*.o data/*.db

.PHONY: all build docker-build docker-run-upload docker-run-findrec clean