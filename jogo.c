#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define WINDOW_LARG 1000
#define WINDOW_ALT 600

typedef struct {    //Personagem Luke
    SDL_Rect ret;
    int veloc;
    int velPulo;
    int velY;
    bool pulando;
    bool abaixando;
} Character;

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erro ao inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Luke's Adventure",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_LARG,
        WINDOW_ALT,
        0
    );

    if (!window) {
        printf("Erro ao criar janela: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erro ao criar renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Configuração inicial do personagem
    Character Luke;
    Luke.ret.x = 100;
    Luke.ret.y = WINDOW_ALT - 150;
    Luke.ret.w = 50;
    Luke.ret.h = 100;
    Luke.veloc = 5;
    Luke.velPulo = -15;
    Luke.velY = 0;
    Luke.pulando = false;
    Luke.abaixando = false;

    // Estados das teclas
    bool esqPress = false;
    bool dirPress = false;
    bool baixoPress = false;
    bool puloPress = false;

    bool rodando = true;
    SDL_Event e;

    while (rodando) {
        // Espera até 16ms por um evento (~60 FPS)
        if (SDL_WaitEventTimeout(&e, 16)) {
            if (e.type == SDL_QUIT) {
                rodando = false;
            }

            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LEFT:  esqPress = true; break;
                    case SDL_SCANCODE_RIGHT: dirPress = true; break;
                    case SDL_SCANCODE_DOWN:  baixoPress = true; break;
                    case SDL_SCANCODE_SPACE: puloPress = true; break;
                    default: break;
                }
            }

            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LEFT:  esqPress = false; break;
                    case SDL_SCANCODE_RIGHT: dirPress = false; break;
                    case SDL_SCANCODE_DOWN:  baixoPress = false; break;
                    case SDL_SCANCODE_SPACE: puloPress = false; break;
                    default: break;
                }
            }
        }

        // --- Lógica de movimento ---

        if (esqPress) {
            Luke.ret.x -= Luke.veloc;
        }
        if (dirPress) {
            Luke.ret.x += Luke.veloc;
        }

        // Pulo (só se não estiver abaixado)
        if (puloPress && !Luke.pulando && !Luke.abaixando) {
            Luke.pulando = true;
            Luke.velY = Luke.velPulo;
            puloPress = false; // evita segurar espaço e pular infinitamente
        }

        // Abaixar
        if (baixoPress && !Luke.pulando) {
            if (!Luke.abaixando) {
                Luke.abaixando = true;
                Luke.ret.h = 50;
                Luke.ret.y += 50;
            }
        } else {
            if (Luke.abaixando) {
                Luke.abaixando = false;
                Luke.ret.y -= 50;
                Luke.ret.h = 100;
            }
        }

        // Gravidade
        if (Luke.pulando) {
            Luke.ret.y += Luke.velY;
            Luke.velY += 1; // aceleração da gravidade

            if (Luke.ret.y >= WINDOW_ALT - 150) {
                Luke.ret.y = WINDOW_ALT - 150;
                Luke.pulando = false;
                Luke.velY = 0;
            }
        }

        // Renderização
        SDL_SetRenderDrawColor(renderer, 6, 0, 53, 255); // fundo azul #060035
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // personagem branco
        SDL_RenderFillRect(renderer, &Luke.ret);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
