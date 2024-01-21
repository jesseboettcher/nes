//
//  GameScene.m
//  VirtualNes
//
//  Created by Jesse Boettcher on 10/31/22.
//

#import "GameScene.h"

#include "io/display.hpp"
#include "io/files.hpp"
#include "io/prompt.hpp"
#include "system/nes.hpp"

#include <thread>

std::unique_ptr<Nes> _nes;

void modifyPixedData (Nes& nes, NesDisplay::Color * pixelData, size_t lengthInBytes, CGSize size)
{
    NSUInteger pixelCount = lengthInBytes / sizeof ( NesDisplay::Color );

    for ( NSUInteger index = 0; index < pixelCount; index ++ )
    {
        pixelData[index] = nes.display().display_buffer()[index];
    }
    nes.display().ready_for_display_ = false;
}

@implementation GameScene {
    
    SKMutableTexture *_display_texture;
    SKSpriteNode *_display_sprite;
    
    std::thread* _vm_thread;
}

- (void)constructor
{
    _nes = std::make_unique<Nes>(nullptr);
    _vm_thread = new std::thread([]{ CommandPrompt::instance().launch_prompt(*_nes); });
}

- (void)interrupt
{
    _nes->user_interrupt();
}

- (void)execute:(NSString*)command
{
    std::cout << "Executing command " << [command UTF8String] << std::endl;
    std::string command_str = [command UTF8String];
    CommandPrompt::instance().write_command(command_str);
}

- (void)loadROM:(NSString *)abs_path
{
    std::unique_ptr<NesFileParser> cartridge;
    std::filesystem::path path([abs_path UTF8String]);//"/Users/jesse/Downloads/roms/donkey_kong.nes");
    cartridge = std::make_unique<NesFileParser>(path);
    
    _nes->load_cartridge(std::move(cartridge));
}

- (void)shutdown
{
    CommandPrompt::instance().shutdown();
    _nes->shutdown();
    if (_vm_thread)
    {
        _vm_thread->join();
    }
}

- (void)didMoveToView:(SKView *)view {
    
    self.size = {NesDisplay::WIDTH, NesDisplay::HEIGHT};
    
    _display_texture = [[SKMutableTexture alloc] initWithSize:self.size pixelFormat:kCVPixelFormatType_32RGBA];
    _display_sprite = [[SKSpriteNode alloc] initWithTexture:_display_texture];
    _display_sprite.size = self.size;
    [self addChild:_display_sprite];
}

- (void)touchDownAtPoint:(CGPoint)pos {
}

- (void)touchMovedToPoint:(CGPoint)pos {
}

- (void)touchUpAtPoint:(CGPoint)pos {
}

- (void)keyDown:(NSEvent *)theEvent {
    switch (theEvent.keyCode) {
        case 0x31 /* SPACE */:
            break;

        default:
            NSLog(@"keyDown:'%@' keyCode: 0x%02X", theEvent.characters, theEvent.keyCode);
            break;
    }
}

- (void)mouseDown:(NSEvent *)theEvent {
    [self touchDownAtPoint:[theEvent locationInNode:self]];
}
- (void)mouseDragged:(NSEvent *)theEvent {
    [self touchMovedToPoint:[theEvent locationInNode:self]];
}
- (void)mouseUp:(NSEvent *)theEvent {
    [self touchUpAtPoint:[theEvent locationInNode:self]];
}

-(void)update:(CFTimeInterval)currentTime {
    if (_nes->display().ready_for_display_)
    {
        CGSize displaySize = _display_sprite.size;
        [_display_texture modifyPixelDataWithBlock:^void(void* pixelData, size_t len) {modifyPixedData(*_nes, (NesDisplay::Color*)pixelData, len, displaySize);} ];
    }
}

@end
