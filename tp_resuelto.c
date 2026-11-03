// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

// Cuenta caracteres hasta el '\0' (reemplazo de strlen, no se usa string.h).
int strLength(char* s) {
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

char* strDuplicate(char* src) {
    if (src == NULL) return NULL;
    int len = strLength(src);
    // len + 1: el '\0' tambien ocupa un byte.
    char* dup = (char*) malloc(sizeof(char) * (len + 1));
    if (dup == NULL) return NULL;
    // Se copia hasta len inclusive, asi el terminador queda copiado.
    for (int i = 0; i <= len; i++) dup[i] = src[i];
    return dup;
}

int strCompare(char* s1, char* s2) {
    int i = 0;
    // Avanzar mientras coincidan; si s1 termina, s1[i] == s2[i] solo si s2 tambien termina.
    while (s1[i] != '\0' && s1[i] == s2[i]) i++;
    // Se compara como unsigned char para que los caracteres > 127 no den negativos.
    unsigned char c1 = (unsigned char) s1[i];
    unsigned char c2 = (unsigned char) s2[i];
    if (c1 == c2) return 0;
    // Un prefijo es menor: su '\0' (0) es menor que cualquier caracter del otro.
    return (c1 < c2) ? 1 : -1;
}

char* strConcatenate(char* src1, char* src2) {
    int len1 = strLength(src1);
    int len2 = strLength(src2);
    char* res = (char*) malloc(sizeof(char) * (len1 + len2 + 1));
    if (res != NULL) {
        for (int i = 0; i < len1; i++) res[i] = src1[i];
        // Se copia el '\0' de src2 (j == len2) como terminador del resultado.
        for (int j = 0; j <= len2; j++) res[len1 + j] = src2[j];
    }
    // La funcion toma posesion de ambos argumentos: se liberan siempre, incluso si
    // fallo el malloc, para que el llamador nunca tenga que liberarlos.
    free(src1);
    free(src2);
    return res;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

void gameBoardDelete(GameBoard* board);   // la usa gameBoardNew para limpiar si falla malloc

// Reserva un RowNode con todas sus celdas en 0 y next en NULL. Devuelve NULL si falla malloc.
RowNode* rowNodeNew() {
    RowNode* row = (RowNode*) malloc(sizeof(RowNode));
    if (row == NULL) return NULL;
    for (int c = 0; c < GRID_COLS; c++) row->cells[c] = 0;
    row->next = NULL;
    return row;
}

GameBoard* gameBoardNew() {
    GameBoard* board = (GameBoard*) malloc(sizeof(GameBoard));
    if (board == NULL) return NULL;

    // Se inicializa todo ANTES de pedir las filas: si una falla, gameBoardDelete puede
    // recorrer las listas sin leer los punteros q no sirven
    board->first_row = NULL;
    board->queue_head = NULL;
    board->queue_tail = NULL;
    board->first_particle = NULL;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->current_piece.active = 0;
    board->score = 0;
    board->tick_timer = 0;

    // Cada fila nueva se inserta en la cabeza (O(1), sin recorrer la lista).
    // Como todas arrancan vacias, el orden de creacion no importa.
    for (int i = 0; i < GRID_ROWS; i++) {
        RowNode* row = rowNodeNew();
        if (row == NULL) {
            // Libera las filas ya enlazadas y el propio board.
            gameBoardDelete(board);
            return NULL;
        }
        row->next = board->first_row;
        board->first_row = row;
    }
    return board;
}

void gameBoardDelete(GameBoard* board) {
    if (board == NULL) return;

    // En los tres recorridos se guarda next ANTES del free: despues de liberar
    // el nodo ya no se puede leer curr->next.
    RowNode* row = board->first_row;
    while (row != NULL) {
        RowNode* next = row->next;
        free(row);
        row = next;
    }

    PieceQueueNode* piece = board->queue_head;
    while (piece != NULL) {
        PieceQueueNode* next = piece->next;
        free(piece);
        piece = next;
    }

    ParticleNode* part = board->first_particle;
    while (part != NULL) {
        ParticleNode* next = part->next;
        free(part);
        part = next;
    }

    free(board);
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    if (board == NULL) return;
    ParticleNode* part = (ParticleNode*) malloc(sizeof(ParticleNode));
    // Sin memoria se descarta solo este efecto visual; el juego sigue.
    if (part == NULL) return;
    part->x = x;
    part->y = y;
    part->vel_x = ((rand() % 100) / 10.0f) - 5.0f;
    part->vel_y = ((rand() % 50) / -10.0f) - 2.0f;
    part->color = color;
    part->life_timer = 30 + (rand() % 30);
    // Insercion en la cabeza: O(1) y sin limite de cantidad (a diferencia de MAX_PARTICLES).
    part->next = board->first_particle;
    board->first_particle = part;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    if (board == NULL) return 0;
    int lines_cleared = 0;
    RowNode* prev = NULL;
    RowNode* curr = board->first_row;
    int r = 0;   // indice visual de curr, para ubicar las particulas en pantalla

    while (curr != NULL) {
        int is_full = 1;
        for (int c = 0; c < GRID_COLS; c++) {
            if (curr->cells[c] == 0) {
                is_full = 0;
                break;
            }
        }

        RowNode* next = curr->next;
        if (is_full) {
            lines_cleared++;
            for (int c = 0; c < GRID_COLS; c++) {
                gameBoardAddParticle(board,
                                     OFFSET_X + c * CELL_SIZE + CELL_SIZE/2,
                                     OFFSET_Y + r * CELL_SIZE + CELL_SIZE/2,
                                     curr->cells[c]);
            }

            // Desenlazar curr: el anterior (o la cabeza) pasa a apuntar a next.
            // Las filas de arriba "bajan" solas: no se copia ninguna celda.
            if (prev == NULL) board->first_row = next;
            else prev->next = next;

            // Se pide la fila nueva antes de liberar curr. Si malloc falla, se recicla
            // curr limpiandolo, asi el tablero nunca queda con menos de 20 filas.
            RowNode* fresh = rowNodeNew();
            if (fresh == NULL) {
                fresh = curr;
                for (int c = 0; c < GRID_COLS; c++) fresh->cells[c] = 0;
            } else {
                free(curr);
            }
            fresh->next = board->first_row;
            board->first_row = fresh;

            // Si la fila borrada era la cabeza, ahora el antecesor de next es la fila
            // nueva (no NULL). Si no, prev sigue siendo el antecesor correcto.
            if (prev == NULL) prev = fresh;
            // r no cambia de sentido: se quito una fila arriba de next y se agrego otra,
            // asi que next sigue en la posicion r + 1.
        } else {
            prev = curr;
        }
        curr = next;
        r++;
    }
    return lines_cleared;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    if (board == NULL) return;
    PieceQueueNode* node = (PieceQueueNode*) malloc(sizeof(PieceQueueNode));
    if (node == NULL) return;
    node->type = type;
    node->next = NULL;
    // Gracias a queue_tail la insercion al final es O(1), sin recorrer la cola.
    if (board->queue_tail == NULL) {
        // Cola vacia: el nodo es a la vez cabeza y cola.
        board->queue_head = node;
    } else {
        board->queue_tail->next = node;
    }
    board->queue_tail = node;
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board == NULL || board->queue_head == NULL) return 0;
    PieceQueueNode* first = board->queue_head;
    if (out_type != NULL) *out_type = first->type;
    board->queue_head = first->next;
    // Si se saco el ultimo nodo, tail apuntaria a memoria liberada: se pone en NULL.
    if (board->queue_head == NULL) board->queue_tail = NULL;
    free(first);
    return 1;
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS (COMPLETAR) =========

int tests_ok = 0;
int tests_fail = 0;

void check(int cond, char* nombre) {
    if (cond) {
        tests_ok++;
    } else {
        tests_fail++;
        printf("  FALLO: %s\n", nombre);
    }
}

// Igualdad de strings sin string.h, apoyada en strCompare.
int strIguales(char* a, char* b) {
    return a != NULL && b != NULL && strCompare(a, b) == 0;
}

int contarFilas(GameBoard* board) {
    int n = 0;
    for (RowNode* row = board->first_row; row != NULL; row = row->next) n++;
    return n;
}

int contarParticulas(GameBoard* board) {
    int n = 0;
    for (ParticleNode* p = board->first_particle; p != NULL; p = p->next) n++;
    return n;
}

int filaVacia(RowNode* row) {
    if (row == NULL) return 0;
    for (int c = 0; c < GRID_COLS; c++) if (row->cells[c] != 0) return 0;
    return 1;
}

int tableroVacio(GameBoard* board) {
    for (RowNode* row = board->first_row; row != NULL; row = row->next) {
        if (!filaVacia(row)) return 0;
    }
    return 1;
}

void llenarFila(GameBoard* board, int index) {
    RowNode* row = getRowAt(board, index);
    for (int c = 0; c < GRID_COLS; c++) row->cells[c] = 1 + (c % 7);
}

void testStrDuplicate() {
    char* s = strDuplicate("");
    check(s != NULL && s[0] == '\0', "strDuplicate string vacio");
    free(s);

    s = strDuplicate("x");
    check(s != NULL && s[0] == 'x' && s[1] == '\0', "strDuplicate un caracter");
    free(s);

    char* original = "Tetris 2026!";
    s = strDuplicate(original);
    check(strIguales(s, original), "strDuplicate distintos caracteres");
    check(s != original, "strDuplicate devuelve memoria nueva");
    free(s);
}

void testStrCompare() {
    check(strCompare("", "") == 0, "strCompare dos vacios");
    check(strCompare("a", "a") == 0, "strCompare un caracter iguales");
    check(strCompare("a", "b") == 1, "strCompare a < b");
    check(strCompare("b", "a") == -1, "strCompare b > a");
    check(strCompare("casa", "casas") == 1, "strCompare prefijo es menor");
    check(strCompare("casas", "casa") == -1, "strCompare prefijo es menor (al reves)");
    check(strCompare("", "a") == 1, "strCompare vacio es menor");
}

void testStrConcatenate() {
    // Los argumentos salen de strDuplicate porque strConcatenate los libera.
    char* s = strConcatenate(strDuplicate(""), strDuplicate("abc"));
    check(strIguales(s, "abc"), "strConcatenate vacio + 3 caracteres");
    free(s);

    s = strConcatenate(strDuplicate("hola "), strDuplicate("mundo"));
    check(strIguales(s, "hola mundo"), "strConcatenate dos de 5 caracteres");
    free(s);
}

void testCola() {
    GameBoard* board = gameBoardNew();
    check(board != NULL, "cola: gameBoardNew");
    if (board == NULL) return;

    for (int i = 0; i < 5; i++) gameBoardEnqueuePiece(board, i);

    int fifo_ok = 1;
    for (int i = 0; i < 5; i++) {
        int type = -1;
        if (!gameBoardDequeuePiece(board, &type) || type != i) fifo_ok = 0;
    }
    check(fifo_ok, "cola: 5 pops en orden FIFO");

    int type = 42;
    check(gameBoardDequeuePiece(board, &type) == 0, "cola: 6to pop devuelve 0");
    check(type == 42, "cola: pop vacio no modifica out_type");
    check(board->queue_head == NULL && board->queue_tail == NULL, "cola: head y tail en NULL");

    gameBoardEnqueuePiece(board, 6);
    check(board->queue_head != NULL && board->queue_head == board->queue_tail,
          "cola: push despues de vaciar deja head == tail");
    check(gameBoardDequeuePiece(board, &type) == 1 && type == 6, "cola: push y pop posterior");
    check(board->queue_head == NULL && board->queue_tail == NULL, "cola: vacia otra vez");

    // Dejar piezas encoladas para que gameBoardDelete tambien libere la cola.
    gameBoardEnqueuePiece(board, 1);
    gameBoardEnqueuePiece(board, 2);
    gameBoardDelete(board);
}

void testTableroNuevo() {
    GameBoard* board = gameBoardNew();
    check(board != NULL, "tablero: gameBoardNew");
    if (board == NULL) return;
    check(contarFilas(board) == GRID_ROWS, "tablero: nace con 20 filas");
    check(tableroVacio(board), "tablero: nace vacio");
    check(board->score == 0 && board->tick_timer == 0 && !board->current_piece.active,
          "tablero: score, tick_timer y pieza en 0");
    check(board->queue_head == NULL && board->first_particle == NULL, "tablero: listas en NULL");
    check(gameBoardCheckAndClearLines(board) == 0, "tablero: vacio no borra lineas");
    gameBoardDelete(board);
    gameBoardDelete(NULL);   // debe tolerar NULL sin romper
}

void testUltimaFila() {
    GameBoard* board = gameBoardNew();
    if (board == NULL) { check(0, "ultima fila: gameBoardNew"); return; }

    llenarFila(board, 19);
    getRowAt(board, 18)->cells[3] = 5;   // bloque suelto que deberia bajar a la fila 19
    RowNode* vieja_cabeza = board->first_row;

    check(gameBoardCheckAndClearLines(board) == 1, "ultima fila: devuelve 1");
    // vieja_cabeza sigue viva, asi que la fila nueva no puede tener su misma direccion.
    check(board->first_row != vieja_cabeza && filaVacia(board->first_row),
          "ultima fila: first_row es la fila nueva vacia");
    check(getRowAt(board, 1) == vieja_cabeza, "ultima fila: la vieja cabeza bajo a la fila 1");
    check(contarFilas(board) == GRID_ROWS, "ultima fila: siguen 20 filas");
    check(getRowAt(board, 19)->cells[3] == 5, "ultima fila: el bloque de arriba bajo una fila");
    check(contarParticulas(board) == GRID_COLS, "ultima fila: una particula por celda");
    gameBoardDelete(board);   // libera tambien las particulas
}

void testDosPrimerasFilas() {
    GameBoard* board = gameBoardNew();
    if (board == NULL) { check(0, "filas 0 y 1: gameBoardNew"); return; }

    // La fila 0 es la cabeza: prueba el caso prev == NULL dos veces seguidas.
    llenarFila(board, 0);
    llenarFila(board, 1);
    check(gameBoardCheckAndClearLines(board) == 2, "filas 0 y 1: devuelve 2");
    check(contarFilas(board) == GRID_ROWS, "filas 0 y 1: siguen 20 filas");
    check(tableroVacio(board), "filas 0 y 1: el tablero queda vacio");
    check(contarParticulas(board) == 2 * GRID_COLS, "filas 0 y 1: 20 particulas");
    gameBoardDelete(board);
}

void testFilasNoConsecutivas() {
    GameBoard* board = gameBoardNew();
    if (board == NULL) { check(0, "filas 17 y 19: gameBoardNew"); return; }

    llenarFila(board, 17);
    llenarFila(board, 19);
    getRowAt(board, 18)->cells[0] = 2;
    getRowAt(board, 16)->cells[9] = 7;
    check(gameBoardCheckAndClearLines(board) == 2, "filas 17 y 19: devuelve 2");
    check(contarFilas(board) == GRID_ROWS, "filas 17 y 19: siguen 20 filas");
    check(getRowAt(board, 19)->cells[0] == 2, "filas 17 y 19: fila 18 baja a la 19");
    check(getRowAt(board, 18)->cells[9] == 7, "filas 17 y 19: fila 16 baja a la 18");
    gameBoardDelete(board);
}

void correrTests() {
    printf("Corriendo tests...\n");
    testStrDuplicate();
    testStrCompare();
    testStrConcatenate();
    testCola();
    testTableroNuevo();
    testUltimaFila();
    testDosPrimerasFilas();
    testFilasNoConsecutivas();
    printf("Tests: %d OK, %d fallidos.\n", tests_ok, tests_fail);
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
