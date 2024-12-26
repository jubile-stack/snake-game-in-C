#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 20
#define GRID_OFFSET 50
#define GRID_WIDTH ((SCREEN_WIDTH - 2 * GRID_OFFSET) / CELL_SIZE)
#define GRID_HEIGHT ((SCREEN_HEIGHT - 2 * GRID_OFFSET) / CELL_SIZE)
#define EASY_OBSTACLES 0
#define MEDIUM_OBSTACLES 3
#define HARD_OBSTACLES 6

typedef enum { EASY, MEDIUM, HARD, NONE } Difficulty;
typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

// Segment du serpent
typedef struct {
    int x, y;
} Segment;

// Jeu Snake
typedef struct {
    Segment body[GRID_WIDTH * GRID_HEIGHT];
    int length;
    Direction dir;
} Snake;

typedef struct {
    int x, y;
    bool active;
} Obstacle;

typedef struct {
    int x, y;
} Food;

// Variables globales
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *titleFont = NULL;
TTF_Font *menuFont = NULL;
Mix_Music *backgroundMusic = NULL;
Mix_Chunk *clickSound = NULL;

// Prototypes
bool init();
void close();
SDL_Texture *loadTexture(const char *path);
void showSplashScreen();
int showMainMenu();
Difficulty showLevelMenu();
void showBestScores();
void showTutorial();
void startGame(Difficulty difficulty);
void handleInput(Direction *dir, bool *quit, bool *paused, bool *returnToMenu);
void updateSnake(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, bool *gameOver, int *score);
void renderGame(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, int score, bool gameOver, bool paused, SDL_Texture *foodTex, SDL_Texture *bgTex);
// Prototypes manquants
void spawnFood(Food *food, Snake *snake, Obstacle obstacles[], int numObstacles);
void spawnObstacles(Obstacle obstacles[], int numObstacles, Snake *snake, Food *food);
void drawMenu(SDL_Rect rects[], const char *labels[], int count);
bool isMouseInsideRect(int mouseX, int mouseY, SDL_Rect rect);
void renderGame(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, int score, bool gameOver, bool paused, SDL_Texture *foodTex, SDL_Texture *bgTex);
void handleInput(Direction *dir, bool *quit, bool *paused, bool *returnToMenu);
void updateSnake(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, bool *gameOver, int *score);

bool isMouseInsideRect(int mouseX, int mouseY, SDL_Rect rect);
void drawMenu(SDL_Rect rects[], const char *labels[], int count);

int main(int argc, char *argv[]) {
    if (!init()) {
        printf("Failed to initialize.\n");
        return 1;
    }

    if (backgroundMusic) Mix_PlayMusic(backgroundMusic, -1);

    bool quit = false;
    while (!quit) {
        int menuChoice = showMainMenu();

        switch (menuChoice) {
            case 1: { // Jouer
                Difficulty difficulty = showLevelMenu();
                if (difficulty != NONE) {
                    startGame(difficulty);
                }
                break;
            }
            case 2: // Meilleurs Scores
                showBestScores();
                break;
            case 3: // Tutoriel
                showTutorial();
                break;
            case 0: // Quitter
                quit = true;
                break;
        }
    }

    close();
    return 0;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! Error: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! Error: %s\n", TTF_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! Error: %s\n", Mix_GetError());
        return false;
    }

    window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! Error: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer could not be created! Error: %s\n", SDL_GetError());
        return false;
    }

    titleFont = TTF_OpenFont("assets/fonts/title.ttf", 48);
    menuFont = TTF_OpenFont("assets/fonts/menu.ttf", 24);

    if (!titleFont || !menuFont) {
        printf("Failed to load fonts! Error: %s\n", TTF_GetError());
        return false;
    }

    backgroundMusic = Mix_LoadMUS("assets/music/background.mp3");
    clickSound = Mix_LoadWAV("assets/sounds/click.wav");

    if (!backgroundMusic || !clickSound) {
        printf("Failed to load audio! Error: %s\n", Mix_GetError());
    }

    return true;
}

void close() {
    TTF_CloseFont(titleFont);
    TTF_CloseFont(menuFont);
    Mix_FreeMusic(backgroundMusic);
    Mix_FreeChunk(clickSound);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
}

SDL_Texture *loadTexture(const char *path) {
    SDL_Texture *newTexture = IMG_LoadTexture(renderer, path);
    if (!newTexture) {
        printf("Unable to load texture %s! Error: %s\n", path, IMG_GetError());
    }
    return newTexture;
}

void showSplashScreen() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};

    SDL_Surface *titleSurface = TTF_RenderText_Solid(titleFont, "Snake Game", textColor);
    SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, SCREEN_HEIGHT / 2 - titleSurface->h / 2, titleSurface->w, titleSurface->h};
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_FreeSurface(titleSurface);
    SDL_DestroyTexture(titleTexture);

    SDL_RenderPresent(renderer);
    SDL_Delay(2000);
}

int showMainMenu() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const char *labels[] = {"Jouer", "Meilleurs Scores", "Tutoriel", "Quitter"};
    SDL_Rect rects[4];

    for (int i = 0; i < 4; i++) {
        rects[i] = (SDL_Rect){SCREEN_WIDTH / 2 - 100, 200 + i * 60, 200, 50};
    }

    drawMenu(rects, labels, 4);
    SDL_RenderPresent(renderer);

    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 0;

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x, y = e.button.y;
                for (int i = 0; i < 4; i++) {
                    if (isMouseInsideRect(x, y, rects[i])) {
                        Mix_PlayChannel(-1, clickSound, 0);
                        return i + 1;
                    }
                }
            }
        }
    }
}

Difficulty showLevelMenu() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const char *labels[] = {"Facile", "Moyen", "Difficile", "Retour"};
    SDL_Rect rects[4];

    for (int i = 0; i < 4; i++) {
        rects[i] = (SDL_Rect){SCREEN_WIDTH / 2 - 100, 200 + i * 60, 200, 50};
    }

    drawMenu(rects, labels, 4);
    SDL_RenderPresent(renderer);

    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return NONE;

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x, y = e.button.y;
                for (int i = 0; i < 4; i++) {
                    if (isMouseInsideRect(x, y, rects[i])) {
                        Mix_PlayChannel(-1, clickSound, 0);
                        return (i == 0) ? EASY : (i == 1) ? MEDIUM : (i == 2) ? HARD : NONE;
                    }
                }
            }
        }
    }
}

// Ajoutez d'autres fonctionnalités ici, comme les meilleurs scores, le tutoriel et le gameplay.

void showBestScores() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    const char *title = "Meilleurs Scores";
    SDL_Surface *titleSurface = TTF_RenderText_Solid(titleFont, title, textColor);
    SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 50, titleSurface->w, titleSurface->h};
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_FreeSurface(titleSurface);
    SDL_DestroyTexture(titleTexture);

    const char *levels[] = {"Facile", "Moyen", "Difficile"};
    int scores[] = {100, 200, 300}; // Exemple de scores. Remplacez par des valeurs sauvegardées.

    for (int i = 0; i < 3; i++) {
        char scoreText[50];
        snprintf(scoreText, sizeof(scoreText), "%s: %d points", levels[i], scores[i]);
        SDL_Surface *scoreSurface = TTF_RenderText_Solid(menuFont, scoreText, textColor);
        SDL_Texture *scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_Rect scoreRect = {SCREEN_WIDTH / 2 - scoreSurface->w / 2, 150 + i * 60, scoreSurface->w, scoreSurface->h};
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        SDL_FreeSurface(scoreSurface);
        SDL_DestroyTexture(scoreTexture);
    }

    SDL_RenderPresent(renderer);

    SDL_Event e;
    bool backToMenu = false;
    while (!backToMenu) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                backToMenu = true;
            }
        }
    }
}

void showTutorial() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    const char *title = "Tutoriel";
    SDL_Surface *titleSurface = TTF_RenderText_Solid(titleFont, title, textColor);
    SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 50, titleSurface->w, titleSurface->h};
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_FreeSurface(titleSurface);
    SDL_DestroyTexture(titleTexture);

    const char *instructions[] = {
        "Commandes:",
        "Fleche Haut : Monter",
        "Fleche Bas : Descendre",
        "Fleche Gauche : Aller a gauche",
        "Fleche Droite : Aller a droite",
        "P : Pause",
        "M : Retour au menu principal",
        "",
        "Objectif:",
        "Mangez les nourritures rouges pour grandir.",
        "Evitez les obstacles et votre propre corps."
    };

    for (int i = 0; i < 10; i++) {
        SDL_Surface *instructionSurface = TTF_RenderText_Solid(menuFont, instructions[i], textColor);
        SDL_Texture *instructionTexture = SDL_CreateTextureFromSurface(renderer, instructionSurface);
        SDL_Rect instructionRect = {50, 150 + i * 30, instructionSurface->w, instructionSurface->h};
        SDL_RenderCopy(renderer, instructionTexture, NULL, &instructionRect);
        SDL_FreeSurface(instructionSurface);
        SDL_DestroyTexture(instructionTexture);
    }

    SDL_RenderPresent(renderer);

    SDL_Event e;
    bool backToMenu = false;
    while (!backToMenu) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
                backToMenu = true;
            }
        }
    }
}

void startGame(Difficulty difficulty) {
    // Configurer les textures
    SDL_Texture *foodTex = loadTexture("assets/images/food.png");
    SDL_Texture *bgTex = loadTexture("assets/images/background.png");

    // Initialiser le jeu
    Snake snake = {{{GRID_WIDTH / 2, GRID_HEIGHT / 2}}, 3, RIGHT};
    Food food = {0, 0};
    Obstacle obstacles[HARD_OBSTACLES] = {0};
    int score = 0;
    bool gameOver = false, paused = false, returnToMenu = false;

    int numObstacles = (difficulty == EASY) ? EASY_OBSTACLES : (difficulty == MEDIUM) ? MEDIUM_OBSTACLES : HARD_OBSTACLES;
    int delay = (difficulty == EASY) ? 300 : (difficulty == MEDIUM) ? 250 : 200;

    spawnFood(&food, &snake, obstacles, numObstacles);
    spawnObstacles(obstacles, numObstacles, &snake, &food);

    while (!returnToMenu) {
        handleInput(&snake.dir, &returnToMenu, &paused, &gameOver);
        if (!paused && !gameOver) {
            updateSnake(&snake, &food, obstacles, numObstacles, &gameOver, &score);
            delay = (delay > 50) ? delay - (score / 5) : 50;
        }
        renderGame(&snake, &food, obstacles, numObstacles, score, gameOver, paused, foodTex, bgTex);
        SDL_Delay(delay);
    }

    SDL_DestroyTexture(foodTex);
    SDL_DestroyTexture(bgTex);
}
void spawnFood(Food *food, Snake *snake, Obstacle obstacles[], int numObstacles) {
    bool valid;
    do {
        valid = true;
        food->x = (rand() % (GRID_WIDTH - 2)) + 1;
        food->y = (rand() % (GRID_HEIGHT - 2)) + 1;

        for (int i = 0; i < snake->length; i++) {
            if (snake->body[i].x == food->x && snake->body[i].y == food->y) {
                valid = false;
                break;
            }
        }

        for (int i = 0; i < numObstacles; i++) {
            if (obstacles[i].active && obstacles[i].x == food->x && obstacles[i].y == food->y) {
                valid = false;
                break;
            }
        }
    } while (!valid);
}
void spawnObstacles(Obstacle obstacles[], int numObstacles, Snake *snake, Food *food) {
    for (int i = 0; i < numObstacles; i++) {
        bool valid;
        do {
            valid = true;
            obstacles[i].x = (rand() % (GRID_WIDTH - 2)) + 1;
            obstacles[i].y = (rand() % (GRID_HEIGHT - 2)) + 1;

            for (int j = 0; j < snake->length; j++) {
                if (snake->body[j].x == obstacles[i].x && snake->body[j].y == obstacles[i].y) {
                    valid = false;
                    break;
                }
            }

            if (food->x == obstacles[i].x && food->y == obstacles[i].y) {
                valid = false;
            }
        } while (!valid);

        obstacles[i].active = true;
    }
}
void drawMenu(SDL_Rect rects[], const char *labels[], int count) {
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_SetRenderDrawColor(renderer, 50, 50, 200, 255);

    for (int i = 0; i < count; i++) {
        SDL_RenderFillRect(renderer, &rects[i]);
        SDL_Surface *surface = TTF_RenderText_Solid(menuFont, labels[i], textColor);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect textRect = {rects[i].x + (rects[i].w - surface->w) / 2, rects[i].y + (rects[i].h - surface->h) / 2, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &textRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}
bool isMouseInsideRect(int mouseX, int mouseY, SDL_Rect rect) {
    return mouseX >= rect.x && mouseX <= rect.x + rect.w && mouseY >= rect.y && mouseY <= rect.y + rect.h;
}
void renderGame(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, int score, bool gameOver, bool paused, SDL_Texture *foodTex, SDL_Texture *bgTex) {
    if (bgTex) {
        SDL_RenderCopy(renderer, bgTex, NULL, NULL);
    } else {
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
    }

    SDL_Rect foodRect = {GRID_OFFSET + food->x * CELL_SIZE, GRID_OFFSET + food->y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
    SDL_RenderCopy(renderer, foodTex, NULL, &foodRect);

    for (int i = 0; i < numObstacles; i++) {
        if (obstacles[i].active) {
            SDL_Rect obstacleRect = {GRID_OFFSET + obstacles[i].x * CELL_SIZE, GRID_OFFSET + obstacles[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
            SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
            SDL_RenderFillRect(renderer, &obstacleRect);
        }
    }

    // Render the snake
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (int i = 0; i < snake->length; i++) {
        SDL_Rect segmentRect = {GRID_OFFSET + snake->body[i].x * CELL_SIZE, GRID_OFFSET + snake->body[i].y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        SDL_RenderFillRect(renderer, &segmentRect);
    }

    SDL_RenderPresent(renderer);
}
void handleInput(Direction *dir, bool *quit, bool *paused, bool *returnToMenu) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            *quit = true;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP: if (*dir != DOWN) *dir = UP; break;
                case SDLK_DOWN: if (*dir != UP) *dir = DOWN; break;
                case SDLK_LEFT: if (*dir != RIGHT) *dir = LEFT; break;
                case SDLK_RIGHT: if (*dir != LEFT) *dir = RIGHT; break;
                case SDLK_p: *paused = !*paused; break;
                case SDLK_m: *returnToMenu = true; break;
            }
        }
    }
}
void updateSnake(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, bool *gameOver, int *score) {
    for (int i = snake->length; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }

    switch (snake->dir) {
        case UP: snake->body[0].y--; break;
        case DOWN: snake->body[0].y++; break;
        case LEFT: snake->body[0].x--; break;
        case RIGHT: snake->body[0].x++; break;
    }

    if (snake->body[0].x < 0 || snake->body[0].x >= GRID_WIDTH || snake->body[0].y < 0 || snake->body[0].y >= GRID_HEIGHT) {
        *gameOver = true;
    }

    for (int i = 1; i < snake->length; i++) {
        if (snake->body[0].x == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
            *gameOver = true;
        }
    }

    for (int i = 0; i < numObstacles; i++) {
        if (obstacles[i].active && snake->body[0].x == obstacles[i].x && snake->body[0].y == obstacles[i].y) {
            *gameOver = true;
        }
    }

    if (snake->body[0].x == food->x && snake->body[0].y == food->y) {
        snake->length++;
        (*score)++;
        spawnFood(food, snake, obstacles, numObstacles);
    }
}
// code de debogage
int main(int argc, char *argv[]) {
    printf("Démarrage du programme\n");

    if (!init()) {
        printf("Initialisation échouée\n");
        return 1;
    }

    printf("Initialisation réussie\n");

    if (backgroundMusic) {
        Mix_PlayMusic(backgroundMusic, -1);
        printf("Musique de fond jouée\n");
    }

    bool quit = false;
    while (!quit) {
        printf("Affichage du menu principal\n");
        int menuChoice = showMainMenu();

        switch (menuChoice) {
            case 1: {
                printf("Option: Jouer\n");
                Difficulty difficulty = showLevelMenu();
                if (difficulty != NONE) {
                    printf("Démarrage du jeu avec difficulté %d\n", difficulty);
                    startGame(difficulty);
                }
                break;
            }
            case 2:
                printf("Option: Meilleurs Scores\n");
                showBestScores();
                break;
            case 3:
                printf("Option: Tutoriel\n");
                showTutorial();
                break;
            case 0:
                printf("Option: Quitter\n");
                quit = true;
                break;
            default:
                printf("Option invalide\n");
                break;
        }
    }

    printf("Fermeture du programme\n");
    close();
    return 0;
}

