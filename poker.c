#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>

#define TAMANHODECK 52
#define RAISE_VALOR 50

typedef enum
{
    COPAS,
    OUROS,
    ESPADILHA,
    PAUS
} Terno;

const char *suit_str[] = {"♥", "♦", "♣", "♠"};
const char *rank_str[] = {
    "??", "??", "2", "3", "4", "5", "6", "7",
    "8", "9", "10", "J", "Q", "K", "A"};

typedef struct
{
    uint8_t rank;
    Terno terno;
} Card;

typedef struct
{
    Card cartas[TAMANHODECK];
    int topo;
} Baralho;

typedef struct
{
    Card hole[2];
    int fichas;
    int aposta; // Aposta total desta rodada de betting
    bool fold;
    char nome[32];
} Jogador;

void ini_deck(Baralho *d);
void embaralhar_deck(Baralho *d);
Card comprar_carta(Baralho *d);
void impri_card(Card c);
void queimar(Baralho *d);
void tela_inicial(void);
void distribuir_inicial(Jogador *p, Jogador *bot, Baralho *d);
void mostrar_buraco(Jogador *p);
void mostrar_mesa(Card mesa[], int n);

void ordenar(Card *v, int n);
bool tem_flush(Card *c, int n, int *naipe_flush);
int maior_straight(Card *c, int n);
void contar(Card *c, int n, int *freq);

typedef struct
{
    int classificacao;
    int alto[5];
} HandValue;

HandValue avalia_mao(Card *hole, Card *mesa);
int comparar(HandValue a, HandValue b);
const char *nome_mao(int r);

// Funções de Aposta e Depósito
int acao_apostar(Jogador *jog, Jogador *bot, int *pot, int current_bet);
void depositar_fichas(Jogador *p);

/* Implementação */

void ini_deck(Baralho *d)
{
    int idx = 0;
    for (int s = 0; s < 4; s++)
    {
        for (int r = 2; r <= 14; r++)
        {
            d->cartas[idx].rank = r;
            d->cartas[idx].terno = s;
            idx++;
        }
    }
    d->topo = 0;
}

void embaralhar_deck(Baralho *d)
{
    for (int i = TAMANHODECK - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card temp = d->cartas[i];
        d->cartas[i] = d->cartas[j];
        d->cartas[j] = temp;
    }
    d->topo = 0;
}

Card comprar_carta(Baralho *d)
{
    if (d->topo >= TAMANHODECK)
        d->topo = 0;
    return d->cartas[d->topo++];
}

void impri_card(Card c)
{
    printf("[ %s%s ] ", rank_str[c.rank], suit_str[c.terno]);
}

void queimar(Baralho *d)
{
    if (d->topo < TAMANHODECK)
        d->topo++;
}

void tela_inicial()
{
    system("cls || clear");

    printf("┌─────────────────────────────────────────────────┐ \n");
    printf("│                                                 │ \n");
    printf("│             ♠ POKER UNBET ♠                     │ \n");
    printf("│                                                 │ \n");
    printf("│           ┌───────────┐ ┌───────────┐           │ \n");
    printf("│           │ A         │ │ K         │           │ \n");
    printf("│           │ ♣         │ │ ♦         │           │ \n");
    printf("│           │         A │ │         K │           │ \n");
    printf("│           └───────────┘ └───────────┘           │ \n");
    printf("│                                                 │ \n");
    printf("│         Pressione ENTER para começar...         │ \n");
    printf("│                                                 │ \n");
    printf("└─────────────────────────────────────────────────┘ \n");

    getchar();
}

void distribuir_inicial(Jogador *p, Jogador *bot, Baralho *d)
{
    p->hole[0] = comprar_carta(d);
    bot->hole[0] = comprar_carta(d);
    p->hole[1] = comprar_carta(d);
    bot->hole[1] = comprar_carta(d);
}

void mostrar_buraco(Jogador *p)
{
    printf("\n Suas cartas: ");
    impri_card(p->hole[0]);
    impri_card(p->hole[1]);
    printf("\n");
}

void mostrar_mesa(Card mesa[], int n)
{
    printf("\n Mesa: ");
    for (int i = 0; i < n; i++)
        impri_card(mesa[i]);
    printf("\n");
}

/* ---- Avaliação de Mão ----*/

void ordenar(Card *v, int n)
{
    for (int i = 0; i < n - 1; i++)
        for (int j = i + 1; j < n; j++)
            if (v[j].rank > v[i].rank)
            {
                Card tmp = v[i];
                v[i] = v[j];
                v[j] = tmp;
            }
}

bool tem_flush(Card *c, int n, int *naipe_flush)
{
    int count[4] = {0};

    for (int i = 0; i < n; i++)
    {
        count[c[i].terno]++;
        if (count[c[i].terno] >= 5)
        {
            *naipe_flush = c[i].terno;
            return true;
        }
    }
    return false;
}

int maior_straight(Card *c, int n)
{
    int ranks[15] = {0};

    for (int i = 0; i < n; i++)
    {
        if (c[i].rank >= 2 && c[i].rank <= 14)
            ranks[c[i].rank] = 1;
    }

    if (ranks[14] && ranks[5] && ranks[4] && ranks[3] && ranks[2])
        return 5; // A-5 (Wheel)

    for (int i = 14; i >= 5; i--)
        if (ranks[i] && ranks[i - 1] && ranks[i - 2] && ranks[i - 3] && ranks[i - 4])
            return i;

    return 0;
}

void contar(Card *c, int n, int *freq)
{
    for (int i = 0; i < 15; i++)
        freq[i] = 0;

    for (int i = 0; i < n; i++)
        freq[c[i].rank]++;
}

HandValue avalia_mao(Card *hole, Card *mesa)
{
    Card c[7];

    for (int i = 0; i < 2; i++)
        c[i] = hole[i];
    for (int i = 0; i < 5; i++)
        c[i + 2] = mesa[i];

    ordenar(c, 7);

    int freq[15];
    contar(c, 7, freq);

    int naipe_flush = -1;
    bool flush = tem_flush(c, 7, &naipe_flush);
    int reto = maior_straight(c, 7);

    // Straight Flush / Royal
    if (flush)
    {
        Card fc[7];
        int f = 0;
        for (int i = 0; i < 7; i++)
            if (c[i].terno == naipe_flush)
                fc[f++] = c[i];

        int s = maior_straight(fc, f);
        if (s)
        {
            if (s == 14)
                return (HandValue){9, {14, 0, 0, 0, 0}}; // Royal Flush
            return (HandValue){8, {s, 0, 0, 0, 0}};      // Straight Flush
        }
    }

    // Quadra
    for (int r = 14; r >= 2; r--)
        if (freq[r] == 4)
        {
            int kicker = 0;
            for (int k = 14; k >= 2; k--)
                if (freq[k] && k != r)
                {
                    kicker = k;
                    break;
                }
            return (HandValue){7, {r, kicker, 0, 0, 0}};
        }

    // Full House
    int trinca = 0, par = 0;
    for (int r = 14; r >= 2; r--)
        if (freq[r] >= 3)
        {
            trinca = r;
            break;
        }
    if (trinca)
    {
        for (int r = 14; r >= 2; r--)
            if (r != trinca && freq[r] >= 2)
            {
                par = r;
                break;
            }
        if (par)
            return (HandValue){6, {trinca, par, 0, 0, 0}};
    }

    // Flush
    if (flush)
    {
        int v[5] = {0};
        int j = 0;
        for (int i = 0; i < 7 && j < 5; i++)
            if (c[i].terno == naipe_flush)
                v[j++] = c[i].rank;
        return (HandValue){5, {v[0], v[1], v[2], v[3], v[4]}};
    }

    // Straight
    if (reto)
    {
        return (HandValue){4, {reto, 0, 0, 0, 0}};
    }

    // Trinca
    for (int r = 14; r >= 2; r--)
        if (freq[r] == 3)
        {
            int k[2] = {0, 0};
            int j = 0;
            for (int h = 14; h >= 2; h--)
                if (freq[h] && h != r)
                {
                    k[j++] = h;
                    if (j == 2)
                        break;
                }
            return (HandValue){3, {r, k[0], k[1], 0, 0}};
        }

    // Dois Pares
    int p1 = 0, p2 = 0;
    for (int r = 14; r >= 2; r--)
        if (freq[r] == 2)
        {
            if (!p1)
                p1 = r;
            else if (!p2)
            {
                p2 = r;
                break;
            }
        }

    if (p1 && p2)
    {
        int kicker = 0;
        for (int r = 14; r >= 2; r--)
            if (freq[r] == 1 && r != p1 && r != p2)
            {
                kicker = r;
                break;
            }
        return (HandValue){2, {p1, p2, kicker, 0, 0}};
    }

    // Um Par
    if (p1)
    {
        int k[3] = {0};
        int j = 0;
        for (int r = 14; r >= 2; r--)
            if (freq[r] == 1 && r != p1)
            {
                k[j++] = r;
                if (j == 3)
                    break;
            }
        return (HandValue){1, {p1, k[0], k[1], k[2], 0}};
    }

    // Carta Alta
    return (HandValue){0, {c[0].rank, c[1].rank, c[2].rank, c[3].rank, c[4].rank}};
}

int comparar(HandValue a, HandValue b)
{
    if (a.classificacao > b.classificacao)
        return 1;
    if (a.classificacao < b.classificacao)
        return -1;

    for (int i = 0; i < 5; i++)
    {
        if (a.alto[i] > b.alto[i])
            return 1;
        if (a.alto[i] < b.alto[i])
            return -1;
    }
    return 0;
}

const char *nome_mao(int r)
{
    switch (r)
    {
    case 9:
        return "Royal Flush";
    case 8:
        return "Straight Flush";
    case 7:
        return "Quadra";
    case 6:
        return "Full House (Casa Cheia)";
    case 5:
        return "Flush";
    case 4:
        return "Straight (Sequência)";
    case 3:
        return "Trinca";
    case 2:
        return "Dois Pares";
    case 1:
        return "Um Par";
    default:
        return "Carta Alta";
    }
}

/* ====== AÇÕES DE APOSTA*/

int acao_apostar(Jogador *jog, Jogador *bot, int *pot, int current_bet)
{
    int escolha = 0;

    // O quanto o jogador precisa pagar para igualar (current_bet é a aposta mais alta na mesa)
    int to_call = current_bet - jog->aposta;

    // Se to_call for negativo, significa que jog já apostou mais. Isso não deve acontecer
    if (to_call < 0)
        to_call = 0;

    printf("\n Pote atual: %d | Suas fichas: %d | Ficha do bot: %d\n", *pot, jog->fichas, bot->fichas);
    printf("Aposta atual da mesa: %d (Seu call é %d)\n", current_bet, to_call);
    printf("Escolha sua ação: \n");
    printf("1 - Pagar (Call) \n");
    printf("2 - Aumentar (Raise +%d)\n", RAISE_VALOR);
    printf("3 - Desistir (Fold) \n");
    printf("Opção: ");

    if (scanf(" %d", &escolha) != 1)
        escolha = 1;
    getchar(); // Limpar buffer

    if (escolha == 3) // Fold
    {
        jog->fold = true;
        printf("\nVocê desistiu (fold). \n");
        return -1; // Indica fold do jogador
    }
    else if (escolha == 2) // Raise
    {
        // O valor total que o jogador irá apostar (para igualar + RAISE_VALOR)
        int new_bet = current_bet + RAISE_VALOR;
        int to_pay = new_bet - jog->aposta;

        if (jog->fichas < to_pay)
            to_pay = jog->fichas; // All-in

        jog->fichas -= to_pay;
        jog->aposta += to_pay;
        *pot += to_pay;
        printf("\nVocê aumentou: pagou %d (Nova aposta total: %d). \n", to_pay, jog->aposta);

        // --- Decisão do Bot ---
        int bot_action = 1; // 80% de chance de Call/Raise
        int r = rand() % 100;

        // 20% de chance de foldar se o jogador aumentar
        if (r < 20 || bot->fichas <= 0)
        {
            bot_action = 3; // Fold
        }

        if (bot_action == 3) // Bot Fold
        {
            bot->fold = true;
            printf("Bot desistiu do aumento! \n");
            return 1; // Bot deu fold
        }
        else // Bot Call (Pagar)
        {
            int bot_pay = jog->aposta - bot->aposta;

            if (bot->fichas < bot_pay)
                bot_pay = bot->fichas; // All-in

            bot->fichas -= bot_pay;
            bot->aposta += bot_pay;
            *pot += bot_pay;
            printf("Bot pagou seu aumento (%d). \n", bot_pay);
            return 0; // Ação concluída (bot pagou)
        }
    }
    else // Call (Pagar)
    {
        int to_pay = to_call;
        if (to_pay > jog->fichas)
            to_pay = jog->fichas; // All-in

        jog->fichas -= to_pay;
        jog->aposta += to_pay;
        *pot += to_pay;
        printf("\nVocê pagou %d (call). \n", to_pay);

        // O bot também deve igualar a aposta se o jogador tiver aumentado antes (embora a lógica acima já resolva)
        // o bot apenas acompanha o que o jogador fez no Call.
        return 0;
    }
}

/* ====== DEPÓSITO DE FICHAS SIMULADO ====== */

void depositar_fichas(Jogador *p)
{
    char nome_cartao[32];
    char numero_cartao[20];
    int valor_deposito = 500; // Valor fixo de fichas a adicionar
    int cvc[5];

    system("cls || clear");
    printf("\n === ADICIONAR FICHAS (SIMULADO) === \n");
    printf("\n Fichas atuais: %d\n", p->fichas);

    // 1. Entrada de Nome
    printf("Nome no Cartão (Ex: Joao da Silva): ");
    scanf(" %31[^\n]", nome_cartao);
    getchar(); // Limpar

    // 2. Entrada de Cartão
    printf("Número do Cartão (16 dígitos): ");
    scanf(" %19s", numero_cartao);
    getchar(); // Limpar

    printf("Codigo de segurança(CVC): ");
    scanf(" %19s", cvc);
    getchar(); // Limpar

    // 3. Simulação de Processamento
    if (strlen(numero_cartao) != 16)
    {
        // Se o número de dígitos não for 16, apenas informa
        printf("\nAviso: O número do cartão deve ter 16 dígitos.\n");
    }

    // 4. Adicionar Saldo
    p->fichas += valor_deposito;
    printf("\n Depósito de %d fichas efetuado com sucesso (Cartão de %s)! \n", valor_deposito, nome_cartao);
    printf("\n Novo saldo de fichas: %d\n", p->fichas);

    printf("\n Pressione ENTER para continuar...");
    getchar();
}

/* ================= PRINCIPAL*/

int main()
{
    setlocale(LC_ALL, "pt_BR.UTF-8");
    srand(time(NULL));

    tela_inicial();

    Jogador jogador = {.fichas = 500, .fold = false, .aposta = 0};
    strcpy(jogador.nome, "Você");

    Jogador bot = {.fichas = 500, .fold = false, .aposta = 0};
    strcpy(bot.nome, "Bot");

    while (jogador.fichas > 0 && bot.fichas > 0)
    {
        system("cls || clear");
        printf("=== MENU PRINCIPAL === \n");
        printf("Fichas: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
        printf("1 - Iniciar Nova Rodada \n");
        printf("2 - Sair do Jogo \n");
        printf("3 - Adicionar Fichas (Depósito Simulado) \n");
        printf("Opção: ");

        int menu_escolha;
        if (scanf(" %d", &menu_escolha) != 1)
        {
            menu_escolha = 1;
        }
        getchar(); // Limpar buffer

        if (menu_escolha == 2)
        {
            printf("\nObrigado por jogar! Tchau!\n");
            break;
        }
        else if (menu_escolha == 3)
        {
            depositar_fichas(&jogador);
            continue;
        }

        printf("\n=== NOVA RODADA === \n");

        Baralho deck;
        ini_deck(&deck);
        embaralhar_deck(&deck);

        jogador.fold = false;
        bot.fold = false;
        jogador.aposta = 0;
        bot.aposta = 0;

        int pote = 0;
        int blind = 20;

        // Pagamento dos Blinds
        int jog_pay = (blind > jogador.fichas) ? jogador.fichas : blind;
        jogador.fichas -= jog_pay;
        jogador.aposta += jog_pay;

        int bot_pay = (blind > bot.fichas) ? bot.fichas : blind;
        bot.fichas -= bot_pay;
        bot.aposta += bot_pay;

        pote = jog_pay + bot_pay;

        printf("\nBlinds pagos: Você=%d | Bot=%d. Pote inicial: %d\n", jog_pay, bot_pay, pote);

        Card mesa[5] = {{0}};
        distribuir_inicial(&jogador, &bot, &deck);
        mostrar_buraco(&jogador);

        // Variável que armazena a aposta mais alta da mesa para a próxima ação
        int aposta_mesa = blind;

        // ---------- PRÉ-FLOP APOSTAS ----------
        printf("\n--- PRÉ-FLOP: apostas ---\n");
        int res = acao_apostar(&jogador, &bot, &pote, aposta_mesa);

        if (jogador.aposta > bot.aposta)
            aposta_mesa = jogador.aposta;
        if (bot.aposta > jogador.aposta)
            aposta_mesa = bot.aposta;

        if (res == -1) // Jogador foldou
        {
            printf("\nVocê perdeu a rodada (fold). Bot ganha o pote %d.\n", pote);
            bot.fichas += pote;
        }
        else if (bot.fold) // Bot foldou
        {
            printf("\nBot foldou. Você ganha o pote %d!\n", pote);
            jogador.fichas += pote;
        }

        // Se a rodada terminou por fold, pula para o final do loop
        if (jogador.fold || bot.fold)
        {
            printf("Fichas: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
            printf("Pressione ENTER para continuar...");
            getchar();
            continue;
        }

        // Resetar apostas entre as rodadas de betting
        jogador.aposta = 0;
        bot.aposta = 0;
        aposta_mesa = 0;

        // ---------- FLOP ----------
        queimar(&deck);
        mesa[0] = comprar_carta(&deck);
        mesa[1] = comprar_carta(&deck);
        mesa[2] = comprar_carta(&deck);
        mostrar_mesa(mesa, 3);
        mostrar_buraco(&jogador);
        printf("\n--- FLOP: apostas ---\n");

        res = acao_apostar(&jogador, &bot, &pote, aposta_mesa);

        if (jogador.aposta > bot.aposta)
            aposta_mesa = jogador.aposta;
        if (bot.aposta > jogador.aposta)
            aposta_mesa = bot.aposta;

        if (res == -1)
        {
            printf("\nVocê perdeu a rodada (fold). Bot ganha o pote %d.\n", pote);
            bot.fichas += pote;
        }
        else if (bot.fold)
        {
            printf("\nBot foldou. Você ganha o pote %d!\n", pote);
            jogador.fichas += pote;
        }

        if (jogador.fold || bot.fold)
        {
            printf("Fichas: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
            printf("Pressione ENTER para continuar...");
            getchar();
            continue;
        }

        jogador.aposta = 0;
        bot.aposta = 0;
        aposta_mesa = 0;

        // ---------- TURN (VEZ) ----------
        queimar(&deck);
        mesa[3] = comprar_carta(&deck);
        mostrar_mesa(mesa, 4);
        mostrar_buraco(&jogador);
        printf("\n--- TURN: apostas ---\n");

        res = acao_apostar(&jogador, &bot, &pote, aposta_mesa);

        if (jogador.aposta > bot.aposta)
            aposta_mesa = jogador.aposta;
        if (bot.aposta > jogador.aposta)
            aposta_mesa = bot.aposta;

        if (res == -1)
        {
            printf("\nVocê perdeu a rodada (fold). Bot ganha o pote %d.\n", pote);
            bot.fichas += pote;
        }
        else if (bot.fold)
        {
            printf("\nBot foldou. Você ganha o pote %d!\n", pote);
            jogador.fichas += pote;
        }

        if (jogador.fold || bot.fold)
        {
            printf("Fichas: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
            printf("Pressione ENTER para continuar...");
            getchar();
            continue;
        }

        jogador.aposta = 0;
        bot.aposta = 0;
        aposta_mesa = 0;

        // ---------- RIVER (RIO) ----------
        queimar(&deck);
        mesa[4] = comprar_carta(&deck);
        mostrar_mesa(mesa, 5);
        mostrar_buraco(&jogador);
        printf("\n--- RIVER: apostas ---\n");

        res = acao_apostar(&jogador, &bot, &pote, aposta_mesa);

        if (jogador.aposta > bot.aposta)
            aposta_mesa = jogador.aposta;
        if (bot.aposta > jogador.aposta)
            aposta_mesa = bot.aposta;

        if (res == -1)
        {
            printf("\nVocê perdeu a rodada (fold). Bot ganha o pote %d.\n", pote);
            bot.fichas += pote;
        }
        else if (bot.fold)
        {
            printf("\nBot foldou. Você ganha o pote %d!\n", pote);
            jogador.fichas += pote;
        }

        if (jogador.fold || bot.fold)
        {
            printf("Fichas: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
            printf("Pressione ENTER para continuar...");
            getchar();
            continue;
        }

        // ---------- SHOWDOWN (REVELAÇÃO) ----------
        printf("\n\n=== SHOWDOWN ===\n");
        printf("Pote total: %d\n", pote);

        HandValue jogador_mao = avalia_mao(jogador.hole, mesa);
        HandValue bot_mao = avalia_mao(bot.hole, mesa);

        printf("\nCartas do Bot: ");
        impri_card(bot.hole[0]);
        impri_card(bot.hole[1]);
        printf(" -> Mão: %s\n", nome_mao(bot_mao.classificacao));

        printf("Suas Cartas: ");
        impri_card(jogador.hole[0]);
        impri_card(jogador.hole[1]);
        printf(" -> Mão: %s\n", nome_mao(jogador_mao.classificacao));

        int comparacao = comparar(jogador_mao, bot_mao);

        if (comparacao > 0)
        {
            printf("\nParabéns! Você venceu a rodada com %s!\n", nome_mao(jogador_mao.classificacao));
            jogador.fichas += pote;
        }
        else if (comparacao < 0)
        {
            printf("\nO Bot venceu a rodada com %s!\n", nome_mao(bot_mao.classificacao));
            bot.fichas += pote;
        }
        else
        {
            printf("\nEmpate (Split Pot)! O pote é dividido.\n");
            jogador.fichas += pote / 2;
            bot.fichas += pote / 2;
        }

        printf("\n--- FIM DA RODADA ---\n");
        printf("Fichas: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
        printf("Pressione ENTER para continuar...");
        getchar();
    }

    // Fim de Jogo
    system("cls || clear");
    printf("\n====================================\n");
    if (jogador.fichas > bot.fichas)
        printf("PARABÉNS! VOCÊ GANHOU O JOGO! \n");
    else
        printf("FIM DE JOGO. O BOT VENCEU. FRACO E SEM TALENTO. \n");
    printf("Fichas finais: Você=%d | Bot=%d\n", jogador.fichas, bot.fichas);
    printf("====================================\n");

    return 0;
}