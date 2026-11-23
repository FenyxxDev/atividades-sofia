/*
 * Sistema de Streaming de Videos - manipulacao de arquivos binarios em C
 *
 * Arquivos:
 *   videos.bin   -> registros do tipo Video
 *   usuarios.bin -> registros do tipo Usuario (possui vetor com IDs de videos favoritos)
 *
 * Relacionamento: Usuario.favoritos[] guarda IDs de Video (1 usuario -> N videos).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQ_VIDEOS   "videos.bin"
#define ARQ_USUARIOS "usuarios.bin"
#define ARQ_TEMP     "temp.bin"

#define MAX_FAV 10

typedef struct {
    int  id;
    char titulo[60];
    char genero[30];
    int  duracao;   /* em minutos */
    int  ano;
} Video;

typedef struct {
    int  id;
    char nome[50];
    char email[50];
    int  qtdFav;             /* quantos favoritos estao em uso */
    int  favoritos[MAX_FAV]; /* IDs dos videos favoritados */
} Usuario;

/* ===================== Entrada de dados ===================== */

void lerString(const char *msg, char *dest, int tam) {
    printf("%s", msg);
    if (fgets(dest, tam, stdin) == NULL) {
        dest[0] = '\0';
        return;
    }
    dest[strcspn(dest, "\n")] = '\0';
}

int lerInt(const char *msg) {
    char buf[32];
    int valor;
    while (1) {
        printf("%s", msg);
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            exit(0); /* fim da entrada (EOF): encerra para nao travar */
        if (sscanf(buf, "%d", &valor) == 1)
            return valor;
        printf("Valor invalido. Digite um numero inteiro.\n");
    }
}

/* ===================== Busca (retorna posicao ou -1) ===================== */

int buscarVideo(int id, Video *saida) {
    FILE *f = fopen(ARQ_VIDEOS, "rb");
    Video v;
    int pos = 0;
    if (f == NULL) return -1;
    while (fread(&v, sizeof(Video), 1, f) == 1) {
        if (v.id == id) {
            if (saida != NULL) *saida = v;
            fclose(f);
            return pos;
        }
        pos++;
    }
    fclose(f);
    return -1;
}

int buscarUsuario(int id, Usuario *saida) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    Usuario u;
    int pos = 0;
    if (f == NULL) return -1;
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        if (u.id == id) {
            if (saida != NULL) *saida = u;
            fclose(f);
            return pos;
        }
        pos++;
    }
    fclose(f);
    return -1;
}

/* ===================== Impressao ===================== */

void imprimirVideo(const Video *v) {
    printf("[ID %d] %s | Genero: %s | %d min | Ano: %d\n",
           v->id, v->titulo, v->genero, v->duracao, v->ano);
}

void imprimirUsuario(const Usuario *u) {
    printf("[ID %d] %s | %s | Favoritos: %d\n", u->id, u->nome, u->email, u->qtdFav);
}

/* ===================== Remocao generica =====================
 * Cria um arquivo novo com todos os registros, menos o que sera excluido.
 * Depois fecha os dois, apaga o antigo e renomeia o novo.
 * Funciona para qualquer registro cujo PRIMEIRO campo seja o int id.
 */
int removerRegistro(const char *nomeArq, size_t tamReg, int id) {
    FILE *orig = fopen(nomeArq, "rb");
    FILE *novo;
    void *buf;
    int idReg, achou = 0;

    if (orig == NULL) return 0;
    novo = fopen(ARQ_TEMP, "wb");
    if (novo == NULL) {
        fclose(orig);
        return 0;
    }
    buf = malloc(tamReg);
    if (buf == NULL) {
        fclose(orig);
        fclose(novo);
        return 0;
    }

    while (fread(buf, tamReg, 1, orig) == 1) {
        memcpy(&idReg, buf, sizeof(int));
        if (idReg == id)
            achou = 1;                  /* pula este registro */
        else
            fwrite(buf, tamReg, 1, novo);
    }

    free(buf);
    fclose(orig);
    fclose(novo);

    if (achou) {
        remove(nomeArq);
        rename(ARQ_TEMP, nomeArq);
    } else {
        remove(ARQ_TEMP);
    }
    return achou;
}

/* ===================== VIDEOS ===================== */

void cadastrarVideo(void) {
    Video v;
    FILE *f;

    v.id = lerInt("ID do video: ");
    if (buscarVideo(v.id, NULL) != -1) {
        printf("Ja existe um video com esse ID.\n");
        return;
    }
    lerString("Titulo: ", v.titulo, sizeof(v.titulo));
    lerString("Genero: ", v.genero, sizeof(v.genero));
    v.duracao = lerInt("Duracao (min): ");
    v.ano     = lerInt("Ano de lancamento: ");

    f = fopen(ARQ_VIDEOS, "ab");
    if (f == NULL) {
        printf("Erro ao abrir %s\n", ARQ_VIDEOS);
        return;
    }
    fwrite(&v, sizeof(Video), 1, f);
    fclose(f);
    printf("Video cadastrado com sucesso!\n");
}

void consultarVideo(void) {
    Video v;
    int id = lerInt("ID do video: ");
    if (buscarVideo(id, &v) == -1)
        printf("Video nao encontrado.\n");
    else
        imprimirVideo(&v);
}

void listarVideos(void) {
    FILE *f = fopen(ARQ_VIDEOS, "rb");
    Video v;
    int cont = 0;
    if (f == NULL) {
        printf("Nenhum video cadastrado.\n");
        return;
    }
    printf("\n--- VIDEOS ---\n");
    while (fread(&v, sizeof(Video), 1, f) == 1) {
        imprimirVideo(&v);
        cont++;
    }
    fclose(f);
    if (cont == 0) printf("Nenhum video cadastrado.\n");
}

void atualizarVideo(void) {
    Video v;
    FILE *f;
    int pos, id = lerInt("ID do video a atualizar: ");

    pos = buscarVideo(id, &v);
    if (pos == -1) {
        printf("Video nao encontrado.\n");
        return;
    }
    printf("Dados atuais: ");
    imprimirVideo(&v);

    lerString("Novo titulo: ", v.titulo, sizeof(v.titulo));
    lerString("Novo genero: ", v.genero, sizeof(v.genero));
    v.duracao = lerInt("Nova duracao (min): ");
    v.ano     = lerInt("Novo ano: ");

    f = fopen(ARQ_VIDEOS, "rb+");
    if (f == NULL) {
        printf("Erro ao abrir %s\n", ARQ_VIDEOS);
        return;
    }
    fseek(f, (long)pos * sizeof(Video), SEEK_SET); /* vai ate o registro */
    fwrite(&v, sizeof(Video), 1, f);               /* sobrescreve */
    fclose(f);
    printf("Video atualizado!\n");
}

/* Ao excluir um video, ele tambem sai dos favoritos de todos os usuarios,
 * para nao deixar IDs "soltos" (integridade do relacionamento). */
void limparVideoDosFavoritos(int idVideo) {
    FILE *f = fopen(ARQ_USUARIOS, "rb+");
    Usuario u;
    int i, j, mudou;
    if (f == NULL) return;

    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        mudou = 0;
        for (i = 0; i < u.qtdFav; i++) {
            if (u.favoritos[i] == idVideo) {
                for (j = i; j < u.qtdFav - 1; j++)
                    u.favoritos[j] = u.favoritos[j + 1];
                u.qtdFav--;
                mudou = 1;
                break;
            }
        }
        if (mudou) {
            fseek(f, -(long)sizeof(Usuario), SEEK_CUR); /* volta 1 registro */
            fwrite(&u, sizeof(Usuario), 1, f);
            fseek(f, 0, SEEK_CUR); /* necessario ao alternar escrita -> leitura */
        }
    }
    fclose(f);
}

void removerVideo(void) {
    int id = lerInt("ID do video a remover: ");
    if (removerRegistro(ARQ_VIDEOS, sizeof(Video), id)) {
        limparVideoDosFavoritos(id);
        printf("Video removido (e retirado dos favoritos dos usuarios).\n");
    } else {
        printf("Video nao encontrado.\n");
    }
}

/* ===================== USUARIOS ===================== */

void cadastrarUsuario(void) {
    Usuario u;
    FILE *f;

    memset(&u, 0, sizeof(Usuario));
    u.id = lerInt("ID do usuario: ");
    if (buscarUsuario(u.id, NULL) != -1) {
        printf("Ja existe um usuario com esse ID.\n");
        return;
    }
    lerString("Nome: ", u.nome, sizeof(u.nome));
    lerString("Email: ", u.email, sizeof(u.email));
    u.qtdFav = 0;

    f = fopen(ARQ_USUARIOS, "ab");
    if (f == NULL) {
        printf("Erro ao abrir %s\n", ARQ_USUARIOS);
        return;
    }
    fwrite(&u, sizeof(Usuario), 1, f);
    fclose(f);
    printf("Usuario cadastrado com sucesso!\n");
}

void consultarUsuario(void) {
    Usuario u;
    int id = lerInt("ID do usuario: ");
    if (buscarUsuario(id, &u) == -1)
        printf("Usuario nao encontrado.\n");
    else
        imprimirUsuario(&u);
}

void listarUsuarios(void) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    Usuario u;
    int cont = 0;
    if (f == NULL) {
        printf("Nenhum usuario cadastrado.\n");
        return;
    }
    printf("\n--- USUARIOS ---\n");
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        imprimirUsuario(&u);
        cont++;
    }
    fclose(f);
    if (cont == 0) printf("Nenhum usuario cadastrado.\n");
}

void atualizarUsuario(void) {
    Usuario u;
    FILE *f;
    int pos, id = lerInt("ID do usuario a atualizar: ");

    pos = buscarUsuario(id, &u);
    if (pos == -1) {
        printf("Usuario nao encontrado.\n");
        return;
    }
    printf("Dados atuais: ");
    imprimirUsuario(&u);

    lerString("Novo nome: ", u.nome, sizeof(u.nome));
    lerString("Novo email: ", u.email, sizeof(u.email));

    f = fopen(ARQ_USUARIOS, "rb+");
    if (f == NULL) {
        printf("Erro ao abrir %s\n", ARQ_USUARIOS);
        return;
    }
    fseek(f, (long)pos * sizeof(Usuario), SEEK_SET);
    fwrite(&u, sizeof(Usuario), 1, f);
    fclose(f);
    printf("Usuario atualizado!\n");
}

void removerUsuario(void) {
    int id = lerInt("ID do usuario a remover: ");
    if (removerRegistro(ARQ_USUARIOS, sizeof(Usuario), id))
        printf("Usuario removido.\n");
    else
        printf("Usuario nao encontrado.\n");
}

/* ===================== FAVORITOS (relacionamento) ===================== */

void gravarUsuarioNaPosicao(const Usuario *u, int pos) {
    FILE *f = fopen(ARQ_USUARIOS, "rb+");
    if (f == NULL) return;
    fseek(f, (long)pos * sizeof(Usuario), SEEK_SET);
    fwrite(u, sizeof(Usuario), 1, f);
    fclose(f);
}

void favoritarVideo(void) {
    Usuario u;
    Video v;
    int i, posU, idVideo;

    posU = buscarUsuario(lerInt("ID do usuario: "), &u);
    if (posU == -1) {
        printf("Usuario nao encontrado.\n");
        return;
    }
    if (u.qtdFav >= MAX_FAV) {
        printf("Limite de %d favoritos atingido.\n", MAX_FAV);
        return;
    }
    idVideo = lerInt("ID do video a favoritar: ");
    if (buscarVideo(idVideo, &v) == -1) {
        printf("Video nao encontrado.\n");
        return;
    }
    for (i = 0; i < u.qtdFav; i++) {
        if (u.favoritos[i] == idVideo) {
            printf("Este video ja esta nos favoritos.\n");
            return;
        }
    }
    u.favoritos[u.qtdFav++] = idVideo;
    gravarUsuarioNaPosicao(&u, posU);
    printf("\"%s\" adicionado aos favoritos de %s!\n", v.titulo, u.nome);
}

void desfavoritarVideo(void) {
    Usuario u;
    int i, j, posU, idVideo;

    posU = buscarUsuario(lerInt("ID do usuario: "), &u);
    if (posU == -1) {
        printf("Usuario nao encontrado.\n");
        return;
    }
    idVideo = lerInt("ID do video a desfavoritar: ");
    for (i = 0; i < u.qtdFav; i++) {
        if (u.favoritos[i] == idVideo) {
            for (j = i; j < u.qtdFav - 1; j++)
                u.favoritos[j] = u.favoritos[j + 1];
            u.qtdFav--;
            gravarUsuarioNaPosicao(&u, posU);
            printf("Favorito removido.\n");
            return;
        }
    }
    printf("Este video nao esta nos favoritos do usuario.\n");
}

/* ===================== RELATORIOS ===================== */

/* Usuarios com os TITULOS dos videos favoritados (dado vem do outro arquivo) */
void relatorioUsuariosFavoritos(void) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    Usuario u;
    Video v;
    int i, cont = 0;

    if (f == NULL) {
        printf("Nenhum usuario cadastrado.\n");
        return;
    }
    printf("\n===== RELATORIO: USUARIOS E SEUS VIDEOS FAVORITOS =====\n");
    while (fread(&u, sizeof(Usuario), 1, f) == 1) {
        printf("\nUsuario: %s (ID %d) - %s\n", u.nome, u.id, u.email);
        if (u.qtdFav == 0) {
            printf("   (nenhum favorito)\n");
        } else {
            for (i = 0; i < u.qtdFav; i++) {
                if (buscarVideo(u.favoritos[i], &v) != -1)
                    printf("   %d. %s (%s, %d min, %d)\n",
                           i + 1, v.titulo, v.genero, v.duracao, v.ano);
                else
                    printf("   %d. [video ID %d nao encontrado]\n", i + 1, u.favoritos[i]);
            }
        }
        cont++;
    }
    fclose(f);
    if (cont == 0) printf("Nenhum usuario cadastrado.\n");
}

/* Videos com os NOMES dos usuarios que os favoritaram */
void relatorioVideosFavoritadosPor(void) {
    FILE *fv = fopen(ARQ_VIDEOS, "rb");
    FILE *fu;
    Video v;
    Usuario u;
    int i, total, cont = 0;

    if (fv == NULL) {
        printf("Nenhum video cadastrado.\n");
        return;
    }
    printf("\n===== RELATORIO: VIDEOS E QUEM FAVORITOU =====\n");
    while (fread(&v, sizeof(Video), 1, fv) == 1) {
        printf("\nVideo: %s (ID %d) - %s\n", v.titulo, v.id, v.genero);
        total = 0;
        fu = fopen(ARQ_USUARIOS, "rb");
        if (fu != NULL) {
            while (fread(&u, sizeof(Usuario), 1, fu) == 1) {
                for (i = 0; i < u.qtdFav; i++) {
                    if (u.favoritos[i] == v.id) {
                        printf("   - %s\n", u.nome);
                        total++;
                        break;
                    }
                }
            }
            fclose(fu);
        }
        if (total == 0) printf("   (ninguem favoritou)\n");
        else printf("   Total: %d usuario(s)\n", total);
        cont++;
    }
    fclose(fv);
    if (cont == 0) printf("Nenhum video cadastrado.\n");
}

/* ===================== MENUS ===================== */

void menuVideos(void) {
    int op;
    do {
        printf("\n--- VIDEOS ---\n");
        printf("1. Cadastrar\n2. Consultar por ID\n3. Listar todos\n");
        printf("4. Atualizar\n5. Remover\n0. Voltar\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: cadastrarVideo();  break;
            case 2: consultarVideo();  break;
            case 3: listarVideos();    break;
            case 4: atualizarVideo();  break;
            case 5: removerVideo();    break;
            case 0: break;
            default: printf("Opcao invalida.\n");
        }
    } while (op != 0);
}

void menuUsuarios(void) {
    int op;
    do {
        printf("\n--- USUARIOS ---\n");
        printf("1. Cadastrar\n2. Consultar por ID\n3. Listar todos\n");
        printf("4. Atualizar\n5. Remover\n0. Voltar\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: cadastrarUsuario();  break;
            case 2: consultarUsuario();  break;
            case 3: listarUsuarios();    break;
            case 4: atualizarUsuario();  break;
            case 5: removerUsuario();    break;
            case 0: break;
            default: printf("Opcao invalida.\n");
        }
    } while (op != 0);
}

int main(void) {
    int op;
    do {
        printf("\n========== STREAMING ==========\n");
        printf("1. Videos\n");
        printf("2. Usuarios\n");
        printf("3. Favoritar video\n");
        printf("4. Desfavoritar video\n");
        printf("5. Relatorio: usuarios e seus favoritos\n");
        printf("6. Relatorio: videos e quem favoritou\n");
        printf("0. Sair\n");
        op = lerInt("Opcao: ");
        switch (op) {
            case 1: menuVideos();                 break;
            case 2: menuUsuarios();               break;
            case 3: favoritarVideo();             break;
            case 4: desfavoritarVideo();          break;
            case 5: relatorioUsuariosFavoritos(); break;
            case 6: relatorioVideosFavoritadosPor(); break;
            case 0: printf("Encerrando...\n");    break;
            default: printf("Opcao invalida.\n");
        }
    } while (op != 0);
    return 0;
}