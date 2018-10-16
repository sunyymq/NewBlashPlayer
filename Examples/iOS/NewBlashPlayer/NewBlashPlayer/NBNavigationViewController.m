//
//  NBNavigationViewController.m
//  NewBlashPlayer
//
//  Created by liu enbao on 19/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#import "NBNavigationViewController.h"

@interface NBNavigationViewController ()

@end

@implementation NBNavigationViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

// support rotate
-(BOOL)shouldAutorotate{
    return [self.topViewController shouldAutorotate];
}

// support orientation
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    return [self.topViewController supportedInterfaceOrientations];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
