#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
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
#define MAX_LIVES 3
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
    bool isBonus;
} Food;

bool init(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font);
void close(SDL_Window *window, SDL_Renderer *renderer, TTF_Font *font, SDL_Texture *foodTex, SDL_Texture *bgTex);
SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path);
Difficulty showMenu(SDL_Renderer *renderer, TTF_Font *font);
bool isMouseInsideRect(int mouseX, int mouseY, SDL_Rect rect);
void spawnFood(Food *food, Snake *snake, Obstacle obstacles[], int numObstacles);
void spawnObstacles(Obstacle obstacles[], int numObstacles, Snake *snake, Food *food);
void handleInput(Direction *dir, bool *quit, bool *paused);
void updateSnake(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, bool *gameOver, int *score, int *lives, bool *paused);
void render(SDL_Renderer *renderer, Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, int score, int lives, bool gameOver, bool paused, TTF_Font *font, SDL_Texture *foodTex, SDL_Texture *bgTex);
void drawMenu(SDL_Renderer *renderer, TTF_Font *font, SDL_Rect easyRect, SDL_Rect mediumRect, SDL_Rect hardRect, const char *title);
void drawGrid(SDL_Renderer *renderer);
void drawSnake(SDL_Renderer *renderer, Snake *snake);
void drawSegment(SDL_Renderer *renderer, int x, int y, int size, SDL_Color bodyColor, SDL_Color shadowColor);
void drawObstacle(SDL_Renderer *renderer, int x, int y);

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    TTF_Font *font = NULL;
    SDL_Texture *foodTex = NULL;
    SDL_Texture *bgTex = NULL;

    if (!init(&window, &renderer, &font)) {
        printf("Failed to initialize SDL!\n");
        return 1;
    }

    // Charger les textures
    foodTex = loadTexture(renderer, "C:\\Users\\Olusegun\\Downloads\\jeu\\food.png");
    bgTex = loadTexture(renderer, "background.png");

    // Afficher le menu et récupérer la difficulté choisie
    Difficulty difficulty = showMenu(renderer, font);

    // Configurer les obstacles et la vitesse en fonction de la difficulté
    int numObstacles = 0;
    int initialDelay = 300;
    switch (difficulty) {
        case EASY:
            numObstacles = EASY_OBSTACLES;
            initialDelay = 300;
            break;
        case MEDIUM:
            numObstacles = MEDIUM_OBSTACLES;
            initialDelay = 250;
            break;
        case HARD:
            numObstacles = HARD_OBSTACLES;
            initialDelay = 200;
            break;
        default:
            close(window, renderer, font, foodTex, bgTex);
            return 0;
    }

    srand((unsigned int)time(NULL));

    // Initialisation du jeu
    Snake snake = {{{GRID_WIDTH / 2, GRID_HEIGHT / 2}}, 3, RIGHT};
    Food food = {0, 0, false};
    Obstacle obstacles[HARD_OBSTACLES] = {0};
    bool quit = false, gameOver = false, paused = false;
    int score = 0, lives = MAX_LIVES;
    int delay = initialDelay;

    spawnFood(&food, &snake, obstacles, numObstacles);
    spawnObstacles(obstacles, numObstacles, &snake, &food);

    while (!quit) {
        handleInput(&snake.dir, &quit, &paused);
        if (!gameOver && !paused) {
            updateSnake(&snake, &food, obstacles, numObstacles, &gameOver, &score, &lives, &paused);
            delay = initialDelay - (score * 5);
            if (delay < 50) delay = 50;
        }
        render(renderer, &snake, &food, obstacles, numObstacles, score, lives, gameOver, paused, font, foodTex, bgTex);
        SDL_Delay(delay);
    }

    close(window, renderer, font, foodTex, bgTex);
    return 0;
}

bool init(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! Error: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! Error: %s\n", TTF_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! Error: %s\n", IMG_GetError());
        return false;
    }

    *window = SDL_CreateWindow("Jeu Snake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!*window) {
        printf("Window could not be created! Error: %s\n", SDL_GetError());
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        printf("Renderer could not be created! Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        return false;
    }

    *font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 24);
    if (!*font) {
        printf("Failed to load font! Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        return false;
    }

    return true;
}

SDL_Texture *loadTexture(SDL_Renderer *renderer, const char *path) {
    SDL_Texture *newTexture = IMG_LoadTexture(renderer, path);
    if (!newTexture) {
        printf("Unable to load texture %s! Error: %s\n", path, IMG_GetError());
    }
    return newTexture;
}

Difficulty showMenu(SDL_Renderer *renderer, TTF_Font *font) {
    bool quit = false;
    Difficulty selectedDifficulty = NONE;

    SDL_Rect easyRect = {SCREEN_WIDTH / 2 - 100, 200, 200, 50};
    SDL_Rect mediumRect = {SCREEN_WIDTH / 2 - 100, 300, 200, 50};
    SDL_Rect hardRect = {SCREEN_WIDTH / 2 - 100, 400, 200, 50};

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
                selectedDifficulty = NONE;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;

                if (isMouseInsideRect(mouseX, mouseY, easyRect)) {
                    selectedDifficulty = EASY;
                    quit = true;
                } else if (isMouseInsideRect(mouseX, mouseY, mediumRect)) {
                    selectedDifficulty = MEDIUM;
                    quit = true;
                } else if (isMouseInsideRect(mouseX, mouseY, hardRect)) {
                    selectedDifficulty = HARD;
                    quit = true;
                }
            }
        }

        drawMenu(renderer, font, easyRect, mediumRect, hardRect, "Jeu Snake - Choisis ton niveau");
    }

    return selectedDifficulty;
}

bool isMouseInsideRect(int mouseX, int mouseY, SDL_Rect rect) {
    return mouseX >= rect.x && mouseX <= rect.x + rect.w && mouseY >= rect.y && mouseY <= rect.y + rect.h;
}

void drawMenu(SDL_Renderer *renderer, TTF_Font *font, SDL_Rect easyRect, SDL_Rect mediumRect, SDL_Rect hardRect, const char *title) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color buttonColor = {50, 50, 200, 255};
    SDL_Color copyrightColor = {100, 100, 100, 255};

    SDL_Surface *titleSurface = TTF_RenderText_Solid(font, title, textColor);
    SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - titleSurface->w / 2, 100, titleSurface->w, titleSurface->h};
    SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);
    SDL_FreeSurface(titleSurface);
    SDL_DestroyTexture(titleTexture);

    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g, buttonColor.b, buttonColor.a);

    SDL_RenderFillRect(renderer, &easyRect);
    SDL_Surface *easySurface = TTF_RenderText_Solid(font, "Facile", textColor);
    SDL_Texture *easyTexture = SDL_CreateTextureFromSurface(renderer, easySurface);
    SDL_Rect easyTextRect = {easyRect.x + (easyRect.w - easySurface->w) / 2, easyRect.y + (easyRect.h - easySurface->h) / 2, easySurface->w, easySurface->h};
    SDL_RenderCopy(renderer, easyTexture, NULL, &easyTextRect);
    SDL_FreeSurface(easySurface);
    SDL_DestroyTexture(easyTexture);

    SDL_RenderFillRect(renderer, &mediumRect);
    SDL_Surface *mediumSurface = TTF_RenderText_Solid(font, "Moyen", textColor);
    SDL_Texture *mediumTexture = SDL_CreateTextureFromSurface(renderer, mediumSurface);
    SDL_Rect mediumTextRect = {mediumRect.x + (mediumRect.w - mediumSurface->w) / 2, mediumRect.y + (mediumRect.h - mediumSurface->h) / 2, mediumSurface->w, mediumSurface->h};
    SDL_RenderCopy(renderer, mediumTexture, NULL, &mediumTextRect);
    SDL_FreeSurface(mediumSurface);
    SDL_DestroyTexture(mediumTexture);

    SDL_RenderFillRect(renderer, &hardRect);
    SDL_Surface *hardSurface = TTF_RenderText_Solid(font, "Difficile", textColor);
    SDL_Texture *hardTexture = SDL_CreateTextureFromSurface(renderer, hardSurface);
    SDL_Rect hardTextRect = {hardRect.x + (hardRect.w - hardSurface->w) / 2, hardRect.y + (hardRect.h - hardSurface->h) / 2, hardSurface->w, hardSurface->h};
    SDL_RenderCopy(renderer, hardTexture, NULL, &hardTextRect);
    SDL_FreeSurface(hardSurface);
    SDL_DestroyTexture(hardTexture);

    SDL_Surface *copyrightSurface = TTF_RenderText_Solid(font, "fait par Jubile-stack", copyrightColor);
    SDL_Texture *copyrightTexture = SDL_CreateTextureFromSurface(renderer, copyrightSurface);
    SDL_Rect copyrightRect = {10, SCREEN_HEIGHT - 30, copyrightSurface->w, copyrightSurface->h};
    SDL_RenderCopy(renderer, copyrightTexture, NULL, &copyrightRect);
    SDL_FreeSurface(copyrightSurface);
    SDL_DestroyTexture(copyrightTexture);

    SDL_RenderPresent(renderer);
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

void handleInput(Direction *dir, bool *quit, bool *paused) {
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
            }
        }
    }
}

void updateSnake(Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, bool *gameOver, int *score, int *lives, bool *paused) {
    for (int i = snake->length; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }
    switch (snake->dir) {
        case UP: snake->body[0].y--; break;
        case DOWN: snake->body[0].y++; break;
        case LEFT: snake->body[0].x--; break;
        case RIGHT: snake->body[0].x++; break;
    }

    if (snake->body[0].x < 0 || snake->body[0].x >= GRID_WIDTH ||
        snake->body[0].y < 0 || snake->body[0].y >= GRID_HEIGHT) {
        (*lives)--;
        if (*lives <= 0) {
            *gameOver = true;
        } else {
            *paused = true;
        }
    }

    for (int i = 1; i < snake->length; i++) {
        if (snake->body[0].x == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
            *gameOver = true;
        }
    }

    for (int i = 0; i < numObstacles; i++) {
        if (obstacles[i].active && snake->body[0].x == obstacles[i].x && snake->body[0].y == obstacles[i].y) {
            (*lives)--;
            if (*lives <= 0) {
                *gameOver = true;
            } else {
                *paused = true;
            }
        }
    }

    if (snake->body[0].x == food->x && snake->body[0].y == food->y) {
        snake->length++;
        (*score)++;
        spawnFood(food, snake, obstacles, numObstacles);
    }
}

void render(SDL_Renderer *renderer, Snake *snake, Food *food, Obstacle obstacles[], int numObstacles, int score, int lives, bool gameOver, bool paused, TTF_Font *font, SDL_Texture *foodTex, SDL_Texture *bgTex) {
    if (bgTex) {
        SDL_RenderCopy(renderer, bgTex, NULL, NULL);
    } else {
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
    }

    drawGrid(renderer);

    if (foodTex) {
        SDL_Rect foodRect = {GRID_OFFSET + food->x * CELL_SIZE, GRID_OFFSET + food->y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        SDL_RenderCopy(renderer, foodTex, NULL, &foodRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect foodRect = {GRID_OFFSET + food->x * CELL_SIZE, GRID_OFFSET + food->y * CELL_SIZE, CELL_SIZE, CELL_SIZE};
        SDL_RenderFillRect(renderer, &foodRect);
    }

    for (int i = 0; i < numObstacles; i++) {
        if (obstacles[i].active) {
            drawObstacle(renderer, GRID_OFFSET + obstacles[i].x * CELL_SIZE, GRID_OFFSET + obstacles[i].y * CELL_SIZE);
        }
    }

    drawSnake(renderer, snake);

    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Score: %d | Vies: %d", score, lives);
    SDL_Surface *infoSurface = TTF_RenderText_Solid(font, infoText, (SDL_Color){255, 255, 255, 255});
    SDL_Texture *infoTexture = SDL_CreateTextureFromSurface(renderer, infoSurface);
    SDL_Rect infoRect = {10, 10, infoSurface->w, infoSurface->h};
    SDL_RenderCopy(renderer, infoTexture, NULL, &infoRect);
    SDL_FreeSurface(infoSurface);
    SDL_DestroyTexture(infoTexture);

    if (paused) {
        SDL_Surface *pausedSurface = TTF_RenderText_Solid(font, "PAUSE", (SDL_Color){255, 255, 0, 255});
        SDL_Texture *pausedTexture = SDL_CreateTextureFromSurface(renderer, pausedSurface);
        SDL_Rect pausedRect = {SCREEN_WIDTH / 2 - pausedSurface->w / 2, SCREEN_HEIGHT / 2, pausedSurface->w, pausedSurface->h};
        SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
        SDL_FreeSurface(pausedSurface);
        SDL_DestroyTexture(pausedTexture);
    }

    if (gameOver) {
        SDL_Surface *gameOverSurface = TTF_RenderText_Solid(font, "GAME OVER!", (SDL_Color){255, 0, 0, 255});
        SDL_Texture *gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
        SDL_Rect gameOverRect = {SCREEN_WIDTH / 2 - gameOverSurface->w / 2, SCREEN_HEIGHT / 2, gameOverSurface->w, gameOverSurface->h};
        SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
        SDL_FreeSurface(gameOverSurface);
        SDL_DestroyTexture(gameOverTexture);
    }

    SDL_RenderPresent(renderer);
}

void drawGrid(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    for (int x = GRID_OFFSET; x < SCREEN_WIDTH - GRID_OFFSET; x += CELL_SIZE) {
        SDL_RenderDrawLine(renderer, x, GRID_OFFSET, x, SCREEN_HEIGHT - GRID_OFFSET);
    }
    for (int y = GRID_OFFSET; y < SCREEN_HEIGHT - GRID_OFFSET; y += CELL_SIZE) {
        SDL_RenderDrawLine(renderer, GRID_OFFSET, y, SCREEN_WIDTH - GRID_OFFSET, y);
    }
}

void drawSnake(SDL_Renderer *renderer, Snake *snake) {
    SDL_Color headColor = {0, 255, 0, 255};
    SDL_Color headShadow = {0, 150, 0, 255};
    drawSegment(renderer, GRID_OFFSET + snake->body[0].x * CELL_SIZE + 1,
                GRID_OFFSET + snake->body[0].y * CELL_SIZE + 1, CELL_SIZE - 2, headColor, headShadow);

    SDL_Color bodyColor = {50, 200, 50, 255};
    SDL_Color bodyShadow = {30, 100, 30, 255};
    for (int i = 1; i < snake->length; i++) {
        drawSegment(renderer, GRID_OFFSET + snake->body[i].x * CELL_SIZE + 2,
                    GRID_OFFSET + snake->body[i].y * CELL_SIZE + 2, CELL_SIZE - 4, bodyColor, bodyShadow);
    }
}

void drawSegment(SDL_Renderer *renderer, int x, int y, int size, SDL_Color bodyColor, SDL_Color shadowColor) {
    SDL_Rect shadowRect = {x + 2, y + 2, size, size};
    SDL_SetRenderDrawColor(renderer, shadowColor.r, shadowColor.g, shadowColor.b, shadowColor.a);
    SDL_RenderFillRect(renderer, &shadowRect);

    SDL_Rect bodyRect = {x, y, size, size};
    SDL_SetRenderDrawColor(renderer, bodyColor.r, bodyColor.g, bodyColor.b, bodyColor.a);
    SDL_RenderFillRect(renderer, &bodyRect);
}

void drawObstacle(SDL_Renderer *renderer, int x, int y) {
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_Rect rect = {x, y, CELL_SIZE, CELL_SIZE};
    SDL_RenderFillRect(renderer, &rect);
}
