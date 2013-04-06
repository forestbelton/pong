#include <nds.h>
#include "pong.h"

int main() {
  Initialize();
  
  while(1) {
    scanKeys();
    
    Update(keysHeld());
    Draw();
    
    swiWaitForVBlank();
    oamUpdate(&oamMain);
  }
  
  return 0;
}
