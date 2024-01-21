//
//  GameScene.h
//  VirtualNes
//
//  Created by Jesse Boettcher on 10/31/22.
//

#import <SpriteKit/SpriteKit.h>

@interface GameScene : SKScene

- (void)loadROM:(NSString *)abs_path;

- (void)constructor;
- (void)shutdown;

- (void)interrupt;
- (void)execute:(NSString*)command;

@end
