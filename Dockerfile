FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
      g++ make cmake && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copiar apenas os arquivos necessários
COPY src/ ./src/
COPY include/ ./include/
COPY Makefile ./

# Compilar DENTRO do container
RUN make build

# Criar diretório data dentro do container (para o path hardcoded)
RUN mkdir -p /app/data

# Diretório para dados persistentes
VOLUME ["/data"]

# Variáveis de ambiente
ENV CSV_PATH=/data/input.csv \
    DATA_DIR=/data/db \
    LOG_LEVEL=info

CMD ["bash", "-lc", "echo 'Use: docker run ... upload|findrec'; ls -l bin/"]