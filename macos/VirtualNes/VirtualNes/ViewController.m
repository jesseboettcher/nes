//
//  ViewController.m
//  VirtualNes
//
//  Created by Jesse Boettcher on 10/31/22.
//

#import "ViewController.h"
#import "GameScene.h"

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Load the SKScene from 'GameScene.sks'
    GameScene *scene = (GameScene *)[SKScene nodeWithFileNamed:@"GameScene"];
    
    [scene constructor];
    
    // Set the scale mode to scale to fit the window
    scene.scaleMode = SKSceneScaleModeAspectFill;
    
    // Present the scene
    [self.skView presentScene:scene];
    
    self.skView.showsFPS = YES;
    self.skView.showsNodeCount = YES;

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(shutdown)
                                                 name:NSApplicationWillTerminateNotification object:nil];
}

- (void) shutdown
{
    GameScene *scene = (GameScene *)[self.skView scene];
    [scene shutdown];
}

- (IBAction)loadRom:(id)sender
{
    // Create the File Open Dialog class.
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];

    // Enable the selection of files in the dialog.
    [openDlg setCanChooseFiles:YES];

    // Multiple files not allowed
    [openDlg setAllowsMultipleSelection:NO];

    // Can't select a directory
    [openDlg setCanChooseDirectories:NO];

    // Display the dialog. If the OK button was pressed,
    // process the files.
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        // Get an array containing the full filenames of all
        // files and directories selected.
        NSArray* urls = [openDlg URLs];
        NSString * file_path = (NSString *)[urls[0] path];
        [(GameScene *)[self.skView scene] loadROM:file_path];
    }
    [(GameScene *)[self.skView scene] execute:@"run"];
}

- (IBAction)interrupt:(id)sender
{
    [(GameScene *)[self.skView scene] interrupt];
}

- (IBAction)enterCommand:(id)sender
{
    NSTextField *field = [NSTextField textFieldWithString:@""];
    CGSize size;
    size.width = 200;
    size.height = [field frame].size.height;
    [field setFrameSize:size];
    
    NSAlert * alert = [[NSAlert alloc] init];
    alert.informativeText = @"Enter a command:";
    alert.accessoryView = field;
    alert.window.initialFirstResponder = field;
    [alert runModal];
    
    NSString* cmd = [field stringValue];
    [(GameScene *)[self.skView scene] execute:cmd];
}

@end
