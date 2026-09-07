/*
 * ============================================================
 * SISTEMA DE GERENCIAMENTO DE BIBLIOTECA UNIVERSITÁRIA
 * ============================================================
 * Disciplina: Programação em C
 * Atividade: Sistema de Gestão de Biblioteca
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * ETAPA 1 - ESTRUTURAS DE DADOS (STRUCTS E ARRAYS)
 * ============================================================ */

#define MAX_LIVROS    100   /* Capacidade máxima do acervo       */
#define MAX_USUARIOS   50   /* Limite de usuários cadastrados     */
#define MAX_EMPRESTIMOS 5   /* Máximo de livros por usuário       */

/*
 * Struct Livro
 * Armazena os dados de cada exemplar do acervo.
 * status: 0 = disponível | 1 = emprestado
 */
typedef struct {
    char titulo[100];
    char autor[100];
    char isbn[20];
    int  status;
} Livro;

/*
 * Struct Usuario
 * Armazena os dados de cada usuário cadastrado.
 * livrosEmprestados: vetor com os índices dos livros atualmente
 *                    em posse do usuário.
 */
typedef struct {
    char nome[100];
    int  id;
    int  livrosEmprestados[MAX_EMPRESTIMOS];
    int  qtdEmprestimos;
} Usuario;

/* Arrays globais que representam o banco de dados em memória */
Livro   acervo[MAX_LIVROS];
Usuario usuarios[MAX_USUARIOS];
int     totalLivros   = 0;
int     totalUsuarios = 0;

/* ============================================================
 * ETAPA 3 - FILA DE ESPERA PARA LIVROS POPULARES
 * Implementada com lista encadeada + malloc/free
 * ============================================================ */

/* Nó da fila: guarda o ID do usuário e o índice do livro desejado */
typedef struct NoFila {
    int           usuarioId;
    int           livroIdx;
    struct NoFila *proximo;
} NoFila;

/* Struct da fila: ponteiros para frente (dequeue) e fundo (enqueue) */
typedef struct {
    NoFila *frente;
    NoFila *fundo;
    int     tamanho;
} Fila;

/* ============================================================
 * ETAPA 4 - PILHA PARA OPERAÇÕES DE DESFAZER (UNDO)
 * Implementada com lista encadeada + malloc/free
 * ============================================================ */

/* Nó da pilha: guarda uma descrição textual da ação e dados
 * suficientes para revertê-la. */
typedef struct NoPilha {
    char            descricao[200];
    int             tipo;       /* 1=empréstimo 2=devolução 3=cad.livro 4=cad.usuário */
    int             livroIdx;
    int             usuarioIdx;
    struct NoPilha *proximo;
} NoPilha;

/* Struct da pilha: ponteiro para o topo */
typedef struct {
    NoPilha *topo;
    int      tamanho;
} Pilha;

/* Instâncias globais da fila e da pilha */
Fila  filaEspera;
Pilha pilhaUndo;

/* ============================================================
 * OPERAÇÕES DE FILA (Etapa 3)
 * ============================================================ */

/* Inicializa a fila como vazia */
void inicializarFila(Fila *f) {
    f->frente  = NULL;
    f->fundo   = NULL;
    f->tamanho = 0;
}

/*
 * enqueue – insere um novo pedido no fundo da fila.
 * Usa malloc para alocar dinamicamente cada nó.
 */
void enqueue(Fila *f, int usuarioId, int livroIdx) {
    NoFila *novo = (NoFila *)malloc(sizeof(NoFila));
    if (!novo) {
        printf("Erro: memoria insuficiente para a fila.\n");
        return;
    }
    novo->usuarioId = usuarioId;
    novo->livroIdx  = livroIdx;
    novo->proximo   = NULL;

    if (f->fundo == NULL) {
        /* Fila estava vazia: frente e fundo apontam para o único nó */
        f->frente = novo;
        f->fundo  = novo;
    } else {
        f->fundo->proximo = novo;
        f->fundo          = novo;
    }
    f->tamanho++;
    printf("  >> Usuario ID %d adicionado a fila de espera para '%s'.\n",
           usuarioId, acervo[livroIdx].titulo);
}

/*
 * dequeue – remove e retorna o nó da frente da fila.
 * Responsabilidade de liberar (free) é de quem chama.
 */
NoFila *dequeue(Fila *f) {
    if (f->frente == NULL) return NULL;
    NoFila *removido = f->frente;
    f->frente = f->frente->proximo;
    if (f->frente == NULL) f->fundo = NULL;
    f->tamanho--;
    return removido;
}

/* ============================================================
 * OPERAÇÕES DE PILHA (Etapa 4)
 * ============================================================ */

/* Inicializa a pilha como vazia */
void inicializarPilha(Pilha *p) {
    p->topo    = NULL;
    p->tamanho = 0;
}

/*
 * push – empilha uma nova ação no topo.
 * Usa malloc para alocar dinamicamente cada nó.
 */
void push(Pilha *p, const char *descricao, int tipo, int livroIdx, int usuarioIdx) {
    NoPilha *novo = (NoPilha *)malloc(sizeof(NoPilha));
    if (!novo) {
        printf("Erro: memoria insuficiente para a pilha.\n");
        return;
    }
    strcpy(novo->descricao, descricao);   /* Etapa 6: uso de strcpy */
    novo->tipo       = tipo;
    novo->livroIdx   = livroIdx;
    novo->usuarioIdx = usuarioIdx;
    novo->proximo    = p->topo;           /* Novo nó aponta para o antigo topo */
    p->topo          = novo;
    p->tamanho++;
}

/*
 * pop – desempilha e retorna o nó do topo.
 * Responsabilidade de liberar (free) é de quem chama.
 */
NoPilha *pop(Pilha *p) {
    if (p->topo == NULL) return NULL;
    NoPilha *removido = p->topo;
    p->topo = p->topo->proximo;
    p->tamanho--;
    return removido;
}

/* ============================================================
 * ETAPAS 5 e 6 – FUNÇÕES AUXILIARES DE STRINGS
 * Uso de <string.h>: strcpy, strcat, strcmp, strstr
 * ============================================================ */

/*
 * buscarLivroPorTitulo – percorre o acervo e retorna o índice
 * do primeiro livro cujo título contém 'termo' (busca parcial).
 * Usa strstr() conforme exigido na Etapa 6.
 * Retorna -1 se não encontrar.
 */
int buscarLivroPorTitulo(const char *termo) {
    int i;
    for (i = 0; i < totalLivros; i++) {
        if (strstr(acervo[i].titulo, termo) != NULL) {
            return i;
        }
    }
    return -1;
}

/*
 * buscarUsuarioPorId – percorre o vetor de usuários e retorna o
 * índice do usuário com o ID informado.
 * Retorna -1 se não encontrar.
 */
int buscarUsuarioPorId(int id) {
    int i;
    for (i = 0; i < totalUsuarios; i++) {
        if (usuarios[i].id == id) return i;
    }
    return -1;
}

/* ============================================================
 * ETAPA 1 – CADASTRAR LIVROS E USUÁRIOS
 * Usa strcpy (Etapa 6) para copiar título, autor e isbn.
 * Registra a ação na pilha de undo.
 * ============================================================ */

void cadastrarLivro(const char *titulo, const char *autor, const char *isbn) {
    char desc[200];

    if (totalLivros >= MAX_LIVROS) {
        printf("Acervo cheio! Nao e possivel cadastrar mais livros.\n");
        return;
    }

    /* Etapa 6: uso de strcpy para copiar dados para a struct */
    strcpy(acervo[totalLivros].titulo, titulo);
    strcpy(acervo[totalLivros].autor,  autor);
    strcpy(acervo[totalLivros].isbn,   isbn);
    acervo[totalLivros].status = 0; /* começa disponível */

    /* Montar descrição para o undo usando strcpy + strcat */
    strcpy(desc, "Cadastro do livro: ");
    strcat(desc, titulo);
    push(&pilhaUndo, desc, 3, totalLivros, -1);

    printf("  >> Livro '%s' cadastrado com sucesso. Indice: [%d]\n",
           titulo, totalLivros);
    totalLivros++;
}

void cadastrarUsuario(const char *nome, int id) {
    char desc[200];

    if (totalUsuarios >= MAX_USUARIOS) {
        printf("Limite de usuarios atingido!\n");
        return;
    }
    /* Verificar ID duplicado usando strcmp (Etapa 6) */
    if (buscarUsuarioPorId(id) >= 0) {
        printf("Erro: ja existe um usuario com ID %d.\n", id);
        return;
    }

    strcpy(usuarios[totalUsuarios].nome, nome);
    usuarios[totalUsuarios].id             = id;
    usuarios[totalUsuarios].qtdEmprestimos = 0;

    strcpy(desc, "Cadastro do usuario: ");
    strcat(desc, nome);
    push(&pilhaUndo, desc, 4, -1, totalUsuarios);

    printf("  >> Usuario '%s' (ID: %d) cadastrado com sucesso.\n", nome, id);
    totalUsuarios++;
}

/* ============================================================
 * ETAPA 2 – EMPRESTAR E DEVOLVER LIVROS
 * Usa operadores aritméticos para atualizar status.
 * ============================================================ */

/*
 * emprestarLivro – verifica disponibilidade e realiza o empréstimo.
 * Se o livro estiver indisponível, oferece entrada na fila de espera.
 * Registra a ação na pilha de undo.
 */
void emprestarLivro(int usuarioIdx, int livroIdx) {
    char desc[200];
    int  opcao;

    /* Verificar disponibilidade */
    if (acervo[livroIdx].status == 1) {
        printf("  Livro '%s' esta emprestado no momento.\n",
               acervo[livroIdx].titulo);
        printf("  Deseja entrar na fila de espera? (1=Sim / 0=Nao): ");
        scanf("%d", &opcao);
        getchar();
        if (opcao == 1) {
            enqueue(&filaEspera, usuarios[usuarioIdx].id, livroIdx);
        }
        return;
    }

    /* Verificar limite de empréstimos por usuário */
    if (usuarios[usuarioIdx].qtdEmprestimos >= MAX_EMPRESTIMOS) {
        printf("  Usuario '%s' ja atingiu o limite de %d emprestimos.\n",
               usuarios[usuarioIdx].nome, MAX_EMPRESTIMOS);
        return;
    }

    /* Atualizar status usando operador aritmético: 0 -> 1 */
    acervo[livroIdx].status = acervo[livroIdx].status + 1;

    /* Adicionar livro na lista do usuário */
    usuarios[usuarioIdx].livrosEmprestados[usuarios[usuarioIdx].qtdEmprestimos] = livroIdx;
    usuarios[usuarioIdx].qtdEmprestimos++;   /* operador aritmético */

    /* Registrar ação na pilha */
    strcpy(desc, "Emprestimo: ");
    strcat(desc, usuarios[usuarioIdx].nome);
    strcat(desc, " -> ");
    strcat(desc, acervo[livroIdx].titulo);
    push(&pilhaUndo, desc, 1, livroIdx, usuarioIdx);

    printf("  >> Livro '%s' emprestado para '%s' com sucesso.\n",
           acervo[livroIdx].titulo, usuarios[usuarioIdx].nome);
}

/*
 * devolverLivro – processa a devolução de um livro.
 * Após a devolução, verifica se há alguém na fila de espera
 * e notifica o próximo usuário.
 */
void devolverLivro(int usuarioIdx, int livroIdx) {
    char    desc[200];
    int     i, j, encontrou = 0;
    NoFila *atual, *anterior;

    if (acervo[livroIdx].status == 0) {
        printf("  O livro '%s' ja esta disponivel — nao foi emprestado.\n",
               acervo[livroIdx].titulo);
        return;
    }

    /* Atualizar status: 1 -> 0 (operador aritmético) */
    acervo[livroIdx].status = acervo[livroIdx].status - 1;

    /* Remover livro da lista de empréstimos do usuário */
    for (i = 0; i < usuarios[usuarioIdx].qtdEmprestimos; i++) {
        if (usuarios[usuarioIdx].livrosEmprestados[i] == livroIdx) {
            /* Deslocar os elementos seguintes para a esquerda */
            for (j = i; j < usuarios[usuarioIdx].qtdEmprestimos - 1; j++) {
                usuarios[usuarioIdx].livrosEmprestados[j] =
                    usuarios[usuarioIdx].livrosEmprestados[j + 1];
            }
            usuarios[usuarioIdx].qtdEmprestimos--;
            encontrou = 1;
            break;
        }
    }

    if (!encontrou) {
        printf("  Aviso: livro nao estava registrado nos emprestimos do usuario.\n");
    }

    /* Registrar devolução na pilha */
    strcpy(desc, "Devolucao: ");
    strcat(desc, acervo[livroIdx].titulo);
    strcat(desc, " por ");
    strcat(desc, usuarios[usuarioIdx].nome);
    push(&pilhaUndo, desc, 2, livroIdx, usuarioIdx);

    printf("  >> Livro '%s' devolvido por '%s' com sucesso.\n",
           acervo[livroIdx].titulo, usuarios[usuarioIdx].nome);

    /* Verificar fila de espera para este livro */
    atual    = filaEspera.frente;
    anterior = NULL;
    while (atual != NULL) {
        if (atual->livroIdx == livroIdx) {
            int nextIdx = buscarUsuarioPorId(atual->usuarioId);
            printf("  >> Notificacao: o usuario '%s' (ID: %d) e o proximo da fila"
                   " para '%s'.\n",
                   nextIdx >= 0 ? usuarios[nextIdx].nome : "desconhecido",
                   atual->usuarioId,
                   acervo[livroIdx].titulo);

            /* Remover da fila */
            if (anterior == NULL) {
                filaEspera.frente = atual->proximo;
            } else {
                anterior->proximo = atual->proximo;
            }
            if (filaEspera.fundo == atual) {
                filaEspera.fundo = anterior;
            }
            filaEspera.tamanho--;
            free(atual);   /* liberar memória alocada por malloc */
            break;
        }
        anterior = atual;
        atual    = atual->proximo;
    }
}

/* ============================================================
 * ETAPA 4 – DESFAZER ÚLTIMA AÇÃO (UNDO)
 * ============================================================ */

void desfazerAcao() {
    NoPilha *acao = pop(&pilhaUndo);
    int      k, m;

    if (acao == NULL) {
        printf("  Nenhuma acao para desfazer.\n");
        return;
    }

    printf("  Desfazendo: %s\n", acao->descricao);

    switch (acao->tipo) {
        case 1: /* Reverter empréstimo → livro volta a disponível */
            if (acao->livroIdx >= 0)
                acervo[acao->livroIdx].status = 0;
            if (acao->usuarioIdx >= 0 && usuarios[acao->usuarioIdx].qtdEmprestimos > 0)
                usuarios[acao->usuarioIdx].qtdEmprestimos--;
            break;

        case 2: /* Reverter devolução → livro volta a emprestado */
            if (acao->livroIdx >= 0)
                acervo[acao->livroIdx].status = 1;
            if (acao->usuarioIdx >= 0 &&
                usuarios[acao->usuarioIdx].qtdEmprestimos < MAX_EMPRESTIMOS) {
                k = usuarios[acao->usuarioIdx].qtdEmprestimos;
                usuarios[acao->usuarioIdx].livrosEmprestados[k] = acao->livroIdx;
                usuarios[acao->usuarioIdx].qtdEmprestimos++;
            }
            break;

        case 3: /* Reverter cadastro de livro */
            if (totalLivros > 0) totalLivros--;
            break;

        case 4: /* Reverter cadastro de usuário */
            if (totalUsuarios > 0) totalUsuarios--;
            break;

        default:
            printf("  Tipo de acao desconhecido — nao foi possivel desfazer.\n");
    }

    free(acao);   /* liberar memória do nó desempilhado */
    printf("  >> Acao desfeita com sucesso.\n");
}

/* ============================================================
 * FUNÇÕES DE EXIBIÇÃO (LISTAGENS)
 * ============================================================ */

void listarLivros() {
    int i;
    printf("\n--- ACERVO DA BIBLIOTECA (%d livro(s)) ---\n", totalLivros);
    if (totalLivros == 0) {
        printf("  Nenhum livro cadastrado.\n");
        return;
    }
    for (i = 0; i < totalLivros; i++) {
        printf("  [%d] %-40s | Autor: %-25s | ISBN: %-15s | %s\n",
               i,
               acervo[i].titulo,
               acervo[i].autor,
               acervo[i].isbn,
               acervo[i].status == 0 ? "Disponivel" : "Emprestado");
    }
}

void listarUsuarios() {
    int i, j;
    printf("\n--- USUARIOS CADASTRADOS (%d usuario(s)) ---\n", totalUsuarios);
    if (totalUsuarios == 0) {
        printf("  Nenhum usuario cadastrado.\n");
        return;
    }
    for (i = 0; i < totalUsuarios; i++) {
        printf("  [ID: %d] %-30s | Emprestimos ativos: %d",
               usuarios[i].id, usuarios[i].nome, usuarios[i].qtdEmprestimos);
        if (usuarios[i].qtdEmprestimos > 0) {
            printf(" (");
            for (j = 0; j < usuarios[i].qtdEmprestimos; j++) {
                int idx = usuarios[i].livrosEmprestados[j];
                printf("%s%s", acervo[idx].titulo,
                       j < usuarios[i].qtdEmprestimos - 1 ? ", " : "");
            }
            printf(")");
        }
        printf("\n");
    }
}

void exibirFilaEspera() {
    NoFila *atual;
    int     pos = 1;
    printf("\n--- FILA DE ESPERA (%d pedido(s)) ---\n", filaEspera.tamanho);
    if (filaEspera.tamanho == 0) {
        printf("  Fila vazia.\n");
        return;
    }
    atual = filaEspera.frente;
    while (atual != NULL) {
        int uIdx = buscarUsuarioPorId(atual->usuarioId);
        printf("  %d. Usuario: %-25s | Livro: %s\n",
               pos++,
               uIdx >= 0 ? usuarios[uIdx].nome : "(desconhecido)",
               acervo[atual->livroIdx].titulo);
        atual = atual->proximo;
    }
}

void exibirHistoricoAcoes() {
    NoPilha *atual;
    int      i = 1;
    printf("\n--- HISTORICO DE ACOES / PILHA UNDO (%d acao(oes)) ---\n",
           pilhaUndo.tamanho);
    if (pilhaUndo.tamanho == 0) {
        printf("  Nenhuma acao registrada.\n");
        return;
    }
    atual = pilhaUndo.topo;
    while (atual != NULL) {
        printf("  %d. %s\n", i++, atual->descricao);
        atual = atual->proximo;
    }
}

/* ============================================================
 * MENU PRINCIPAL
 * ============================================================ */

void exibirMenu() {
    printf("\n========================================\n");
    printf("   SISTEMA DE GESTAO DA BIBLIOTECA     \n");
    printf("========================================\n");
    printf("  1.  Cadastrar livro\n");
    printf("  2.  Cadastrar usuario\n");
    printf("  3.  Emprestar livro\n");
    printf("  4.  Devolver livro\n");
    printf("  5.  Buscar livro por titulo\n");
    printf("  6.  Listar todos os livros\n");
    printf("  7.  Listar todos os usuarios\n");
    printf("  8.  Exibir fila de espera\n");
    printf("  9.  Exibir historico de acoes\n");
    printf("  10. Desfazer ultima acao (Undo)\n");
    printf("  0.  Sair\n");
    printf("----------------------------------------\n");
    printf("  Opcao: ");
}

/* ============================================================
 * FUNÇÃO PRINCIPAL
 * ============================================================ */

int main() {
    int  opcao;
    char titulo[100], autor[100], isbn[20], nome[100], termo[100];
    int  id, livroIdx, usuarioIdx, usuarioId;

    /* Inicializar estruturas dinâmicas */
    inicializarFila(&filaEspera);
    inicializarPilha(&pilhaUndo);

    do {
        exibirMenu();
        scanf("%d", &opcao);
        getchar(); /* limpar o '\n' restante no buffer */

        switch (opcao) {

            /* --- Cadastrar livro --- */
            case 1:
                printf("  Titulo : "); fgets(titulo, sizeof(titulo), stdin);
                titulo[strcspn(titulo, "\n")] = '\0';
                printf("  Autor  : "); fgets(autor, sizeof(autor), stdin);
                autor[strcspn(autor,  "\n")] = '\0';
                printf("  ISBN   : "); fgets(isbn, sizeof(isbn), stdin);
                isbn[strcspn(isbn,   "\n")] = '\0';
                cadastrarLivro(titulo, autor, isbn);
                break;

            /* --- Cadastrar usuário --- */
            case 2:
                printf("  Nome : "); fgets(nome, sizeof(nome), stdin);
                nome[strcspn(nome, "\n")] = '\0';
                printf("  ID   : "); scanf("%d", &id); getchar();
                cadastrarUsuario(nome, id);
                break;

            /* --- Emprestar livro --- */
            case 3:
                listarUsuarios();
                printf("\n  Informe o ID do usuario: ");
                scanf("%d", &usuarioId); getchar();
                usuarioIdx = buscarUsuarioPorId(usuarioId);
                if (usuarioIdx < 0) {
                    printf("  Usuario nao encontrado.\n");
                    break;
                }
                listarLivros();
                printf("\n  Informe o indice do livro: ");
                scanf("%d", &livroIdx); getchar();
                if (livroIdx < 0 || livroIdx >= totalLivros) {
                    printf("  Indice de livro invalido.\n");
                    break;
                }
                emprestarLivro(usuarioIdx, livroIdx);
                break;

            /* --- Devolver livro --- */
            case 4:
                listarUsuarios();
                printf("\n  Informe o ID do usuario: ");
                scanf("%d", &usuarioId); getchar();
                usuarioIdx = buscarUsuarioPorId(usuarioId);
                if (usuarioIdx < 0) {
                    printf("  Usuario nao encontrado.\n");
                    break;
                }
                listarLivros();
                printf("\n  Informe o indice do livro a devolver: ");
                scanf("%d", &livroIdx); getchar();
                if (livroIdx < 0 || livroIdx >= totalLivros) {
                    printf("  Indice de livro invalido.\n");
                    break;
                }
                devolverLivro(usuarioIdx, livroIdx);
                break;

            /* --- Buscar livro por título --- */
            case 5:
                printf("  Termo de busca: ");
                fgets(termo, sizeof(termo), stdin);
                termo[strcspn(termo, "\n")] = '\0';
                livroIdx = buscarLivroPorTitulo(termo);
                if (livroIdx < 0) {
                    printf("  Nenhum livro encontrado com o termo '%s'.\n", termo);
                } else {
                    printf("  Encontrado: [%d] '%s' - %s | ISBN: %s | %s\n",
                           livroIdx,
                           acervo[livroIdx].titulo,
                           acervo[livroIdx].autor,
                           acervo[livroIdx].isbn,
                           acervo[livroIdx].status == 0 ? "Disponivel" : "Emprestado");
                }
                break;

            case 6:  listarLivros();          break;
            case 7:  listarUsuarios();        break;
            case 8:  exibirFilaEspera();      break;
            case 9:  exibirHistoricoAcoes();  break;
            case 10: desfazerAcao();          break;

            case 0:
                printf("\nSaindo do sistema. Ate logo!\n");
                break;

            default:
                printf("  Opcao invalida. Tente novamente.\n");
        }

    } while (opcao != 0);

    /* ============================================================
     * ETAPA 7 – LIBERAÇÃO DE MEMÓRIA (boa prática)
     * Libera todos os nós alocados com malloc antes de encerrar.
     * ============================================================ */

    /* Liberar fila */
    {
        NoFila *nf = filaEspera.frente;
        while (nf) {
            NoFila *tmp = nf->proximo;
            free(nf);
            nf = tmp;
        }
    }

    /* Liberar pilha */
    {
        NoPilha *np = pilhaUndo.topo;
        while (np) {
            NoPilha *tmp = np->proximo;
            free(np);
            np = tmp;
        }
    }

    return 0;
}