# === Base ===
FROM ubuntu:22.04

# === Dependências ===
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make cmake && \
    rm -rf /var/lib/apt/lists/*

# === Diretório de trabalho ===
WORKDIR /app

# === Copiar código-fonte e cabeçalhos ===
COPY src/ ./src/
COPY include/ ./include/
COPY Makefile ./

# === Compilar dentro do container ===
RUN make build

# === Criar diretório de dados ===
RUN mkdir -p /app/data

# === Volume de dados persistente ===
VOLUME ["/data"]

# === Instrução ao usuário ===
CMD ["bash", "-lc", "echo '✅ Imagem pronta! Monte o diretório data e execute: /app/bin/upload /data/artigo.csv /data/data.db'"]
