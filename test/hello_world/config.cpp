#include <vector>
#include <stdio.h>

#include "builder.h"

void main() { 
  builder.workspace.setOutputFolder("test");
  
  builder.workspace.addPlatform( "Win32", EPlatformTypeX86 );
  builder.workspace.addPlatform( "x64", EPlatformTypeX64 );
  builder.workspace.addConfiguration( "Debug" );
  builder.workspace.addConfiguration( "Release" );

  auto p = builder.project.create( "builder", CONSOLE_APPLICATION );
  std::vector<const char*> srcFiles{"builder.cpp", "process.cpp"};
  std::vector<const char*> headerFiles{"process.h"};
  std::vector<const char*> testFiles{"TEST\\builder.h", "test\\config.cpp"};
  for( auto f : srcFiles ) {
    builder.project.addFile( p, f, "Source Files" );
  }
  for( auto f : headerFiles ) {
    builder.project.addFile( p, f, "Header Files" );
  }
  for( auto f : testFiles ) {
    builder.project.addFile( p, f, "Test" );
  }
  builder.workspace.addProject( p );
  builder.generate();
}