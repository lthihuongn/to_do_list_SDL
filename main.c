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
void renderText(SDL_Renderer *rend, TTF_Font *font, const char *text, SDL_Rect rect, SDL_Color textColor, SDL_Color backgroundColor, bool isEditing, bool isDragging) {
    // Dessiner le rectangle de fond
    SDL_SetRenderDrawColor(rend, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(rend, &rect);

    SDL_SetRenderDrawColor(rend, textColor.r, textColor.g, textColor.b, textColor.a);
    SDL_RenderDrawRect(rend, &rect);

    // Copier la chaîne de texte dans une nouvelle mémoire tampon pour éviter les problèmes avec strtok
    char textBuffer[MAX_TEXT_LENGTH];
    strncpy(textBuffer, text, MAX_TEXT_LENGTH);
    textBuffer[MAX_TEXT_LENGTH - 1] = '\0';

    char *token = strtok(textBuffer, "\n");
    int totalHeight = 0;

    while (token != NULL) {
        SDL_Surface *textSurface = TTF_RenderText_Blended_Wrapped(font, token, textColor, rect.w - 20);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(rend, textSurface);
        SDL_FreeSurface(textSurface);

        int text_width, text_height;
        SDL_QueryTexture(textTexture, NULL, NULL, &text_width, &text_height);

        SDL_Rect renderQuad = {rect.x + 10, rect.y + 5 + totalHeight, text_width, text_height};
        SDL_RenderCopy(rend, textTexture, NULL, &renderQuad);

        SDL_DestroyTexture(textTexture);

        // Passer au jeton suivant
        token = strtok(NULL, "\n");
        if (token != NULL) {
            // Ajouter la hauteur de la ligne à la hauteur totale
            totalHeight += text_height;
        }
    }
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

    bool somethingChanged = false;

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
                        // Utilisez une valeur plus grande pour la hauteur initiale (par exemple, 40)
                        textLines[numLines].rect = (SDL_Rect){10, 40 + numLines * (40 + 5), TEXTBOX_WIDTH, 40};
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

                            // Stocker la position y initiale
                            int textWidth, textHeight;
                            TTF_SizeText(font, textLines[i].text, &textWidth, &textHeight);
                            textLines[i].rect.y = textLines[i].rect.y + (textLines[i].rect.h - textHeight) / 2;
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
                        // Vérifier si la zone de texte est en cours d'édition pour la première fois
                        if (textLines[i].inputText[0] == '\0') {
                            // Si oui, centrer le texte verticalement dans la zone de texte
                            int textWidth, textHeight;
                            TTF_SizeText(font, textLines[i].inputText, &textWidth, &textHeight);

                            textLines[i].rect.y = textLines[i].rect.y + (textLines[i].rect.h - textHeight) / 2;
                        }

                        // Concaténer le texte de saisie
                        strcat(textLines[i].inputText, event.text.text);

                        // Ajuster la position verticale pour centrer le texte
                        int textWidth, textHeight;
                        TTF_SizeText(font, textLines[i].inputText, &textWidth, &textHeight);

                        // Vérifier si le texte dépasse la largeur de la zone de texte
                        if (textWidth > textLines[i].rect.w - 20) {
                            // Si le texte dépasse, empêcher le texte de dépasser la largeur de la zone de texte
                            textLines[i].inputText[strlen(textLines[i].inputText) - 1] = '\0';
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
                            strncpy(textLines[i].text, textLines[i].inputText, 12);  // Limiter le texte à 12 caractères //mais permet à la mémoire de marcher ?
                            textLines[i].text[12] = '\0';
                            textLines[i].inputText[0] = '\0';

                            // Ajuster la hauteur de la zone de texte en fonction du texte entré
                            int textWidth, textHeight;
                            TTF_SizeText(font, textLines[i].text, &textWidth, &textHeight);
                            textLines[i].rect.h = textHeight + 10;

                            // Centrer le texte verticalement
                            textLines[i].rect.y = textLines[i].rect.y + (textLines[i].rect.h - MIN_TEXTBOX_HEIGHT) / 2;
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

            SDL_Rect renderQuad = {columns[i].rect.x + WIDTH / 6 - textSurface->w / 2, 0, textSurface->w, textSurface->h};
            SDL_RenderCopy(rend, textTexture, NULL, &renderQuad);
            SDL_DestroyTexture(textTexture);
        }

        // Dessiner le bouton "Add"
        // Dimensions du bouton "Add"
        int buttonWidth = BUTTON_WIDTH;
        int buttonHeight = BUTTON_HEIGHT;

        // Position du bouton en bas à droite
        int buttonX = WIDTH - buttonWidth;
        int buttonY = HEIGHT - buttonHeight;

        // Dessine le bouton "Add"
        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
        SDL_Rect rect = {buttonX, buttonY, buttonWidth, buttonHeight};
        SDL_RenderFillRect(rend, &rect);

        // Dessine le texte au centre du bouton "Add"
        SDL_Color textColor = {255, 255, 255, 255}; // Couleur du texte (blanc)
        SDL_Surface *textSurfaceButton = TTF_RenderText_Solid(font, "Add", textColor);
        SDL_Texture *textButton = SDL_CreateTextureFromSurface(rend, textSurfaceButton);

        int textWidth, textHeight;
        SDL_QueryTexture(textButton, NULL, NULL, &textWidth, &textHeight);

        SDL_Rect textRect = {buttonX + (buttonWidth - textWidth) / 2, buttonY + (buttonHeight - textHeight) / 2, textWidth, textHeight};
        SDL_RenderCopy(rend, textButton, NULL, &textRect);

        // Libère la surface et la texture du texte
        SDL_FreeSurface(textSurfaceButton);
        SDL_DestroyTexture(textButton);

        // Dessiner le texte "Right-click to delete" à côté du bouton "Add"
        SDL_Color deleteTextColor = {255, 255, 255, 255};
        SDL_Surface *textSurfaceDelete = TTF_RenderText_Solid(font, "Right click to delete - Enter to save the task", deleteTextColor);
        SDL_Texture *textDelete = SDL_CreateTextureFromSurface(rend, textSurfaceDelete);
        SDL_FreeSurface(textSurfaceDelete);

        SDL_Rect renderQuadDelete = {WIDTH - BUTTON_WIDTH - textSurfaceDelete->w - 10, HEIGHT - BUTTON_HEIGHT, textSurfaceDelete->w, textSurfaceDelete->h};
        SDL_RenderCopy(rend, textDelete, NULL, &renderQuadDelete);
        SDL_DestroyTexture(textDelete);

        // Dessiner les zones de texte
        for (int i = 0; i < numLines; ++i) {
            SDL_Color color = {0, 0, 0}; // Couleur du texte (noir)
            SDL_Color backgroundColor = {255, 255, 255};
            renderText(rend, font, textLines[i].text, textLines[i].rect, color, backgroundColor, textLines[i].isEditing, textLines[i].isDragging);



            if (textLines[i].isEditing) {
                SDL_Rect inputRect = {textLines[i].rect.x, textLines[i].rect.y, textLines[i].rect.w, textLines[i].rect.h};
                renderText(rend, font, textLines[i].inputText, inputRect, color, backgroundColor, textLines[i].isEditing, textLines[i].isDragging);

            }
        }

// Mettre à jour l'affichage
        SDL_RenderPresent(rend);

        // Ajouter un délai pour réduire l'utilisation du processeur
        SDL_Delay(16); // Pause de 16 millisecondes


        // Si quelque chose a changé, mettez à jour l'affichage
        if (somethingChanged) {
            SDL_RenderPresent(rend);
            somethingChanged = false;  // Réinitialisez l'indicateur
        }

    }
    // Sauvegarder les tâches avant de quitter
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
