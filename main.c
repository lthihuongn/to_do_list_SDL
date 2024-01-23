#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 640
#define HEIGHT 480
#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 40
#define TEXTBOX_WIDTH 200
#define TEXTBOX_HEIGHT 30

#define MAX_LINES 5

typedef struct {
    char text[256];
    SDL_Rect rect;
} TextLine;

void renderText(SDL_Renderer *rend, TTF_Font *font, const char *text, SDL_Rect rect, SDL_Color textColor, SDL_Color backgroundColor) {
    SDL_SetRenderDrawColor(rend, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(rend, &rect);

    SDL_SetRenderDrawColor(rend, textColor.r, textColor.g, textColor.b, textColor.a);
    SDL_RenderDrawRect(rend, &rect);

    SDL_Surface *textSurface = TTF_RenderText_Blended_Wrapped(font, text, textColor, rect.w - 20);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
    SDL_FreeSurface(textSurface);

    SDL_Rect renderQuad = {rect.x + 10, rect.y + 5, rect.w - 20, rect.h - 10};
    SDL_RenderCopy(rend, textTexture, NULL, &renderQuad);

    SDL_DestroyTexture(textTexture);
}

void ajouterZoneDeTexte(SDL_Renderer *rend, TTF_Font *font, TextLine *lines, int *numLines, char *inputText, bool *isEditing) {
    SDL_Color color = {0, 0, 0};
    SDL_Color backgroundColor = {255, 255, 255};

    // Rendre la ligne en cours d'édition
    if (*isEditing) {
        SDL_Rect inputRect = {10, 40 + *numLines * (TEXTBOX_HEIGHT + 5), TEXTBOX_WIDTH, TEXTBOX_HEIGHT};
        renderText(rend, font, inputText, inputRect, color, backgroundColor);
    }

    // Rendre les lignes existantes
    for (int i = 0; i < *numLines; ++i) {
        SDL_Rect renderRect = {10, 40 + i * (TEXTBOX_HEIGHT + 5), TEXTBOX_WIDTH, TEXTBOX_HEIGHT};
        renderText(rend, font, lines[i].text, renderRect, color, backgroundColor);
    }
}

void ajouterLigneDeTexte(TextLine *lines, int *numLines, const char *text) {
    if (*numLines < MAX_LINES) {
        strcpy(lines[*numLines].text, text);
        (*numLines)++;
    }
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 0;
    }

    SDL_Window *wind = SDL_CreateWindow("To do list",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        WIDTH, HEIGHT, 0);
    if (!wind) {
        printf("Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer *rend = SDL_CreateRenderer(wind, -1, render_flags);
    if (!rend) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 0;
    }

    if (TTF_Init() != 0) {
        printf("Error initializing SDL_ttf: %s\n", TTF_GetError());
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 0;
    }

    TTF_Font *font = TTF_OpenFont("C:\\SDL\\todolist\\Roboto-Regular.ttf", 30);
    if (!font) {
        printf("Error loading font: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 0;
    }

    TextLine lines[MAX_LINES];
    int numLines = 0;

    char inputText[256] = "";
    bool isEditing = false;

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (mouseX >= WIDTH - BUTTON_WIDTH && mouseY >= HEIGHT - BUTTON_HEIGHT) {
                    if (isEditing) {
                        ajouterLigneDeTexte(lines, &numLines, inputText);
                        isEditing = false;
                        inputText[0] = '\0';
                    } else {
                        isEditing = true;
                    }
                }
            } else if (event.type == SDL_KEYDOWN && isEditing) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    ajouterLigneDeTexte(lines, &numLines, inputText);
                    isEditing = false;
                    inputText[0] = '\0';
                } else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    inputText[strlen(inputText) - 1] = '\0';
                }
            } else if (event.type == SDL_TEXTINPUT && isEditing) {
                if (strlen(inputText) + strlen(event.text.text) < sizeof(inputText) - 1) {
                    strcat(inputText, event.text.text);
                }
            }
        }

        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        SDL_RenderClear(rend);

        int columnWidth = WIDTH / 3;

        SDL_Rect columnRects[3] = {
                {0, 0, columnWidth, HEIGHT},
                {columnWidth, 0, columnWidth, HEIGHT},
                {2 * columnWidth, 0, columnWidth, HEIGHT}};

        SDL_SetRenderDrawColor(rend, 206, 206, 206, 206);
        SDL_RenderFillRect(rend, &columnRects[0]);

        SDL_Color textColor = {255, 255, 255, 0};

        SDL_Surface *textSurfaceToDo = TTF_RenderText_Solid(font, "To Do", textColor);
        SDL_Texture *textToDo = SDL_CreateTextureFromSurface(rend, textSurfaceToDo);
        SDL_FreeSurface(textSurfaceToDo);

        SDL_Rect renderQuadToDo = {0, 0, textSurfaceToDo->w, textSurfaceToDo->h};
        SDL_RenderCopy(rend, textToDo, NULL, &renderQuadToDo);
        SDL_DestroyTexture(textToDo);

        SDL_SetRenderDrawColor(rend, 128, 216, 238, 255);
        SDL_RenderFillRect(rend, &columnRects[1]);

        SDL_Surface *textSurfaceInProgress = TTF_RenderText_Solid(font, "In Progress", textColor);
        SDL_Texture *textInProgress = SDL_CreateTextureFromSurface(rend, textSurfaceInProgress);
        SDL_FreeSurface(textSurfaceInProgress);

        SDL_Rect renderQuadInProgress = {columnWidth, 0, textSurfaceInProgress->w, textSurfaceInProgress->h};
        SDL_RenderCopy(rend, textInProgress, NULL, &renderQuadInProgress);
        SDL_DestroyTexture(textInProgress);

        SDL_SetRenderDrawColor(rend, 168, 255, 185, 255);
        SDL_RenderFillRect(rend, &columnRects[2]);

        SDL_Surface *textSurfaceDone = TTF_RenderText_Solid(font, "Done", textColor);
        SDL_Texture *textDone = SDL_CreateTextureFromSurface(rend, textSurfaceDone);
        SDL_FreeSurface(textSurfaceDone);

        SDL_Rect renderQuadDone = {2 * columnWidth, 0, textSurfaceDone->w, textSurfaceDone->h};
        SDL_RenderCopy(rend, textDone, NULL, &renderQuadDone);
        SDL_DestroyTexture(textDone);

        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_Rect buttonRect = {WIDTH - BUTTON_WIDTH, HEIGHT - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT};
        SDL_RenderFillRect(rend, &buttonRect);

        SDL_Surface *textSurfaceButton = TTF_RenderText_Solid(font, "Add", textColor);
        SDL_Texture *textButton = SDL_CreateTextureFromSurface(rend, textSurfaceButton);
        SDL_FreeSurface(textSurfaceButton);

        SDL_Rect renderQuadButton = {WIDTH - BUTTON_WIDTH, HEIGHT - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT};
        SDL_RenderCopy(rend, textButton, NULL, &renderQuadButton);
        SDL_DestroyTexture(textButton);

        ajouterZoneDeTexte(rend, font, lines, &numLines, inputText, &isEditing);

        SDL_RenderPresent(rend);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wind);
    SDL_Quit();
    return 0;
}
