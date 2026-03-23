#import "BaseAuthenticator.h"
#include "jni.h"

JNIEXPORT jstring JNICALL Java_net_kdt_pojavlaunch_value_MinecraftAccount_getAccessTokenFromKeychain(JNIEnv *env, jclass clazz, jstring xuid) {
    (void)clazz;
    if (xuid == NULL) {
        return (*env)->NewStringUTF(env, "offline");
    }

    const char *xuidC = (*env)->GetStringUTFChars(env, xuid, 0);
    NSString *profileKey = @(xuidC);
    NSString *accessToken = nil;
    if ([profileKey hasPrefix:@"offline:"]) {
        accessToken = @"offline";
    } else {
        accessToken = [NSClassFromString(@"MicrosoftAuthenticator") tokenDataOfProfile:profileKey][@"accessToken"];
    }
    (*env)->ReleaseStringUTFChars(env, xuid, xuidC);
    if (accessToken.length == 0) {
        accessToken = @"offline";
    }
    return (*env)->NewStringUTF(env, accessToken.UTF8String);
}
