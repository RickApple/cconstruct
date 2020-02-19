//
//  main.cpp
//  hello_world
//
//  Created by Rick Appleton on 05/02/2020.
//  Copyright © 2020 Daedalus Development. All rights reserved.
//

#include "function.h"

int main(int argc, const char* argv[]) {
  const char* lib_string = getLibraryString();
  return strcmp(lib_string, "Hello from library");
}
