//
//  ViewController.m
//  NewBlashPlayer
//
//  Created by liu enbao on 17/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#import "RootViewController.h"
#import "NBPlayerViewController.h"

@interface RootViewController () {

}

@end

@implementation RootViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)onEnterScene:(id)sender {
    NBPlayerViewController* _playerViewControler = [[NBPlayerViewController alloc] initWithNibName:@"NBPlayerViewController" bundle:nil];
    
    [self presentViewController:_playerViewControler animated:YES completion:nil];
}

- (BOOL)shouldAutorotate {
    return YES;
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskPortrait;
}

@end
