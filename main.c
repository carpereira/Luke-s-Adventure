#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define WINDOW_LARG 1000
#define WINDOW_ALT 600
#define GRAVIDADE 1
#define MAX_OBSTACULOS 10
#define MAX_INIMIGOS 5
#define MAX_CORACOES 5
#define MAX_VIDAS 5

typedef struct {
    SDL_Rect ret;
    bool solido;
    bool plataforma;
} Obstaculo;

typedef struct {
    SDL_Rect ret;
    int veloc;
    int velPulo;
    int velY;
    bool pulando;
    bool abaixando;
    int coracoes;
    int vidas;
    int invencivel; // frames de invencibilidade após tomar dano
} Character;

typedef struct {
    SDL_Rect ret;
    int dir;
    int veloc;
} Inimigo;

bool colidem(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erro ao inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Luke's Adventure",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_LARG, WINDOW_ALT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erro ao criar renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // --- Personagem ---
    Character Luke;
    Luke.ret.w = 50;
    Luke.ret.h = 100;
    Luke.ret.x = 100;
    Luke.ret.y = WINDOW_ALT - 150;
    Luke.veloc = 5;
    Luke.velPulo = -15;
    Luke.velY = 0;
    Luke.pulando = false;
    Luke.abaixando = false;
    Luke.coracoes = MAX_CORACOES;
    Luke.vidas = MAX_VIDAS;
    Luke.invencivel = 0;

    // --- Fase ---
    int cameraX = 0;
    int faseLargura = 3000;
    int faseAtual = 1;

    // --- Obstáculos ---
    Obstaculo obstaculos[MAX_OBSTACULOS];
    int qtdObs = 0;

    obstaculos[qtdObs++] = (Obstaculo){{0, WINDOW_ALT - 50, faseLargura, 50}, true, false};
    obstaculos[qtdObs++] = (Obstaculo){{600, WINDOW_ALT - 120, 100, 20}, true, true};
    obstaculos[qtdObs++] = (Obstaculo){{1000, WINDOW_ALT - 200, 150, 20}, true, true};
    obstaculos[qtdObs++] = (Obstaculo){{1300, WINDOW_ALT - 280, 150, 20}, true, true};
    obstaculos[qtdObs++] = (Obstaculo){{2000, WINDOW_ALT - 120, 100, 20}, true, true};

    SDL_Rect porta = {2800, WINDOW_ALT - 150, 50, 100};

    // --- Inimigos ---
    Inimigo inimigos[MAX_INIMIGOS];
    int qtdInimigos = 3;
    inimigos[0] = (Inimigo){{800, WINDOW_ALT - 150, 50, 50}, -1, 2};
    inimigos[1] = (Inimigo){{1600, WINDOW_ALT - 150, 50, 50}, 1, 3};
    inimigos[2] = (Inimigo){{2300, WINDOW_ALT - 150, 50, 50}, -1, 2};

    // --- Estados de tecla ---
    bool esqPress = false, dirPress = false, baixoPress = false, puloPress = false;
    bool rodando = true;
    SDL_Event e;

    // --- Loop Pincipal ---
    while (rodando) {
        if (SDL_WaitEventTimeout(&e, 16)) {
            if (e.type == SDL_QUIT)
                rodando = false;

            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LEFT: esqPress = true; break;
                    case SDL_SCANCODE_RIGHT: dirPress = true; break;
                    case SDL_SCANCODE_DOWN: baixoPress = true; break;
                    case SDL_SCANCODE_SPACE: puloPress = true; break;
                }
            }
            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LEFT: esqPress = false; break;
                    case SDL_SCANCODE_RIGHT: dirPress = false; break;
                    case SDL_SCANCODE_DOWN: baixoPress = false; break;
                    case SDL_SCANCODE_SPACE: puloPress = false; break;
                }
            }
        }

        // --- Movimento horizontal ---
        int movimentoX = 0;
        if (esqPress) movimentoX -= Luke.veloc;
        if (dirPress) movimentoX += Luke.veloc;
        Luke.ret.x += movimentoX;

        // --- Colisão lateral ---
        for (int i = 0; i < qtdObs; i++) {
            if (!obstaculos[i].solido) continue;
            if (colidem(Luke.ret, obstaculos[i].ret)) {
                if (movimentoX > 0) Luke.ret.x = obstaculos[i].ret.x - Luke.ret.w;
                else if (movimentoX < 0) Luke.ret.x = obstaculos[i].ret.x + obstaculos[i].ret.w;
            }
        }

        // --- Gravidade e pulo ---
        if (puloPress && !Luke.pulando && !Luke.abaixando) {
            Luke.pulando = true;
            Luke.velY = Luke.velPulo;
            puloPress = false;
        }

        Luke.ret.y += Luke.velY;
        Luke.velY += GRAVIDADE;

        bool noChao = false;
        for (int i = 0; i < qtdObs; i++) {
            if (!obstaculos[i].solido) continue;
            if (colidem(Luke.ret, obstaculos[i].ret)) {
                if (obstaculos[i].plataforma) {
                    if (Luke.velY > 0 && (Luke.ret.y + Luke.ret.h) - Luke.velY <= obstaculos[i].ret.y) {
                        Luke.ret.y = obstaculos[i].ret.y - Luke.ret.h;
                        Luke.velY = 0;
                        Luke.pulando = false;
                        noChao = true;
                    }
                    continue;
                }

                if (Luke.velY > 0 && Luke.ret.y + Luke.ret.h > obstaculos[i].ret.y) {
                    Luke.ret.y = obstaculos[i].ret.y - Luke.ret.h;
                    Luke.velY = 0;
                    Luke.pulando = false;
                    noChao = true;
                } else if (Luke.velY < 0 && Luke.ret.y < obstaculos[i].ret.y + obstaculos[i].ret.h) {
                    Luke.ret.y = obstaculos[i].ret.y + obstaculos[i].ret.h;
                    Luke.velY = 0;
                }
            }
        }

        if (!noChao && Luke.velY == 0)
            Luke.pulando = true;

        // --- Abaixar ---
        if (baixoPress && !Luke.pulando) {
            if (!Luke.abaixando) {
                Luke.abaixando = true;
                Luke.ret.h = 50;
                Luke.ret.y += 50;
            }
        } else if (Luke.abaixando) {
            Luke.abaixando = false;
            Luke.ret.y -= 50;
            Luke.ret.h = 100;
        }

        // --- Atualiza inimigos ---
        for (int i = 0; i < qtdInimigos; i++) {
            inimigos[i].ret.x += inimigos[i].dir * inimigos[i].veloc;
            if (inimigos[i].ret.x < 500 || inimigos[i].ret.x > 2500)
                inimigos[i].dir *= -1; // muda direção
        }

        // --- Dano por colisão com inimigos ---
        if (Luke.invencivel > 0) Luke.invencivel--;

        for (int i = 0; i < qtdInimigos; i++) {
            if (colidem(Luke.ret, inimigos[i].ret) && Luke.invencivel == 0) {
                Luke.coracoes--;
                Luke.invencivel = 60; // ~1 segundo de invencibilidade
                if (Luke.coracoes <= 0) {
                    Luke.vidas--;
                    Luke.coracoes = MAX_CORACOES;
                    Luke.ret.x = 100;
                    Luke.ret.y = WINDOW_ALT - 150;
                }
                if (Luke.vidas <= 0) {
                    printf("Game Over!\n");
                    rodando = false;
                }
            }
        }

        // --- Câmera ---
        cameraX = Luke.ret.x - WINDOW_LARG / 2 + Luke.ret.w / 2;
        if (cameraX < 0) cameraX = 0;
        if (cameraX > faseLargura - WINDOW_LARG)
            cameraX = faseLargura - WINDOW_LARG;

        // --- Renderização ---
        SDL_SetRenderDrawColor(renderer, 20, 20, 70, 255);
        SDL_RenderClear(renderer);

        // Fundo
        SDL_SetRenderDrawColor(renderer, 40, 40, 120, 255);
        SDL_Rect fundo = {0, 0, WINDOW_LARG, WINDOW_ALT};
        SDL_RenderFillRect(renderer, &fundo);

        // Obstáculos
        for (int i = 0; i < qtdObs; i++) {
            SDL_Rect obsTela = obstaculos[i].ret;
            obsTela.x -= cameraX;
            if (obstaculos[i].plataforma)
                SDL_SetRenderDrawColor(renderer, 150, 100, 50, 255);
            else
                SDL_SetRenderDrawColor(renderer, 100, 60, 20, 255);
            SDL_RenderFillRect(renderer, &obsTela);
        }

        // Inimigos
        SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
        for (int i = 0; i < qtdInimigos; i++) {
            SDL_Rect inimTela = inimigos[i].ret;
            inimTela.x -= cameraX;
            SDL_RenderFillRect(renderer, &inimTela);
        }

        // Porta
        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
        SDL_Rect portaTela = porta;
        portaTela.x -= cameraX;
        SDL_RenderFillRect(renderer, &portaTela);

        // Luke
        if (Luke.invencivel % 10 < 5)
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        else
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255); // piscando durante invencibilidade
        SDL_Rect lukeTela = {Luke.ret.x - cameraX, Luke.ret.y, Luke.ret.w, Luke.ret.h};
        SDL_RenderFillRect(renderer, &lukeTela);

        // --- HUD ---
        int margem = 20;
        int coracaoTam = 25;
        for (int i = 0; i < MAX_CORACOES; i++) {
            if (i < Luke.coracoes)
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            else
                SDL_SetRenderDrawColor(renderer, 60, 0, 0, 255);
            SDL_Rect c = {margem + i * (coracaoTam + 5), margem, coracaoTam, coracaoTam};
            SDL_RenderFillRect(renderer, &c);
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect vidasRect = {margem, margem + 40, 20 * Luke.vidas, 10};
        SDL_RenderFillRect(renderer, &vidasRect);

        SDL_SetRenderDrawColor(renderer, 200, 200, 0, 255);
        SDL_Rect faseRect = {WINDOW_LARG - 150, margem, 100, 20};
        SDL_RenderFillRect(renderer, &faseRect);

        SDL_RenderPresent(renderer);

        // --- Fim da fase ---
        if (SDL_HasIntersection(&lukeTela, &portaTela)) {
            printf("Fase %d concluida!\n", faseAtual);
            SDL_Delay(800);
            faseAtual++;
            Luke.ret.x = 100;
            Luke.ret.y = WINDOW_ALT - 150;
            cameraX = 0;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

