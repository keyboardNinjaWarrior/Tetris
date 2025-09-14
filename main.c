#include <io.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <conio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <Windows.h>

#define SCREEN_WIDTH		51
#define	SCREEN_HEIGHT		22
#define GAME_HEIGHT			20
#define	GAME_WIDTH			10
#define TETROMINO_HEIGHT	2
#define TETROMINO_WIDTH		4
#define TIME				1.5

// escape sequences
#define ESC					"\x1b"
#define CUF(x)				"[" #x "C"
#define	ASCII				"(B"
#define DRAW				"(0"
#define BUFFER				"[?1049h"
#define MAIN				"[?1049l"
#define CUR_SHOW			"[?25l"

#define	UPPER_SEPERATION	"\x77"					// ┬
#define BLOCK				"\x61"			// ▒
#define LEFT_SEPERATION		"\x75"					// ┤
#define DASH				"\x71"			// ─
#define UPPER_LEFT_CORNER	"\x6c"					// ┌
#define RIGHT_SEPERATION	"\x74"			// ├
#define UPPER_RIGHT_CORNER	"\x6b"					// ┐
#define PIPE				"\x78"			// │
#define BOTTOM_RIGHT_CORNER	"\x6a"					// └
#define BOTTOM_LEFT_CORNER	"\x6d"			// ┘

#define CYAN				"[38;2;0;255;255m"
#define YELLOW				"[38;2;255;255;0m"
#define PINK				"[38;2;255;192;203m"
#define BLUE				"[38;2;0;0;255m"
#define ORANGE				"[38;2;255;165;0m"
#define GREEN				"[38;2;0;255;0m"
#define RED					"[38;2;255;0;0m"
#define GREY				"[38;2;59;59;59m"
#define DEFAULT				"[0m"

struct cordinates
{
	short int x;
	short int y;
} padding = { 0,0 };
typedef struct cordinates cordinates;

enum type { EMPTY, I, J, L, O, S, T, Z };
enum rotation { ZERO, NINETY, ONE_EIGHTY, TWO_SEVENTTY };

struct Tetromino
{
	bool tetromino[TETROMINO_HEIGHT][TETROMINO_WIDTH];

	enum type type;
	struct cordinates dimensions;
	struct cordinates index;
	enum rotation angle;

	cordinates clockwise_offset[4][5];
	cordinates anticlockwise_offset[4][5];
} current, next, previous, prediction;

// Screen height = 20 + 2 (extra rows)
struct Grid
{
	bool pixel;
	enum type color;
} grid[GAME_HEIGHT + 2][GAME_WIDTH];
typedef struct Grid Grid;

short int complete_rows[4] = { -1,-1,-1,-1 };
int score = 0;

// functions
static void GetAnyInput							(void);
static bool CheckTetromino						(struct Tetromino*);
static void PrintTetromino						(struct Tetromino*);
static void EraseTetromino						(struct Tetromino*);
inline static bool PrintGrid					(void);
inline static void Game							(void);
inline static void GameOver						(void);
inline static void SaveGrid						(void);
inline static void ExitTetris					(void);
inline static void UpdateScore					(void);
inline static void WaitForInput					(void);
inline static void SetGameScreen				(void);
inline static void SetEmptyScreen				(void);
inline static void RotateClockwise				(void);
inline static void PredictTetromino				(void);
inline static void SetInitialScreen				(void);
inline static void EraseNextTetromino			(void);
inline static void RemoveCompleteRows			(void);
inline static void PrintNextTetromino			(void);
inline static void SetNewScreenBuffer			(void);
inline static void GetConsoleDimensions			(void);
inline static void RotateCounterclockwise		(void);
inline static void SetWindowsTitle				(char*);
inline static void PrintScore					(cordinates);
inline static void SetColor						(enum type*);
inline static void SetTetrominoI				(struct Tetromino*);
inline static void SetTetrominoT				(struct Tetromino*);
inline static void SetTetrominoL				(struct Tetromino*);
inline static void SetTetrominoJ				(struct Tetromino*);
inline static void SetTetrominoZ				(struct Tetromino*);
inline static void SetTetrominoS				(struct Tetromino*);
inline static void SetTetrominoO				(struct Tetromino*);
inline static void SetTetrominoNull				(struct Tetromino*);
inline static void SetCommonRotationOffset		(struct Tetromino*);
inline static void Goto							(struct cordinates);
inline static void WriteOnScreen				(char[SCREEN_WIDTH], cordinates);
inline static unsigned short int RandomIndex	(void);

// inline static int PowerOfTen					(int);

int main(void)
{
	// meta
	srand((unsigned int)time(NULL));
	SetNewScreenBuffer();
	SetWindowsTitle("Tetris!");
	GetConsoleDimensions();
	SetInitialScreen();
	SetEmptyScreen();

	WriteOnScreen(" _____    _        _     ", (cordinates) { 13, 4 });
	WriteOnScreen("|_   _|__| |_ _ __(_)___ ", (cordinates) { 13, 5 });
	WriteOnScreen("  | |/ _ \\ __| '__| / __|", (cordinates) { 13, 6 });
	WriteOnScreen("  | |  __/ |_| |  | \\__ \\", (cordinates) { 13, 7 });
	WriteOnScreen("  |_|\\___|\\__|_|  |_|___/", (cordinates) { 13, 8 });
	WriteOnScreen("Press any key to play", (cordinates) { 15, SCREEN_HEIGHT - 1 });
	GetAnyInput();

	SetEmptyScreen();
	SetGameScreen();
	Game();
	GameOver();

	ExitTetris();

	return 0;
}

// gets  a random character and dumps the  return
// value because Visual Studio wont shut up
void GetAnyInput(void)
{
	char c = _getch();
}

inline static void SetWindowsTitle(char* title) 
{
	printf("\x1B]0;%s\x1B\x5c", title);
}

inline static void SetNewScreenBuffer(void)
{

	printf(ESC BUFFER);						  // new screen buffer
	printf(ESC "[0;0f");					  // moves the curser to (0,0)
}

inline static void ExitTetris(void)
{
	printf(ESC MAIN);							  // restores to the main buffer
	printf(ESC CUR_SHOW);						  // show cursor
}

inline static void GetConsoleDimensions(void)
{
	CONSOLE_SCREEN_BUFFER_INFO screen;

	// gets console properties and stores them in screen
	// and checks if it is successful

	// excpetion handling should be improved
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &screen))
	{
		fprintf(stderr, "\n" "SetNewBuffer: Unable to get information of buffer.\n"		 \
			"Exited with GetLastError." "\n");
		exit(GetLastError());
	}

	padding.x = (screen.dwSize.X / 2) - (SCREEN_WIDTH / 2);
	padding.y = (screen.dwSize.Y / 2) - (SCREEN_HEIGHT / 2);
}

inline static void Goto(cordinates position)
{
	printf(ESC "[%d;%dH", position.y, position.x);
}

inline static void SetInitialScreen(void)
{
	printf(ESC CUR_SHOW);						  // hide cursor
	printf(ESC DRAW);							  // entering into drawing mode

	// printing the above border
	WriteOnScreen(UPPER_LEFT_CORNER, (cordinates) { -1, -1 });
	for (int i = 0; i < SCREEN_WIDTH; i++)
	{
		printf(DASH);
	}
	printf(UPPER_RIGHT_CORNER);

	//prints middle part
	for (int i = 0; i < SCREEN_HEIGHT; i++)
	{
		WriteOnScreen(PIPE, (cordinates) { -1, i });
		WriteOnScreen(PIPE, (cordinates) { SCREEN_WIDTH, i });
	}

	//prints the lower part of the box
	WriteOnScreen(BOTTOM_LEFT_CORNER, (cordinates) { -1, SCREEN_HEIGHT });
	for (int i = 0; i < SCREEN_WIDTH; i++)
	{
		printf(DASH);
	}
	printf(BOTTOM_RIGHT_CORNER);

	printf(ESC ASCII);							  // return to ascii mode
}

// makes the screen buffer empty
inline static void SetEmptyScreen(void)
{
	for (int i = 0; i < SCREEN_HEIGHT; i++)
	{
		for (int j = 0; j < SCREEN_WIDTH; j++)
		{
			WriteOnScreen(" ", (cordinates) { j, i });
		}
	}
}

// use this function when there is combination of
// Goto and printf function
inline static void WriteOnScreen(char string[SCREEN_WIDTH], cordinates position)
{
	Goto((cordinates) { padding.x + position.x, padding.y + position.y });
	for (int i = 0; string[i] != '\0' && i <= (SCREEN_WIDTH - position.x); i++)
	{
		fprintf(stdout, "%c", string[i]);
	}
}

inline static void SetGameScreen(void)
{
	// the left borders of the game
	for (int i = 0; i < SCREEN_HEIGHT - 1; i++)
	{
		WriteOnScreen("<!", (cordinates) { 0, i });
	}

	// the right border of the game
	for (int i = 0; i < SCREEN_HEIGHT - 1; i++)
	{
		WriteOnScreen("!>", (cordinates) { (GAME_WIDTH * 2) + 2, i });
	}

	// the lower borders
	Goto((cordinates) { padding.x + 2, padding.y + (SCREEN_HEIGHT - 2) });
	for (int i = 0; i < (GAME_WIDTH * 2); i++)
	{
		printf("=");
	}
	Goto((cordinates) { padding.x + 2, padding.y + (SCREEN_HEIGHT - 1) });
	for (int i = 0; i < (GAME_WIDTH * 2); i = i + 2)
	{
		printf("\\" "/");
	}

	// border that divides game and scoreboard
	printf(ESC DRAW);							  // drawing mode
	WriteOnScreen(UPPER_SEPERATION, (cordinates) { (GAME_WIDTH * 2) + 4, -1 });
	for (int i = 0; i < SCREEN_HEIGHT; i++)
	{
		WriteOnScreen(PIPE, (cordinates) { (GAME_WIDTH * 2) + 4, i });
	}
	WriteOnScreen("\x76", (cordinates) { (GAME_WIDTH * 2) + 4, SCREEN_HEIGHT });

	// the scoreboard section

	// the  width  is   equillent   to  remaining 
	// distance of screen  which is less than the 
	// game width and its borders
	short int width = SCREEN_WIDTH - ((GAME_WIDTH * 2) + 5);
	Goto((cordinates) { padding.x + width - 1, padding.y });
	for (int i = 0; i < width; i++)
	{
		printf(BLOCK);
	}
	Goto((cordinates) { padding.x + width - 1, padding.y + 2 });
	for (int i = 0; i < width; i++)
	{
		printf(BLOCK);
	}
	Goto((cordinates) { padding.x + width - 1, padding.y + 4 });
	for (int i = 0; i < width; i++)
	{
		printf(BLOCK);
	}
	WriteOnScreen(RIGHT_SEPERATION, (cordinates) { width - 2, 5 });
	for (int i = 0; i < width; i++)
	{
		printf(DASH);
	}
	WriteOnScreen(LEFT_SEPERATION, (cordinates) { SCREEN_WIDTH, 5 });

	// the upcoming tetromino section
	WriteOnScreen(RIGHT_SEPERATION, (cordinates) { width - 2, SCREEN_HEIGHT - 7});
	for (int i = 0; i < width; i++)
	{
		printf(DASH);
	}
	WriteOnScreen(LEFT_SEPERATION, (cordinates) { SCREEN_WIDTH, SCREEN_HEIGHT - 7});
	Goto((cordinates) { padding.x + width - 1, padding.y + SCREEN_HEIGHT - 6 });

	for (int i = 0; i < width; i++)
	{
		printf(BLOCK);
	}
	Goto((cordinates) { padding.x + width - 1, padding.y + SCREEN_HEIGHT - 4 });
	for (int i = 0; i < width; i++)
	{
		printf(BLOCK);
	}
	Goto((cordinates) { padding.x + width - 1, padding.y + SCREEN_HEIGHT - 1 });
	for (int i = 0; i < width; i++)
	{
		printf(BLOCK);
	}
	printf(ESC ASCII);							  // end drawing mode

	WriteOnScreen("Score", (cordinates) { width + 9, 1 });
	WriteOnScreen("Upcoming", (cordinates) { width + 8, SCREEN_HEIGHT - 5 });

	WriteOnScreen("0", (cordinates) {2 + (GAME_WIDTH * 2) + 2 + 13, 3 });
	
	if (_setmode(_fileno(stdout), _O_U16TEXT) == -1)
	{
		// bad error handling ...
		// think of something else ...
		fprintf(stderr, "\n" "Unable to change the mode" "\n");
	}

	wprintf	( 
			 ESC "[%d;%dH" L"⣿⣸⢸⣼⡟⣿⣉⢿⢸⡼⣞⢿⣿⣿⣞⣷⡽⢿⡾⣿⢾⣇⣿⢹⣧⢿"	\
			 ESC "[%d;%dH" L"⣇⣿⡇⣿⣧⢣⣿⠮⡋⠿⣮⣐⢽⣻⠿⠜⡿⠷⡅⢣⡜⣿⢹⢸⣿⡜"	\
			 ESC "[%d;%dH" L"⣸⣿⡇⣿⢨⠏⢀⢄⣀⢌⢺⣿⣿⣿⣿⣷⢊⣀⣀⢄⠈⢞⢸⢸⣿⡇"	\
			 ESC "[%d;%dH" L"⣿⣿⢹⢻⠘⢰⣇⡟⡍⡇⣾⣿⣿⣿⣿⣯⡦⡏⣽⡞⣧⠸⢸⣼⣿⣿"	\
			 ESC "[%d;%dH" L"⣿⣿⡏⡾⢸⣸⣏⠿⡾⢣⣿⣿⣿⣿⣿⣿⣧⠿⠾⣳⣿⣼⣏⣿⣿⣿"	\
			 ESC "[%d;%dH" L"⣿⣿⣿⡵⠛⣿⣿⣛⣻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣛⣻⣿⡇⣼⣿⣿⣿"	\
			 ESC "[%d;%dH" L"⣿⣿⣿⣷⡀⢿⣿⣿⣿⣿⣿⣿⡿⢿⣿⣿⣿⣿⣿⣿⡿⣰⣿⣿⣿⣿"	\
			 ESC "[%d;%dH" L"⢿⣿⣿⣿⣿⡜⣿⣿⣿⣿⣿⣿⣾⣿⣼⣿⣿⣿⣿⡿⣱⣿⣿⣿⣿⣿"	\
			 ESC "[%d;%dH" L"⡼⣿⣿⣿⣿⢰⡍⡛⡿⢿⣿⣿⣿⣿⣿⡿⠟⣻⢥⣿⣿⣿⣿⣿⣿⠁",	\

			 padding.y +  6, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y +  7, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y +  8, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y +  9, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y + 10, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y + 11, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y + 12, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y + 13, padding.x + 2 + (GAME_WIDTH * 2) + 3,
			 padding.y + 14, padding.x + 2 + (GAME_WIDTH * 2) + 3
			);

	if (_setmode(_fileno(stdout), _O_TEXT) == -1)
	{
		// bad error handling ...
		// think of something else ...
		fprintf(stderr, "\n" "Unable to change the mode" "\n");
	}
}

inline static void SetTetrominoI(struct Tetromino* tetromino)
{
	for (int i = 0; i < 4; i++)
	{
		tetromino->tetromino[0][i] = true;
	}

	tetromino->type = I;
	tetromino->dimensions.x = 4;
	tetromino->dimensions.y = 1;
	tetromino->angle = ZERO;

	// from 0 to 270
	tetromino->clockwise_offset[ZERO][0].x =  2;
	tetromino->clockwise_offset[ZERO][0].y =  1;
	tetromino->clockwise_offset[ZERO][1].x =  0;
	tetromino->clockwise_offset[ZERO][1].y =  1;
	tetromino->clockwise_offset[ZERO][2].x =  3;
	tetromino->clockwise_offset[ZERO][2].y =  1;
	tetromino->clockwise_offset[ZERO][3].x =  0;
	tetromino->clockwise_offset[ZERO][3].y =  0;
	tetromino->clockwise_offset[ZERO][4].x =  3;
	tetromino->clockwise_offset[ZERO][4].y =  3;

	// from 0 to 90
	tetromino->anticlockwise_offset[ZERO][0].x =  1;
	tetromino->anticlockwise_offset[ZERO][0].y =  1;
	tetromino->anticlockwise_offset[ZERO][1].x =  0;
	tetromino->anticlockwise_offset[ZERO][1].y =  1;
	tetromino->anticlockwise_offset[ZERO][2].x =  3;
	tetromino->anticlockwise_offset[ZERO][2].y =  1;
	tetromino->anticlockwise_offset[ZERO][3].x =  0;
	tetromino->anticlockwise_offset[ZERO][3].y =  3;
	tetromino->anticlockwise_offset[ZERO][4].x =  3;
	tetromino->anticlockwise_offset[ZERO][4].y =  0;

	// from 90 to 0
	tetromino->clockwise_offset[NINETY][0].x = -1;
	tetromino->clockwise_offset[NINETY][0].y = -1;
	tetromino->clockwise_offset[NINETY][1].x =  0;
	tetromino->clockwise_offset[NINETY][1].y = -1;
	tetromino->clockwise_offset[NINETY][2].x = -3;
	tetromino->clockwise_offset[NINETY][2].y = -1;
	tetromino->clockwise_offset[NINETY][3].x =  0;
	tetromino->clockwise_offset[NINETY][3].y = -3;
	tetromino->clockwise_offset[NINETY][4].x = -3;
	tetromino->clockwise_offset[NINETY][4].y =  0;

	// from 90 to 180
	tetromino->anticlockwise_offset[NINETY][0].x = -1;
	tetromino->anticlockwise_offset[NINETY][0].y = -2;
	tetromino->anticlockwise_offset[NINETY][1].x = -3;
	tetromino->anticlockwise_offset[NINETY][1].y = -2;
	tetromino->anticlockwise_offset[NINETY][2].x =  0;
	tetromino->anticlockwise_offset[NINETY][2].y = -2;
	tetromino->anticlockwise_offset[NINETY][3].x = -3;
	tetromino->anticlockwise_offset[NINETY][3].y = -3;
	tetromino->anticlockwise_offset[NINETY][4].x =  0;
	tetromino->anticlockwise_offset[NINETY][4].y =  0;

	// from 180 to 90
	tetromino->clockwise_offset[ONE_EIGHTY][0].x =  1;
	tetromino->clockwise_offset[ONE_EIGHTY][0].y =  2;
	tetromino->clockwise_offset[ONE_EIGHTY][1].x =  3;
	tetromino->clockwise_offset[ONE_EIGHTY][1].y =  2;
	tetromino->clockwise_offset[ONE_EIGHTY][2].x =  0;
	tetromino->clockwise_offset[ONE_EIGHTY][2].y =  2;
	tetromino->clockwise_offset[ONE_EIGHTY][3].x =  3;
	tetromino->clockwise_offset[ONE_EIGHTY][3].y =  3;
	tetromino->clockwise_offset[ONE_EIGHTY][4].x =  0;
	tetromino->clockwise_offset[ONE_EIGHTY][4].y =  0;

	// from 180 to 270
	tetromino->anticlockwise_offset[ONE_EIGHTY][0].x =  2;
	tetromino->anticlockwise_offset[ONE_EIGHTY][0].y =  2;
	tetromino->anticlockwise_offset[ONE_EIGHTY][1].x =  3;
	tetromino->anticlockwise_offset[ONE_EIGHTY][1].y =  2;
	tetromino->anticlockwise_offset[ONE_EIGHTY][2].x =  0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][2].y =  2;
	tetromino->anticlockwise_offset[ONE_EIGHTY][3].x =  3;
	tetromino->anticlockwise_offset[ONE_EIGHTY][3].y =  0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][4].x =  0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][4].y =  3;
	
	// from 270 to 180
	tetromino->clockwise_offset[TWO_SEVENTTY][0].x = -2;
	tetromino->clockwise_offset[TWO_SEVENTTY][0].y = -2;
	tetromino->clockwise_offset[TWO_SEVENTTY][1].x = -3;
	tetromino->clockwise_offset[TWO_SEVENTTY][1].y = -2;
	tetromino->clockwise_offset[TWO_SEVENTTY][2].x =  0;
	tetromino->clockwise_offset[TWO_SEVENTTY][2].y = -2;
	tetromino->clockwise_offset[TWO_SEVENTTY][3].x = -3;
	tetromino->clockwise_offset[TWO_SEVENTTY][3].y =  0;
	tetromino->clockwise_offset[TWO_SEVENTTY][4].x =  0;
	tetromino->clockwise_offset[TWO_SEVENTTY][4].y = -3;

	// from 270 to 0
	tetromino->anticlockwise_offset[TWO_SEVENTTY][0].x = -2;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][0].y = -1;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][1].x =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][1].y = -1;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][2].x = -3;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][2].y = -1;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][3].x =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][3].y =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][4].x = -3;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][4].y = -3;
}

inline static void SetTetrominoT(struct Tetromino* tetromino)
{
	tetromino->tetromino[1][0] = true;
	tetromino->tetromino[1][1] = true;
	tetromino->tetromino[1][2] = true;

	tetromino->tetromino[0][1] = true;

	tetromino->type = T;
	tetromino->dimensions.x = 3;
	tetromino->dimensions.y = 2;
	tetromino->angle = ZERO;

	SetCommonRotationOffset(tetromino);
}

inline static void SetTetrominoL(struct Tetromino* tetromino)
{
	tetromino->tetromino[1][0] = true;
	tetromino->tetromino[1][1] = true;
	tetromino->tetromino[1][2] = true;

	tetromino->tetromino[0][2] = true;

	tetromino->type = L;
	tetromino->dimensions.x = 3;
	tetromino->dimensions.y = 2;
	tetromino->angle = ZERO;

	SetCommonRotationOffset(tetromino);
}

static void SetTetrominoJ(struct Tetromino* tetromino)
{
	tetromino->tetromino[1][0] = true;
	tetromino->tetromino[1][1] = true;
	tetromino->tetromino[1][2] = true;

	tetromino->tetromino[0][0] = true;

	tetromino->type = J;
	tetromino->dimensions.x = 3;
	tetromino->dimensions.y = 2;
	tetromino->angle = ZERO;

	SetCommonRotationOffset(tetromino);
}

inline static void SetTetrominoS(struct Tetromino* tetromino)
{
	tetromino->tetromino[1][0] = true;
	tetromino->tetromino[1][1] = true;

	tetromino->tetromino[0][1] = true;
	tetromino->tetromino[0][2] = true;

	tetromino->type = Z;
	tetromino->dimensions.x = 3;
	tetromino->dimensions.y = 2;
	tetromino->angle = ZERO;

	SetCommonRotationOffset(tetromino);
}

inline static void SetTetrominoZ(struct Tetromino* tetromino)
{
	tetromino->tetromino[0][0] = true;
	tetromino->tetromino[0][1] = true;

	tetromino->tetromino[1][1] = true;
	tetromino->tetromino[1][2] = true;

	tetromino->type = S;
	tetromino->dimensions.x = 3;
	tetromino->dimensions.y = 2;
	tetromino->angle = ZERO;

	SetCommonRotationOffset(tetromino);
}

inline static void SetTetrominoO(struct Tetromino* tetromino)
{
	tetromino->tetromino[0][0] = true;
	tetromino->tetromino[0][1] = true;

	tetromino->tetromino[1][0] = true;
	tetromino->tetromino[1][1] = true;

	tetromino->type = O;
	tetromino->dimensions.x = 2;
	tetromino->dimensions.y = 2;
	tetromino->angle = ZERO;

	tetromino->clockwise_offset[ZERO][0].x = 0;
	tetromino->clockwise_offset[ZERO][0].y = 0;
	tetromino->clockwise_offset[ZERO][1].x = 0;
	tetromino->clockwise_offset[ZERO][1].y = 0;
	tetromino->clockwise_offset[ZERO][2].x = 0;
	tetromino->clockwise_offset[ZERO][2].y = 0;
	tetromino->clockwise_offset[ZERO][3].x = 0;
	tetromino->clockwise_offset[ZERO][3].y = 0;
	tetromino->clockwise_offset[ZERO][4].x = 0;
	tetromino->clockwise_offset[ZERO][4].y = 0;

	// from 0 to 90
	tetromino->anticlockwise_offset[ZERO][0].x = 0;
	tetromino->anticlockwise_offset[ZERO][0].y = 0;
	tetromino->anticlockwise_offset[ZERO][1].x = 0;
	tetromino->anticlockwise_offset[ZERO][1].y = 0;
	tetromino->anticlockwise_offset[ZERO][2].x = 0;
	tetromino->anticlockwise_offset[ZERO][2].y = 0;
	tetromino->anticlockwise_offset[ZERO][3].x = 0;
	tetromino->anticlockwise_offset[ZERO][3].y = 0;
	tetromino->anticlockwise_offset[ZERO][4].x = 0;
	tetromino->anticlockwise_offset[ZERO][4].y = 0;

	// from 90 to 0
	tetromino->clockwise_offset[NINETY][0].x = 0;
	tetromino->clockwise_offset[NINETY][0].y = 0;
	tetromino->clockwise_offset[NINETY][1].x = 0;
	tetromino->clockwise_offset[NINETY][1].y = 0;
	tetromino->clockwise_offset[NINETY][2].x = 0;
	tetromino->clockwise_offset[NINETY][2].y = 0;
	tetromino->clockwise_offset[NINETY][3].x = 0;
	tetromino->clockwise_offset[NINETY][3].y = 0;
	tetromino->clockwise_offset[NINETY][4].x = 0;
	tetromino->clockwise_offset[NINETY][4].y = 0;

	// from 90 to 180
	tetromino->anticlockwise_offset[NINETY][0].x = 0;
	tetromino->anticlockwise_offset[NINETY][0].y = 0;
	tetromino->anticlockwise_offset[NINETY][1].x = 0;
	tetromino->anticlockwise_offset[NINETY][1].y = 0;
	tetromino->anticlockwise_offset[NINETY][2].x = 0;
	tetromino->anticlockwise_offset[NINETY][2].y = 0;
	tetromino->anticlockwise_offset[NINETY][3].x = 0;
	tetromino->anticlockwise_offset[NINETY][3].y = 0;
	tetromino->anticlockwise_offset[NINETY][4].x = 0;
	tetromino->anticlockwise_offset[NINETY][4].y = 0;

	// from 180 to 90
	tetromino->clockwise_offset[ONE_EIGHTY][0].x = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][0].y = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][1].x = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][1].y = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][2].x = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][2].y = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][3].x = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][3].y = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][4].x = 0;
	tetromino->clockwise_offset[ONE_EIGHTY][4].y = 0;

	// from 180 to 270
	tetromino->anticlockwise_offset[ONE_EIGHTY][0].x = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][0].y = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][1].x = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][1].y = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][2].x = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][2].y = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][3].x = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][3].y = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][4].x = 0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][4].y = 0;

	// from 270 to 180
	tetromino->clockwise_offset[TWO_SEVENTTY][0].x = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][0].y = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][1].x = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][1].y = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][2].x = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][2].y = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][3].x = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][3].y = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][4].x = 0;
	tetromino->clockwise_offset[TWO_SEVENTTY][4].y = 0;

	// from 270 to 0
	tetromino->anticlockwise_offset[TWO_SEVENTTY][0].x = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][0].y = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][1].x = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][1].y = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][2].x = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][2].y = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][3].x = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][3].y = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][4].x = 0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][4].y = 0;
}

inline static void SetTetrominoNull(struct Tetromino* tetromino)
{
	for (int i = 0; i < TETROMINO_HEIGHT; i++)
	{
		for (int j = 0; j < TETROMINO_WIDTH; j++)
		{
			tetromino->tetromino[i][j] = false;
		}
	}

	tetromino->type = EMPTY;
}

inline static void SetCommonRotationOffset(struct Tetromino* tetromino)
{
	// from 0 to 270
	tetromino->clockwise_offset[ZERO][0].x =  1;
	tetromino->clockwise_offset[ZERO][0].y =  0;
	tetromino->clockwise_offset[ZERO][1].x =  0;
	tetromino->clockwise_offset[ZERO][1].y =  0;
	tetromino->clockwise_offset[ZERO][2].x =  0;
	tetromino->clockwise_offset[ZERO][2].y =  1;
	tetromino->clockwise_offset[ZERO][3].x =  1;
	tetromino->clockwise_offset[ZERO][3].y = -2;
	tetromino->clockwise_offset[ZERO][4].x =  0;
	tetromino->clockwise_offset[ZERO][4].y = -2;

	// from 0 to 90
	tetromino->anticlockwise_offset[ZERO][0].x =  0;
	tetromino->anticlockwise_offset[ZERO][0].y =  0;
	tetromino->anticlockwise_offset[ZERO][1].x =  1;
	tetromino->anticlockwise_offset[ZERO][1].y =  0;
	tetromino->anticlockwise_offset[ZERO][2].x =  1;
	tetromino->anticlockwise_offset[ZERO][2].y =  1;
	tetromino->anticlockwise_offset[ZERO][3].x =  0;
	tetromino->anticlockwise_offset[ZERO][3].y = -2;
	tetromino->anticlockwise_offset[ZERO][4].x =  1;
	tetromino->anticlockwise_offset[ZERO][4].y = -2;

	// from 90 to 0
	tetromino->clockwise_offset[NINETY][0].x =  0;
	tetromino->clockwise_offset[NINETY][0].y =  0;
	tetromino->clockwise_offset[NINETY][1].x = -1;
	tetromino->clockwise_offset[NINETY][1].y =  0;
	tetromino->clockwise_offset[NINETY][2].x = -1;
	tetromino->clockwise_offset[NINETY][2].y = -1;
	tetromino->clockwise_offset[NINETY][3].x =  0;
	tetromino->clockwise_offset[NINETY][3].y =  2;
	tetromino->clockwise_offset[NINETY][4].x = -1;
	tetromino->clockwise_offset[NINETY][4].y =  2;

	// from 90 to 180
	tetromino->anticlockwise_offset[NINETY][0].x =  0;
	tetromino->anticlockwise_offset[NINETY][0].y = -1;
	tetromino->anticlockwise_offset[NINETY][1].x = -1;
	tetromino->anticlockwise_offset[NINETY][1].y = -1;
	tetromino->anticlockwise_offset[NINETY][2].x = -1;
	tetromino->anticlockwise_offset[NINETY][2].y = -2;
	tetromino->anticlockwise_offset[NINETY][3].x =  0;
	tetromino->anticlockwise_offset[NINETY][3].y =  1;
	tetromino->anticlockwise_offset[NINETY][4].x = -1;
	tetromino->anticlockwise_offset[NINETY][4].y =  1;

	// from 180 to 90
	tetromino->clockwise_offset[ONE_EIGHTY][0].x =  0;
	tetromino->clockwise_offset[ONE_EIGHTY][0].y =  1;
	tetromino->clockwise_offset[ONE_EIGHTY][1].x =  1;
	tetromino->clockwise_offset[ONE_EIGHTY][1].y =  1;
	tetromino->clockwise_offset[ONE_EIGHTY][2].x =  1;
	tetromino->clockwise_offset[ONE_EIGHTY][2].y =  2;
	tetromino->clockwise_offset[ONE_EIGHTY][3].x =  0;
	tetromino->clockwise_offset[ONE_EIGHTY][3].y = -1;
	tetromino->clockwise_offset[ONE_EIGHTY][4].x =  1;
	tetromino->clockwise_offset[ONE_EIGHTY][4].y = -1;

	// from 180 to 270
	tetromino->anticlockwise_offset[ONE_EIGHTY][0].x =  1;
	tetromino->anticlockwise_offset[ONE_EIGHTY][0].y =  1;
	tetromino->anticlockwise_offset[ONE_EIGHTY][1].x =  0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][1].y =  1;
	tetromino->anticlockwise_offset[ONE_EIGHTY][2].x =  0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][2].y =  2;
	tetromino->anticlockwise_offset[ONE_EIGHTY][3].x =  1;
	tetromino->anticlockwise_offset[ONE_EIGHTY][3].y = -1;
	tetromino->anticlockwise_offset[ONE_EIGHTY][4].x =  0;
	tetromino->anticlockwise_offset[ONE_EIGHTY][4].y = -1;
	
	// from 270 to 180
	tetromino->clockwise_offset[TWO_SEVENTTY][0].x = -1;
	tetromino->clockwise_offset[TWO_SEVENTTY][0].y = -1;
	tetromino->clockwise_offset[TWO_SEVENTTY][1].x =  0;
	tetromino->clockwise_offset[TWO_SEVENTTY][1].y = -1;
	tetromino->clockwise_offset[TWO_SEVENTTY][2].x =  0;
	tetromino->clockwise_offset[TWO_SEVENTTY][2].y = -2;
	tetromino->clockwise_offset[TWO_SEVENTTY][3].x = -1;
	tetromino->clockwise_offset[TWO_SEVENTTY][3].y =  1;
	tetromino->clockwise_offset[TWO_SEVENTTY][4].x =  0;
	tetromino->clockwise_offset[TWO_SEVENTTY][4].y =  1;

	// from 270 to 0
	tetromino->anticlockwise_offset[TWO_SEVENTTY][0].x = -1;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][0].y =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][1].x =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][1].y =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][2].x =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][2].y = -1;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][3].x = -1;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][3].y =  2;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][4].x =  0;
	tetromino->anticlockwise_offset[TWO_SEVENTTY][4].y =  2;
}

inline static unsigned short int RandomIndex(void)
{
	return (unsigned short int) (rand() % 7);
}

inline static void SetColor(enum type *color)
{
	switch (*color)
	{
	case I:
		printf(ESC CYAN);

		break;

	case O:
		printf(ESC YELLOW);

		break;

	case T:
		printf(ESC PINK);

		break;

	case J:
		printf(ESC BLUE);

		break;

	case L:
		printf(ESC ORANGE);

		break;

	case S:
		printf(ESC GREEN);

		break;

	case Z:
		printf(ESC RED);

		break;
	}
}

inline static void PredictTetromino(void)
{
	prediction = current;
	while (CheckTetromino(&prediction))
	{
		--prediction.index.y;
	}
	++prediction.index.y;

	printf(ESC GREY);
	PrintTetromino(&prediction);
}

inline static void Game(void)
{

	void (*SetTetromino[7])(struct Tetromino*) = {
														&SetTetrominoI,
														&SetTetrominoJ,
														&SetTetrominoL,
														&SetTetrominoO,
														&SetTetrominoS,
														&SetTetrominoT,
														&SetTetrominoZ	
	};

	SetTetrominoNull(&current);
	SetTetrominoNull(&next);

	SetTetromino[RandomIndex()](&next);
	
	while (true)
	{
		current = next;
		EraseNextTetromino();
		SetTetrominoNull(&next);

		SetTetromino[RandomIndex()](&next);
		SetColor(&next.type);
		PrintNextTetromino();

		current.index.x = 4;
		current.index.y = GAME_HEIGHT;

		if (! CheckTetromino(&current))
		{
			break;
		}
		
		do 
		{
			PredictTetromino();

			SetColor(&current.type);
			PrintTetromino(&current);

			WaitForInput();

			EraseTetromino(&prediction);
			EraseTetromino(&current);

			--current.index.y;
		} while (CheckTetromino(&current));

		SaveGrid();
		SetTetrominoNull(&current);
		
		// PrintGrid also checks for complete rows
		// It would be ugly  so I didn't rename it
		// The  functionality   is baked into  the 
		// function
		if (PrintGrid())
		{
			UpdateScore();
			PrintScore((cordinates) { 2 + (GAME_WIDTH * 2) + 2 + 13, 3});
			RemoveCompleteRows();
		}
	}
}

static void PrintTetromino(struct Tetromino* tetromino)
{
	for (int i = 0; i < tetromino->dimensions.y; i++)
	{
		for (int j = 0; j < tetromino->dimensions.x; j++)
		{
			switch (tetromino->angle)
			{
			case ZERO:
				if ((GAME_HEIGHT - tetromino->index.y) + i < 0)
					continue;

				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + (j * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + i });
				break;

			case NINETY:
				if ((GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.x - 1) - j < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + (i * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.x - 1) - j });
				break;

			case ONE_EIGHTY:
				if ((GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.y - (i + 1)) < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + ((tetromino->dimensions.x - (j + 1)) * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.y - (i + 1)) });
				break;

			case TWO_SEVENTTY:
				if ((GAME_HEIGHT - tetromino->index.y) + j < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + ((tetromino->dimensions.y - (i + 1)) * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + j });
				break;
			}

			if (tetromino->tetromino[i][j])
			{
				printf("[]");
				continue;
			}
			
			printf(ESC CUF(2));
		}
	}
}

// The if statement in each case is for the checking
// if any turn has caused the tetromino to start from
// index or fall bellow 0 y-axis
static void EraseTetromino(struct Tetromino* tetromino)
{
	for (int i = 0; i < tetromino->dimensions.y; i++)
	{
		for (int j = 0; j < tetromino->dimensions.x; j++)
		{
			switch (tetromino->angle)
			{
			case ZERO:
				if ((GAME_HEIGHT - tetromino->index.y) + i < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + (j * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + i });
				break;

			case NINETY:
				if ((GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.x - 1) - j < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + (i * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.x - 1) - j });
				break;

			case ONE_EIGHTY:
				if ((GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.y - (i + 1)) < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + ((tetromino->dimensions.x - (j + 1)) * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.y - (i + 1)) });
				break;

			case TWO_SEVENTTY:
				if ((GAME_HEIGHT - tetromino->index.y) + j < 0)
					continue;
				Goto((cordinates) { padding.x + 2 + (tetromino->index.x * 2) + ((tetromino->dimensions.y - (i + 1)) * 2), padding.y + (GAME_HEIGHT - tetromino->index.y) + j });
				break;
			}

			if (tetromino->tetromino[i][j])
			{
				printf("  ");
				continue;
			}

			printf(ESC CUF(2));
		}
	}
}

static bool CheckTetromino(struct Tetromino* tetromino)
{
	TCHAR c;
	DWORD count = 0;

	for (int i = 0; i < tetromino->dimensions.y; i++)
	{
		for (int j = 0; j < tetromino->dimensions.x; j++)
		{
			switch(tetromino->angle)
			{
			case ZERO:
				if ((GAME_HEIGHT - tetromino->index.y) + i < 0)
					continue;
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + (tetromino->index.x * 2) + (j * 2) - 1, padding.y + (GAME_HEIGHT - tetromino->index.y) + i - 1},
					&count);
				break;

			case NINETY:
				if ((GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.x - 1) - j < 0)
					continue;
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + (tetromino->index.x * 2) + (i * 2) - 1, padding.y + (GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.x - 1) - j - 1},
					&count);
				break;

			case ONE_EIGHTY:
				if ((GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.y - (i + 1)) < 0)
					continue;
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + (tetromino->index.x * 2) + ((tetromino->dimensions.x - (j + 1)) * 2) - 1, padding.y + (GAME_HEIGHT - tetromino->index.y) + (tetromino->dimensions.y - (i + 1)) - 1},
					&count);
				break;

			case TWO_SEVENTTY:
				if ((GAME_HEIGHT - tetromino->index.y) + j < 0)
					continue;
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + (tetromino->index.x * 2) + ((tetromino->dimensions.y - (i + 1)) * 2) - 1, padding.y + (GAME_HEIGHT - tetromino->index.y) + j - 1},
					&count);
				break;
			}

			if (tetromino->tetromino[i][j] && c != 32)
			{
				return false;
			}
		}
	}

	return true;
}

inline static void PrintNextTetromino(void)
{
	// accross the game width and normal padding and
	// in the middle of scoreboard
	for (int i = 0; i < next.dimensions.y; i++)
	{
		Goto((cordinates) { padding.x + 2 + ((GAME_WIDTH * 2) + 3) + (13 - next.dimensions.x), padding.y + SCREEN_HEIGHT - 3 + i });
		for (int j = 0; j < next.dimensions.x; j++)
		{
			if (next.tetromino[i][j])
			{
				printf("[]");
			}
			else
			{
				printf(ESC CUF(2));
			}
		}
	}
}

inline static void SaveGrid(void)
{
	for (int i = 0; i < current.dimensions.y; i++)
	{
		for (int j = 0; j < current.dimensions.x; j++)
		{
			if (!current.tetromino[i][j])	continue;

			switch (current.angle)
			{
			case ZERO:
				grid[current.index.y - i][current.index.x + j].pixel = current.tetromino[i][j];
				grid[current.index.y - i][current.index.x + j].color = current.type;
				break;

			case NINETY:
				grid[current.index.y - (current.dimensions.x - (j + 1))][current.index.x + i].pixel = current.tetromino[i][j];
				grid[current.index.y - (current.dimensions.x - (j + 1))][current.index.x + i].color = current.type;
				break;

			case ONE_EIGHTY:
				grid[current.index.y - (current.dimensions.y - (i + 1))][current.index.x + (current.dimensions.x - (j + 1))].pixel = current.tetromino[i][j];
				grid[current.index.y - (current.dimensions.y - (i + 1))][current.index.x + (current.dimensions.x - (j + 1))].color = current.type;
				break;

			case TWO_SEVENTTY:
				grid[current.index.y - j][current.index.x + (current.dimensions.y - (i + 1))].pixel = current.tetromino[i][j];
				grid[current.index.y - j][current.index.x + (current.dimensions.y - (i + 1))].color = current.type;
				break;
			}
		}
	}
}

// I am guilty of using chatgbt here
// I am really sorry. I tried to look for solution
// everywhere :(

// using `previous = current` outside of the while 
// loop causes a weird bug
inline static void WaitForInput(void)
{
	time_t now = time(NULL);
	char c;

	while ((time(NULL) - now) < TIME)
	{
		// checkes the input buffer
		if (_kbhit())
		{
			c = _getch();

			switch (c)
			{
			case 72:
				// Up key is pressed
				// flips backward

				EraseTetromino(&prediction);

				RotateClockwise();
				PredictTetromino();

				SetColor(&current.type);
				PrintTetromino(&current);

				break;

			case 80:
				// Down key is pressed
				// soft drop

				return;

			case 77:
				// Right key is pressed

				previous = current;

				EraseTetromino(&prediction);
				EraseTetromino(&current);

				++current.index.x;

				if (!CheckTetromino(&current))
				{
					current = previous;
				}

				PredictTetromino();

				SetColor(&current.type);
				PrintTetromino(&current);

				break;

			case 75:
				// Left key is pressed

				previous = current;

				EraseTetromino(&prediction);
				EraseTetromino(&current);

				--current.index.x;
				if (!CheckTetromino(&current))
				{
					current = previous;
				}

				PredictTetromino();

				SetColor(&current.type);
				PrintTetromino(&current);

				break;

			case ' ':
				// Space is pressed

				EraseTetromino(&prediction);
				EraseTetromino(&current);

				current = prediction;

				return;

			case 'z':
				// 'z' is pressed
				// flips forwardgi

				EraseTetromino(&prediction);

				RotateCounterclockwise();
				PredictTetromino();

				SetColor(&current.type);
				PrintTetromino(&current);

				break;
			}
		}
	}
}

inline static void RotateCounterclockwise(void)
{
	previous = current;
	EraseTetromino(&current);
	if (current.angle == TWO_SEVENTTY)
	{
		current.angle = ZERO;
	}
	else
	{
		++current.angle;
	}

	for (int i = 0; i < 5; i++)
	{
		current.index.x += current.anticlockwise_offset[previous.angle][i].x;
		current.index.y += current.anticlockwise_offset[previous.angle][i].y;

		if (CheckTetromino(&current))
		{
			goto End;
		}

		current.index.x -= current.anticlockwise_offset[previous.angle][i].x;
		current.index.y -= current.anticlockwise_offset[previous.angle][i].y;
	}

	current = previous;

End:		;
}

inline static void RotateClockwise(void)
{
	previous = current;
	EraseTetromino(&current);
	if (current.angle == ZERO)
	{
		current.angle = TWO_SEVENTTY;
	}
	else
	{
		--current.angle;
	}

	for (int i = 0; i < 5; i++)
	{
		current.index.x += current.clockwise_offset[previous.angle][i].x;
		current.index.y += current.clockwise_offset[previous.angle][i].y;

		if (CheckTetromino(&current))
		{
			goto End;
		}

		current.index.x -= current.clockwise_offset[previous.angle][i].x;
		current.index.y -= current.clockwise_offset[previous.angle][i].y;

	}

	current = previous;

End:	;
}

// There is an alternative sollution I thought:
// 1. Use a single variable row and store three states in it
// 2. Allocate memory for completed_rows

inline static bool PrintGrid(void)
{
	bool row = false, complete_row;
	short int k = 0;

	for (int i = 0; i < GAME_HEIGHT; i++)
	{
		complete_row = true;
		for (int j = 0; j < GAME_WIDTH; j++)
		{
			if (grid[i][j].pixel)
			{
				SetColor(&grid[i][j].color);
				Goto((cordinates) { padding.x + 2 + (j * 2), padding.y + (GAME_HEIGHT - 1) - i });
				printf("[]");

				if (!row) row = true;

				continue;
			}

			complete_row = false;
		}
		
		if (complete_row && k < 4)	complete_rows[k++] = i;
		if (!row)	break;
	}

	return complete_rows[0] != -1 ? true : false;
}

inline static void RemoveCompleteRows(void)
{
	int i = complete_rows[0];
	complete_rows[0] = -1;
	short int j = 1, difference = 1;
	bool row;

	for (; i < GAME_HEIGHT + 2; i++)
	{

		while (j < 4 && complete_rows[j] == i + difference)
		{
			++difference;
			complete_rows[j++] = -1;
		}

		row = false;
		for (int k = 0; k < GAME_WIDTH; k++)
		{
			if (grid[i][k].pixel)	row = true;
			
			if (i + difference < GAME_HEIGHT + 2)
			{
				grid[i][k] = grid[i + difference][k];
			}
			else
			{
				grid[i][k] = (Grid){ false, EMPTY };
			}

			Goto((cordinates) { padding.x + 2 + (k * 2), padding.y + (GAME_HEIGHT - 1) - i });
			if (grid[i][k].pixel)
			{
				SetColor(&grid[i][k].color);
				printf("[]");
			}
			else
			{
				printf("  ");
			}
		}

		if (!row)	break;
	}
}

inline static void UpdateScore(void)
{
	short int i;
	for (i = 0; i < 4 && complete_rows[i] != -1; i++)
		;

	switch (i)
	{
	case 1:
		score += 4;
		break;

	case 2:
		score += 10;
		break;

	case 3:
		score += 30;
		break;

	case 4:
		score += 120;
		break;
	}
}

inline static void EraseNextTetromino(void)
{
	for (int i = 0; i < next.dimensions.y; i++)
	{
		Goto((cordinates) { padding.x + 2 + ((GAME_WIDTH * 2) + 3) + (13 - next.dimensions.x), padding.y + SCREEN_HEIGHT - 3 + i });
		for (int j = 0; j < next.dimensions.x; j++)
		{
			printf("  ");
		}
	}
}

inline static int PowerOfTen(int times)
{
	int power = 10;
	for (; times != 1; power *= 10, times--);
	return power;
}

inline static void PrintScore(cordinates index)
{
	static int length = 1;
	for (; score / PowerOfTen(length) != 0; length++);
	
	Goto((cordinates) { padding.x + index.x - length / 2, padding.y + index.y });
	printf(ESC DEFAULT "%d", score);
}

inline static void GameOver(void)
{
	printf(ESC DEFAULT ESC DRAW);

	WriteOnScreen(DASH, (cordinates) { 2 + (GAME_WIDTH * 2) + 2, -1 });
	WriteOnScreen(DASH, (cordinates) { 2 + (GAME_WIDTH * 2) + 2, SCREEN_HEIGHT });
	WriteOnScreen(PIPE, (cordinates) { SCREEN_WIDTH, 5 });
	WriteOnScreen(PIPE, (cordinates) { SCREEN_WIDTH , 15 });

	printf(ESC ASCII);

	SetEmptyScreen();
	

	WriteOnScreen("  ________   __  _______ ____ _   _________  __"		, (cordinates) { 2, 7 + 0 });
	WriteOnScreen(" / ___/ _ | /  |/  / __// __ \\ | / / __/ _ \\/ /"	, (cordinates) { 2, 7 + 1 });
	WriteOnScreen("/ (_ / __ |/ /|_/ / _/ / /_/ / |/ / _// , _/_/"		, (cordinates) { 2, 7 + 2 });
	WriteOnScreen("\\___/_/ |_/_/  /_/___/ \\____/|___/___/_/|_(_)"		, (cordinates) { 2, 7 + 3 });

	WriteOnScreen("Your Score", (cordinates) { 20, 7 + 5 });
	
	PrintScore((cordinates) { SCREEN_WIDTH / 2, 7 + 7 });
	
	GetAnyInput(); 
}
