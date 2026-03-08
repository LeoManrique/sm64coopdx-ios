#ifdef TARGET_IOS

#import <UIKit/UIKit.h>

#include "rom_checker.h"

unsigned int platform_ios_get_refresh_rate(void) {
    return (unsigned int)[UIScreen mainScreen].maximumFramesPerSecond;
}

// ---- ROM File Picker ----

static bool sPickerActive = false;

@interface SM64RomPickerDelegate : NSObject <UIDocumentPickerDelegate>
@end

@implementation SM64RomPickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url = urls.firstObject;
    if (url) {
        BOOL accessed = [url startAccessingSecurityScopedResource];

        // Copy to temp so the file remains accessible after releasing the security scope
        NSString *tempPath = [NSTemporaryDirectory() stringByAppendingPathComponent:url.lastPathComponent];
        [[NSFileManager defaultManager] removeItemAtPath:tempPath error:nil];

        NSError *error = nil;
        [[NSFileManager defaultManager] copyItemAtURL:url
                                                toURL:[NSURL fileURLWithPath:tempPath]
                                                error:&error];

        if (accessed) {
            [url stopAccessingSecurityScopedResource];
        }

        if (!error) {
            rom_on_drop_file([tempPath UTF8String]);
        }
    }
    sPickerActive = false;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    sPickerActive = false;
}

@end

static SM64RomPickerDelegate *sPickerDelegate = nil;

void platform_ios_open_rom_picker(void) {
    if (sPickerActive) return;
    sPickerActive = true;

    if (!sPickerDelegate) {
        sPickerDelegate = [[SM64RomPickerDelegate alloc] init];
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        // Use "public.data" UTI to allow selecting any file, then validate after selection
        UIDocumentPickerViewController *picker =
            [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.data"]
                                                                  inMode:UIDocumentPickerModeImport];
        picker.delegate = sPickerDelegate;
        picker.allowsMultipleSelection = NO;

        UIViewController *rootVC = [UIApplication sharedApplication].keyWindow.rootViewController;
        if (rootVC) {
            [rootVC presentViewController:picker animated:YES completion:nil];
        } else {
            sPickerActive = false;
        }
    });
}

bool platform_ios_is_picker_active(void) {
    return sPickerActive;
}

#endif
