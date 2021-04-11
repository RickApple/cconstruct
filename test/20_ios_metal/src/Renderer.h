//
//  Renderer.h
//  TestGame
//
//  Created by Rick Appleton on 18/05/2020.
//  Copyright © 2020 Daedalus Development. All rights reserved.
//

#import <MetalKit/MetalKit.h>

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

- (nonnull instancetype)initWithMetalKitView:(nonnull MTKView*)view;

@end
