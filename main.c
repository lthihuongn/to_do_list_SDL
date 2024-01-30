#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 800
#define HEIGHT 600
#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 40
#define TEXTBOX_WIDTH 200
#define MIN_TEXTBOX_HEIGHT 30
#define MAX_TEXT_LENGTH 256

// Structure pour représenter une zone de texte
typedef struct {
    char text[MAX_TEXT_LENGTH];
    SDL_Rect rect;
    bool isEditing;
    bool isDragging;
    char inputText[MAX_TEXT_LENGTH];
    int column; // Nouveau champ pour stocker l'index de la colonne
} TextLine;

// Structure pour représenter une colonne
typedef struct {
    SDL_Rect rect;
    char title[MAX_TEXT_LENGTH];
    TextLine lines[100]; // Tableau de lignes pour les zones de texte
    int numLines;
    SDL_Color color; // Nouveau champ pour stocker la couleur de la colonne
} Column;

// Fonction pour vérifier si un point est à l'intérieur d'un rectangle
bool isPointInRect(const SDL_Point *point, const SDL_Rect *rect) {
    return point->x >= rect->x && point->x < rect->x + rect->w &&
           point->y >= rect->y && point->y < rect->y + rect->h;
}

// Fonction pour sauvegarder les données dans un fichier
void saveTasksToFile(TextLine *textLines, int numLines) {
    FILE *file = fopen("tasks.txt", "w");
    if (file != NULL) {
        for (int i = 0; i < numLines; ++i) {
            fprintf(file, "%s\n", textLines[i].text);
        }
        fclose(file);
        printf("Tasks saved to file.\n");
    } else {
        printf("Error opening tasks.txt for writing.\n");
    }
}


// Fonction pour charger les données depuis un fichier
void loadTasksFromFile(TextLine *textLines, int *numLines) {
    FILE *file = fopen("tasks.txt", "r");
    if (file != NULL) {
        *numLines = 0;
        while (*numLines < 100 && fgets(textLines[*numLines].text, sizeof(textLines[*numLines].text), file) != NULL) {
            // Supprimer le saut de ligne du texte lu depuis le fichier
            textLines[*numLines].text[strcspn(textLines[*numLines].text, "\n")] = '\0';
            ++(*numLines);
        }
        fclose(file);
        printf("Tasks loaded from file.\n");

        // Mettre à jour les zones de texte après le chargement des tâches
        for (int i = 0; i < *numLines; ++i) {
            textLines[i].rect = (SDL_Rect){10, 40 + i * (MIN_TEXTBOX_HEIGHT + 5), TEXTBOX_WIDTH, MIN_TEXTBOX_HEIGHT};
            textLines[i].isEditing = false;
            textLines[i].isDragging = false;
            textLines[i].inputText[0] = '\0';
        }
    } else {
        printf("Error opening tasks.txt for reading.\n");
    }
}

// Fonction pour afficher un texte avec fond coloré
void renderText(SDL_Renderer *rend, TTF_Font *font, const char *text, SDL_Rect rect, SDL_Color textColor, SDL_Color backgroundColor) {
    SDL_SetRenderDrawColor(rend, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(rend, &rect);

    SDL_SetRenderDrawColor(rend, textColor.r, textColor.g, textColor.b, textColor.a);
    SDL_RenderDrawRect(rend, &rect);

    // Charger le texte dans une surface
    SDL_Surface *textSurface = TTF_RenderText_Blended_Wrapped(font, text, textColor, rect.w - 20);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
    SDL_FreeSurface(textSurface);

    // Centrer le texte dans la zone de texte
    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect renderQuad = {rect.x + 10, rect.y + (rect.h - textHeight) / 2, rect.w - 20, textHeight};
    SDL_RenderCopy(rend, textTexture, NULL, &renderQuad);

    SDL_DestroyTexture(textTexture);
}


// Fonction pour obtenir la hauteur d'une ligne de texte
int getTextLineHeight(TTF_Font *font) {
    // Utiliser la police pour obtenir la hauteur de ligne
    return TTF_FontHeight(font);
}



int main(int argc, char *argv[]) {

    SDL_Window *wind;
    SDL_Renderer *rend;
    TTF_Font *font;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    wind = SDL_CreateWindow("To Do List", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    if (!wind) {
        printf("Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    rend = SDL_CreateRenderer(wind, -1, render_flags);
    if (!rend) {
        printf("Error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 1;
    }

    if (TTF_Init() != 0) {
        printf("Error initializing SDL_ttf: %s\n", TTF_GetError());
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont("C:\\SDL\\todolist\\Roboto-Regular.ttf", 30);
    if (!font) {
        printf("Error loading font: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(wind);
        SDL_Quit();
        return 1;
    }

    // Définir les couleurs pour chaque colonne
    SDL_Color colorToDo = {221, 221, 221, 255}; // Rouge
    SDL_Color colorInProgress = {128, 234, 250, 255}; // Vert
    SDL_Color colorDone = {175, 239, 196, 255}; // Bleu

    // Initialiser les colonnes avec leurs couleurs
    Column columns[3];
    columns[0].rect = (SDL_Rect){0, 0, WIDTH / 3, HEIGHT};
    columns[0].numLines = 0;
    strcpy(columns[0].title, "To Do");
    columns[0].color = colorToDo;

    columns[1].rect = (SDL_Rect){WIDTH / 3, 0, WIDTH / 3, HEIGHT};
    columns[1].numLines = 0;
    strcpy(columns[1].title, "In Progress");
    columns[1].color = colorInProgress;

    columns[2].rect = (SDL_Rect){2 * WIDTH / 3, 0, WIDTH / 3, HEIGHT};
    columns[2].numLines = 0;
    strcpy(columns[2].title, "Done");
    columns[2].color = colorDone;

    TextLine textLines[100]; // Tableau de 100 lignes pour les zones de texte
    int numLines = 0;
    loadTasksFromFile(textLines, &numLines);

    bool running = true;
    SDL_Event event;

    FILE *file = NULL;

    // Chargez les tâches depuis le fichier
    file = fopen("tasks.txt", "r");
    if (file != NULL) {
        numLines = 0;
        while (numLines < 100 && fscanf(file, "%d %[^\n]", &textLines[numLines].column, textLines[numLines].text) == 2) {
            ++numLines;
        }
        fclose(file);
        printf("Tasks loaded from file.\n");

        // Mettez à jour les zones de texte après le chargement des tâches
        for (int i = 0; i < numLines; ++i) {
            textLines[i].rect = (SDL_Rect){10, 40 + i * (MIN_TEXTBOX_HEIGHT + 5), TEXTBOX_WIDTH, MIN_TEXTBOX_HEIGHT};
            textLines[i].isEditing = false;
            textLines[i].isDragging = false;
            textLines[i].inputText[0] = '\0';
        }
    } else {
        printf("Error opening tasks.txt for reading.\n");
    }


    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                // Vérifier si le clic est sur le bouton "Add"
                if (event.button.button == SDL_BUTTON_LEFT && isPointInRect(&(SDL_Point){mouseX, mouseY}, &(SDL_Rect){WIDTH - BUTTON_WIDTH, HEIGHT - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT})) {
                    // Ajouter une nouvelle zone de texte dans la colonne "To Do"
                    if (numLines < 100) {
                        textLines[numLines].rect = (SDL_Rect){10, 40 + numLines * (MIN_TEXTBOX_HEIGHT + 5), TEXTBOX_WIDTH, MIN_TEXTBOX_HEIGHT};
                        textLines[numLines].isEditing = true;
                        textLines[numLines].isDragging = false;
                        textLines[numLines].text[0] = '\0';
                        textLines[numLines].inputText[0] = '\0';
                        textLines[numLines].column = 0; // La nouvelle tâche appartient à la colonne "To Do"
                        numLines++;
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    // Vérifier si le clic est sur une zone de texte existante pour la supprimer
                    for (int i = 0; i < numLines; ++i) {
                        if (isPointInRect(&(SDL_Point){mouseX, mouseY}, &(textLines[i].rect))) {
                            // Supprimer la ligne en décalant les éléments suivants dans le tableau
                            for (int j = i; j < numLines - 1; ++j) {
                                textLines[j] = textLines[j + 1];
                            }
                            numLines--;
                            break;
                        }
                    }
                } else {
                    // Vérifier si le clic est sur une zone de texte existante pour la déplacer
                    for (int i = 0; i < numLines; ++i) {
                        if (isPointInRect(&(SDL_Point){mouseX, mouseY}, &(textLines[i].rect))) {
                            for (int j = 0; j < numLines; ++j) {
                                textLines[j].isEditing = false;
                                textLines[j].isDragging = false;
                            }
                            textLines[i].isEditing = true;
                            textLines[i].isDragging = true;
                        } else {
                            textLines[i].isEditing = false;
                            textLines[i].isDragging = false;
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                // Désactiver le déplacement lorsque le bouton de la souris est relâché
                for (int i = 0; i < numLines; ++i) {
                    textLines[i].isDragging = false;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                // Déplacer la zone de texte en cours d'édition si elle est en cours de déplacement
                for (int i = 0; i < numLines; ++i) {
                    if (textLines[i].isDragging) {
                        textLines[i].rect.x = event.motion.x - textLines[i].rect.w / 2;
                        textLines[i].rect.y = event.motion.y - textLines[i].rect.h / 2;
                    }
                }
                // Gérer la saisie clavier
            } else if (event.type == SDL_TEXTINPUT && numLines > 0) {
                // Gérer la saisie de texte dans la zone de texte en cours d'édition
                for (int i = 0; i < numLines; ++i) {
                    if (textLines[i].isEditing) {
                        if (strlen(textLines[i].inputText) + strlen(event.text.text) < MAX_TEXT_LENGTH - 1) {
                            strcat(textLines[i].inputText, event.text.text);
                        }
                        break;
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                // Gérer le retour chariot pour finaliser la saisie dans la zone de texte en cours d'édition
                if (event.key.keysym.sym == SDLK_RETURN && numLines > 0) {
                    for (int i = 0; i < numLines; ++i) {
                        if (textLines[i].isEditing) {
                            textLines[i].isEditing = false;
                            textLines[i].isDragging = false;
                            strncpy(textLines[i].text, textLines[i].inputText, MAX_TEXT_LENGTH - 1);
                            textLines[i].text[MAX_TEXT_LENGTH - 1] = '\0';
                            textLines[i].inputText[0] = '\0';

                            // Ajuster la hauteur de la zone de texte en fonction du texte entré
                            int textWidth, textHeight;
                            TTF_SizeText(font, textLines[i].text, &textWidth, &textHeight);
                            textLines[i].rect.h = textHeight + 10;
                        }
                    }
                } else if (event.key.keysym.sym == SDLK_BACKSPACE && numLines > 0) {
                    // Gérer la touche de suppression pour effacer le texte
                    for (int i = 0; i < numLines; ++i) {
                        if (textLines[i].isEditing && strlen(textLines[i].inputText) > 0) {
                            textLines[i].inputText[strlen(textLines[i].inputText) - 1] = '\0';
                            break;
                        }
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                // Effacer une zone de texte si le bouton droit de la souris est cliqué
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                for (int i = 0; i < numLines; ++i) {
                    if (isPointInRect(&(SDL_Point){mouseX, mouseY}, &(textLines[i].rect))) {
                        // Supprimer la ligne
                        for (int j = i; j < numLines - 1; ++j) {
                            textLines[j] = textLines[j + 1];
                        }
                        numLines--;
                        break;
                    }
                }
            }
        }

        // Effacer l'écran
        SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
        SDL_RenderClear(rend);

        // Dessiner les colonnes
        for (int i = 0; i < 3; ++i) {
            SDL_SetRenderDrawColor(rend, columns[i].color.r, columns[i].color.g, columns[i].color.b, columns[i].color.a);
            SDL_RenderFillRect(rend, &(columns[i].rect));

            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, columns[i].title, textColor);
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
            SDL_FreeSurface(textSurface);

            SDL_Rect renderQuad = {columns[i].rect.x + (columns[i].rect.w - textSurface->w) / 2, 0, textSurface->w, textSurface->h};
            SDL_RenderCopy(rend, textTexture, NULL, &renderQuad);
            SDL_DestroyTexture(textTexture);
        }

        // Dessiner le bouton "Add"
        SDL_Color textColor = {255, 255, 255, 0};
        SDL_Rect buttonRect = {WIDTH - BUTTON_WIDTH, HEIGHT - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT};
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_RenderFillRect(rend, &buttonRect);

        SDL_Surface *textSurfaceButton = TTF_RenderText_Solid(font, "Add", textColor);
        SDL_Texture *textButton = SDL_CreateTextureFromSurface(rend, textSurfaceButton);
        SDL_FreeSurface(textSurfaceButton);

        SDL_Rect renderQuadButton = {WIDTH - BUTTON_WIDTH, HEIGHT - BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT};
        SDL_RenderCopy(rend, textButton, NULL, &renderQuadButton);
        SDL_DestroyTexture(textButton);

        // Dessiner le texte "Right-click to delete" à côté du bouton "Add"
        SDL_Color deleteTextColor = {255, 255, 255, 255};
        SDL_Surface *textSurfaceDelete = TTF_RenderText_Solid(font, "Right click to delete", deleteTextColor);
        SDL_Texture *textDelete = SDL_CreateTextureFromSurface(rend, textSurfaceDelete);
        SDL_FreeSurface(textSurfaceDelete);

        SDL_Rect renderQuadDelete = {WIDTH - BUTTON_WIDTH - textSurfaceDelete->w - 10, HEIGHT - BUTTON_HEIGHT, textSurfaceDelete->w, textSurfaceDelete->h};
        SDL_RenderCopy(rend, textDelete, NULL, &renderQuadDelete);
        SDL_DestroyTexture(textDelete);

        // Dessiner les zones de texte
        for (int i = 0; i < numLines; ++i) {
            SDL_Color color = {0, 0, 0};
            SDL_Color backgroundColor = {255, 255, 255};
            renderText(rend, font, textLines[i].text, textLines[i].rect, color, backgroundColor);

            if (textLines[i].isEditing) {
                SDL_Rect inputRect = {textLines[i].rect.x, textLines[i].rect.y, textLines[i].rect.w, textLines[i].rect.h};
                renderText(rend, font, textLines[i].inputText, inputRect, color, backgroundColor);
            }
        }

        // Mettre à jour l'affichage
        SDL_RenderPresent(rend);

        // Ajouter un délai pour réduire l'utilisation du processeur
        SDL_Delay(16); // Pause de 16 millisecondes

        // Sauvegarder les tâches avant de quitter



    }
    saveTasksToFile(textLines, numLines);

    // Enregistrez les tâches dans le fichier
    FILE *saveFile = fopen("tasks.txt", "w");
    if (saveFile != NULL) {
        for (int i = 0; i < numLines; ++i) {
            fprintf(saveFile, "%d %s\n", textLines[i].column, textLines[i].text);
        }
        fclose(saveFile);
        printf("Tasks saved to file.\n");
    } else {
        printf("Error opening tasks.txt for writing.\n");
    }


    // Libérer la mémoire et quitter
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wind);
    SDL_Quit();

    return 0;
}
