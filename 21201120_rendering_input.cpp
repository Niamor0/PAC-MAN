# Folder: teammate-part/
# ├─ include/
# │  ├─ state.hpp
# │  ├─ render.hpp
# │  └─ input.hpp
# ├─ src/
# │  ├─ render.cpp
# │  ├─ input.cpp
# │  └─ main.cpp
# ├─ CMakeLists.txt
# ├─ .gitignore
# └─ README.md

// =====================================
// File: include/state.hpp
// =====================================
#pragma once
#include <chrono>

// High-level UI/app states (menus vs gameplay)
enum class GameState { MENU, PLAYING, POSTGAME_MENU, QUIT_CONFIRM_MENU };

// Global (single-definition in src/main.cpp)
extern GameState gState;

// Hover flags for menus
extern int hoverPlay, hoverExit;                 // main menu
extern int hoverPG_PlayAgain, hoverPG_Quit;      // post-game menu
extern int hoverQC_PlayAgain, hoverQC_Quit;      // quit confirm menu (Yes/No)


// =====================================
// File: include/render.hpp
// =====================================
#pragma once

// Router for display based on gState
void renderDisplay();

// View/projection
void reshapeView(int w,int h);

// Individual screens (optional external use)
void renderMenu();
void renderGame();
void renderPostGameMenu();
void renderQuitConfirmMenu();
void renderGameOverOverlay();


// =====================================
// File: include/input.hpp
// =====================================
#pragma once
#include <GL/glut.h>

void onSpecialKey(int key,int x,int y);
void onKeyDown(unsigned char k,int x,int y);
void onMouseClick(int button,int state,int x,int y);
void onPassiveMotion(int x,int y);


// =====================================
// File: src/render.cpp
// =====================================
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include "state.hpp"

// Pull in core pieces from your part
#include "../your-part/include/config.hpp"
#include "../your-part/include/types.hpp"
#include "../your-part/include/maze.hpp"
#include "../your-part/include/logic.hpp"
#include "../your-part/include/powerups.hpp"
#include "../your-part/include/util.hpp"

// Use the pac namespace for core
using namespace pac;

// ====== local draw utils ======
static void drawTextColor(const char* s,float x,float y,float r,float g,float b,void* font){
  glColor3f(r,g,b); glRasterPos2f(x,y);
  while(*s) glutBitmapCharacter(font,*s++);
}
static void drawQuad(float x0,float y0,float x1,float y1, RGBc c){
  glColor3f(c.r,c.g,c.b); glBegin(GL_QUADS);
  glVertex2f(x0,y0); glVertex2f(x1,y0); glVertex2f(x1,y1); glVertex2f(x0,y1); glEnd();
}
static void drawQuadA(float x0,float y0,float x1,float y1, float r,float g,float b,float a){
  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(r,g,b,a); glBegin(GL_QUADS);
  glVertex2f(x0,y0); glVertex2f(x1,y0); glVertex2f(x1,y1); glVertex2f(x0,y1);
  glEnd(); glDisable(GL_BLEND);
}
static void drawCircle(float cx,float cy,float r, RGBc c){
  glColor3f(c.r,c.g,c.b); glBegin(GL_TRIANGLE_FAN);
  glVertex2f(cx,cy);
  for(int i=0;i<=40;i++){ float a=i*0.157f; glVertex2f(cx+r*std::cos(a), cy+r*std::sin(a)); }
  glEnd();
}

// ===== Watermelon (🍉)
static void drawWatermelon(float cx,float cy){
  const float PI = 3.14159265f; const float R=0.35f; float Rw=R, Rw_in=R-0.05f, Rw_white_in=Rw_in-0.03f, Rf=Rw_white_in;
  glColor3f(1.0f, 0.15f, 0.20f); glBegin(GL_TRIANGLE_FAN); glVertex2f(cx, cy);
  for(int i=0;i<=64;i++){ float a=(PI*i)/64.0f; glVertex2f(cx + Rf*std::cos(a), cy - Rf*std::sin(a)); } glEnd();
  glColor3f(0.98f, 0.98f, 0.98f); glBegin(GL_TRIANGLE_STRIP);
  for(int i=0;i<=64;i++){ float a=(PI*i)/64.0f; glVertex2f(cx + Rw_in*std::cos(a), cy - Rw_in*std::sin(a)); glVertex2f(cx + Rw_white_in*std::cos(a), cy - Rw_white_in*std::sin(a)); } glEnd();
  glColor3f(0.10f, 0.70f, 0.20f); glBegin(GL_TRIANGLE_STRIP);
  for(int i=0;i<=64;i++){ float a=(PI*i)/64.0f; glVertex2f(cx + Rw*std::cos(a), cy - Rw*std::sin(a)); glVertex2f(cx + Rw_in*std::cos(a),  cy - Rw_in*std::sin(a)); } glEnd();
  RGBc seedCol{0.05f,0.05f,0.05f}; float sr=0.03f;
  drawCircle(cx - 0.16f, cy - 0.18f, sr, seedCol);
  drawCircle(cx + 0.00f, cy - 0.22f, sr, seedCol);
  drawCircle(cx + 0.16f, cy - 0.18f, sr, seedCol);
  drawCircle(cx - 0.08f, cy - 0.28f, sr, seedCol);
  drawCircle(cx + 0.08f, cy - 0.28f, sr, seedCol);
}

// ===== Heart (💖)
static void drawHeart(float cx, float cy){
  const float r = 0.18f; const float dx = 0.16f; const float dy = 0.05f; const RGBc heartCol{1.00f, 0.20f, 0.70f};
  drawCircle(cx - dx, cy + dy, r, heartCol);
  drawCircle(cx + dx, cy + dy, r, heartCol);
  glColor3f(heartCol.r, heartCol.g, heartCol.b);
  glBegin(GL_TRIANGLES);
    glVertex2f(cx - (dx + r*0.70f), cy + dy*0.2f);
    glVertex2f(cx + (dx + r*0.70f), cy + dy*0.2f);
    glVertex2f(cx,                  cy - 0.28f);
  glEnd();
  glColor3f(1.0f, 0.85f, 0.95f);
  glBegin(GL_TRIANGLES);
    glVertex2f(cx + 0.05f, cy + 0.20f);
    glVertex2f(cx + 0.12f, cy + 0.18f);
    glVertex2f(cx + 0.08f, cy + 0.25f);
  glEnd();
}

// ===== Pac-Man RIGHT local sprite
static void drawPacRightSpriteLocal(float r){
  const float TWO_PI = 6.283185307f; const float mouthDeg = 58.0f; const float mouth = mouthDeg * 3.14159265f/180;
  glColor3f(PAC_COL.r, PAC_COL.g, PAC_COL.b);
  glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for(int i=0;i<=100;i++){ float a = mouth*0.5f + (TWO_PI - mouth) * (i/100.0f); glVertex2f(r*std::cos(a), r*std::sin(a)); }
  glEnd();
  glColor3f(0.0f,0.0f,0.0f);
  glBegin(GL_TRIANGLES);
    float a0 = -mouth*0.5f, a1 = +mouth*0.5f;
    glVertex2f(0.0f,0.0f); glVertex2f(r*std::cos(a0), r*std::sin(a0)); glVertex2f(r*std::cos(a1), r*std::sin(a1));
  glEnd();
  RGBc eyeW{1,1,1}, eyeB{0.10f,0.65f,1.0f}; float ew=r*0.23f, ep=r*0.12f;
  drawCircle(r*0.20f, r*0.25f, ew, eyeW); drawCircle(r*0.27f, r*0.25f, ep, eyeB);
  RGBc lip{0.85f,0.10f,0.10f}; drawCircle(r*0.88f, -r*0.08f, r*0.06f, lip);
  RGBc bow{0.90f,0.10f,0.10f}; glColor3f(bow.r,bow.g,bow.b); glBegin(GL_TRIANGLES);
    glVertex2f(-r*0.15f, +r*0.60f); glVertex2f(-r*0.55f, +r*0.45f); glVertex2f(-r*0.35f, +r*0.80f);
  glEnd();
  glBegin(GL_TRIANGLES);
    glVertex2f(-r*0.05f, +r*0.62f); glVertex2f(+r*0.25f, +r*0.78f); glVertex2f(+r*0.05f, +r*0.40f);
  glEnd();
  drawCircle(+r*0.02f, +r*0.58f, r*0.10f, bow);
}

// ===== Ghost sprite
static void drawGhostSprite(float cx, float cy, float r, RGBc bodyCol){
  const float PI = 3.14159265f; float w=r*2.0f, h=r*2.2f, halfW=w*0.5f;
  glColor3f(bodyCol.r, bodyCol.g, bodyCol.b);
  glBegin(GL_TRIANGLE_FAN); glVertex2f(cx, cy + h*0.15f);
  for(int i=0;i<=64;i++){ float a = PI*(i/64.0f); glVertex2f(cx + halfW*std::cos(a), cy + h*0.15f + halfW*std::sin(a)); }
  glEnd();
  glBegin(GL_QUADS);
    glVertex2f(cx - halfW, cy - h*0.55f); glVertex2f(cx + halfW, cy - h*0.55f);
    glVertex2f(cx + halfW, cy + h*0.15f); glVertex2f(cx - halfW, cy + h*0.15f);
  glEnd();
  float bumpR = w/6.0f; float startX = cx - halfW + bumpR;
  for(int k=0;k<3;k++){ float bx = startX + k*(2*bumpR);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(bx, cy - h*0.55f);
    for(int i=0;i<=32;i++){ float a = 3.14159265f + 3.14159265f*(i/32.0f); glVertex2f(bx + bumpR*std::cos(a), cy - h*0.55f + bumpR*std::sin(a)); }
    glEnd();
  }
  RGBc eyeW{1,1,1}, eyeB{0.10f,0.65f,1.0f}; float eyeR=r*0.28f, pupilR=r*0.16f;
  float lcx=cx - r*0.35f, rcx=cx + r*0.15f, ey=cy + r*0.25f;
  drawCircle(lcx, ey, eyeR, eyeW); drawCircle(rcx, ey, eyeR, eyeW);
  drawCircle(lcx - r*0.10f, ey, pupilR, eyeB); drawCircle(rcx - r*0.10f, ey, pupilR, eyeB);
}

void renderMaze(){
  glClearColor(BG_COL.r,BG_COL.g,BG_COL.b,1.0f); glClear(GL_COLOR_BUFFER_BIT);
  for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++){
    float x=c,y=(ROWS-1-r);
    if(MAZE[r][c]==WALL||MAZE[r][c]==GATE) drawQuad(x+0.06f,y+0.06f,x+0.94f,y+0.94f,WALL_COL);
    else if(MAZE[r][c]==DOTCELL) drawCircle(x+0.5f,y+0.5f,0.08f,DOT_COL);
  }
}

void renderActors(){
  glPushMatrix();
  glTranslatef(pacman.x, pacman.y, 0.0f);
  float extraFlip = (/* death flip visuals handled by angle in core */ 0.0f);
  extern Runtime RT; // defined in main.cpp
  glRotatef(RT.pacAngleDeg + extraFlip, 0.0f, 0.0f, 1.0f);
  drawPacRightSpriteLocal(pacman.radius * 1.15f);
  glPopMatrix();
  RGBc gc[4]={BLINKY_COL,PINKY_COL,INKY_COL,CLYDE_COL};
  for(int i=0;i<4;i++) drawGhostSprite(ghosts[i].x, ghosts[i].y, ghosts[i].radius * 1.05f, gc[i]);
}

void renderSupers(){ for(int i=0;i<MAX_SUPERS;i++){ if(!supers[i].active) continue; drawWatermelon(cellCenterX(supers[i].c), cellCenterY(supers[i].r)); }}
void renderHeart(){ if(!heart.active) return; drawHeart(cellCenterX(heart.c), cellCenterY(heart.r)); }

void renderHUD(){
  char buf[128];
  using Clock = std::chrono::steady_clock; extern Runtime RT;
  float secs=std::chrono::duration<float>(Clock::now()-RT.tStart).count();
  std::snprintf(buf,sizeof(buf),"SCORE:%d  LIVES:%d  TIME:%.1fs", RT.score, RT.lives, secs);
  drawTextColor(buf,0.6f,ROWS+0.3f,HUD_COL.r,HUD_COL.g,HUD_COL.b,GLUT_BITMAP_9_BY_15);
  if(RT.paused && !RT.winGame && !RT.gameOver) drawTextColor("PAUSED",8,10,1,1,1,GLUT_BITMAP_HELVETICA_18);
  if(RT.winGame)  drawTextColor("YOU WIN!",8,10,1,1,0,GLUT_BITMAP_HELVETICA_18);
}

// ===== BIG GAME OVER overlay
static float strokeTextWidth(const char* s, void* font=GLUT_STROKE_MONO_ROMAN){ float w=0.0f; for(const char* p=s; *p; ++p) w += glutStrokeWidth(font, *p); return w; }
void renderGameOverOverlay(){
  const char* msg = "GAME OVER"; float padX=2.0f, padY=1.2f; float x0=padX, x1=COLS-padX; float yMid = ROWS * 0.55f; float y0 = yMid - padY, y1 = yMid + padY;
  drawQuadA(x0, y0, x1, y1, 0.0f, 0.0f, 0.0f, 0.55f);
  void* font = GLUT_STROKE_MONO_ROMAN; float wUnits = strokeTextWidth(msg, font); float targetW = (x1 - x0) * 0.88f; float scale = (wUnits>0.0f)?(targetW/wUnits):0.01f; const float STROKE_H = 119.05f;
  float textW_world = wUnits * scale; float textH_world = STROKE_H * scale; float tx = (COLS - textW_world) * 0.5f; float ty = yMid - textH_world * 0.5f;
  glPushMatrix(); glTranslatef(tx + 0.05f, ty - 0.05f, 0.0f); glScalef(scale, scale, 1.0f); glLineWidth(6.0f); glColor3f(0,0,0); for(const char* p = msg; *p; ++p) glutStrokeCharacter(font, *p); glPopMatrix();
  glPushMatrix(); glTranslatef(tx, ty, 0.0f); glScalef(scale, scale, 1.0f); glLineWidth(5.0f); glColor3f(1,1,1); for(const char* p = msg; *p; ++p) glutStrokeCharacter(font, *p); glPopMatrix();
}

// ===== Menus
void renderMenu(){
  glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
  drawTextColor("MAIN MENU", 6.0f, 13.0f, 0.0f, 1.0f, 1.0f, GLUT_BITMAP_TIMES_ROMAN_24);
  if(hoverPlay) drawTextColor("Play PAC-MAN", 6.0f, 9.0f, 1.0f, 0.3f, 0.3f, GLUT_BITMAP_HELVETICA_18);
  else          drawTextColor("Play PAC-MAN", 6.0f, 9.0f, 1.0f, 0.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
  if(hoverExit) drawTextColor("Exit the Game", 6.0f, 7.0f, 1.0f, 1.0f, 0.6f, GLUT_BITMAP_HELVETICA_18);
  else          drawTextColor("Exit the Game", 6.0f, 7.0f, 1.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
  glutSwapBuffers();
}

void renderPostGameMenu(){
  glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
  drawQuadA(2.0f, ROWS*0.72f, COLS-2.0f, ROWS*0.72f+1.4f, 0,0,0,0.55f);
  drawTextColor("Select an option", 6.2f, 13.0f, 1,1,1, GLUT_BITMAP_TIMES_ROMAN_24);
  float y1 = 9.0f, y2 = 7.0f; const char* o1="Play Again"; const char* o2="Quit Game";
  if(hoverPG_PlayAgain) drawTextColor(o1, 7.0f, y1, 1.0f, 0.3f, 0.3f, GLUT_BITMAP_HELVETICA_18); else drawTextColor(o1, 7.0f, y1, 1.0f, 0.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
  if(hoverPG_Quit)      drawTextColor(o2, 7.0f, y2, 1.0f, 1.0f, 0.6f, GLUT_BITMAP_HELVETICA_18); else drawTextColor(o2, 7.0f, y2, 1.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
  glutSwapBuffers();
}

void renderQuitConfirmMenu(){
  glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
  drawQuadA(2.0f, ROWS*0.72f, COLS-2.0f, ROWS*0.72f+1.4f, 0,0,0,0.55f);
  drawTextColor("CONFIRM", 6.6f, 13.0f, 1,1,1, GLUT_BITMAP_TIMES_ROMAN_24);
  float yYes=9.0f, yNo=7.0f;
  if(hoverQC_PlayAgain) drawTextColor("Yes", 7.0f, yYes, 1.0f, 0.3f, 0.3f, GLUT_BITMAP_HELVETICA_18); else drawTextColor("Yes", 7.0f, yYes, 1.0f, 0.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
  if(hoverQC_Quit)      drawTextColor("No",  7.0f, yNo,  1.0f, 1.0f, 0.6f, GLUT_BITMAP_HELVETICA_18); else drawTextColor("No",  7.0f, yNo,  1.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
  glutSwapBuffers();
}

void renderGame(){
  renderMaze();
  renderActors();
  renderSupers();
  renderHeart();
  renderHUD();
  extern Runtime RT; if(RT.gameOver && gState==GameState::PLAYING) renderGameOverOverlay();
  glutSwapBuffers();
}

void renderDisplay(){
  if(gState==GameState::MENU) { renderMenu(); return; }
  if(gState==GameState::PLAYING) { renderGame(); return; }
  if(gState==GameState::POSTGAME_MENU) { renderPostGameMenu(); return; }
  if(gState==GameState::QUIT_CONFIRM_MENU) { renderQuitConfirmMenu(); return; }
}

void reshapeView(int w,int h){
  glViewport(0,0,w,h); glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0,COLS,0,ROWS+1.2); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}


// =====================================
// File: src/input.cpp
// =====================================
#include <GL/glut.h>
#include "state.hpp"
#include "render.hpp"

// Core
#include "../your-part/include/types.hpp"
#include "../your-part/include/config.hpp"
#include "../your-part/include/logic.hpp"

using namespace pac;

// Global from main.cpp
extern Runtime RT;

static inline float windowToWorldY(int y){ int winH=glutGet(GLUT_WINDOW_HEIGHT); return (float)(winH - y) / winH * (ROWS+1.2f); }

void onSpecialKey(int key,int,int){
  if(gState!=GameState::PLAYING || RT.gameOver || RT.winGame || RT.deathActive) return;
  if(key==GLUT_KEY_UP){   pacman.vx=0;  pacman.vy=+1; RT.pacAngleDeg =  90.0f; }
  if(key==GLUT_KEY_DOWN){ pacman.vx=0;  pacman.vy=-1; RT.pacAngleDeg = -90.0f; }
  if(key==GLUT_KEY_LEFT){ pacman.vx=-1; pacman.vy=0;  RT.pacAngleDeg = 180.0f; }
  if(key==GLUT_KEY_RIGHT){pacman.vx=+1; pacman.vy=0;  RT.pacAngleDeg =   0.0f; }
}

void onKeyDown(unsigned char k,int,int){
  if(k==27) exit(0);
  if(k=='p'||k=='P'){ if(gState==GameState::PLAYING && !RT.deathActive && !RT.gameOver && !RT.winGame){ RT.paused=!RT.paused; glutPostRedisplay(); } }
  if(k=='r'||k=='R'){ if(gState==GameState::PLAYING) startNewGame(RT); }
}

void onMouseClick(int button,int state,int x,int y){
  if(button!=GLUT_LEFT_BUTTON || state!=GLUT_DOWN) return; float wy = windowToWorldY(y);
  if(gState==GameState::MENU){ if(wy>8.5f && wy<9.5f){ gState=GameState::PLAYING; startNewGame(RT); return; } if(wy>6.5f && wy<7.5f){ exit(0); } return; }
  if(gState==GameState::POSTGAME_MENU){ if(wy>8.5f && wy<9.5f){ startNewGame(RT); return; } if(wy>6.5f && wy<7.5f){ gState=GameState::QUIT_CONFIRM_MENU; glutPostRedisplay(); return; } return; }
  if(gState==GameState::QUIT_CONFIRM_MENU){ if(wy>8.5f && wy<9.5f){ RT.paused=false; RT.gameOver=false; RT.winGame=false; RT.deathActive=false; RT.postMenuShown=false; gState=GameState::MENU; RT.tStart = RT.tLast = std::chrono::steady_clock::now(); glutPostRedisplay(); return; } if(wy>6.5f && wy<7.5f){ gState=GameState::POSTGAME_MENU; glutPostRedisplay(); return; } return; }
}

void onPassiveMotion(int x,int y){ float wy = windowToWorldY(y);
  if(gState==GameState::MENU){ hoverPlay=(wy>8.5f&&wy<9.5f); hoverExit=(wy>6.5f&&wy<7.5f); glutPostRedisplay(); return; }
  if(gState==GameState::POSTGAME_MENU){ hoverPG_PlayAgain=(wy>8.5f&&wy<9.5f); hoverPG_Quit=(wy>6.5f&&wy<7.5f); glutPostRedisplay(); return; }
  if(gState==GameState::QUIT_CONFIRM_MENU){ hoverQC_PlayAgain=(wy>8.5f && wy<9.5f); hoverQC_Quit=(wy>6.5f && wy<7.5f); glutPostRedisplay(); return; }
}


// =====================================
// File: src/main.cpp
// =====================================
#include <GL/glut.h>
#include "state.hpp"
#include "render.hpp"
#include "input.hpp"

// Core headers from your part
#include "../your-part/include/config.hpp"
#include "../your-part/include/types.hpp"
#include "../your-part/include/maze.hpp"
#include "../your-part/include/logic.hpp"
#include "../your-part/include/powerups.hpp"

using namespace pac;

// Single definitions (declared extern in headers)
GameState gState = GameState::MENU;
int hoverPlay=0, hoverExit=0, hoverPG_PlayAgain=0, hoverPG_Quit=0, hoverQC_PlayAgain=0, hoverQC_Quit=0;
Runtime RT{};

static void displayRouter(){ renderDisplay(); }
static void reshapeCB(int w,int h){ reshapeView(w,h); }

// 60 FPS-ish timer
static void timerCB(int){
  using Clock = std::chrono::steady_clock;
  // If not in gameplay, we still need to handle the 2s hold after game over
  if(gState==GameState::PLAYING){
    if(RT.deathActive){ /* step handles death timing */ pac::step(RT); }
    else if(!RT.paused){ pac::step(RT); }
    // Show big overlay inside renderGame();
    if(RT.gameOver && !RT.postMenuShown){
      float tHold = std::chrono::duration<float>(Clock::now() - RT.tGameOverAt).count();
      if(tHold >= 2.0f){ RT.postMenuShown = true; gState = GameState::POSTGAME_MENU; }
    }
  }
  glutPostRedisplay(); glutTimerFunc(16, timerCB, 0);
}

int main(int argc,char** argv){
  std::srand((unsigned)time(NULL));
  copyMazeFromTemplate();
  // Let startNewGame() set counters when user clicks Play; still prep powerups arrays
  resetSupers(RT); resetHeart(RT);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
  glutInitWindowSize(760,820);
  glutCreateWindow("PAC-MAN — GLUT (Render/Input Shell)");
  glutDisplayFunc(displayRouter);
  glutReshapeFunc(reshapeCB);
  glutSpecialFunc(onSpecialKey);
  glutKeyboardFunc(onKeyDown);
  glutMouseFunc(onMouseClick);
  glutPassiveMotionFunc(onPassiveMotion);

  RT.tStart = RT.tLast = std::chrono::steady_clock::now();
  glutTimerFunc(16, timerCB, 0);
  glutMainLoop();
  return 0;
}


// =====================================
// File: CMakeLists.txt
// =====================================
cmake_minimum_required(VERSION 3.15)
project(pacman_glut_shell CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/../your-part/include)

add_executable(pacman
  src/main.cpp
  src/render.cpp
  src/input.cpp
  # Core sources from your-part (adjust the relative path if needed)
  ../your-part/src/config.cpp
  ../your-part/src/maze.cpp
  ../your-part/src/powerups.cpp
  ../your-part/src/logic.cpp
  ../your-part/src/util.cpp
)

target_link_libraries(pacman ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})


// =====================================
// File: .gitignore
// =====================================
build/
*.o
*.obj
*.pdb
*.exe
*.DS_Store
cmake-build-*/


// =====================================
// File: README.md
// =====================================
# PAC-MAN — Teammate Part (Rendering, Input & App Shell)

This folder contains the **rendering, menus, input handlers, and app shell**. It links against the **core logic/power-ups** in `../your-part/`.

## Build
```bash
mkdir -p build && cd build
cmake ..
make -j
./pacman
```

> If your folder structure differs, update the paths in `CMakeLists.txt` that point to `../your-part/`.

## Controls
- **Arrow Keys**: Move
- **P**: Pause/Resume
- **R**: Restart (while playing)
- **ESC**: Quit

## Ownership
- This folder is owned by the **Rendering/Input teammate**.
- The **Core** (game loop, AI, power-ups) lives in `your-part/` and is owned by Asif.
