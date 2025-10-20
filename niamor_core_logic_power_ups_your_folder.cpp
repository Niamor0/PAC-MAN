# Folder: your-part/
# ├─ include/
# │  ├─ config.hpp
# │  ├─ types.hpp
# │  ├─ maze.hpp
# │  ├─ powerups.hpp
# │  ├─ logic.hpp
# │  └─ util.hpp
# └─ src/
#    ├─ config.cpp
#    ├─ maze.cpp
#    ├─ powerups.cpp
#    ├─ logic.cpp
#    └─ util.cpp

// =============================
// File: include/types.hpp
// =============================
#pragma once
#include <chrono>

namespace pac {

struct RGBc { float r,g,b; };

struct Actor { float x, y, vx, vy, radius; };

// Maze cells
enum Cell { WALL=1, DOTCELL=0, EMPTY=-1, GATE=2 };

// Runtime bag (timers & counters)
struct Runtime {
  int pelletsTotal=0, pelletsEaten=0, score=0, lives=3;
  bool paused=false, gameOver=false, winGame=false;
  bool deathActive=false, postMenuShown=false;
  float pacAngleDeg=0.0f;
  float lastSuperSpawnAt=0.0f, lastHeartSpawnAt=0.0f;
  std::chrono::steady_clock::time_point tStart, tLast, tDeathStart, tGameOverAt;
};

// Power ups
struct SuperFood { bool active; int r,c; };
struct Heart     { bool active; int r,c; };

} // namespace pac


// =============================
// File: include/config.hpp
// =============================
#pragma once
#include "types.hpp"

namespace pac {

// Grid size
inline constexpr int COLS = 19;
inline constexpr int ROWS = 21;

// Speeds / pacing
inline constexpr float PAC_SPEED    = 4.2f;
inline constexpr float GHOST_SPEED0 = 3.0f;
inline constexpr float GHOST_STEP   = 0.35f;
inline constexpr int   STEP_EVERY_S = 15;

// Colors
extern RGBc BG_COL;
extern RGBc WALL_COL;
extern RGBc DOT_COL;
extern RGBc HUD_COL;
extern RGBc PAC_COL;
extern RGBc BLINKY_COL;
extern RGBc PINKY_COL;
extern RGBc INKY_COL;
extern RGBc CLYDE_COL;

// Init (call once at app start if needed)
void initConfig();

} // namespace pac


// =============================
// File: include/maze.hpp
// =============================
#pragma once
#include "types.hpp"
#include "config.hpp"

namespace pac {

extern int MAZE[ROWS][COLS];

// Copy template into MAZE
void copyMazeFromTemplate();
// Count DOTCELL in current MAZE
int  countDots();

} // namespace pac


// =============================
// File: include/powerups.hpp
// =============================
#pragma once
#include "types.hpp"
#include "config.hpp"

namespace pac {

inline constexpr int MAX_SUPERS = 3;

// Global state (owned by core)
extern SuperFood supers[MAX_SUPERS];
extern Heart     heart;

// Spawning cadence
inline constexpr float SUPER_SPAWN_INTERVAL = 10.0f; // seconds
inline constexpr float HEART_SPAWN_INTERVAL = 10.0f; // seconds

// Reset
void resetSupers(Runtime& rt);
void resetHeart (Runtime& rt);

// Tick-time spawners (call from step)
void maybeSpawnSupers(Runtime& rt, float elapsedSeconds);
void maybeSpawnHeart (Runtime& rt, float elapsedSeconds);

// Eat checks (call after pac update)
void checkEatSuper(Runtime& rt, const Actor& pac);
void checkEatHeart(Runtime& rt,       Actor& pac);

// Lightweight accessors for renderer (optional)
int  activeSuperCount();

} // namespace pac


// =============================
// File: include/util.hpp
// =============================
#pragma once
#include "types.hpp"
#include "config.hpp"

namespace pac {

int   yToRow(float y);
int   xToCol(float x);
bool  blockedForPac(int r,int c);
bool  blockedForGhostCell(int r,int c);
float clampf(float v,float lo,float hi);

float cellCenterX(int c);
float cellCenterY(int r);
bool  atCellCenter(float x,float y,int r,int c,float eps=0.06f);

} // namespace pac


// =============================
// File: include/logic.hpp
// =============================
#pragma once
#include "types.hpp"
#include "config.hpp"

namespace pac {

// Global actors & ghost dirs (core-owned)
extern Actor pacman;
extern Actor ghosts[4];
extern int   gDx[4];
extern int   gDy[4];

// Lifecycle
void resetActors(Runtime& rt);
void initGhostDirsRandom();

void eatPellet(Runtime& rt);

// Death flow
void triggerDeath(Runtime& rt);
void finalizeDeath(Runtime& rt);

// Movement helpers
void worldToNextCell(int r,int c,int dx,int dy,int& nr,int& nc);
bool ghostCanGo(int r,int c,int dx,int dy);
void chooseGhostDirWithChase(int i,int r,int c);

// Per-frame updates
void updatePac(Runtime& rt, float dt);
void updateGhosts(Runtime& rt, float dt);

// Game loop tick (call from timer callback)
void step(Runtime& rt);

// New game
void startNewGame(Runtime& rt);

} // namespace pac


// =============================
// File: src/config.cpp
// =============================
#include "config.hpp"

namespace pac {

RGBc BG_COL     {0.02f,0.02f,0.02f};
RGBc WALL_COL   {0.95f,0.25f,0.25f};
RGBc DOT_COL    {0.95f,0.95f,0.95f};
RGBc HUD_COL    {1.00f,1.00f,1.00f};
RGBc PAC_COL    {1.00f,0.95f,0.00f};
RGBc BLINKY_COL {1.00f,0.00f,0.00f};
RGBc PINKY_COL  {1.00f,0.55f,0.90f};
RGBc INKY_COL   {0.00f,1.00f,1.00f};
RGBc CLYDE_COL  {1.00f,0.60f,0.00f};

void initConfig() {
  // (Reserved for future dynamic tuning)
}

} // namespace pac


// =============================
// File: src/maze.cpp
// =============================
#include "maze.hpp"

namespace pac {

static const int MAZE_TEMPLATE[ROWS][COLS] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1},
  {1,0,1,1,1,0,0,1,0,1,1,0,1,0,0,1,1,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
  {1,0,1,0,1,1,0,1,1,1,1,1,1,0,1,0,1,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
  {1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,1,1},
  {1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1},
  {1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1},
  {1,0,0,0,1,0,0,0,1,2,1,0,0,0,1,0,0,0,1},
  {1,1,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,1,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1},
  {1,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1},
  {1,0,1,0,1,1,0,1,1,1,1,1,1,0,1,0,1,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
  {1,0,1,1,1,0,0,1,0,1,1,0,1,0,0,1,1,0,1},
  {1,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

int MAZE[ROWS][COLS] = {};

void copyMazeFromTemplate(){
  for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) MAZE[r][c]=MAZE_TEMPLATE[r][c];
}

int countDots(){
  int pelletsTotal=0;
  for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) if(MAZE[r][c]==DOTCELL) pelletsTotal++;
  return pelletsTotal;
}

} // namespace pac


// =============================
// File: src/util.cpp
// =============================
#include <cmath>
#include "util.hpp"
#include "maze.hpp"

namespace pac {

int yToRow(float y){ return ROWS-1-(int)std::floor(y); }
int xToCol(float x){ return (int)std::floor(x); }

bool blockedForPac(int r,int c){
  return (r<0||r>=ROWS||c<0||c>=COLS||MAZE[r][c]==WALL||MAZE[r][c]==GATE);
}
bool blockedForGhostCell(int r,int c){
  return (r<0||r>=ROWS||c<0||c>=COLS||MAZE[r][c]==WALL);
}

float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }

float cellCenterX(int c){ return c + 0.5f; }
float cellCenterY(int r){ return (ROWS-1-r) + 0.5f; }

bool atCellCenter(float x,float y,int r,int c,float eps){
  return std::fabs(x - cellCenterX(c)) < eps && std::fabs(y - cellCenterY(r)) < eps;
}

} // namespace pac


// =============================
// File: src/powerups.cpp
// =============================
#include <cstdlib>
#include <cmath>
#include "powerups.hpp"
#include "maze.hpp"
#include "util.hpp"

namespace pac {

SuperFood supers[MAX_SUPERS];
Heart     heart{false,0,0};

static bool isValidSpawnCellCommon(int r,int c, const Actor& pac, const Actor ghosts[4]){
  if(r<0||r>=ROWS||c<0||c>=COLS) return false;
  if(MAZE[r][c]==WALL || MAZE[r][c]==GATE) return false;
  int rp=yToRow(pac.y), cp=xToCol(pac.x);
  if(r==rp && c==cp) return false;
  for(int i=0;i<4;i++){
    int rg=yToRow(ghosts[i].y), cg=xToCol(ghosts[i].x);
    if(r==rg && c==cg) return false;
  }
  return true;
}

void resetSupers(Runtime& rt){
  for(int i=0;i<MAX_SUPERS;i++) supers[i] = {false,0,0};
  rt.lastSuperSpawnAt = 0.0f;
}
void resetHeart(Runtime& rt){
  heart = {false,0,0};
  rt.lastHeartSpawnAt = 0.0f;
}

static int countActiveSupersImpl(){
  int n=0; for(int i=0;i<MAX_SUPERS;i++) if(supers[i].active) n++; return n;
}
int activeSuperCount(){ return countActiveSupersImpl(); }

static void spawnOneSuper(const Actor& pac, const Actor ghosts[4]){
  for(int tries=0; tries<200; ++tries){
    int r = std::rand()%ROWS; int c = std::rand()%COLS;
    if(!isValidSpawnCellCommon(r,c,pac,ghosts)) continue;
    // avoid heart & existing supers overlap
    bool conflict=false;
    for(int i=0;i<MAX_SUPERS;i++) if(supers[i].active && supers[i].r==r && supers[i].c==c) {conflict=true;break;}
    if(heart.active && heart.r==r && heart.c==c) conflict=true;
    if(conflict) continue;
    for(int i=0;i<MAX_SUPERS;i++) if(!supers[i].active){ supers[i] = {true,r,c}; return; }
    return;
  }
}

void maybeSpawnSupers(Runtime& rt, float elapsed){
  if(elapsed - rt.lastSuperSpawnAt >= SUPER_SPAWN_INTERVAL){
    rt.lastSuperSpawnAt = elapsed;
    if(countActiveSupersImpl() < MAX_SUPERS){
      extern Actor ghosts[4]; extern Actor pacman; // from logic.cpp
      spawnOneSuper(pacman, ghosts);
    }
  }
}

static void spawnHeartImpl(const Actor& pac, const Actor ghosts[4]){
  for(int tries=0; tries<200; ++tries){
    int r = std::rand()%ROWS; int c = std::rand()%COLS;
    if(!isValidSpawnCellCommon(r,c,pac,ghosts)) continue;
    // avoid supers overlap
    bool conflict=false;
    for(int i=0;i<MAX_SUPERS;i++) if(supers[i].active && supers[i].r==r && supers[i].c==c) {conflict=true;break;}
    if(conflict) continue;
    heart = {true,r,c};
    return;
  }
}

void maybeSpawnHeart(Runtime& rt, float elapsed){
  if(heart.active) return;
  if(elapsed - rt.lastHeartSpawnAt >= HEART_SPAWN_INTERVAL){
    rt.lastHeartSpawnAt = elapsed;
    extern Actor ghosts[4]; extern Actor pacman; // from logic.cpp
    spawnHeartImpl(pacman, ghosts);
  }
}

void checkEatSuper(Runtime& rt, const Actor& pac){
  for(int i=0;i<MAX_SUPERS;i++){
    if(!supers[i].active) continue;
    float cx = cellCenterX(supers[i].c);
    float cy = cellCenterY(supers[i].r);
    float dx = pac.x - cx; float dy = pac.y - cy;
    float rr = (pac.radius + 0.30f);
    if(dx*dx + dy*dy <= rr*rr){ supers[i].active=false; rt.score += 100; }
  }
}

void checkEatHeart(Runtime& rt, Actor& pac){
  (void)pac; // not used now
  if(!heart.active) return;
  float cx = cellCenterX(heart.c);
  float cy = cellCenterY(heart.r);
  float dx = pacman.x - cx; float dy = pacman.y - cy;
  float rr = (pacman.radius + 0.28f);
  if(dx*dx + dy*dy <= rr*rr){
    heart.active = false;
    if(rt.lives < 3) rt.lives += 1;
  }
}

} // namespace pac


// =============================
// File: src/logic.cpp
// =============================
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "logic.hpp"
#include "maze.hpp"
#include "util.hpp"
#include "powerups.hpp"

namespace pac {

Actor pacman {9.5f,15.5f,0,0,0.33f};
Actor ghosts[4] = {
  {9.5f,10.5f,0,0,0.33f},
  {7.5f,10.5f,0,0,0.33f},
  {11.5f,10.5f,0,0,0.33f},
  {9.5f, 9.5f,0,0,0.33f}
};

int gDx[4]={0}, gDy[4]={0};

static inline float nowSeconds(const Runtime& rt){
  using Clock = std::chrono::steady_clock;
  return std::chrono::duration<float>(Clock::now() - rt.tStart).count();
}

void resetActors(Runtime& rt){
  pacman = {9.5f,15.5f,0,0,0.33f};
  ghosts[0] = {9.5f,10.5f,0,0,0.33f};
  ghosts[1] = {7.5f,10.5f,0,0,0.33f};
  ghosts[2] = {11.5f,10.5f,0,0,0.33f};
  ghosts[3] = {9.5f, 9.5f,0,0,0.33f};
  rt.pacAngleDeg = 0.0f;
}

void initGhostDirsRandom(){
  for(int i=0;i<4;i++){
    int d=std::rand()%4; int dx[4]={1,-1,0,0}; int dy[4]={0,0,1,-1};
    gDx[i]=dx[d]; gDy[i]=dy[d];
  }
}

void eatPellet(Runtime& rt){
  int r=yToRow(pacman.y), c=xToCol(pacman.x);
  if(MAZE[r][c]==DOTCELL){ MAZE[r][c]=EMPTY; rt.pelletsEaten++; rt.score+=10; }
  if(rt.pelletsEaten==rt.pelletsTotal){ rt.winGame=true; rt.paused=true; }
}

void triggerDeath(Runtime& rt){
  if(rt.deathActive || rt.gameOver || rt.winGame) return;
  rt.deathActive = true;
  rt.tDeathStart = std::chrono::steady_clock::now();
  pacman.vx = 0.0f; pacman.vy = 0.0f;
}

void finalizeDeath(Runtime& rt){
  rt.lives -= 1;
  if(rt.lives <= 0){
    rt.gameOver = true; rt.paused = true; rt.deathActive = false;
    rt.tGameOverAt = std::chrono::steady_clock::now();
    rt.postMenuShown = false;
    return;
  }
  // Reset for next life
  resetActors(rt);
  initGhostDirsRandom();
  rt.deathActive = false;
}

void worldToNextCell(int r,int c,int dx,int dy,int& nr,int& nc){
  nr=r; nc=c; if(dx>0) nc=c+1; if(dx<0) nc=c-1; if(dy>0) nr=r-1; if(dy<0) nr=r+1;
}

bool ghostCanGo(int r,int c,int dx,int dy){
  int nr,nc; worldToNextCell(r,c,dx,dy,nr,nc); return !blockedForGhostCell(nr,nc);
}

void chooseGhostDirWithChase(int i,int r,int c){
  int curDx=gDx[i], curDy=gDy[i];
  struct D{int dx,dy;}; D dirs[4]={{1,0},{-1,0},{0,1},{0,-1}}; // right,left,up,down
  float bestScore = 1e9f; int bestDx=0,bestDy=0;
  for(auto d:dirs){
    if(d.dx==-curDx && d.dy==-curDy) continue;
    if(!ghostCanGo(r,c,d.dx,d.dy)) continue;
    int nr,nc; worldToNextCell(r,c,d.dx,d.dy,nr,nc);
    float nx=cellCenterX(nc), ny=cellCenterY(nr);
    float score = std::fabs(nx - pacman.x) + std::fabs(ny - pacman.y) + (std::rand()%100)*0.001f;
    if(score<bestScore){ bestScore=score; bestDx=d.dx; bestDy=d.dy; }
  }
  if(bestDx||bestDy){ gDx[i]=bestDx; gDy[i]=bestDy; return; }
  for(auto d:dirs){ if(ghostCanGo(r,c,d.dx,d.dy)){ gDx[i]=d.dx; gDy[i]=d.dy; return; } }
  gDx[i]=0; gDy[i]=0;
}

void updatePac(Runtime& rt, float dt){
  float nx=pacman.x+pacman.vx*PAC_SPEED*dt, ny=pacman.y+pacman.vy*PAC_SPEED*dt;
  if(!blockedForPac(yToRow(pacman.y),xToCol(nx))) pacman.x=clampf(nx,0.5f,COLS-0.5f);
  if(!blockedForPac(yToRow(ny),xToCol(pacman.x))) pacman.y=clampf(ny,0.5f,ROWS-0.5f);
  if(std::fabs(pacman.vx)>1e-4f || std::fabs(pacman.vy)>1e-4f){
    rt.pacAngleDeg = std::atan2(pacman.vy, pacman.vx) * 180.0f / 3.14159265f;
  }
  eatPellet(rt);
  checkEatSuper(rt, pacman);
  checkEatHeart(rt, pacman);
}

void updateGhosts(Runtime& rt, float dt){
  using Clock = std::chrono::steady_clock;
  float elapsed=std::chrono::duration<float>(Clock::now()-rt.tStart).count();
  float gs=GHOST_SPEED0+(int(elapsed)/STEP_EVERY_S)*GHOST_STEP;
  const float GHOST_MAX = PAC_SPEED - 0.4f; if(gs>GHOST_MAX) gs=GHOST_MAX;

  for(int i=0;i<4;i++){
    int r=yToRow(ghosts[i].y); int c=xToCol(ghosts[i].x);
    if(gDx[i]!=0) ghosts[i].y = cellCenterY(r);
    if(gDy[i]!=0) ghosts[i].x = cellCenterX(c);
    if(atCellCenter(ghosts[i].x, ghosts[i].y, r, c)){
      chooseGhostDirWithChase(i,r,c);
      int nr,nc; worldToNextCell(r,c,gDx[i],gDy[i],nr,nc);
      if(blockedForGhostCell(nr,nc)){ gDx[i]=gDy[i]=0; }
    }
    ghosts[i].x += gDx[i]*gs*dt; ghosts[i].y += gDy[i]*gs*dt;
    ghosts[i].x = clampf(ghosts[i].x, 0.5f, COLS-0.5f);
    ghosts[i].y = clampf(ghosts[i].y, 0.5f, ROWS-0.5f);
    float dx=ghosts[i].x-pacman.x, dy=ghosts[i].y-pacman.y; float rr=(ghosts[i].radius+pacman.radius-0.04f);
    if(dx*dx+dy*dy < rr*rr){ triggerDeath(rt); return; }
  }
}

// NOTE: This core module does not know about UI states/menus.
// Callers should decide when to skip gameplay (e.g., when in menus or paused).
void step(Runtime& rt){
  using Clock = std::chrono::steady_clock;
  auto now = Clock::now();
  float dt = std::chrono::duration<float>(now - rt.tLast).count();
  rt.tLast = now;

  // Death animation hold
  if(rt.deathActive){
    float tSince = std::chrono::duration<float>(now - rt.tDeathStart).count();
    if(tSince >= 1.0f) finalizeDeath(rt);
    return;
  }

  // If paused, still allow game-over → post menu timing to progress (handled by outer app)
  if(rt.paused){
    return;
  }

  // Normal updates
  float elapsed=std::chrono::duration<float>(now - rt.tStart).count();
  maybeSpawnSupers(rt, elapsed);
  maybeSpawnHeart (rt, elapsed);
  updatePac(rt, dt);
  updateGhosts(rt, dt);
}

void startNewGame(Runtime& rt){
  rt.paused=false; rt.gameOver=false; rt.winGame=false; rt.deathActive=false;
  rt.score=0; rt.pelletsEaten=0; rt.lives=3; rt.postMenuShown=false;
  copyMazeFromTemplate();
  rt.pelletsTotal = countDots();
  resetActors(rt);
  initGhostDirsRandom();
  resetSupers(rt);
  resetHeart(rt);
  rt.tStart = rt.tLast = std::chrono::steady_clock::now();
}

} // namespace pac
