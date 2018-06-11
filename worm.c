/**
 * Based on the ncurses tutorials at http://www.paulgriffiths.net/program/c/curses.php
 */

#include <curses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "scheduler.h"
#include "util.h"

// Defines used to track the worm direction
#define DIR_NORTH 0
#define DIR_EAST 1
#define DIR_SOUTH 2
#define DIR_WEST 3

// Game parameters
#define INIT_WORM_LENGTH 4
#define WORM_HORIZONTAL_INTERVAL 200
#define WORM_VERTICAL_INTERVAL 300
#define BOARD_WIDTH 50
#define BOARD_HEIGHT 25

/**
 * In-memory representation of the game board
 * Zero represents an empty cell
 * Positive numbers represent worm cells (which count up at each time step until they reach worm_length)
 * Negative numbers represent apple cells (which count up at each time step)
 */
int board[BOARD_HEIGHT][BOARD_WIDTH];

// Worm parameters
int worm_dir = DIR_NORTH;
int worm_length = INIT_WORM_LENGTH;

// Apple parameters
int apple_age = 120;

/**
 * Convert a board row number to a screen position
 * \param   row   The board row number to convert
 * \return        A corresponding row number for the ncurses screen
 */
int screen_row(int row) {
  return 2 + row;
}

/**
 * Convert a board column number to a screen position
 * \param   col   The board column number to convert
 * \return        A corresponding column number for the ncurses screen
 */
int screen_col(int col) {
  return 2 + col;
}

// Initialize the board display by printing the title and edges
void init_display() {
  // Print Title Line
  move(screen_row(-2), screen_col(BOARD_WIDTH/2 - 5));
  addch(ACS_DIAMOND);
  addch(ACS_DIAMOND);
  printw(" Worm! ");
  addch(ACS_DIAMOND);
  addch(ACS_DIAMOND);
  
  // Print corners
  mvaddch(screen_row(-1), screen_col(-1), ACS_ULCORNER);
  mvaddch(screen_row(-1), screen_col(BOARD_WIDTH), ACS_URCORNER);
  mvaddch(screen_row(BOARD_HEIGHT), screen_col(-1), ACS_LLCORNER);
  mvaddch(screen_row(BOARD_HEIGHT), screen_col(BOARD_WIDTH), ACS_LRCORNER);
  
  // Print top and bottom edges
  for(int col=0; col<BOARD_WIDTH; col++) {
    mvaddch(screen_row(-1), screen_col(col), ACS_HLINE);
    mvaddch(screen_row(BOARD_HEIGHT), screen_col(col), ACS_HLINE);
  }
  
  // Print left and right edges
  for(int row=0; row<BOARD_HEIGHT; row++) {
    mvaddch(screen_row(row), screen_col(-1), ACS_VLINE);
    mvaddch(screen_row(row), screen_col(BOARD_WIDTH), ACS_VLINE);
  }
  
  refresh();
}

// Show a game over message, wait for a key press, then stop the game scheduler
void end_game() {
  mvprintw(screen_row(BOARD_HEIGHT/2)-1, screen_col(BOARD_WIDTH/2)-6, "            ");
  mvprintw(screen_row(BOARD_HEIGHT/2),   screen_col(BOARD_WIDTH/2)-6, " Game Over! ");
  mvprintw(screen_row(BOARD_HEIGHT/2)+1, screen_col(BOARD_WIDTH/2)-6, "            ");
  refresh();
  timeout(-1);
  getch();
  stop_scheduler();
}

// Draw the actual game board. Board state is stored in a single 2D integer array.
void draw_board() {
  // Loop over cells of the game board
  for(int r=0; r<BOARD_HEIGHT; r++) {
    for(int c=0; c<BOARD_WIDTH; c++) {
      if(board[r][c] == 0) {  // Draw blank spaces
        mvaddch(screen_row(r), screen_col(c), ' ');
      } else if(board[r][c] > 0) {  // Draw worm
        mvaddch(screen_row(r), screen_col(c), ACS_CKBOARD);
      } else {  // Draw apple spinner thing
        char spinner_chars[] = {'|', '/', '-', '\\'};
        mvaddch(screen_row(r), screen_col(c), spinner_chars[abs(board[r][c] % 4)]);
      }
    }
  }
  
  // Draw the score
  mvprintw(screen_row(-2), screen_col(BOARD_WIDTH-9), "Score %03d\r", worm_length-INIT_WORM_LENGTH);
  
  refresh();
}

// Check for keyboard input and respond accordingly
void read_input() {
  int key = getch();
  if(key == KEY_UP && worm_dir != DIR_SOUTH) {
    worm_dir = DIR_NORTH;
  } else if(key == KEY_RIGHT && worm_dir != DIR_WEST) {
    worm_dir = DIR_EAST;
  } else if(key == KEY_DOWN && worm_dir != DIR_NORTH) {
    worm_dir = DIR_SOUTH;
  } else if(key == KEY_LEFT && worm_dir != DIR_EAST) {
    worm_dir = DIR_WEST;
  } else if(key == 'q') {
    stop_scheduler();
    return;
  }
}

// Update the age of each segment of the worm and move the worm into its next position
void update_worm() {
  int worm_row;
  int worm_col;
  
  // "Age" each existing segment of the worm
  for(int r=0; r<BOARD_HEIGHT; r++) {
    for(int c=0; c<BOARD_WIDTH; c++) {
      if(board[r][c] == 1) {  // Found the head of the worm. Save position
        worm_row = r;
        worm_col = c;
      }
      
      // Add 1 to the age of the worm segment
      if(board[r][c] > 0) {
        board[r][c]++;
        
        // Remove the worm segment if it is too old
        if(board[r][c] > worm_length) {
          board[r][c] = 0;
        }
      }
    }
  }
  
  // Move the worm into a new space
  if(worm_dir == DIR_NORTH) {
    worm_row--;
  } else if(worm_dir == DIR_SOUTH) {
    worm_row++;
  } else if(worm_dir == DIR_EAST) {
    worm_col++;
  } else if(worm_dir == DIR_WEST) {
    worm_col--;
  }
  
  // Check for edge collisions
  if(worm_row < 0 || worm_row >= BOARD_HEIGHT || worm_col < 0 || worm_col >= BOARD_WIDTH) {
    end_game();
    return;
  }
  
  // Check for worm collisions
  if(board[worm_row][worm_col] > 0) {
    end_game();
    return;
  }
  
  // Check for apple collisions
  if(board[worm_row][worm_col] < 0) {
    // Worm gets longer
    worm_length++;
  }
  
  // Add the worm's new position
  board[worm_row][worm_col] = 1;
  
  // Update the worm movement speed to deal with rectangular cursors
  if(worm_dir == DIR_NORTH || worm_dir == DIR_SOUTH) {
    update_job_interval(WORM_VERTICAL_INTERVAL);
  } else {
    update_job_interval(WORM_HORIZONTAL_INTERVAL);
  }
}

// Add to the apple numbers to control their duration and animation
void update_apples() {
  // "Age" each apple
  for(int r=0; r<BOARD_HEIGHT; r++) {
    for(int c=0; c<BOARD_WIDTH; c++) {
      if(board[r][c] < 0) {  // Add one to each apple cell
        board[r][c]++;
      }
    }
  }
}

// Pick a random location and generate an apple there
void generate_apple() {
  // Repeatedly try to insert an apple at a random empty cell
  while(1) {
    int r = rand() % BOARD_HEIGHT;
    int c = rand() % BOARD_WIDTH;
    
    // If the cell is empty, add an apple
    if(board[r][c] == 0) {
      // Pick a random age between apple_age/2 and apple_age*1.5
      // Negative numbers represent apples, so negate the whole value
      board[r][c] = -((rand() % apple_age) + apple_age / 2);
      return;
    }
  }
}

// Entry point: Set up the game, create jobs, then run the scheduler
int main(void) {
  WINDOW* mainwin = initscr();
  if(mainwin == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(2);
  }
  
  // Seed random number generator with the time in milliseconds
  srand(time_ms());
  
  noecho();               // Don't print keys when pressed
  keypad(mainwin, true);  // Support arrow keys
  timeout(0);             // Non-blocking keyboard access
  
  init_display();
  
  // Zero out the board contents
  memset(board, 0, BOARD_WIDTH*BOARD_HEIGHT*sizeof(int));
  
  // Put the worm at the middle of the board
  board[BOARD_HEIGHT/2][BOARD_WIDTH/2] = 1;
  
  // Create a job to update the worm every 200ms
  add_job(update_worm, 200);
  
  // Create a job to draw the board every 33ms
  add_job(draw_board, 33);
  
  // Create a job to read user input every 150ms
  add_job(read_input, 150);
  
  // Create a job to update apples every 120ms
  add_job(update_apples, 120);
  
  // Create a job to generate new apples every 2 seconds
  add_job(generate_apple, 2000);
  
  // Generate a few apples immediately
  for(int i=0; i<5; i++) {
    generate_apple();
  }
  
  // Run the task queue
  run_scheduler();
  
  // Clean up window
  delwin(mainwin);
  endwin();

  return 0;
}
