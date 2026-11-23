#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQ_PRODUTOS "produtos.bin"
#define ARQ_VENDAS   "vendas.bin"
#define ARQ_TMP      "temp.bin"
#define MAX_ITENS    10
#define TAM_STR      50

typedef struct {
    int   id;
    char  descricao[TAM_STR];
    float preco;
    int   estoque;
} Produto;

typedef struct {
    int   id;
    char  cliente[TAM_STR];
    int   qtdItens;
    int   idProdutos[MAX_ITENS];   
    int   quantidades[MAX_ITENS];
    float precos[MAX_ITENS];       
    float total;
} Venda;

static void lerLinha(const char *msg, char *dest, int n) {
    printf("%s", msg);
    if (!fgets(dest, n, stdin)) { dest[0] = '\0'; return; }
    dest[strcspn(dest, "\n")] = '\0';
}

static int lerInt(const char *msg) {
    char b[64];
    lerLinha(msg, b, sizeof b);
    return atoi(b);
}

static float lerFloat(const char *msg) {
    char b[64];
    lerLinha(msg, b, sizeof b);
    for (char *p = b; *p; p++) if (*p == ',') *p = '.';
    return (float)atof(b);
}

static void garantirArquivo(const char *nome) {
    FILE *f = fopen(nome, "ab");
    if (f) fclose(f);
}


static long buscarRegistro(const char *arq, size_t tam, int id, void *out) {
    FILE *f = fopen(arq, "rb");
    if (!f) return -1;
    void *buf = malloc(tam);
    long pos = 0, achou = -1;
    while (fread(buf, tam, 1, f) == 1) {
        if (*(int *)buf == id) {
            if (out) memcpy(out, buf, tam);
            achou = pos;
            break;
        }
        pos++;
    }
    free(buf);
    fclose(f);
    return achou;
}


static int anexarRegistro(const char *arq, size_t tam, const void *reg) {
    FILE *f = fopen(arq, "ab");
    if (!f) return 0;
    fwrite(reg, tam, 1, f);
    fclose(f);
    return 1;
}


static int atualizarRegistro(const char *arq, size_t tam, long pos, const void *reg) {
    FILE *f = fopen(arq, "rb+");
    if (!f) return 0;
    fseek(f, pos * (long)tam, SEEK_SET);
    fwrite(reg, tam, 1, f);
    fclose(f);
    return 1;
}

static int removerRegistro(const char *arq, size_t tam, int id) {
    FILE *orig = fopen(arq, "rb");
    FILE *novo = fopen(ARQ_TMP, "wb");
    if (!orig || !novo) {
        if (orig) fclose(orig);
        if (novo) fclose(novo);
        return 0;
    }
    void *buf = malloc(tam);
    int achou = 0;
    while (fread(buf, tam, 1, orig) == 1) {
        if (*(int *)buf == id) achou = 1;
        else fwrite(buf, tam, 1, novo);
    }
    free(buf);
    fclose(orig);
    fclose(novo);
    if (!achou) { remove(ARQ_TMP); return 0; }
    remove(arq);
    rename(ARQ_TMP, arq);
    return 1;
}

static int proximoId(const char *arq, size_t tam) {
    FILE *f = fopen(arq, "rb");
    int maior = 0;
    if (!f) return 1;
    void *buf = malloc(tam);
    while (fread(buf, tam, 1, f) == 1)
        if (*(int *)buf > maior) maior = *(int *)buf;
    free(buf);
    fclose(f);
    return maior + 1;
}

static void imprimirProduto(const Produto *p) {
    printf("ID %-3d | %-30s | R$ %8.2f | estoque: %d\n",
           p->id, p->descricao, p->preco, p->estoque);
}

static void cadastrarProduto(void) {
    Produto p;
    memset(&p, 0, sizeof p);
    p.id = proximoId(ARQ_PRODUTOS, sizeof(Produto));
    lerLinha("Descricao: ", p.descricao, TAM_STR);
    p.preco = lerFloat("Preco: ");
    p.estoque = lerInt("Quantidade em estoque: ");
    if (p.descricao[0] == '\0' || p.preco < 0 || p.estoque < 0) {
        puts("Dados invalidos. Cadastro cancelado.");
        return;
    }
    if (anexarRegistro(ARQ_PRODUTOS, sizeof p, &p))
        printf("Produto cadastrado com ID %d.\n", p.id);
    else
        puts("Erro ao gravar arquivo.");
}

static void consultarProduto(void) {
    Produto p;
    int id = lerInt("ID do produto: ");
    if (buscarRegistro(ARQ_PRODUTOS, sizeof p, id, &p) < 0) puts("Produto nao encontrado.");
    else imprimirProduto(&p);
}

static void listarProdutos(void) {
    FILE *f = fopen(ARQ_PRODUTOS, "rb");
    Produto p;
    int n = 0;
    puts("\n===== ESTOQUE DE PRODUTOS =====");
    if (f) {
        while (fread(&p, sizeof p, 1, f) == 1) { imprimirProduto(&p); n++; }
        fclose(f);
    }
    if (n == 0) puts("(nenhum produto cadastrado)");
}

static void atualizarProduto(void) {
    Produto p;
    char b[64];
    int id = lerInt("ID do produto a atualizar: ");
    long pos = buscarRegistro(ARQ_PRODUTOS, sizeof p, id, &p);
    if (pos < 0) { puts("Produto nao encontrado."); return; }
    imprimirProduto(&p);
    puts("(Pressione ENTER para manter o valor atual)");

    lerLinha("Nova descricao: ", b, sizeof b);
    if (b[0]) { strncpy(p.descricao, b, TAM_STR - 1); p.descricao[TAM_STR - 1] = '\0'; }

    lerLinha("Novo preco: ", b, sizeof b);
    if (b[0]) {
        for (char *c = b; *c; c++) if (*c == ',') *c = '.';
        float v = (float)atof(b);
        if (v >= 0) p.preco = v;
    }

    lerLinha("Novo estoque: ", b, sizeof b);
    if (b[0]) {
        int v = atoi(b);
        if (v >= 0) p.estoque = v;
    }

    if (atualizarRegistro(ARQ_PRODUTOS, sizeof p, pos, &p)) puts("Produto atualizado.");
    else puts("Erro ao atualizar.");
}

static int produtoEmVendas(int idProduto) {
    FILE *f = fopen(ARQ_VENDAS, "rb");
    Venda v;
    int achou = 0;
    if (!f) return 0;
    while (!achou && fread(&v, sizeof v, 1, f) == 1)
        for (int i = 0; i < v.qtdItens; i++)
            if (v.idProdutos[i] == idProduto) { achou = 1; break; }
    fclose(f);
    return achou;
}

static void removerProduto(void) {
    int id = lerInt("ID do produto a remover: ");
    if (buscarRegistro(ARQ_PRODUTOS, sizeof(Produto), id, NULL) < 0) {
        puts("Produto nao encontrado.");
        return;
    }
    if (produtoEmVendas(id)) {
        puts("Nao e possivel remover: o produto consta em vendas registradas.");
        return;
    }
    if (removerRegistro(ARQ_PRODUTOS, sizeof(Produto), id)) puts("Produto removido.");
    else puts("Erro ao remover.");
}




static void ajustarEstoque(const Venda *v, int sinal) {
    for (int i = 0; i < v->qtdItens; i++) {
        Produto p;
        long pos = buscarRegistro(ARQ_PRODUTOS, sizeof p, v->idProdutos[i], &p);
        if (pos < 0) continue;
        p.estoque += sinal * v->quantidades[i];
        atualizarRegistro(ARQ_PRODUTOS, sizeof p, pos, &p);
    }
}

static void montarItens(Venda *v) {
    v->qtdItens = 0;
    v->total = 0;
    while (v->qtdItens < MAX_ITENS) {
        Produto p;
        int id = lerInt("ID do produto (0 para finalizar): ");
        if (id == 0) break;
        if (buscarRegistro(ARQ_PRODUTOS, sizeof p, id, &p) < 0) {
            puts("Produto nao encontrado.");
            continue;
        }
        int repetido = 0;
        for (int i = 0; i < v->qtdItens; i++)
            if (v->idProdutos[i] == id) repetido = 1;
        if (repetido) { puts("Produto ja incluido nesta venda."); continue; }

        imprimirProduto(&p);
        int q = lerInt("Quantidade: ");
        if (q <= 0 || q > p.estoque) {
            puts("Quantidade invalida ou estoque insuficiente.");
            continue;
        }
        int n = v->qtdItens;
        v->idProdutos[n]  = id;
        v->quantidades[n] = q;
        v->precos[n]      = p.preco;
        v->total         += q * p.preco;
        v->qtdItens++;
    }
}

static void cadastrarVenda(void) {
    Venda v;
    memset(&v, 0, sizeof v);
    v.id = proximoId(ARQ_VENDAS, sizeof(Venda));
    lerLinha("Nome do cliente: ", v.cliente, TAM_STR);
    montarItens(&v);
    if (v.qtdItens == 0) { puts("Venda sem itens. Cancelada."); return; }
    ajustarEstoque(&v, -1);
    if (anexarRegistro(ARQ_VENDAS, sizeof v, &v))
        printf("Venda %d registrada. Total: R$ %.2f\n", v.id, v.total);
    else
        puts("Erro ao gravar arquivo.");
}

static void imprimirVenda(const Venda *v) {
    printf("\nVenda #%d | Cliente: %s\n", v->id, v->cliente);
    for (int i = 0; i < v->qtdItens; i++) {
        Produto p;
        const char *nome = "[produto removido]";
        if (buscarRegistro(ARQ_PRODUTOS, sizeof p, v->idProdutos[i], &p) >= 0)
            nome = p.descricao;
        printf("   - %-30s x%-3d R$ %8.2f = R$ %8.2f\n", nome, v->quantidades[i],
               v->precos[i], v->quantidades[i] * v->precos[i]);
    }
    printf("   TOTAL: R$ %.2f\n", v->total);
}

static void consultarVenda(void) {
    Venda v;
    int id = lerInt("ID da venda: ");
    if (buscarRegistro(ARQ_VENDAS, sizeof v, id, &v) < 0) puts("Venda nao encontrada.");
    else imprimirVenda(&v);
}

static void relatorioVendas(void) {
    FILE *f = fopen(ARQ_VENDAS, "rb");
    Venda v;
    int n = 0;
    float geral = 0;
    puts("\n===== RELATORIO DE VENDAS (com nomes dos produtos) =====");
    if (f) {
        while (fread(&v, sizeof v, 1, f) == 1) {
            imprimirVenda(&v);
            geral += v.total;
            n++;
        }
        fclose(f);
    }
    if (n == 0) puts("(nenhuma venda registrada)");
    else printf("\nTotal de vendas: %d | Faturamento geral: R$ %.2f\n", n, geral);
}

static void relatorioProdutosVendas(void) {
    FILE *fp = fopen(ARQ_PRODUTOS, "rb");
    Produto p;
    int n = 0;
    puts("\n===== PRODUTOS E SUAS VENDAS =====");
    if (fp) {
        while (fread(&p, sizeof p, 1, fp) == 1) {
            n++;
            printf("\n[%d] %s (estoque: %d)\n", p.id, p.descricao, p.estoque);
            FILE *fv = fopen(ARQ_VENDAS, "rb");
            Venda v;
            int achou = 0;
            if (fv) {
                while (fread(&v, sizeof v, 1, fv) == 1)
                    for (int i = 0; i < v.qtdItens; i++)
                        if (v.idProdutos[i] == p.id) {
                            printf("   Venda #%d - cliente %s - %d unidade(s)\n",
                                   v.id, v.cliente, v.quantidades[i]);
                            achou = 1;
                        }
                fclose(fv);
            }
            if (!achou) puts("   (nunca vendido)");
        }
        fclose(fp);
    }
    if (n == 0) puts("(nenhum produto cadastrado)");
}

static void atualizarVenda(void) {
    Venda antiga, nova;
    char b[64];
    int id = lerInt("ID da venda a atualizar: ");
    long pos = buscarRegistro(ARQ_VENDAS, sizeof antiga, id, &antiga);
    if (pos < 0) { puts("Venda nao encontrada."); return; }
    imprimirVenda(&antiga);
    nova = antiga;

    lerLinha("Novo nome do cliente (ENTER mantem): ", b, sizeof b);
    if (b[0]) { strncpy(nova.cliente, b, TAM_STR - 1); nova.cliente[TAM_STR - 1] = '\0'; }

    lerLinha("Refazer os itens da venda? (s/n): ", b, sizeof b);
    if (b[0] == 's' || b[0] == 'S') {
        ajustarEstoque(&antiga, +1);          /* devolve itens antigos */
        montarItens(&nova);
        if (nova.qtdItens == 0) {
            puts("Nenhum item informado. Itens originais mantidos.");
            ajustarEstoque(&antiga, -1);
            nova = antiga;
        } else {
            ajustarEstoque(&nova, -1);
        }
    }
    if (atualizarRegistro(ARQ_VENDAS, sizeof nova, pos, &nova)) puts("Venda atualizada.");
    else puts("Erro ao atualizar.");
}

static void removerVenda(void) {
    Venda v;
    int id = lerInt("ID da venda a remover: ");
    if (buscarRegistro(ARQ_VENDAS, sizeof v, id, &v) < 0) {
        puts("Venda nao encontrada.");
        return;
    }
    if (removerRegistro(ARQ_VENDAS, sizeof v, id)) {
        ajustarEstoque(&v, +1);   /* devolve os itens ao estoque */
        puts("Venda removida e estoque restaurado.");
    } else {
        puts("Erro ao remover.");
    }
}

/* ===================== Menu ===================== */

static void menu(void) {
    puts("\n========== VENDAS E ESTOQUE ==========");
    puts(" 1 - Cadastrar produto");
    puts(" 2 - Consultar produto por ID");
    puts(" 3 - Atualizar produto");
    puts(" 4 - Remover produto");
    puts(" 5 - Cadastrar venda");
    puts(" 6 - Consultar venda por ID");
    puts(" 7 - Atualizar venda");
    puts(" 8 - Remover venda");
    puts(" 9 - Relatorio: estoque (todos os produtos)");
    puts("10 - Relatorio: vendas com nomes dos produtos");
    puts("11 - Relatorio: produtos e vendas em que aparecem");
    puts(" 0 - Sair");
}

int main(void) {
    garantirArquivo(ARQ_PRODUTOS);
    garantirArquivo(ARQ_VENDAS);
    int op;
    do {
        menu();
        op = lerInt("Opcao: ");
        switch (op) {
            case 1:  cadastrarProduto();        break;
            case 2:  consultarProduto();        break;
            case 3:  atualizarProduto();        break;
            case 4:  removerProduto();          break;
            case 5:  cadastrarVenda();          break;
            case 6:  consultarVenda();          break;
            case 7:  atualizarVenda();          break;
            case 8:  removerVenda();            break;
            case 9:  listarProdutos();          break;
            case 10: relatorioVendas();         break;
            case 11: relatorioProdutosVendas(); break;
            case 0:  puts("Encerrando...");     break;
            default: puts("Opcao invalida.");
        }
    } while (op != 0);
    return 0;
}