#ifdef TARGET_IOS

#import <UIKit/UIKit.h>

unsigned int platform_ios_get_refresh_rate(void) {
    return (unsigned int)[UIScreen mainScreen].maximumFramesPerSecond;
}

#endif
