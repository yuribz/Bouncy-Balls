#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>


#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 1200
#define FPS 24
#define BALLS_PER_SUBSPACE 4
#define BALL_CORNER_COUNT 4
#define FRAME_DELAY (1000 / FPS)
#define pyth(a, b) (sqrt(pow(a, 2) + pow(b, 2)))

int subspace_size_x;
int subspace_size_y;
int subspace_count;

/*
    A simple vector for storing two dimensional data.
    Uses integers to represent whole numbers.
*/
typedef struct vec2i {
    int   x;
    int   y;
} vec2i;

/*
    A simple vector for storing two dimensional data. 
    Uses doubles to represent floating point numbers.
*/
typedef struct vec2 {
    double  x;
    double  y;
} vec2;

/*
    A struct to store data for a single ball.
    Contains fields for integer radius, a integer 2D vector for position, and
    an integer 2D vector for the balls velocity (i.e. direction).

    The ball struct also contains the information about the subspaces where
    the ball is located in. This is used for collision optimization.
    The subspaces field is an int array of size 4.
    The order of the corners is: top-left, top-right, bottom-left, bottom-right.
*/
typedef struct Ball {
    int     radius;
    vec2    pos;
    vec2    dir;
    int     subspaces[BALL_CORNER_COUNT];
} Ball;

/*
    Simple Linked List like struct for storing references to balls
    within each subspace. Used for collision optimization.
*/
typedef struct LinkedContainer {
    int                 subspace;
    Ball*               ball;
    LinkedContainer*    next;
} LinkedContainer;

SDL_Window *win;
SDL_Renderer *ren;


/*
    Sets up the SDL window and renderer.
*/
int setup() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    win = SDL_CreateWindow(
        "Bouncy Balls",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);

    if (win == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (ren == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    return 0;
}

void calculateSubspaces(Ball* ball) {
    #define SW SCREEN_WIDTH
    #define SH SCREEN_HEIGHT

    double left = ball->pos.x - ball->radius;
    double right = ball->pos.x + ball->radius;
    double up = ball->pos.y - ball->radius;
    double down = ball->pos.y + ball->radius;

    // Subspaces per row.
    int spr = SW / subspace_size_x;

    // Subspaces per column.
    int spc = SH / subspace_size_y;

    ball->subspaces[0] = (left / SW) + (up / SH) * spr;
    ball->subspaces[1] = (right / SW) + (up / SH) * spr;
    ball->subspaces[2] = (left / SW) + (down / SH) * spr;
    ball->subspaces[3] = (right / SW) + (down / SH) * spr;

    #undef SW
    #undef SH
}

/*
    Creates a ball of specified radius, located at specified x and y coordinates.
    Returns the pointer to the created ball.
*/
Ball* makeBall(int x, int y, int r) {
    Ball* ball = (Ball*) malloc(sizeof(Ball));

    ball->radius = r;
    
    ball->pos.x = x;
    ball->pos.y = y;

    ball->dir.x = 0.0;
    ball->dir.y = 0.0;

    calculateSubspaces(ball);

    return ball;
}

/*
    Draws a single ball using the midpoint algorithm.
*/
void drawBall(Ball* ball) {
    int x = ball->radius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (ball->radius << 1);

    int x0 = ball->pos.x;
    int y0 = ball->pos.y;

    while (x >= y)
    {
        SDL_RenderDrawPoint(ren, x0 + x, y0 + y);
        SDL_RenderDrawPoint(ren, x0 + y, y0 + x);
        SDL_RenderDrawPoint(ren, x0 - y, y0 + x);
        SDL_RenderDrawPoint(ren, x0 - x, y0 + y);
        SDL_RenderDrawPoint(ren, x0 - x, y0 - y);
        SDL_RenderDrawPoint(ren, x0 - y, y0 - x);
        SDL_RenderDrawPoint(ren, x0 + y, y0 - x);
        SDL_RenderDrawPoint(ren, x0 + x, y0 - y);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        
        if (err > 0)
        {
            x--;
            dx += 2;
            err += dx - (ball->radius << 1);
        }
    }
}

/*
    Makes the ball change position according to its direction and position.
*/
void moveBall(Ball *ball) {
    ball->pos.x += ball->dir.x;
    ball->pos.y += ball->dir.y;
}

/*
    Checks if two balls are overlapping with each other.
*/
bool overlaps(Ball *a, Ball *b) {
    double c = pyth(a->pos.x - b->pos.x, a->pos.y - b->pos.y);

    return c < a->radius + b->radius;
}

/*
    Normalizes the supplied vec2.
*/
void norm(vec2* v){
    double magnitute = sqrt(pow(v->x, 2) + pow(v->y, 2));
    v->x /= magnitute;
    v->y /= magnitute;
}

/*
    Calculates the dot product of the supplied vec2.
*/
double dot(vec2* v1, vec2* v2){
    return v1->x * v2->x + v1->y * v2->y;
}

/*
    Calculate the final velocities after collision for both balls.
    Assuming both balls are the same mass (which they should be).
*/
void bounce(Ball *a, Ball *b) {
    // A vector that records the distance between the centers of the ball
    // along both axes.
    vec2 n = {
        .x = b->pos.x - a->pos.x,
        .y = b->pos.y - a->pos.y
    };

    // Normalized.
    norm(&n);

    // Projection of the balls' velocities onto the vector n.
    double scalar_product = dot(&(a->dir), &n) - dot(&(b->dir), &n);

    // Update velocities.
    a->dir.x = a->dir.x - scalar_product * n.x;
    a->dir.y = a->dir.y - scalar_product * n.y;

    b->dir.x = b->dir.x + scalar_product * n.x;
    b->dir.y = b->dir.y + scalar_product * n.y;
}

/*
    Check if the ball is bouncing off the wall. Reverse its corresponding
    velocity component if it is.
*/
void bounceWall(Ball *a) {
    double left  = a->pos.x - a->radius;
    double right = a->pos.x + a->radius;

    double up   = a->pos.y - a->radius;
    double down = a->pos.y + a->radius;

    // Horizontal bounce
    if (left < 0 || right > SCREEN_WIDTH) { 
        a->dir.x *= -1; 
        if (a->dir.x > 0) {
            a->pos.x = a->radius + 1;
        }
        else {
            a->pos.x = SCREEN_WIDTH - a->radius - 1;
        }
    }
    // Vertical bounce
    if (up < 0 || down > SCREEN_HEIGHT) { 
        a->dir.y *= -1;
        if (a->dir.y > 0) {
            a->pos.y = a->radius + 1;
        }
        else {
            a->pos.y = SCREEN_HEIGHT - a->radius - 1;
        }
    }
}

/*
    Renders all the balls at once, while also checking for
    collision between balls and the walls.
*/
void renderBalls(int amnt, Ball* balls[]) {
    /*
        TODO: Optimize collision.
    */
    for (int i = 0; i < amnt; i++) {
        for (int j = 0; j < amnt; j++) {
            if (i == j) continue;
            else {
                if (overlaps(balls[i], balls[j])) {
                    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
                    bounce(balls[i], balls[j]);
                    break;
                }
                else {
                    SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);
                }
            }
        }

        drawBall(balls[i]);
        for (int k = 0; k < 4; k++) {
            printf("%d\t", balls[i]->subspaces[k]);
        }
        printf("\n");
        printf("There are %d subspaces!\n", subspace_count);
        moveBall(balls[i]);
        bounceWall(balls[i]);
    }
}

/*
    Main function.
    The intented usage is to provide two numerical arguments:
    - Argument 1 is the number of balls to render on the screen.
    - Argument 2 is the size of every ball (as a radius).
*/
int main(int argc, char* argv[]) {
    bool running;

    // Amount of balls to give in the simulation
    int ball_amnt;
    int radius;

    

    

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number> <radius>\n", argv[0]);
        return 1;
    }
    else {
        ball_amnt = atoi(argv[1]);
        radius = atoi(argv[2]);
    }

    // Calculates the subspace size based on the assumption that each subspace
    // will hold no more than a certian amount of balls.


    subspace_size_x = (radius * 2 * BALLS_PER_SUBSPACE);
    subspace_size_y = (radius * 2 * BALLS_PER_SUBSPACE);
    while (SCREEN_WIDTH % subspace_size_x) {
        subspace_size_x++;
    }
    printf("Each subspace is %d pixels wide\n", subspace_size_x);
    while (SCREEN_HEIGHT % subspace_size_y) {
        subspace_size_y++;
    }
    printf("Each subspace is %d pixels tall\n", subspace_size_y);

    subspace_count = (SCREEN_WIDTH / subspace_size_x) * (SCREEN_HEIGHT / subspace_size_y);

    Ball* balls[ball_amnt];

    if (setup() != 0) {
        SDL_Quit();
        running = false;
        srand(time(NULL));
    }
    else {
        running = true;
    }

    for (int i = 0; i < ball_amnt; i++) {
        balls[i] = makeBall(
            rand() % (SCREEN_WIDTH + 1), 
            rand() % (SCREEN_HEIGHT + 1), 
            radius
            );
        
        balls[i]->dir.x = (rand() % 10) - 5;
        balls[i]->dir.y = (rand() % 10) - 5;
    }

    while (running) {
        SDL_Event e;

        int delta = SDL_GetTicks();

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        renderBalls(ball_amnt, balls);

        while(SDL_PollEvent(&e)) {
            switch (e.type) {
                // quitting the game
                case SDL_QUIT :
                    running = false;
                    break;
                
                // key pressed
                case SDL_KEYDOWN :
                    switch(e.key.keysym.sym) {
                        case SDLK_ESCAPE :
                            SDL_Event quit_event;
                            quit_event.type = SDL_QUIT;
                            SDL_PushEvent(&quit_event);
                            break;
                    }
            }
        }

        SDL_RenderPresent(ren);

        delta = SDL_GetTicks() - delta;
        if (FRAME_DELAY > delta) {
            SDL_Delay(FRAME_DELAY - delta);
        }
    }

    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}