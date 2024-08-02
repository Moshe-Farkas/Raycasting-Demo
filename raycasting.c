#include <SDL2/SDL.h>
#include <stdbool.h>
#include <math.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BLOCK_SIZE 27

#define BOARD_WIDTH 23
#define BOARD_HEIGHT 27

typedef enum {
    EMPTY,
    SOLID_RED,
    SOLID_GREEN,
    SOLID_BLUE,
} GridState;

GridState board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
SDL_Rect leftViewPort = {0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT};
SDL_Rect rightViewPort = {SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT};
SDL_Rect* currentViewPort;

typedef struct {
    SDL_FPoint direction;
    SDL_FPoint pos;
    int w, h;
    float speed;
    int fov; // in deg
} player;

const SDL_Color cRed = {0xff, 0x00, 0x00, 0xff};
const SDL_Color cBlue = {0x00, 0x00, 0xff, 0xff};
const SDL_Color cGreen = {0x00, 0xff, 0x00, 0xff};
const SDL_Color cWhite = {0xff, 0xff, 0xff, 0xff};
const SDL_Color cBlack = {0x00, 0x00, 0x00, 0xff};

bool intersects(float x, float y, float w, float h) {
    // top left corner
    if (board[(int) (y / BLOCK_SIZE)][(int) (x / BLOCK_SIZE)] != EMPTY)
        return true;
    // bottom right corner
    else if (board[(int) ((y + h) / BLOCK_SIZE)][(int) ((x + w) / BLOCK_SIZE)] != EMPTY)
        return true;
    // top right corner
    else if (board[(int) (y / BLOCK_SIZE)][(int) ((x + w) / BLOCK_SIZE)] != EMPTY)
        return true;
    // bottom left corner
    else if (board[(int) ((y + h) / BLOCK_SIZE)][(int) (x / BLOCK_SIZE)] != EMPTY)
        return true;

    return false;
}

double degToRad(double deg) {
    return deg * (M_PI / 180.0f);
}

double radToDeg(double rad) {
    return rad * (180.0f / M_PI);
}

double playerAngle(player* p) {
    double pa = atan2(p->direction.y, p->direction.x);
    pa = radToDeg(pa);
    if (pa < 0)
        pa += 360;
    return pa;
}

void updateKeys(float dt, player* p) {
    const Uint8* keysStates = SDL_GetKeyboardState(NULL);
    if (keysStates[SDL_SCANCODE_A]) {
        double pa = playerAngle(p);
        pa -= 1;
        p->direction.x = cos(degToRad(pa));
        p->direction.y = sin(degToRad(pa));
    }
    if (keysStates[SDL_SCANCODE_D]) {
        double pa = playerAngle(p);
        pa += 1;
        p->direction.x = cos(degToRad(pa));
        p->direction.y = sin(degToRad(pa));
    }
    if (keysStates[SDL_SCANCODE_W]) {
        double newX = p->pos.x + p->direction.x * p->speed * dt;
        double newY = p->pos.y + p->direction.y * p->speed * dt;
        if (!intersects(newX, newY, p->w, p->h)) {
            p->pos.x = newX;
            p->pos.y = newY;
        } 
    }
    if (keysStates[SDL_SCANCODE_S]) {
        double newX = p->pos.x + -p->direction.x * p->speed * dt;
        double newY = p->pos.y + -p->direction.y * p->speed * dt;
        if (!intersects(newX, newY, p->w, p->h)) {
            p->pos.x = newX;
            p->pos.y = newY;
        }
    }
}

void initPlayer(player* p) {
    p->direction.x = 1;
    p->direction.y = 0;
    p->speed = 300;
    p->pos.x = leftViewPort.x + BLOCK_SIZE;
    p->pos.y = leftViewPort.y + BLOCK_SIZE;
    p->w = 15;
    p->h = 15;
    p->fov = 128;
}

void drawMap(SDL_Renderer* renderer) {
    currentViewPort = &leftViewPort;
    int xOffset = currentViewPort->x;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            GridState block = board[i][j];
            SDL_Color c;
            switch (block) {
                case SOLID_RED: c = cRed; break;
                case SOLID_GREEN: c = cGreen; break;
                case SOLID_BLUE: c = cBlue; break;
                case EMPTY: c = cWhite; break;
            }
            SDL_Rect bounds = {xOffset + j * BLOCK_SIZE, i * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 0xff);
            SDL_RenderFillRect(renderer, &bounds);
            SDL_SetRenderDrawColor(renderer, cBlack.r, cBlack.g, cBlack.b, 0xff);
            SDL_RenderDrawRect(renderer, &bounds);
        }
    }
}

SDL_FPoint rayIntersection(double posX, double posY, double xStep, double yStep) {
    double xIntersection = posX;
    double yIntersection = posY;

    while (board[(int)(yIntersection / BLOCK_SIZE)][(int)(xIntersection / BLOCK_SIZE)] == EMPTY) {
        xIntersection += xStep;
        yIntersection += yStep;
    }

    return (SDL_FPoint) {xIntersection, yIntersection};
}

void draw(SDL_Renderer* renderer, player* p) {
    drawMap(renderer);

    // draw player
    SDL_SetRenderDrawColor(renderer, cBlack.r, cBlack.g, cBlack.b, 0xff);
    // player
    SDL_RenderFillRect(renderer, &(SDL_Rect) {p->pos.x, p->pos.y, p->w, p->h});

    double rayAngle = playerAngle(p) - p->fov / 2;

    for (int i = 0; i < p->fov; i++) {
        SDL_FPoint intersection = rayIntersection(p->pos.x, p->pos.y, cos(degToRad(rayAngle)), sin(degToRad(rayAngle)));
        SDL_Color rayColor;
        switch (board[(int) (intersection.y / BLOCK_SIZE)][(int) (intersection.x / BLOCK_SIZE)]) {
            case EMPTY: rayColor = cBlack; break;
            case SOLID_BLUE: rayColor = cBlue; break;
            case SOLID_GREEN: rayColor = cGreen; break;
            case SOLID_RED: rayColor = cRed; break;
        }
        SDL_SetRenderDrawColor(renderer, rayColor.r, rayColor.g, rayColor.b, 0xff);

        double distance = sqrt(pow(intersection.x - p->pos.x, 2) + pow(intersection.y - p->pos.y, 2));
        // map draw ray
        currentViewPort = &leftViewPort;
        SDL_RenderDrawLine(
            renderer,
            p->pos.x + p->w / 2,
            p->pos.y + p->h / 2,
            intersection.x,
            intersection.y
        );

        // 3d draw rectangle in relation to ray distance as rect height
        currentViewPort = &rightViewPort;
        int lineHeight = (BLOCK_SIZE * currentViewPort->h) / distance;
        if (lineHeight > currentViewPort->h)
            lineHeight = currentViewPort->h;
        SDL_RenderDrawRect(
            renderer,
            &(SDL_Rect) {
                currentViewPort->x + i * (currentViewPort->w / p->fov),
                currentViewPort->h / 2 - lineHeight / 2,
                currentViewPort->w / p->fov,
                lineHeight
            }
        );
        rayAngle += 1;
    }
}

void initBoard() {
    // top board
    for (int i = 0; i < BOARD_WIDTH; i++) {
        board[0][i] = SOLID_GREEN;
    }
    // left
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        board[i][0] = SOLID_GREEN;
    }
    // bottom
    for (int i = 0; i < BOARD_WIDTH; i++) {
        board[BOARD_HEIGHT - 1][i] = SOLID_GREEN;
    }
    // right
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        board[i][BOARD_WIDTH - 1] = SOLID_GREEN;
    }

    // obstacles
    for (int i = 2; i < BOARD_HEIGHT / 2; i++) {
        board[i][BOARD_WIDTH / 2] = SOLID_BLUE;
    }
    for (int i = 2; i < BOARD_HEIGHT / 2; i++) {
        board[i][BOARD_WIDTH / 2 + 3] = SOLID_BLUE;
    }

    for (int i = 1; i < BOARD_WIDTH / 3; i++) {
        board[BOARD_HEIGHT - 4][i] = SOLID_RED;
    }
    for (int i = BOARD_WIDTH / 3; i < BOARD_WIDTH - 1; i++) {
        board[BOARD_HEIGHT - 6][i] = SOLID_RED;
    }

    for (int i = 1; i < BOARD_WIDTH / 3; i++) {
        board[5][i] = SOLID_RED;
    }
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("error: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow(
        "raycaster", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );
    if (window == NULL) {
        SDL_Log("error: %s", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_Log("error: %s", SDL_GetError());
        return 1;
    }
    initBoard();
    player* p = &(player) {};
    initPlayer(p);

    Uint32 lastTicks = SDL_GetTicks();
    bool quit = false; SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                }
            }
        }
        Uint32 nowTicks = SDL_GetTicks();
        float dt = (nowTicks - lastTicks) / 1000.0f;
        lastTicks = nowTicks;

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderClear(renderer);

        updateKeys(dt, p);
        draw(renderer, p);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}