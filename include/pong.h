#ifndef PONG_H_
#define PONG_H_

/* Macros and constants */
#define PADDLE_STEP 2
#define MIDDLE(outer, inner) ((outer) - (inner)/2)/2

enum SpriteId {
  ID_Player,
  ID_CPU,
  ID_Ball
};

enum Direction {
  None,
  Up,
  Down
};

/* Aggregate type definitions */
struct Coord {
  int x;
  int y;
};

struct Sprite {
  int               id;
  u16              *gfx;
  struct Coord      pos;
  SpriteSize        sz;
  SpriteColorFormat fmt;
};

struct Ball {
  struct Coord  vel;
  struct Sprite spr;
};

/* Function prototypes */
void Initialize(void);
void Draw      (void);
void Update    (int Keys);

#endif
