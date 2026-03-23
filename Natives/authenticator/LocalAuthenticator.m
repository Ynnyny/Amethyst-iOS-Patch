#include <CommonCrypto/CommonDigest.h>
#import "BaseAuthenticator.h"

static NSString *PLGenerateOfflineUUID(NSString *username) {
    NSString *seed = [NSString stringWithFormat:@"OfflinePlayer:%@", username];
    const char *seedCString = seed.UTF8String;
    unsigned char digest[CC_MD5_DIGEST_LENGTH];
    CC_MD5(seedCString, (CC_LONG)strlen(seedCString), digest);

    digest[6] = (digest[6] & 0x0F) | 0x30;
    digest[8] = (digest[8] & 0x3F) | 0x80;

    return [NSString stringWithFormat:
            @"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            digest[0], digest[1], digest[2], digest[3],
            digest[4], digest[5], digest[6], digest[7],
            digest[8], digest[9], digest[10], digest[11],
            digest[12], digest[13], digest[14], digest[15]];
}

@implementation LocalAuthenticator

- (void)loginWithCallback:(Callback)callback {
    NSString *username = [self.authData[@"input"] stringByTrimmingCharactersInSet:
                          [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (username.length == 0) {
        username = @"Player";
    }

    NSString *offlineUUID = PLGenerateOfflineUUID(username);
    self.authData[@"oldusername"] = self.authData[@"username"] = username;
    self.authData[@"profileId"] = offlineUUID;
    self.authData[@"profilePicURL"] = [NSString stringWithFormat:@"https://mc-heads.net/head/%@/120",
                                       offlineUUID];
    self.authData[@"accessToken"] = @"offline";
    self.authData[@"clientToken"] = @"offline";
    self.authData[@"xuid"] = [NSString stringWithFormat:@"offline:%@", username.lowercaseString];
    self.authData[@"expiresAt"] = @0;
    callback(nil, [super saveChanges]);
}

- (void)refreshTokenWithCallback:(Callback)callback {
    // Nothing to do
    callback(nil, YES);
}

@end
