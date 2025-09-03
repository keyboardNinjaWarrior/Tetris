#include <time.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <Windows.h>

#define SCREEN_WIDTH		51
#define	SCREEN_HEIGHT		22
#define	GAME_WIDTH			20
#define TETROMINO_HEIGHT	2
#define TETROMINO_WIDTH		4
#define TIME				2

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

struct cordinates
{
	short int x;
	short int y;
} padding = { 0,0 };
typedef struct cordinates cordinates;

enum { I, J, L, O, S, T, Z, EMPTY };
enum rotation { ZERO, NINETY, ONE_EIGHTY, TWO_SEVENTTY };

struct Tetromino
{
	bool tetromino[TETROMINO_HEIGHT][TETROMINO_WIDTH];

	short int type;
	struct cordinates dimensions;
	struct cordinates index;
	enum rotation angle;

	struct cordinates rotation_offset[4];

} current, next, previous;

// functions
static void GetAnyInput							(void);
static void PrintTetromino						(struct Tetromino*);
static void EraseTetromino						(struct Tetromino*);
static bool CheckTetromino						(struct Tetromino*);
inline static void Game							(void);
inline static void SetGameScreen				(void);
inline static void SetEmptyScreen				(void);
inline static void ClockwiseRotate				(void);
inline static void RotateTetromino				(void);
inline static void SetInitialScreen				(void);
inline static void SetNewScreenBuffer			(void);
inline static void GetConsoleDimensions			(void);
inline static void SetWindowsTitle				(char*);
inline static void WriteOnScreen				(char[SCREEN_WIDTH], cordinates);
inline static void SetTetrominoI				(struct Tetromino*);
inline static void SetTetrominoT				(struct Tetromino*);
inline static void SetTetrominoL				(struct Tetromino*);
inline static void SetTetrominoJ				(struct Tetromino*);
inline static void SetTetrominoZ				(struct Tetromino*);
inline static void SetTetrominoS				(struct Tetromino*);
inline static void SetTetrominoO				(struct Tetromino*);
inline static void SetTetrominoNull				(struct Tetromino*);
inline static void Goto							(struct cordinates);
inline static void WaitForInput					(void);
inline static void ExitTetris					(void);
inline static unsigned short int RandomIndex	(void);

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
		WriteOnScreen("!>", (cordinates) { GAME_WIDTH + 2, i });
	}

	// the lower borders
	Goto((cordinates) { padding.x + 2, padding.y + (SCREEN_HEIGHT - 2) });
	for (int i = 0; i < GAME_WIDTH; i++)
	{
		printf("=");
	}
	Goto((cordinates) { padding.x + 2, padding.y + (SCREEN_HEIGHT - 1) });
	for (int i = 0; i < GAME_WIDTH; i = i + 2)
	{
		printf("\\" "/");
	}

	// border that divides game and scoreboard
	printf(ESC DRAW);							  // drawing mode
	WriteOnScreen(UPPER_SEPERATION, (cordinates) { GAME_WIDTH + 4, -1 });
	for (int i = 0; i < SCREEN_HEIGHT; i++)
	{
		WriteOnScreen(PIPE, (cordinates) { GAME_WIDTH + 4, i });
	}
	WriteOnScreen("\x76", (cordinates) { GAME_WIDTH + 4, SCREEN_HEIGHT });

	// the scoreboard section

	// the  width  is   equillent   to  remaining 
	// distance of screen  which is less than the 
	// game width and its borders
	short int width = SCREEN_WIDTH - (GAME_WIDTH + 5);
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
	
	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;
	
	tetromino->rotation_offset[NINETY].x = 2;
	tetromino->rotation_offset[NINETY].y = -2;
	
	tetromino->rotation_offset[ONE_EIGHTY].x = -2;
	tetromino->rotation_offset[ONE_EIGHTY].y = 0;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 2;
	tetromino->rotation_offset[TWO_SEVENTTY].y = -1;
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

	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;

	tetromino->rotation_offset[NINETY].x = 0;
	tetromino->rotation_offset[NINETY].y = 0;

	tetromino->rotation_offset[ONE_EIGHTY].x = 0;
	tetromino->rotation_offset[ONE_EIGHTY].y = 1;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 2;
	tetromino->rotation_offset[TWO_SEVENTTY].y = 0;
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

	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;

	tetromino->rotation_offset[NINETY].x = 0;
	tetromino->rotation_offset[NINETY].y = 0;

	tetromino->rotation_offset[ONE_EIGHTY].x = 0;
	tetromino->rotation_offset[ONE_EIGHTY].y = 1;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 2;
	tetromino->rotation_offset[TWO_SEVENTTY].y = 0;
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

	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;

	tetromino->rotation_offset[NINETY].x = 0;
	tetromino->rotation_offset[NINETY].y = 0;
	
	tetromino->rotation_offset[ONE_EIGHTY].x = 0;
	tetromino->rotation_offset[ONE_EIGHTY].y = 1;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 2;
	tetromino->rotation_offset[TWO_SEVENTTY].y = 0;
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

	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;

	tetromino->rotation_offset[NINETY].x = 2;
	tetromino->rotation_offset[NINETY].y = 0;

	tetromino->rotation_offset[ONE_EIGHTY].x = 0;
	tetromino->rotation_offset[ONE_EIGHTY].y = 1;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 0;
	tetromino->rotation_offset[TWO_SEVENTTY].y = 0;
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

	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;

	tetromino->rotation_offset[NINETY].x = 2;
	tetromino->rotation_offset[NINETY].y = 0;

	tetromino->rotation_offset[ONE_EIGHTY].x = 0;
	tetromino->rotation_offset[ONE_EIGHTY].y = 1;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 0;
	tetromino->rotation_offset[TWO_SEVENTTY].y = 0;
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

	tetromino->rotation_offset[ZERO].x = 0;
	tetromino->rotation_offset[ZERO].y = 0;

	tetromino->rotation_offset[NINETY].x = 0;
	tetromino->rotation_offset[NINETY].y = 0;

	tetromino->rotation_offset[ONE_EIGHTY].x = 0;
	tetromino->rotation_offset[ONE_EIGHTY].y = 0;

	tetromino->rotation_offset[TWO_SEVENTTY].x = 0;
	tetromino->rotation_offset[TWO_SEVENTTY].y = 0;
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

inline static unsigned short int RandomIndex(void)
{
	return (unsigned short int) (rand() % 7);
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

	next.index.y = SCREEN_HEIGHT - 3;
	SetTetromino[RandomIndex()](&next);
	// accross the game width and normal padding and
	// in the middle of scoreboard
	next.index.x = (GAME_WIDTH + 3) + (13 - next.dimensions.x);
	PrintTetromino(&next);

	SetTetromino[/*RandomIndex()*/ 6](&current);
	current.index.x = 8;
	current.index.y = 5;

	while (CheckTetromino(&current))
	{
		PrintTetromino(&current);
		WaitForInput();
		EraseTetromino(&current);
		// ++current.index.y;
	}

	GetAnyInput();
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
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + (j * 2), padding.y + tetromino->index.y + i });
				break;

			case NINETY:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + (i * 2), padding.y + tetromino->index.y + (tetromino->dimensions.x - 1) - j });
				break;

			case ONE_EIGHTY:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + ((tetromino->dimensions.x - (j + 1)) * 2), padding.y + tetromino->index.y + (tetromino->dimensions.y - (i + 1)) });
				break;

			case TWO_SEVENTTY:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + ((tetromino->dimensions.y - (i + 1)) * 2), padding.y + tetromino->index.y + j });
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

static void EraseTetromino(struct Tetromino* tetromino)
{
	for (int i = 0; i < tetromino->dimensions.y; i++)
	{
		for (int j = 0; j < tetromino->dimensions.x; j++)
		{
			switch (tetromino->angle)
			{
			case ZERO:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + (j * 2), padding.y + tetromino->index.y + i });
				break;

			case NINETY:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + (i * 2), padding.y + tetromino->index.y + (tetromino->dimensions.x - 1) - j });
				break;

			case ONE_EIGHTY:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + ((tetromino->dimensions.x - (j + 1)) * 2), padding.y + tetromino->index.y + (tetromino->dimensions.y - (i + 1)) });
				break;

			case TWO_SEVENTTY:
				Goto((cordinates) { padding.x + 2 + tetromino->index.x + ((tetromino->dimensions.y - (i + 1)) * 2), padding.y + tetromino->index.y + j });
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
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + tetromino->index.x + (j * 2) - 1, padding.y + tetromino->index.y + i - 1},
					&count);
				break;

			case NINETY:
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + tetromino->index.x + (i * 2) - 1, padding.y + tetromino->index.y + (tetromino->dimensions.x - 1) - j - 1},
					&count);
				break;

			case ONE_EIGHTY:
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + tetromino->index.x + ((tetromino->dimensions.x - (j + 1)) * 2) - 1, padding.y + tetromino->index.y + (tetromino->dimensions.y - (i + 1)) - 1},
					&count);
				break;

			case TWO_SEVENTTY:
				ReadConsoleOutputCharacter(
					GetStdHandle(STD_OUTPUT_HANDLE),
					&c,
					1,
					(COORD) { padding.x + 2 + tetromino->index.x + ((tetromino->dimensions.y - (i + 1)) * 2) - 1, padding.y + tetromino->index.y + j - 1},
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

// I am guilty of using chatgbt here
// I am really sorry. I tried to look for solution
// everywhere :(
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

				EraseTetromino(&current);
				previous = current;

				if (current.angle == ZERO)
				{
					current.angle = TWO_SEVENTTY;
				}
				else
				{
					--current.angle;
				}
				
				RotateTetromino();
				PrintTetromino(&current);

				break;

			case 80:
				// Down key is pressed
				// soft drop

				return;

			case 77:
				// Right key is pressed

				previous = current;
				EraseTetromino(&current);
				current.index.x += 2;
				if (!CheckTetromino(&current))
				{
					current = previous;
				}
				PrintTetromino(&current);

				break;

			case 75:
				// Left key is pressed

				previous = current;
				EraseTetromino(&current);
				current.index.x -= 2;
				if (!CheckTetromino(&current))
				{
					current = previous;
				}
				PrintTetromino(&current);

				break;

			case ' ':
				// Space is pressed
				break;

			case 'z':
				// 'z' is pressed
				// flips forward
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

				ClockwiseRotate();
				RotateTetromino();



				PrintTetromino(&current);

				break;
			}
		}
	}
}

inline static void RotateTetromino(void)
{
	current.index.x = current.index.x - current.rotation_offset[previous.angle].x + current.rotation_offset[current.angle].x;
	current.index.y = current.index.y - current.rotation_offset[previous.angle].y + current.rotation_offset[current.angle].y;
}

inline static void ClockwiseRotate(void)
{
	RotateTetromino();
}

inline static void CounterClockwiseRotate(void)
{
	RotateTetromino();
}