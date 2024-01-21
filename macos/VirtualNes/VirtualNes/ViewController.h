//
//  ViewController.h
//  VirtualNes
//
//  Created by Jesse Boettcher on 10/31/22.
//

#import <Cocoa/Cocoa.h>
#import <SpriteKit/SpriteKit.h>
#import <GameplayKit/GameplayKit.h>

@interface ViewController : NSViewController
@property (assign) IBOutlet SKView *skView;

- (IBAction)interrupt:(id)sender;
- (IBAction)enterCommand:(id)sender;

@end

