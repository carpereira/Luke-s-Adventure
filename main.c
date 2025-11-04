/*
 Tela de entrada
 alteração da dieção do lançamento da rede
 mensagem de mudança de fase
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_LARG 1000
#define WINDOW_ALT 600

#define GRAVIDADE 1
#define MAX_OBSTACULOS 10
#define MAX_INIMIGOS 20
#define MAX_CORACOES 5
#define MAX_VIDAS 5

#define MAX_REDES 50
#define REDE_W 24
#define REDE_H 12

#define TRANSITION_MS 5000
#define FADE_IN_MS 1000
#define HOLD_MS 3000
#define FADE_OUT_MS 1000

/* --- Tipos --- */
typedef struct { SDL_Rect ret; bool solido; bool plataforma; } Obstaculo;
typedef struct {
    SDL_Rect ret;
    int veloc;
    int velPulo;
    int velY;
    bool pulando;
    bool abaixando;
    int coracoes;
    int vidas;
    int invencivel;
    int facing; // direita=1, esquerda=-1
} Character;
typedef struct {
    SDL_Rect ret;
    int dir;
    int veloc;
    int velY;
    bool ativo;
    int tipo;
} Inimigo;
typedef struct {
    SDL_Rect ret;
    int velocidade; // horizontal (fase1) ou vertical (fase2)
    bool ativo;
} Rede;

static bool colidem(SDL_Rect a, SDL_Rect b) { return SDL_HasIntersection(&a, &b); }

static bool podeLevantar(Character c, Obstaculo* obstaculos, int qtdObs) {
    if (!c.abaixando) return true;
    SDL_Rect hitbox = c.ret;
    int dif = 50;
    hitbox.y -= dif; hitbox.h += dif;
    for (int i = 0; i < qtdObs; ++i)
        if (obstaculos[i].solido && colidem(hitbox, obstaculos[i].ret)) return false;
    return true;
}

static SDL_Texture* tryLoadTexture(SDL_Renderer* ren, const char* path) {
    if (!path) return NULL;
    SDL_Texture* tex = IMG_LoadTexture(ren, path);
    if (!tex) SDL_Log("Aviso: não conseguiu carregar textura '%s': %s", path, IMG_GetError());
    return tex;
}

/* --- Máquina de estados --- */
typedef enum { EST_MENU, EST_FASE1, EST_TRANSICAO, EST_FASE2 } EstadoJogo;

/* --- Função principal --- */
int main(int argc, char** argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Erro SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("Aviso: IMG_Init não ativou PNG: %s", IMG_GetError());
    }
    if (TTF_Init() == -1) SDL_Log("Aviso: TTF_Init falhou: %s", TTF_GetError());
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) SDL_Log("Aviso: Mix_OpenAudio falhou: %s", Mix_GetError());

    SDL_Window* window = SDL_CreateWindow("Luke's Adventure", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_LARG, WINDOW_ALT, 0);
    if (!window) { fprintf(stderr,"Erro criar janela: %s\n", SDL_GetError()); goto cleanup_sdl; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { fprintf(stderr,"Erro criar renderer: %s\n", SDL_GetError()); goto cleanup_window; }

    // Texturas opcionais
    SDL_Texture* texLuke = tryLoadTexture(renderer, "assets/luke.png");
    (void)texLuke;

    // carregar fonte (multipath)
    TTF_Font* font = NULL;
    const char* font_paths[] = {
        "assets/DejaVuSans.ttf",
        "DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:\\Windows\\Fonts\\DejaVuSans.ttf",
        NULL
    };
    for (int i = 0; font_paths[i]; ++i) {
        font = TTF_OpenFont(font_paths[i], 36);
        if (font) { SDL_Log("Fonte carregada: %s", font_paths[i]); break; }
    }
    if (!font) SDL_Log("Aviso: não encontrou DejaVuSans.ttf; usará fallback gráfico.");

    // carregar som de transição
    Mix_Music* musicaTrans = NULL;
    musicaTrans = Mix_LoadMUS("assets/transicao.wav");
    if (!musicaTrans) musicaTrans = Mix_LoadMUS("assets/transicao.mp3");
    if (!musicaTrans) SDL_Log("Aviso: não encontrou assets/transicao.(wav|mp3): %s", Mix_GetError());

    SDL_Color textColor = {255,255,255,255};
    srand((unsigned)time(NULL));

    /* --- Inicializações da Fase 1 (mantidas intactas) --- */
    Character Luke;
    Luke.ret.w = 50; Luke.ret.h = 100; Luke.ret.x = 100; Luke.ret.y = WINDOW_ALT - 150;
    Luke.veloc = 10; Luke.velPulo = -15; Luke.velY = 0; Luke.pulando = false; Luke.abaixando = false;
    Luke.coracoes = MAX_CORACOES; Luke.vidas = MAX_VIDAS; Luke.invencivel = 0; Luke.facing = 1;

    int faseLargura = 3000;
    int cameraX = 0;
    int faseAtual = 1;

    Obstaculo obstaculos[MAX_OBSTACULOS];
    int qtdObs = 0;
    obstaculos[qtdObs++] = (Obstaculo){{0, WINDOW_ALT - 50, faseLargura, 50}, true, false};
    obstaculos[qtdObs++] = (Obstaculo){{600, WINDOW_ALT - 120, 100, 20}, true, true};
    obstaculos[qtdObs++] = (Obstaculo){{1000, WINDOW_ALT - 200, 150, 20}, true, true};
    obstaculos[qtdObs++] = (Obstaculo){{1300, WINDOW_ALT - 250, 150, 20}, true, true};
    obstaculos[qtdObs++] = (Obstaculo){{2000, WINDOW_ALT - 120, 100, 20}, true, true};

    SDL_Rect porta = {2800, WINDOW_ALT - 150, 50, 100};

    Inimigo inimigos[MAX_INIMIGOS] = {0};
    int qtdInimigos = 3;
    inimigos[0] = (Inimigo){{800, WINDOW_ALT - 150, 50, 50}, -1, 2, 0, true, 1};
    inimigos[1] = (Inimigo){{1600, WINDOW_ALT - 150, 50, 50}, 1, 3, 0, true, 2};
    inimigos[2] = (Inimigo){{2300, WINDOW_ALT - 150, 50, 50}, -1, 2, 0, true, 3};

    Uint32 ultimoSpawn = SDL_GetTicks();

    Rede redes[MAX_REDES];
    for (int i = 0; i < MAX_REDES; ++i) { redes[i].ativo = false; redes[i].ret = (SDL_Rect){0,0,REDE_W,REDE_H}; redes[i].velocidade = 0; }

    int contadorInimigosMortos = 0;
    bool esqPress=false, dirPress=false, baixoPress=false, puloPress=false;
    bool rodando = true;
    SDL_Event ev;

    /* --- Menu --- */
    EstadoJogo estado = EST_MENU;
    SDL_Rect botao = { WINDOW_LARG/2 - 150, WINDOW_ALT/2 - 60, 300, 120 };
    SDL_Color botaoCor = {255,215,0,255};
    SDL_Color bordaCor = {0,0,0,255};
    bool jogoIniciado = false;

    /* --- Transição control --- */
    bool transitionActive = false;
    Uint32 transitionStartMs = 0;
    int transitionDestPhase = 2;
    const char* transitionMessage = "Você está mudando de fase";

    /* --- Loop principal --- */
    while (rodando) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { rodando = false; break; }

            if (estado == EST_MENU) {
                if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                    int mx = ev.button.x, my = ev.button.y;
                    if (mx >= botao.x && mx <= botao.x + botao.w && my >= botao.y && my <= botao.y + botao.h) {
                        estado = EST_FASE1; jogoIniciado = true;
                    }
                }
                if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_RETURN) {
                    estado = EST_FASE1; jogoIniciado = true;
                }
            } else {
                /* eventos do jogo */
                if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                    switch (ev.key.keysym.scancode) {
                        case SDL_SCANCODE_LEFT: esqPress = true; break;
                        case SDL_SCANCODE_RIGHT: dirPress = true; break;
                        case SDL_SCANCODE_DOWN: baixoPress = true; break;
                        case SDL_SCANCODE_SPACE: puloPress = true; break;
                        case SDL_SCANCODE_X: {
                            for (int idx = 0; idx < MAX_REDES; ++idx) {
                                if (!redes[idx].ativo) {
                                    redes[idx].ret.x = Luke.ret.x + Luke.ret.w/2 - REDE_W/2;
                                    redes[idx].ret.y = Luke.ret.y + Luke.ret.h/2 - REDE_H/2;
                                    if (faseAtual == 1) redes[idx].velocidade = 12 * Luke.facing; // horizontal
                                    else redes[idx].velocidade = -8; // sobe (velocidade aplicada em Y)
                                    redes[idx].ativo = true;
                                    break;
                                }
                            }
                        } break;
                        default: break;
                    }
                }
                if (ev.type == SDL_KEYUP) {
                    switch (ev.key.keysym.scancode) {
                        case SDL_SCANCODE_LEFT: esqPress = false; break;
                        case SDL_SCANCODE_RIGHT: dirPress = false; break;
                        case SDL_SCANCODE_DOWN: baixoPress = false; break;
                        case SDL_SCANCODE_SPACE: puloPress = false; break;
                        default: break;
                    }
                }
            }
        } // fim eventos

        /* --- Lógica conforme estado --- */
        if (estado == EST_MENU) {
            // render apenas do menu abaixo; pular update de jogo
        } else if (estado == EST_TRANSICAO) {
            if (!transitionActive) {
                transitionActive = true;
                transitionStartMs = SDL_GetTicks();
                if (musicaTrans) Mix_PlayMusic(musicaTrans, 1);
            }
            Uint32 now = SDL_GetTicks();
            if (now - transitionStartMs >= TRANSITION_MS) {
                transitionActive = false;
                estado = EST_FASE2;
                faseAtual = 2;
                // posicionar Luke no início da fase2 sem resetar dados da fase1 (conforme pedido)
                Luke.ret.x = 100; Luke.ret.y = WINDOW_ALT - 150; Luke.velY = 0;
                // limpar inimigos ativos (serão spawnados na fase2)
                for (int i = 0; i < MAX_INIMIGOS; ++i) inimigos[i].ativo = false;
                ultimoSpawn = SDL_GetTicks();
                if (musicaTrans) Mix_HaltMusic();
            } else {
                // durante transição, não atualiza física do jogo (só render overlay)
            }
        } else { // EST_FASE1 ou EST_FASE2
            // movimento Luke
            int movX = (dirPress - esqPress) * Luke.veloc;
            Luke.ret.x += movX;
            if (movX > 0) Luke.facing = 1; else if (movX < 0) Luke.facing = -1;

            // colisão lateral (fase1)
            if (faseAtual == 1) {
                for (int i = 0; i < qtdObs; ++i) {
                    if (!obstaculos[i].solido) continue;
                    if (colidem(Luke.ret, obstaculos[i].ret)) {
                        if (movX > 0) Luke.ret.x = obstaculos[i].ret.x - Luke.ret.w;
                        else if (movX < 0) Luke.ret.x = obstaculos[i].ret.x + obstaculos[i].ret.w;
                    }
                }
            }

            // pulo
            if (puloPress && !Luke.pulando && !Luke.abaixando) { Luke.pulando = true; Luke.velY = Luke.velPulo; }
            Luke.ret.y += Luke.velY; Luke.velY += GRAVIDADE;

            bool noChao = false;
            if (faseAtual == 1) {
                for (int i = 0; i < qtdObs; ++i) {
                    if (!obstaculos[i].solido) continue;
                    if (colidem(Luke.ret, obstaculos[i].ret)) {
                        if (Luke.velY > 0 && Luke.ret.y + Luke.ret.h > obstaculos[i].ret.y) {
                            Luke.ret.y = obstaculos[i].ret.y - Luke.ret.h; Luke.velY = 0; Luke.pulando = false; noChao = true;
                        } else if (Luke.velY < 0 && Luke.ret.y < obstaculos[i].ret.y + obstaculos[i].ret.h) {
                            Luke.ret.y = obstaculos[i].ret.y + obstaculos[i].ret.h; Luke.velY = 0; Luke.pulando = true;
                        }
                    }
                }
            } else {
                if (Luke.ret.y + Luke.ret.h >= WINDOW_ALT - 50) { Luke.ret.y = WINDOW_ALT - 150; Luke.velY = 0; Luke.pulando = false; noChao = true; }
            }
            if (!noChao && Luke.velY == 0) Luke.pulando = true;

            // abaixar / levantar
            if (baixoPress && !Luke.pulando) {
                if (!Luke.abaixando) { Luke.abaixando = true; Luke.ret.h = 50; Luke.ret.y += 50; }
            } else if (Luke.abaixando && podeLevantar(Luke, obstaculos, qtdObs)) {
                Luke.abaixando = false; Luke.ret.y -= 50; Luke.ret.h = 100;
            }

            // inimigos fase1 se movimentam lateralmente
            if (faseAtual == 1) {
                for (int i = 0; i < qtdInimigos; ++i) {
                    inimigos[i].ret.x += inimigos[i].dir * inimigos[i].veloc;
                    if (inimigos[i].ret.x < 500 || inimigos[i].ret.x > 2500) inimigos[i].dir *= -1;
                }
            }

            // fase2: spawn de inimigos caindo
            if (faseAtual == 2) {
                Uint32 agora = SDL_GetTicks();
                if (agora - ultimoSpawn > 1200) {
                    for (int i = 0; i < MAX_INIMIGOS; ++i) {
                        if (!inimigos[i].ativo) {
                            inimigos[i].ret = (SDL_Rect){ rand() % (WINDOW_LARG - 40), -50, 40, 40 };
                            inimigos[i].velY = 3 + rand() % 4;
                            inimigos[i].ativo = true;
                            inimigos[i].tipo = 1 + (rand() % 3);
                            break;
                        }
                    }
                    ultimoSpawn = agora;
                }
                for (int i = 0; i < MAX_INIMIGOS; ++i) {
                    if (!inimigos[i].ativo) continue;
                    inimigos[i].ret.y += inimigos[i].velY;
                    if (inimigos[i].ret.y > WINDOW_ALT) inimigos[i].ativo = false;
                }
            }

            // redes movem-se horizontal na fase1, vertical (para cima) na fase2
            for (int r = 0; r < MAX_REDES; ++r) {
                if (!redes[r].ativo) continue;
                if (faseAtual == 1) {
                    redes[r].ret.x += redes[r].velocidade;
                    if (redes[r].ret.x > faseLargura || (redes[r].ret.x + redes[r].ret.w) < 0) redes[r].ativo = false;
                } else {
                    redes[r].ret.y += redes[r].velocidade; // velocidade negativa => sobe
                    if (redes[r].ret.y + redes[r].ret.h < 0) redes[r].ativo = false;
                }
            }

            // colisões redes x inimigos
            for (int r = 0; r < MAX_REDES; ++r) {
                if (!redes[r].ativo) continue;
                for (int i = 0; i < MAX_INIMIGOS; ++i) {
                    if (!inimigos[i].ativo) continue;
                    if (colidem(redes[r].ret, inimigos[i].ret)) {
                        inimigos[i].ativo = false; redes[r].ativo = false;
                        contadorInimigosMortos++;
                        if (contadorInimigosMortos % 10 == 0) if (Luke.vidas < MAX_VIDAS) Luke.vidas++;
                        break;
                    }
                }
            }

            if (Luke.invencivel > 0) Luke.invencivel--;
            for (int i = 0; i < MAX_INIMIGOS; ++i) {
                if (inimigos[i].ativo && colidem(Luke.ret, inimigos[i].ret) && Luke.invencivel == 0) {
                    Luke.coracoes--; Luke.invencivel = 60;
                    if (Luke.coracoes <= 0) { Luke.vidas--; Luke.coracoes = MAX_CORACOES; Luke.ret.x = 100; Luke.ret.y = WINDOW_ALT - 150; }
                    if (Luke.vidas <= 0) { SDL_Log("Game Over"); rodando = false; }
                }
            }

            // detectar trigger: colisão com porta na fase1 -> iniciar transição
            if (faseAtual == 1) {
                SDL_Rect lukeTela = { Luke.ret.x - cameraX, Luke.ret.y, Luke.ret.w, Luke.ret.h };
                SDL_Rect portaTela = porta; portaTela.x -= cameraX;
                if (colidem(lukeTela, portaTela)) {
                    estado = EST_TRANSICAO;
                    transitionActive = false; // forçar iniciar transição no próximo loop
                }
            }

            // limites e câmera
            if (Luke.ret.x < 0) Luke.ret.x = 0;
            if (Luke.ret.x + Luke.ret.w > faseLargura) Luke.ret.x = faseLargura - Luke.ret.w;
            cameraX = Luke.ret.x + Luke.ret.w/2 - WINDOW_LARG/2;
            if (cameraX < 0) cameraX = 0;
            if (cameraX > faseLargura - WINDOW_LARG) cameraX = faseLargura - WINDOW_LARG;
        } // fim atualização jogo

        /* --- RENDER (sempre) --- */
        SDL_SetRenderDrawColor(renderer, 20, 20, 70, 255);
        SDL_RenderClear(renderer);

        if (estado == EST_MENU) {
            // render menu
            SDL_SetRenderDrawColor(renderer, botaoCor.r, botaoCor.g, botaoCor.b, botaoCor.a);
            SDL_RenderFillRect(renderer, &botao);
            SDL_SetRenderDrawColor(renderer, bordaCor.r, bordaCor.g, bordaCor.b, bordaCor.a);
            SDL_RenderDrawRect(renderer, &botao);
            if (font) {
                SDL_Surface* surf = TTF_RenderUTF8_Blended(font, "ENTER", textColor);
                if (surf) {
                    SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_Rect dst = { botao.x + (botao.w - surf->w)/2, botao.y + (botao.h - surf->h)/2, surf->w, surf->h };
                    SDL_RenderCopy(renderer, tx, NULL, &dst);
                    SDL_DestroyTexture(tx); SDL_FreeSurface(surf);
                }
            } else {
                // fallback desenho rudimentar "ENTER"
                SDL_SetRenderDrawColor(renderer, 0,0,0,255);
                int tx = botao.x + 40, ty = botao.y + 30;
                // desenha 'ENTER' simples (linhas)
                SDL_RenderDrawLine(renderer, tx, ty, tx, ty+40);
                SDL_RenderDrawLine(renderer, tx+5, ty, tx+35, ty);
                SDL_RenderDrawLine(renderer, tx+5, ty+20, tx+30, ty+20);
                SDL_RenderDrawLine(renderer, tx+5, ty+40, tx+35, ty+40);
                // N
                int nx = tx + 50;
                SDL_RenderDrawLine(renderer, nx, ty, nx, ty+40);
                SDL_RenderDrawLine(renderer, nx, ty, nx+30, ty+40);
                SDL_RenderDrawLine(renderer, nx+30, ty, nx+30, ty+40);
                // T
                int tx2 = nx + 45;
                SDL_RenderDrawLine(renderer, tx2, ty, tx2+30, ty);
                SDL_RenderDrawLine(renderer, tx2+15, ty, tx2+15, ty+40);
                // E R (aprox.)
                int ex = tx2 + 55;
                SDL_RenderDrawLine(renderer, ex, ty, ex, ty+40);
                SDL_RenderDrawLine(renderer, ex+5, ty, ex+35, ty);
                SDL_RenderDrawLine(renderer, ex+5, ty+20, ex+30, ty+20);
                SDL_RenderDrawLine(renderer, ex+5, ty+40, ex+35, ty+40);
            }

        } else {
            // fundo e elementos (Fase1/Fase2 com câmera)
            if (faseAtual == 3) SDL_SetRenderDrawColor(renderer, 30,30,50,255);
            else SDL_SetRenderDrawColor(renderer, 80,120,200,255);
            SDL_Rect ceu = {0,0,WINDOW_LARG,WINDOW_ALT}; SDL_RenderFillRect(renderer, &ceu);

            // montanhas + sol (fixos)
            SDL_SetRenderDrawColor(renderer, 100,100,120,255);
            SDL_Rect mont1 = {100, WINDOW_ALT - 200, 300, 200}, mont2 = {700, WINDOW_ALT - 250, 400, 250};
            SDL_RenderFillRect(renderer, &mont1); SDL_RenderFillRect(renderer, &mont2);
            SDL_SetRenderDrawColor(renderer, 255,255,100,255);
            SDL_Rect sol = { WINDOW_LARG - 150, 100, 80, 80 }; SDL_RenderFillRect(renderer, &sol);

            // fase1: obstáculos + porta
            if (faseAtual == 1) {
                for (int i=0;i<qtdObs;i++){
                    SDL_Rect obsTela = obstaculos[i].ret; obsTela.x -= cameraX;
                    SDL_SetRenderDrawColor(renderer, 100,60,20,255); SDL_RenderFillRect(renderer, &obsTela);
                }
                SDL_SetRenderDrawColor(renderer, 0,200,0,255);
                SDL_Rect portaTela = { porta.x - cameraX, porta.y, porta.w, porta.h }; SDL_RenderFillRect(renderer, &portaTela);
            } else {
                SDL_SetRenderDrawColor(renderer, 70,70,100,255); SDL_Rect chao = {0, WINDOW_ALT-50, WINDOW_LARG, 50}; SDL_RenderFillRect(renderer, &chao);
            }

            // inimigos
            for (int i=0;i<MAX_INIMIGOS;i++){
                if (!inimigos[i].ativo) continue;
                SDL_SetRenderDrawColor(renderer, 200,30,30,255);
                SDL_Rect inTela = inimigos[i].ret; if (faseAtual==1) inTela.x -= cameraX;
                SDL_RenderFillRect(renderer, &inTela);
            }

            // redes
            SDL_SetRenderDrawColor(renderer, 200,200,200,255);
            for (int r=0;r<MAX_REDES;r++){
                if (!redes[r].ativo) continue;
                SDL_Rect rd = redes[r].ret; if (faseAtual==1) rd.x -= cameraX;
                SDL_RenderFillRect(renderer, &rd);
            }

            // Luke (usar textura se carregada, senão rect)
            SDL_Rect lukeTela = { Luke.ret.x - (faseAtual==1 ? cameraX : 0), Luke.ret.y, Luke.ret.w, Luke.ret.h };
            if (Luke.invencivel % 10 < 5) SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            else SDL_SetRenderDrawColor(renderer, 255,100,100,255);
            SDL_RenderFillRect(renderer, &lukeTela);

            // HUD corações/vidas
            int margem = 20, corTam = 25;
            for (int i=0;i<MAX_CORACOES;i++){
                if (i < Luke.coracoes) SDL_SetRenderDrawColor(renderer,255,0,0,255); else SDL_SetRenderDrawColor(renderer,60,0,0,255);
                SDL_Rect c = { margem + i*(corTam+5), margem, corTam, corTam }; SDL_RenderFillRect(renderer, &c);
            }
            SDL_SetRenderDrawColor(renderer,255,255,255,255); SDL_Rect vidasRect = {margem, margem+40, 20 * Luke.vidas, 10}; SDL_RenderFillRect(renderer, &vidasRect);
        }

        // se transição: desenhar overlay com fade + texto + som (som já iniciado)
        if (estado == EST_TRANSICAO) {
            Uint32 now = SDL_GetTicks();
            Uint32 elapsed = now - transitionStartMs; // 0..TRANSITION_MS
            Uint8 alpha = 255;
            if (elapsed < FADE_IN_MS) { float t=(float)elapsed/FADE_IN_MS; alpha = (Uint8)(t*255.0f); }
            else if (elapsed < FADE_IN_MS + HOLD_MS) alpha = 255;
            else { Uint32 e2 = elapsed - (FADE_IN_MS + HOLD_MS);
                if (e2 >= FADE_OUT_MS) alpha = 0; else { float t = 1.0f - ((float)e2/FADE_OUT_MS); alpha = (Uint8)(t*255.0f); }
            }

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0,0,0, alpha/2); SDL_Rect overlay = {0,0,WINDOW_LARG,WINDOW_ALT}; SDL_RenderFillRect(renderer, &overlay);

            int boxW=560, boxH=120;
            SDL_Rect box = { WINDOW_LARG/2 - boxW/2, WINDOW_ALT/2 - boxH/2, boxW, boxH };
            SDL_SetRenderDrawColor(renderer, 255,215,0, alpha); SDL_RenderFillRect(renderer, &box);
            SDL_SetRenderDrawColor(renderer, 0,0,0, alpha); SDL_RenderDrawRect(renderer, &box);

            if (font) {
                SDL_Surface* surf = TTF_RenderUTF8_Blended(font, transitionMessage, textColor);
                if (surf) {
                    SDL_Texture* tx = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_SetTextureBlendMode(tx, SDL_BLENDMODE_BLEND);
                    SDL_SetTextureAlphaMod(tx, alpha);
                    SDL_Rect dst = { WINDOW_LARG/2 - surf->w/2, WINDOW_ALT/2 - surf->h/2, surf->w, surf->h };
                    SDL_RenderCopy(renderer, tx, NULL, &dst);
                    SDL_DestroyTexture(tx); SDL_FreeSurface(surf);
                }
            } else {
                SDL_SetRenderDrawColor(renderer, 0,0,0, alpha);
                SDL_Rect r = { WINDOW_LARG/2 - 200, WINDOW_ALT/2 - 12, 400, 24 };
                SDL_RenderFillRect(renderer, &r);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    } // fim loop

    // cleanup
    if (musicaTrans) Mix_FreeMusic(musicaTrans);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;

cleanup_window:
    if (window) SDL_DestroyWindow(window);
cleanup_sdl:
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 1;
}


