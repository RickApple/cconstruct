#import <Metal/Metal.h>
#include <stdio.h>

int main() {
  id device = MTLCreateSystemDefaultDevice();
  (void)device;
  printf("Created Metal device\n");
  return 0;
}
