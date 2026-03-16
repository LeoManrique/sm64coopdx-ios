#ifdef TARGET_IOS

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#include "platform.h"
#include "rom_checker.h"

// ---- Documents directory for Files app access ----

const char *platform_ios_get_user_path(void) {
    static char path[SYS_MAX_PATH] = { 0 };
    if (path[0] != '\0') return path;

    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDir = paths.firstObject;
    if (!documentsDir) return NULL;

    strncpy(path, [documentsDir UTF8String], SYS_MAX_PATH - 1);

    // Migrate data from old SDL_GetPrefPath location (Library/Preferences/sm64coopdx/)
    NSString *oldPath = [NSString stringWithFormat:@"%@/Library/Preferences/sm64coopdx", NSHomeDirectory()];
    NSFileManager *fm = [NSFileManager defaultManager];
    BOOL isDir = NO;

    if ([fm fileExistsAtPath:oldPath isDirectory:&isDir] && isDir) {
        NSArray *oldContents = [fm contentsOfDirectoryAtPath:oldPath error:nil];
        if (oldContents.count > 0) {
            for (NSString *item in oldContents) {
                NSString *src = [oldPath stringByAppendingPathComponent:item];
                NSString *dst = [documentsDir stringByAppendingPathComponent:item];
                if (![fm fileExistsAtPath:dst]) {
                    [fm moveItemAtPath:src toPath:dst error:nil];
                }
            }
            [fm removeItemAtPath:oldPath error:nil];
        }
    }

    return path;
}

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
