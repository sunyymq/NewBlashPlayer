//
//  main.m
//  NewBlashPlayer
//
//  Created by liu enbao on 17/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

//#define VALGRIND_REXEC

#define VALGRIND "/usr/local/bin/valgrind"

int main(int argc, char * argv[]) {
#ifdef VALGRIND_REXEC
    /* Using the valgrind build config, rexec ourself
     * in valgrind */
    if (argc < 2 || (argc >= 2 && strcmp(argv[1], "-valgrind") != 0)) {
        execl(VALGRIND, VALGRIND, "--leak-check=full", argv[0], "-valgrind",
              NULL);
    }
#endif
    
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}
