//	IMPORTANT: This program uses unicode wide characters. To compile use the ncursesw library
//	E.g.
//	gcc snake.c -o snake -lncursesw

//	LESS IMPORTANT: to cheat, press 'a', to quit press 'X'

//	Defines
#define _XOPEN_SOURCE_EXTENDED 1
#define WINDOW_SIZE 20
#define SPEED 1
#define SECOND  1000000

//	Includes
#include <ncursesw/curses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		// for time()
#include <unistd.h>		// for usleep()
#include <stdlib.h>		// for atoi()

//	Struct Vector (2d vector)
struct Vector {
	int x;
	int y;
};

//	Vector constants
const struct Vector UP = {0, -1};
const struct Vector DOWN = {0, 1};
const struct Vector RIGHT = {1, 0};
const struct Vector LEFT = {-1, 0};
const struct Vector ZERO = {0, 0};

//	Global variables
struct Vector* snake;
struct Vector head = {10, 10};
struct Vector snakeDir;
int snakeLength = 3;
int alive = 1;
struct Vector trophy;
int trophyValue;
int lastTime;
int wiggle = 0;

//	Function prototypes
void setup();
void borderw(
		wchar_t*, 
		wchar_t*, 
		wchar_t*, 
		wchar_t*, 
		wchar_t*, 
		wchar_t*, 
		wchar_t*, 
		wchar_t* 
		);
void borderwdefault();
void setupSnake();
void readInput();
void updateSnake();
void checkCollisions();
int vecEq(struct Vector, struct Vector);
struct Vector vecAdd(struct Vector, struct Vector);
struct Vector vecScale(struct Vector, float);
struct Vector vecSub(struct Vector, struct Vector);
void updateTrophy();
void placeTrophy();
void drawSnake();
int checkAround();
void drawTrophy();
void winScreen();
void loseScreen();

//	int main(): Main function
int main() {
	setup();

	float timeScale;
	float lengthScale = 10;
	struct Vector lastSeg;
	//	Game Loop
	while(alive && snakeLength < (LINES + COLS)) {
		timeScale = 0.2;
		//timeScale *= snakeDir.x ? 0.5 : 1;
		timeScale *= lengthScale / snakeLength;

		usleep(SECOND * timeScale);
		
		//	Erase last segment of snake
		lastSeg = snake[snakeLength - 1];
		if(!vecEq(lastSeg, ZERO)) {
			move(lastSeg.y, lastSeg.x);
			addch(' ');
		}

		readInput();
		updateSnake();
		//readInput();
		drawSnake();
		checkCollisions();
		updateTrophy();
		drawTrophy();
		refresh();
	}
	
	//	Win and lose screens
	borderwdefault();	
	if(alive) {
		winScreen();
	}
	else {
		loseScreen();
	}
	refresh();
	sleep(5);

	endwin();
}

//	void setup(): Sets up game (screen, snake, etc.)
void setup() {
	setlocale(LC_CTYPE, "");		//	Use local character set (wide chars)
	initscr();						//	Initialize teh screen
	noecho();						//	Disable printing input on the screen
	curs_set(0);					//	Disable displaying the cursor
	keypad(stdscr, 1);				//	Enable arrow key input
	nodelay(stdscr, 1);				//	Makes input non-blockin
	srand((unsigned) time(NULL));	//	Initialize rng
	
	setupSnake();
	placeTrophy();

	borderwdefault();
}

//	Sterling Badner
//	void borderw((wchar_t*)8x): Replacement for border using wide characters
void borderw(
		wchar_t* ls,
		wchar_t* rs,
		wchar_t* ts,
		wchar_t* bs,
		wchar_t* tl,
		wchar_t* tr,
		wchar_t* bl,
		wchar_t* br
		) {
	//	Corners
	move(0, 0);
	addwstr(tl);
	move(0, COLS - 1);
	addwstr(tr);
	move(LINES - 1, 0);
	addwstr(bl);
	move(LINES - 1, COLS - 1);
	addwstr(br);
		
	//	Left
	for(int i = 1; i < LINES - 1; i++) {
		move(i, 0);
		addwstr(ls);
	}
	
	//	Right
	for(int i = 1; i < LINES - 1; i++) {
		move(i, COLS - 1);
		addwstr(rs);
	}

	//	Top
	for(int i = 1; i < COLS - 1; i++) {
		move(0, i);
		addwstr(ts);
	}

	//	Bottom
	for(int i = 1; i < COLS - 1; i++) {
		move(LINES - 1, i);
		addwstr(bs);
	}
}

//	Sterling Badner
//	void borderwdefault(): Wrapper for borderw using double pipe characters
void borderwdefault() {
	//	Default border characters
	wchar_t* ls = L"║";
	wchar_t* rs = L"║";
	wchar_t* ts = L"═";
	wchar_t* bs = L"═";
	wchar_t* tl = L"╔";
	wchar_t* tr = L"╗";
	wchar_t* bl = L"╚";
	wchar_t* br = L"╝";
	
	borderw(ls, rs, ts, bs, tl, tr, bl, br);
}

//	void setupSnake(): Sets up snake
void setupSnake() {
	//	Initialize head
	head.x = COLS / 2;
	head.y = LINES / 2;

	//	Initialize tail
	int length = LINES + COLS;
	snake = (struct Vector*)malloc(sizeof(struct Vector) * length);
	for(int i = 1; i < length; i++) {
		snake[i] = ZERO;
	}
	
	snake[0] = head;

	//	Randomly pick direction
	switch(rand() % 4) {
		case 0:
			snakeDir = UP;
			break;
		case 1:
			snakeDir = DOWN;
			break;
		case 2:
			snakeDir = RIGHT;
			break;
		case 3:
			snakeDir = LEFT;
			break;
	}
}

//	void placeTrophy(): Gives the trophy a random position and value
void placeTrophy() {
	trophy.x = (rand() % (COLS - 2)) + 1;
	trophy.y = (rand() % (LINES - 2)) + 1;
	trophyValue = (rand() % 9) + 1;
	lastTime = time(NULL);
	
	//	If the trophy is in the snake, respawn it
	for(int i = 0; i < snakeLength; i++) {
		if(vecEq(trophy, snake[i])) {
			return placeTrophy();
		}
	}

	//	If the trophy is too far, respawn it
	struct Vector diff = vecSub(trophy, head);
	int dist = abs(diff.x) + abs(diff.y);
	if(dist > snakeLength * (snakeLength / 3) * trophyValue) {
		placeTrophy();
	}
}

//	void readInput(): Handles input
void readInput() {
	int ch = getch();

	//	Select direction from ch
	switch(ch) {
		//	Up
		case KEY_UP:
			snakeDir = UP;
			break;
		//	Down
		case KEY_DOWN:
			snakeDir = DOWN;
			break;
		//	Right
		case KEY_RIGHT:
			snakeDir = RIGHT;
			break;
		//	Left
		case KEY_LEFT:
			snakeDir = LEFT;
			break;
		//	Increment length on 'a' (for testing)
		case 'a':
			snakeLength++;
			break;
		//	Quit on 'X'
		case 'X':
			endwin();
			exit(0);
	}
}

//	void updateSnake(): Updates the snakes body and head
void updateSnake() {
	//	Loop through the snake and move every segment
	for(int i = snakeLength - 1; i > 0; i--) {
		snake[i] = snake[i - 1];
	}

	//	Move the head
	head.x += snakeDir.x;
	head.y += snakeDir.y;
	
	snake[0] = head;
}

//	Sterling Badner
//	void drawSnake(): Draws the snake 
void drawSnake() {
	//	Draw snake
	struct Vector last, curr, next;
	last = ZERO;
	int around;
	wiggle = (wiggle + 1) % 2;	// Wiggle the tail

	//	Loop through segments
	for(int i = snakeLength; i >= 0; i--) {	
		curr = snake[i];
		if(vecEq(curr, ZERO)) {
			continue;
		}

		next = snake[i - 1];
		if(vecEq(next, ZERO)) {
			next = snake[0];
		}		

		around = 0;
		last = vecSub(curr, last);
		next = vecSub(curr, next);

		around |= checkAround(last);
		around |= checkAround(next);		

		move(curr.y, curr.x);
		
		last = curr;

		//	Switch case for snake segments
		switch(around) {
			case(0b0001):
				addwstr(wiggle % 2 ? L"╓" : L"╖");
				break;
			case(0b0010):
				addwstr(wiggle % 2 ? L"╜" : L"╙");
				break;
			case(0b0100):
				addwstr(wiggle % 2 ? L"╕" : L"╛");
				break;
			case(0b1000):
				addwstr(wiggle % 2 ? L"╘" : L"╒");
				break;

			case(0b0011):
				addwstr(L"║");
				break;
			case(0b0101):
				addwstr(L"╗");
				break;
			case(0b0110):
				addwstr(L"╝");
				break;
			case(0b1001):
				addwstr(L"╔");
				break;
			case(0b1010):
				addwstr(L"╚");
				break;
			case(0b1100):
				addwstr(L"═");
				break;
		}

	}

	//	Draw head
	move(head.y, head.x);
	if(snakeDir.x) {
		addch('8');
	}

	else {
		addwstr(L"∞");
	}
}

//	Sterling Badner
//	int checkAround(): Checks which direction a vector faces
	//	Return values
	//		UP: 1
	//		DOWN: 2
	//		RIGHT: 4
	//		LEFT: 8
	//		OTHER: 0
	//	(For use with a bitField)
int checkAround(struct Vector vec) {
	if(vecEq(vec, UP)) {
		return 1;
	}
	if(vecEq(vec, DOWN)) {
		return 2;
	}
	if(vecEq(vec, RIGHT)) {
		return 4;
	}
	if(vecEq(vec, LEFT)) {
		return 8;
	}
	return 0;
}

//	void checkCollisions(): detects collisions with the wall or the tail
void checkCollisions() {
	//	Wall collision
	if(
		head.x < 1 ||
		head.x > COLS - 2 ||
		head.y < 1 ||
		head.y > LINES - 2
	) {
		alive = 0;
	}

	//	Tail collision
	for(int i = 1; i < snakeLength - 1; i++) {
		if(vecEq(snake[i], head)) {
			alive = 0;
		}
	}

	//	Trophy collision
	if(vecEq(head, trophy)) {
		snakeLength += trophyValue;
		placeTrophy();
	}
}

// Vector Functions
//	int vecEq((struct Vector)x2): checks equality of two vectors
int vecEq(struct Vector vec1, struct Vector vec2) {
	return vec1.x == vec2.x && vec1.y == vec2.y;
}

//	struct Vector vecAdd((struct Vector)x2): Adds two vectors
struct Vector vecAdd(struct Vector vec1, struct Vector vec2) {
	struct Vector ret;
	ret.x = vec1.x + vec2.x;
	ret.y = vec1.y + vec2.y;
	return ret;
}

//	struct Vector vecScale(stuct Vector, int): Scales a vector
struct Vector vecScale(struct Vector vec, float scalar) {
	struct Vector ret;
	ret.x = (int)(vec.x * scalar);
	ret.y = (int)(vec.y * scalar);
	return ret;
}

//	struct Vector vecSub((struct Vector, struct Vector)x2): Subs two vectors
struct Vector vecSub(struct Vector vec1, struct Vector vec2) {
	return vecAdd(vec1, vecScale(vec2, -1));
}

//	void updateTrophy(): Updates the trophy
void updateTrophy() {
	int now = time(NULL);
	if(now > lastTime) {
		lastTime = now;
		if(trophyValue-- <= 1) {
			move(trophy.y, trophy.x);
			addch(' ');
			placeTrophy();
		}
	}
}

//	void drawTrophy(): Draws the trophy
void drawTrophy() {
	move(trophy.y, trophy.x);
	addch(trophyValue + '0');
}

//	void winScreen(): Displays a win screen
void winScreen() {
	clear();
	move((LINES / 2) - 1, (COLS / 2) - 6);
	addwstr(L"╔══════════╗");
	move((LINES / 2), (COLS / 2) - 6);
	addwstr(L"║ You Win! ║");
	move((LINES / 2) + 1, (COLS / 2) - 6);
	addwstr(L"╚══════════╝");
}

//	void loseScreen(): Displays a lose screen
void loseScreen() {
	clear();
	move((LINES / 2) - 1, (COLS / 2) - 6);
	addwstr(L"╔═══════════╗");
	move((LINES / 2), (COLS / 2) - 6);
	addwstr(L"║ You Lose! ║");
	move((LINES / 2) + 1, (COLS / 2) - 6);
	addwstr(L"╚═══════════╝");
}
