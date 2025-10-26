#include "bptree.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>

// ---------------- Operações de Bloco ----------------

// Zera o conteúdo de um bloco de memória, preenchendo o bloco com zeros
static void zerarBloco(std::vector<char>& buffer) {
    std::fill(buffer.begin(), buffer.end(), 0);
}

// Escreve um nó da árvore em uma posição específica do arquivo
static void escreverBloco(std::fstream& arq, const BPTreeNode& no, ptr_t posicao) {
    std::vector<char> buffer(BLOCK_SIZE);
    zerarBloco(buffer);
    std::memcpy(buffer.data(), &no, sizeof(BPTreeNode));
    arq.seekp(posicao, std::ios::beg);
    arq.write(buffer.data(), BLOCK_SIZE);
    arq.flush();
}

// Lê um nó da árvore a partir de uma posição específica do arquivo e copia para a estrutura BPTreeNode em memória
static void lerBloco(std::fstream& arq, BPTreeNode& no, ptr_t posicao) {
    std::vector<char> buffer(BLOCK_SIZE);
    arq.seekg(posicao, std::ios::beg);
    arq.read(buffer.data(), BLOCK_SIZE);
    std::memcpy(&no, buffer.data(), sizeof(BPTreeNode));
}

// Calcula o tamanho total (em bytes) do arquivo associado à árvore, retornando a posição final do ponteiro de leitura
static ptr_t tamanhoArquivo(std::fstream& arq) {
    auto atual = arq.tellg();
    arq.seekg(0, std::ios::end);
    auto fim = arq.tellg();
    arq.seekg(atual, std::ios::beg);
    return static_cast<ptr_t>(fim);
}

// Faz uma divisão inteira com arredondamento para cima, é usada para calcular o número total de blocos no arquivo
static int divArredondada(ptr_t a, int b) {
    return static_cast<int>((a + b - 1) / b);
}

// ---------------- Cabeçalho ----------------

// Escreve no início do arquivo, no bloco 0, a posição do nó raiz da árvore, atualizando o cabeçalho da estrutura no disco
static void escreverCabecalho(BPTree& t) {
    std::vector<char> buffer(BLOCK_SIZE);
    zerarBloco(buffer);
    std::memcpy(buffer.data(), &t.rootOffset, sizeof(ptr_t));
    t.arquivo.seekp(0, std::ios::beg);
    t.arquivo.write(buffer.data(), BLOCK_SIZE);
    t.arquivo.flush();
}

// Lê o cabeçalho do arquivo e atualiza o campo rootOffset da estrutura BPTree, indicando onde está a raiz
static void lerCabecalho(BPTree& t) {
    std::vector<char> buffer(BLOCK_SIZE);
    t.arquivo.seekg(0, std::ios::beg);
    t.arquivo.read(buffer.data(), BLOCK_SIZE);
    std::memcpy(&t.rootOffset, buffer.data(), sizeof(ptr_t));
}

// ---------------- Alocação ----------------

// Cria e grava um novo nó folha vazio no final do arquivo, inicializando seus campos com valores padrão e retornando sua posição
static ptr_t alocarNo(BPTree& t) {
    t.arquivo.seekp(0, std::ios::end);
    ptr_t posicao = static_cast<ptr_t>(t.arquivo.tellp());

    BPTreeNode novoNo{};
    novoNo.folha = 1;
    novoNo.nChaves = 0;
    novoNo.proxFolha = -1;

    for (int i = 0; i < MAX_KEYS + 1; i++) novoNo.filhos[i] = -1;
    for (int i = 0; i < MAX_KEYS; i++) novoNo.valores[i] = -1;

    escreverBloco(t.arquivo, novoNo, posicao);
    return posicao;
}

// ---------------- Criar e fechar árvore ----------------

/* Abre um arquivo de índice existente ou cria um novo, inicializando a raiz da árvore
   Também grava o cabeçalho e define o número total de blocos */
void criarArvore(BPTree& t) {
    t.arquivo.open(t.nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
    // Caso o arquivo não exista, cria e inicializa um novo
    if (!t.arquivo.is_open()) {
        std::fstream arq(t.nomeArquivo, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!arq.is_open()) {
            std::cerr << "Erro ao criar o índice: " << t.nomeArquivo << "\n";
            return;
        }

        std::vector<char> buffer(BLOCK_SIZE, 0);
        arq.write(buffer.data(), BLOCK_SIZE);

        BPTreeNode raiz{};
        raiz.folha = 1;
        raiz.nChaves = 0;
        raiz.proxFolha = -1;

        ptr_t posicaoRaiz = BLOCK_SIZE;
        escreverBloco(arq, raiz, posicaoRaiz);

        // Atualiza cabeçalho com a posição da raiz
        std::memcpy(buffer.data(), &posicaoRaiz, sizeof(ptr_t));
        arq.seekp(0, std::ios::beg);
        arq.write(buffer.data(), BLOCK_SIZE);
        arq.flush();
        arq.close();

        // Reabre o arquivo em modo leitura/escrita binário
        t.arquivo.open(t.nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
    }
    lerCabecalho(t);
    ptr_t tam = tamanhoArquivo(t.arquivo);
    t.totalBlocos = divArredondada(tam, BLOCK_SIZE);
    t.blocosLidos = 0;
}

// Fecha o arquivo associado à árvore, caso esteja aberto
void fecharArq(BPTree& t) {
    if (t.arquivo.is_open()) 
        t.arquivo.close();
}

// ---------------- Busca ----------------

// Realiza uma busca binária para encontrar a posição onde uma nova chave deve ser inserida dentro de um nó ordenado
static int limiteSuperior(const key_t* chaves, int n, key_t chave) {
    int inicio = 0;
    int fim = n;
    while (inicio < fim) {
        int meio = (inicio + fim) >> 1;
        if (chaves[meio] <= chave) 
            inicio = meio + 1;
        else fim = meio;
    }
    return inicio;
}

/* Percorre a árvore desde a raiz até encontrar a folha correspondente
   Se a chave for encontrada, retorna o ponteiro para o valor associado. Se não, retorna -1 */
ptr_t buscarChave(BPTree& t, key_t chave) {
    t.blocosLidos = 0;
    BPTreeNode no{};
    ptr_t atual = t.rootOffset;
    while (true) {
        lerBloco(t.arquivo, no, atual);
        t.blocosLidos++;
        if (no.folha) {
            for (int i = 0; i < no.nChaves; i++) {
                if (no.chaves[i] == chave)
                    return no.valores[i];
            }
            return -1; // chave não encontrada
        } else {
            int i = limiteSuperior(no.chaves, no.nChaves, chave);
            ptr_t filho = no.filhos[i];
            if (filho < 0) return -1;
            atual = filho;
        }
    }
}

// ---------------- Inserção ----------------

/* Insere uma chave e seu valor em um nó folha
   Se houver espaço, apenas insere e grava o nó
   Se não, realiza o split e atualiza a chave separadora e a posição do nó direito */
static bool inserirNaFolha(BPTree& t, ptr_t posFolha, key_t chave, ptr_t valor, key_t& chaveSeparadora, ptr_t& posDireita) {
    BPTreeNode folha{};
    lerBloco(t.arquivo, folha, posFolha);

    // Atualiza o valor se a chave já existir
    int posicao = std::lower_bound(folha.chaves, folha.chaves + folha.nChaves, chave) - folha.chaves;
    if (posicao < folha.nChaves && folha.chaves[posicao] == chave) {
        folha.valores[posicao] = valor;
        escreverBloco(t.arquivo, folha, posFolha);
        return false;
    }

    // Verifica se ainda cabe no nó
    if (folha.nChaves < MAX_KEYS) {
        for (int i = folha.nChaves; i > posicao; --i) {
            folha.chaves[i] = folha.chaves[i - 1];
            folha.valores[i] = folha.valores[i - 1];
        }
        folha.chaves[posicao] = chave;
        folha.valores[posicao] = valor;
        folha.nChaves++;
        escreverBloco(t.arquivo, folha, posFolha);
        return false;
    }

    // Split da folha
    BPTreeNode direita{};
    direita.folha = 1;
    direita.nChaves = 0;
    direita.proxFolha = folha.proxFolha;

    ptr_t posicaoDireita = alocarNo(t);

    const int ESQUERDA = MAX_KEYS / 2;
    const int DIREITA = MAX_KEYS - ESQUERDA;

    std::vector<key_t> tempChaves;
    std::vector<ptr_t> tempValores;
    tempChaves.reserve(MAX_KEYS + 1);
    tempValores.reserve(MAX_KEYS + 1);

    for (int i = 0; i < folha.nChaves; i++) {
        tempChaves.push_back(folha.chaves[i]);
        tempValores.push_back(folha.valores[i]);
    }

    tempChaves.insert(tempChaves.begin() + posicao, chave);
    tempValores.insert(tempValores.begin() + posicao, valor);

    folha.nChaves = ESQUERDA;
    for (int i = 0; i < ESQUERDA; i++) {
        folha.chaves[i] = tempChaves[i];
        folha.valores[i] = tempValores[i];
    }

    direita.nChaves = DIREITA;
    for (int i = 0; i < DIREITA; i++) {
        direita.chaves[i] = tempChaves[ESQUERDA + i];
        direita.valores[i] = tempValores[ESQUERDA + i];
    }

    direita.proxFolha = folha.proxFolha;

    escreverBloco(t.arquivo, folha, posFolha);
    escreverBloco(t.arquivo, direita, posicaoDireita);

    chaveSeparadora = direita.chaves[0];
    posDireita = posicaoDireita;
    return true;
}

/* Função recursiva que insere uma chave na árvore B+
   Retorna true se houve split, atualizando a chave separadora e a posição do nó direito */
static bool inserirRecursivo(BPTree& t, ptr_t posicaoAtual, key_t chave, ptr_t valor, key_t& chaveSeparadora, ptr_t& posDireita) {
    BPTreeNode no{};
    lerBloco(t.arquivo, no, posicaoAtual);
    if (no.folha)
        return inserirNaFolha(t, posicaoAtual, chave, valor, chaveSeparadora, posDireita);

    int i = limiteSuperior(no.chaves, no.nChaves, chave);
    ptr_t filho = no.filhos[i];

    if (filho < 0)
        return false;

    key_t chaveFilho = 0;
    ptr_t direitaFilho = -1;
    bool houveSplit = inserirRecursivo(t, filho, chave, valor, chaveFilho, direitaFilho);

    if (!houveSplit) return false;

    // Inserção sem necessidade de divisão
    if (no.nChaves < MAX_KEYS) {
        for (int j = no.nChaves; j > i; --j) {
            no.chaves[j] = no.chaves[j - 1];
            no.filhos[j + 1] = no.filhos[j];
        }
        no.chaves[i] = chaveFilho;
        no.filhos[i + 1] = direitaFilho;
        no.nChaves++;
        escreverBloco(t.arquivo, no, posicaoAtual);
        return false;
    }

    // Split de nó interno
    std::vector<key_t> tempChaves;
    std::vector<ptr_t> tempFilhos;

    for (int j = 0; j < no.nChaves; j++) tempChaves.push_back(no.chaves[j]);
    for (int j = 0; j <= no.nChaves; j++) tempFilhos.push_back(no.filhos[j]);

    tempChaves.insert(tempChaves.begin() + i, chaveFilho);
    tempFilhos.insert(tempFilhos.begin() + i + 1, direitaFilho);

    const int esq = MAX_KEYS / 2;
    const int dir = MAX_KEYS - esq;
    key_t chaveSubir = tempChaves[esq];

    no.nChaves = esq;
    for (int j = 0; j < esq; j++) no.chaves[j] = tempChaves[j];
    for (int j = 0; j <= esq; j++) no.filhos[j] = tempFilhos[j];

    BPTreeNode noDireita{};
    noDireita.folha = 0;
    noDireita.nChaves = dir;
    for (int j = 0; j < dir; j++) noDireita.chaves[j] = tempChaves[esq + 1 + j];
    for (int j = 0; j <= dir; j++) noDireita.filhos[j] = tempFilhos[esq + 1 + j];

    ptr_t posicaoNoDireita = alocarNo(t);
    escreverBloco(t.arquivo, no, posicaoAtual);
    escreverBloco(t.arquivo, noDireita, posicaoNoDireita);

    chaveSeparadora = chaveSubir;
    posDireita = posicaoNoDireita;
    return true;
}

/* Função principal de inserção
   Chama a função inserirRecursivo e, se a raiz for dividida, cria uma nova raiz e atualiza o cabeçalho
   Por fim, recalcula o número de blocos no arquivo */
void inserirChave(BPTree& t, key_t chave, ptr_t posDado) {
    key_t chaveSeparadora = 0;
    ptr_t direita = -1;
    bool houveSplit = inserirRecursivo(t, t.rootOffset, chave, posDado, chaveSeparadora, direita);

    if (!houveSplit) {
        t.totalBlocos = divArredondada(tamanhoArquivo(t.arquivo), BLOCK_SIZE);
        return;
    }

    // Se a raiz foi dividida, cria uma nova raiz
    BPTreeNode novaRaiz{};
    novaRaiz.folha = 0;
    novaRaiz.nChaves = 1;
    novaRaiz.chaves[0] = chaveSeparadora;
    novaRaiz.filhos[0] = t.rootOffset;
    novaRaiz.filhos[1] = direita;

    ptr_t posNovaRaiz = alocarNo(t);
    escreverBloco(t.arquivo, novaRaiz, posNovaRaiz);
    t.rootOffset = posNovaRaiz;
    escreverCabecalho(t);

    t.totalBlocos = divArredondada(tamanhoArquivo(t.arquivo), BLOCK_SIZE);
}
