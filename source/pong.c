/* System headers */
#include <maxmod9.h>
#include <nds.h>

/* Project headers */
#include "pong.h"

/* Resource headers */
#include "field.h"
#include "paddle.h"
#include "ball.h"
#include "soundbank.h"
#include "soundbank_bin.h"

/* Game state */
static int           PlayerScore, CPUScore;
static struct Sprite Player, CPU;
static struct Ball   Ball;

static mm_sound_effect hit = {
  { SFX_HIT } ,			// id
  (int)(1.0f * (1<<10)),	// rate
  0,		// handle
  255,	// volume
  255,	// panning
};

static mm_sound_effect score = {
  { SFX_SCORE } ,			// id
  (int)(1.0f * (1<<10)),	// rate
  0,		// handle
  255,	// volume
  255,	// panning
};

/* A constructor for struct Sprite, which is fairly self explanatory. */
static struct Sprite Sprite_New(int id, int x, int y, SpriteSize sz, SpriteColorFormat fmt, u16 *gfx) {
  struct Sprite spr;
  
  spr.pos.x = x;
  spr.pos.y = y;
  spr.id    = id;
  spr.sz    = sz;
  spr.fmt   = fmt;
  spr.gfx   = gfx;
  
  return spr;
}

/* Draws our primitive sprite structure to screen according to its attributes.
 * This is just a simple wrapper over oamSet. */
static void Sprite_Draw(struct Sprite *spr) {
  oamSet(&oamMain, spr->id, spr->pos.x, spr->pos.y, 0, 0, spr->sz, spr->fmt,
         spr->gfx, -1, false, false, false, false, false);
}

static void Ball_Reset(void) {
  /* The ball always starts in the center. */
  Ball.spr.pos.x = MIDDLE(SCREEN_WIDTH,  8);
  Ball.spr.pos.y = MIDDLE(SCREEN_HEIGHT, 8);
  
  /* Randomize the ball's starting velocity. This really isn't the proper way
   * to achieve random numbers, but it will be fine for our use. */
  Ball.vel.x = 2;
  if(rand() % 2)
    Ball.vel.x = -Ball.vel.x;
  
  switch(rand() % 3) {
    case 0: Ball.vel.y = 0;  break;
    case 1: Ball.vel.y = -2; break;
    case 2: Ball.vel.y = 2;  break;
  }
}

/* Initializes the DS with desired settings and sets up the game state. */
void Initialize(void) {
  u16 *tmp;
  int  field;
  
  /* Initialize the screen, set video modes and allocate the VRAM banks to
   * their needed locations. */
  lcdMainOnBottom();
  
  videoSetMode   (MODE_0_2D);
  videoSetModeSub(MODE_0_2D);
  
  vramSetMainBanks(VRAM_A_MAIN_BG, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG, VRAM_D_LCD);
  
  /* Copy the field background to Background 0. */
  field = bgInit(0, BgType_Text8bpp, BgSize_T_256x256, 0, 1);
	dmaCopy(fieldTiles, bgGetGfxPtr(field), fieldTilesLen);
  dmaCopy(fieldMap,   bgGetMapPtr(field), fieldMapLen  );
	dmaCopy(fieldPal,   BG_PALETTE,         fieldPalLen  );
  
  /* Initialize OAM, allocate space for the paddle sprite and create both paddles. */
  oamInit(&oamMain, SpriteMapping_1D_128, false);
  
  tmp = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);
  dmaCopy(paddleTiles, tmp, paddleTilesLen);
  
  Player = Sprite_New(ID_Player, 0, MIDDLE(SCREEN_HEIGHT, 32), SpriteSize_32x32, SpriteColorFormat_256Color, tmp);
  CPU    = Sprite_New(ID_CPU, SCREEN_WIDTH - 32, MIDDLE(SCREEN_HEIGHT, 32), SpriteSize_32x32, SpriteColorFormat_256Color, tmp);
  
  /* Create the ball and initialize its state. */
  tmp = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);
  dmaCopy(ballTiles, tmp, ballTilesLen);
  
  Ball.spr = Sprite_New(ID_Ball, 0, 0, SpriteSize_8x8, SpriteColorFormat_256Color, tmp);
  Ball_Reset();
  
  SPRITE_PALETTE[1] = RGB15(31, 31, 31);
  
  /* Initialize sound and load sound resources. */
  mmInitDefaultMem((mm_addr)soundbank_bin);
  mmLoadEffect(SFX_HIT);
  mmLoadEffect(SFX_SCORE);
}

void Draw(void) {
  Sprite_Draw(&Player);
  Sprite_Draw(&CPU);
  Sprite_Draw(&Ball.spr);
}

void Update(int Keys) {
  int PlayerDirection, CPUDirection;
  
  /* Update the player's y-coordinate according to the key pressed. */
  if(Keys & KEY_DOWN && Player.pos.y < (SCREEN_HEIGHT - 32 + PADDLE_STEP)) {
    Player.pos.y += PADDLE_STEP;
    PlayerDirection = Down;
  }
  else if(Keys & KEY_UP && Player.pos.y > 0) {
    Player.pos.y -= PADDLE_STEP;
    PlayerDirection = Up;
  }
  else
    PlayerDirection = None;
  
  /* Update ball coordinates. */
  Ball.spr.pos.x += Ball.vel.x;
  Ball.spr.pos.y += Ball.vel.y;
  
  /* Control the AI. This AI will take a step towards the ball if needed on
   * each new frame. */
  if(Ball.spr.pos.y > CPU.pos.y + 32) {
    CPU.pos.y   += PADDLE_STEP;
    CPUDirection = Up;
  }
  else if(Ball.spr.pos.y < CPU.pos.y) {
    CPU.pos.y   -= PADDLE_STEP;
    CPUDirection = Down;
  }
  else
    CPUDirection = None;
  
  /* Update the ball's direction if it hits a wall. */
  if(Ball.spr.pos.y <= 0 || Ball.spr.pos.y >= (SCREEN_HEIGHT - 8)) {
    Ball.vel.y = -Ball.vel.y;
    mmEffectEx(&hit);
  }
  
  /* Check for collisions with player's paddle. */
  if(Ball.spr.pos.x == ((10 + 11) & ~1)) {
    if(Ball.spr.pos.y >= Player.pos.y && Ball.spr.pos.y <= Player.pos.y + 32) {
      Ball.vel.x  = -Ball.vel.x;
      Ball.vel.y += (rand() % 3) - 1;
      
      switch(PlayerDirection) {
        case Up:   Ball.vel.y -= 2;          break;
        case Down: Ball.vel.y += 2;          break;
        case None: Ball.vel.y = -Ball.vel.y; break;
      }
      
      mmEffectEx(&hit);
    }
  }
  
  /* Check for collisions with the CPU's paddle. */
  if(Ball.spr.pos.x == ((SCREEN_WIDTH - 10 - 11 - 8) & ~1)) {
    if(Ball.spr.pos.y >= CPU.pos.y && Ball.spr.pos.y <= CPU.pos.y + 32) {
      Ball.vel.x  = -Ball.vel.x;
      Ball.vel.y += (rand() % 3) - 1;
      
      switch(CPUDirection) {
        case Up:   Ball.vel.y -= 2;          break;
        case Down: Ball.vel.y += 2;          break;
        case None: Ball.vel.y = -Ball.vel.y; break;
      }
      
      mmEffectEx(&hit);
    }
  }
  
  /* Check for points to be scored. */
  if(Ball.spr.pos.x <= 0) {
    ++CPUScore;
    Ball_Reset();
    mmEffectEx(&score);
  }
  
  else if(Ball.spr.pos.x >= SCREEN_WIDTH - 8) {
    ++PlayerScore;
    Ball_Reset();
    mmEffectEx(&score);
  }
}