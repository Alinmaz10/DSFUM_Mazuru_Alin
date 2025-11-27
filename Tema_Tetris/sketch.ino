#include "LedControl.h"
#include <avr/pgmspace.h> 

const int gameOverNotes[] PROGMEM = { 
  440, 392, 349, 330, 294, 262, 196, 131 
};

const int gameOverDurations[] PROGMEM = { 
  200, 200, 200, 200, 200, 200, 400, 800 
};

const int melodyLength = 8; 

const int PIN_DATA_IN = 11;
const int PIN_CLK     = 13;
const int PIN_LOAD    = 10;
const int PIN_BUZZER  = 49;

const int BTN_ROTATE = 41; 
const int BTN_DOWN   = 47; 
const int BTN_LEFT   = 43; 
const int BTN_RIGHT  = 45; 

LedControl lc = LedControl(PIN_DATA_IN, PIN_CLK, PIN_LOAD, 8);

int board[32][16];       
int currentShape[4][4];  
int pieceX = 6;
int pieceY = -3;

unsigned long lastDropTime = 0;
int dropInterval = 600;  

byte shapes[7][4][4] = {
  {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}, 
  {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, 
  {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, 
  {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, 
  {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, 
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}  
};

void setup() {
  pinMode(BTN_ROTATE, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_RIGHT, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  for(int i=0; i<8; i++) {
    lc.shutdown(i, false);
    lc.setIntensity(i, 4); 
    lc.clearDisplay(i);
  }
  
  for(int y=0; y<32; y++)
    for(int x=0; x<16; x++)
      board[y][x] = 0;

  spawnPiece();
  
  tone(PIN_BUZZER, 1000, 100); delay(100);
  tone(PIN_BUZZER, 1500, 200);
}

void drawPixel(int x, int y, bool state) {
  if (x < 0 || x > 15 || y < 0 || y > 31) return;
  int device, row, col;

  if (x < 8) { 
    int matrixIndex = 3 - (y / 8); 
    device = 4 + matrixIndex; 
    row = 7 - x;         
    col = 7 - (y % 8); 
  } else { 
    int matrixIndex = y / 8;
    device = 0 + matrixIndex; 
    row = (x - 8); 
    col = y % 8;   
  }
  lc.setLed(device, row, col, state);
}

void drawPiece(int pX, int pY, int shape[4][4], bool on) {
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if(shape[r][c] == 1) {
        drawPixel(pX + c, pY + r, on);
      }
    }
  }
}

void redrawWholeBoard() {
  for(int y=0; y<32; y++) {
    for(int x=0; x<16; x++) {
      drawPixel(x, y, board[y][x]);
    }
  }
  drawPiece(pieceX, pieceY, currentShape, true);
}

bool isValidMove(int tempX, int tempY, int tempShape[4][4]) {
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if(tempShape[r][c] == 1) {
        int newX = tempX + c;
        int newY = tempY + r;
        if (newX < 0 || newX > 15 || newY > 31) return false;
        if (newY >= 0 && board[newY][newX] == 1) return false;
      }
    }
  }
  return true;
}

void playLineClearSound() {
  tone(PIN_BUZZER, 523, 100); delay(100); 
  tone(PIN_BUZZER, 659, 100); delay(100); 
  tone(PIN_BUZZER, 784, 150); delay(150); 
}

void playGameOverMelody() {

  noTone(PIN_BUZZER);

  for (int i = 0; i < melodyLength; i++) {
   
    int note = pgm_read_word(&gameOverNotes[i]);
    int duration = pgm_read_word(&gameOverDurations[i]);
    tone(PIN_BUZZER, note, duration);
    delay(duration * 1.30); 
    noTone(PIN_BUZZER);
  }
}
void checkLines() {
  for(int y=0; y<32; y++) {
    bool full = true;
    for(int x=0; x<16; x++) {
      if(board[y][x] == 0) full = false;
    }
    if(full) {
      playLineClearSound();
      
      for(int i=y; i>0; i--) {
        for(int x=0; x<16; x++) board[i][x] = board[i-1][x];
      }
      for(int x=0; x<16; x++) board[0][x] = 0;
      
      redrawWholeBoard();
      y--; 
    }
  }
}

void lockPiece() {

  bool stuckOutside = false;
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if(currentShape[r][c] == 1) {

        if (pieceY + r < 0) {
          stuckOutside = true;
        }
      }
    }
  }

  if (stuckOutside) {
    playGameOverMelody();
    delay(1000);
    setup(); 
    return;  
  }

  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      if(currentShape[r][c] == 1) {
        int finalY = pieceY + r;
        int finalX = pieceX + c;
        
        if(finalY >= 0 && finalY <= 31) {
          board[finalY][finalX] = 1;
        }
      }
    }
  }
  
  checkLines();
  spawnPiece();
  
  if(!isValidMove(pieceX, pieceY, currentShape)) {
    playGameOverMelody();
    delay(1000);
    setup(); 
  }
}

void spawnPiece() {
  int id = random(0, 7);
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      currentShape[r][c] = shapes[id][r][c];
    }
  }
  pieceX = 6;
  pieceY = -3;
}

void rotatePiece() {
  int temp[4][4];
  for(int r=0; r<4; r++) {
    for(int c=0; c<4; c++) {
      temp[c][3-r] = currentShape[r][c];
    }
  }
  
  drawPiece(pieceX, pieceY, currentShape, false); 
  
  if(isValidMove(pieceX, pieceY, temp)) {
    for(int r=0; r<4; r++) {
      for(int c=0; c<4; c++) {
        currentShape[r][c] = temp[r][c];
      }
    }
    tone(PIN_BUZZER, 600, 20);
  }
  
  drawPiece(pieceX, pieceY, currentShape, true); 
}


void loop() {
 
  if (digitalRead(BTN_LEFT) == HIGH) {
    drawPiece(pieceX, pieceY, currentShape, false); 
    if(isValidMove(pieceX - 1, pieceY, currentShape)) {
      pieceX--;
      tone(PIN_BUZZER, 400, 20);
    }
    drawPiece(pieceX, pieceY, currentShape, true);
    delay(100); 
  }
  
  if (digitalRead(BTN_RIGHT) == HIGH) {
    drawPiece(pieceX, pieceY, currentShape, false);
    if(isValidMove(pieceX + 1, pieceY, currentShape)) {
      pieceX++;
      tone(PIN_BUZZER, 400, 20);
    }
    drawPiece(pieceX, pieceY, currentShape, true);
    delay(100);
  }

  if (digitalRead(BTN_ROTATE) == HIGH) {
    rotatePiece();
    delay(200); 
  }

  int speed = dropInterval;
  if (digitalRead(BTN_DOWN) == HIGH) speed = 50; 

  if (millis() - lastDropTime > speed) {
    drawPiece(pieceX, pieceY, currentShape, false); 
    
    if(isValidMove(pieceX, pieceY + 1, currentShape)) {
      pieceY++;
      drawPiece(pieceX, pieceY, currentShape, true); 
    } else {
      drawPiece(pieceX, pieceY, currentShape, true); 
      lockPiece();
    }
    
    lastDropTime = millis();
  }
}
