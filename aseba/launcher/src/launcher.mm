#include "launcher.h"
#include <Foundation/Foundation.h>
#ifdef Q_OS_IOS
#include <UIKit/UIKit.h>
#else
#include <AppKit/NSWorkspace.h>
#endif
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

#include <ifaddrs.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

/// Returns the addresses of the discard service (port 9) on every
/// broadcast-capable interface.
///
/// Each array entry contains either a `sockaddr_in` or `sockaddr_in6`.
static NSArray<NSData *> * addressesOfDiscardServiceOnBroadcastCapableInterfaces(void) {
    struct ifaddrs * addrList = NULL;
    int err = getifaddrs(&addrList);
    if (err != 0) {
        return @[];
    }
    NSMutableArray<NSData *> * result = [NSMutableArray array];
    for (struct ifaddrs * cursor = addrList; cursor != NULL; cursor = cursor->ifa_next) {
        if ( (cursor->ifa_flags & IFF_BROADCAST) &&
             (cursor->ifa_addr != NULL)
           ) {
            switch (cursor->ifa_addr->sa_family) {
            case AF_INET: {
                struct sockaddr_in sin = *(struct sockaddr_in *) cursor->ifa_addr;
                sin.sin_port = htons(9);
                NSData * addr = [NSData dataWithBytes:&sin length:sizeof(sin)];
                [result addObject:addr];
            } break;
            case AF_INET6: {
                struct sockaddr_in6 sin6 = *(struct sockaddr_in6 *) cursor->ifa_addr;
                sin6.sin6_port = htons(9);
                NSData * addr = [NSData dataWithBytes:&sin6 length:sizeof(sin6)];
                [result addObject:addr];
            } break;
            default: {
                // do nothing
            } break;
            }
        }
    }
    freeifaddrs(addrList);
    return result;
}


#ifdef Q_OS_IOS
#include <WebKit/WebKit.h>

@interface LauncherDelegate : NSObject<WKNavigationDelegate,UIScrollViewDelegate,WKUIDelegate,WKScriptMessageHandler>
@property (strong,nonatomic) WKWebView* mwebview;
+(WKWebView*)createWebViewWithBaseURL:(NSURL*)url;
@end

@implementation LauncherDelegate

+(void)closeCurrentWebView {
    if([self shareInstance].mwebview !=nil)
    {
        [[self shareInstance].mwebview removeFromSuperview];
        [self shareInstance].mwebview = nil;
    }
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

+(void)askBeforeQuit{
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Are you sure to quit?"
                                                                                message:nil
                                                                         preferredStyle:UIAlertControllerStyleAlert];
       [alertController addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
               if([self shareInstance].mwebview !=nil){
               [[self shareInstance].mwebview removeFromSuperview];
               [self shareInstance].mwebview = nil;
           }
           [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
       }]];
       [alertController addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
           return;
       }]];
       
       [[[[UIApplication sharedApplication] keyWindow]rootViewController] presentViewController:alertController animated:YES completion:^{}];
}

+ (LauncherDelegate*)shareInstance {
    static LauncherDelegate *webviewmanager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        webviewmanager = [[self alloc] init];
    });
    return webviewmanager;
}

+(WKWebView*)createWebViewWithBaseURL:(NSURL*)url {
    
    if([self shareInstance].mwebview == nil)
    {
        WKWebViewConfiguration* conf = [[WKWebViewConfiguration alloc] init];
        conf.preferences.javaScriptCanOpenWindowsAutomatically = true;
        conf.websiteDataStore = [WKWebsiteDataStore defaultDataStore];
        /*
         
         Need to add this code in  lib/download-blob.js
         
         if (/ipad/i.test(navigator.userAgent)) {
             var reader = new FileReader();
             reader.readAsDataURL(blob);
             reader.onloadend = function () {
                var base64data = reader.result;
                window.webkit.messageHandlers.blobReady.postMessage({data: base64data, filename: filename});
            }
         }
         */
        
        [conf.userContentController addScriptMessageHandler:[self shareInstance] name:@"blobReady"];
        conf.applicationNameForUserAgent = @"Thymio Suite iPad";
    
        [self shareInstance].mwebview = [[WKWebView alloc] initWithFrame:[[[UIApplication sharedApplication] keyWindow]rootViewController].view.bounds  configuration:conf];
        [self shareInstance].mwebview.scrollView.bounces = false;
        [self shareInstance].mwebview.scrollView.scrollEnabled = false;
        [self shareInstance].mwebview.scrollView.delegate = [self shareInstance];
        [[self shareInstance].mwebview setNavigationDelegate:[self shareInstance]];
        [[self shareInstance].mwebview setUIDelegate:[self shareInstance]];
        [[self shareInstance].mwebview setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
        
        //Adding close button
        UIButton* b = [UIButton buttonWithType:UIButtonTypeSystem];
        [b setTitle:@"X" forState:UIControlStateNormal];
        [b setTintColor:UIColor.blueColor];
        [b setBackgroundColor:UIColor.whiteColor];
        
        [b.layer setCornerRadius:18];
        [[b.heightAnchor constraintEqualToConstant:36] setActive:YES];
        [[b.widthAnchor constraintEqualToConstant:36] setActive:YES];
        
        [b setTranslatesAutoresizingMaskIntoConstraints:NO];
        [[self shareInstance].mwebview addSubview:b];
        NSString *urlNSStr = url.absoluteString;
		if (isScratch(urlNSStr)){
			[[b.topAnchor constraintEqualToAnchor: [self shareInstance].mwebview.topAnchor constant:5] setActive:YES];
        } else {
			[[b.bottomAnchor constraintEqualToAnchor: [self shareInstance].mwebview.bottomAnchor constant:-5] setActive:YES];	
        }
        [[b.rightAnchor constraintEqualToAnchor: [self shareInstance].mwebview.rightAnchor constant:-15] setActive:YES];
        
        [b addTarget:self  action:@selector(askBeforeQuit) forControlEvents:UIControlEventTouchUpInside];
      
    }

    QString s = QFileInfo(QCoreApplication::applicationDirPath() + "/webapps/").absolutePath();
    NSURL* pathURL = [NSURL fileURLWithPath:s.toNSString()];
    [[self shareInstance].mwebview loadFileURL:url allowingReadAccessToURL:pathURL];
    return [self shareInstance].mwebview;
    
}

bool isScratch(NSString *URL){
   std::string urlStr = std::string([URL UTF8String]);
   return urlStr.find("scratch") != std::string::npos;
}

//File Saving, via the 'blobReady' handler
- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message {
    // window.webkit.messageHandlers.blobReady.postMessage(blob);
    if([message.name isEqualToString:@"blobReady"])
    {
        NSDictionary* d = [message body];
        NSString* b64 = [d objectForKey:@"data"];
        NSString* name = [d objectForKey:@"filename"];
        if(name.length<=4)
            name = [@"Thymio" stringByAppendingString:name];
        NSData* datas = [[NSData alloc] initWithBase64EncodedString:[[b64 componentsSeparatedByString:@","] lastObject] options:0];
        //First save it in the app with a real name
        // create url
        NSURL *url;
        if([name containsString:@".vpl3"]){
            url = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingString:[name stringByReplacingOccurrencesOfString:@".vpl3" withString:@".json"]]];
        } else if([name containsString:@".sb3"]){
            url = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingString:[name stringByAppendingString:@".zip"]]];
        } else {
            url = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingString:name]];
        }
        [datas writeToURL:url atomically:NO];
        
        //Transfer it to the save dialog
        UIActivityViewController* saveDial = [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
        saveDial.modalPresentationStyle = UIModalPresentationPopover;
        saveDial.popoverPresentationController.sourceView = [[[UIApplication sharedApplication] keyWindow]rootViewController].view;
        saveDial.popoverPresentationController.sourceRect = CGRectMake(50, 50, 300, 300);
       
        [[[[UIApplication sharedApplication] keyWindow]rootViewController] presentViewController:saveDial animated:YES completion:^{
            
        }];
        
    }
}
- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:message
                                                                             message:nil
                                                                      preferredStyle:UIAlertControllerStyleAlert];
    [alertController addAction:[UIAlertAction actionWithTitle:@"OK"
                                                        style:UIAlertActionStyleCancel
                                                      handler:^(UIAlertAction *action) {
                                                          completionHandler();
                                                      }]];
    
    
    [[[[UIApplication sharedApplication] keyWindow]rootViewController] presentViewController:alertController animated:YES completion:^{}];
}
- (void)webView:(WKWebView *)webView runJavaScriptConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(BOOL result))completionHandler {
 
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:message
                                                                                message:nil
                                                                         preferredStyle:UIAlertControllerStyleAlert];
       [alertController addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
           completionHandler(YES);
       }]];
       [alertController addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
           completionHandler(NO);
       }]];
       
       [[[[UIApplication sharedApplication] keyWindow]rootViewController] presentViewController:alertController animated:YES completion:^{}];
}


-(void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    //disabling zoom
    if(webView.scrollView.pinchGestureRecognizer && webView.scrollView.pinchGestureRecognizer.isEnabled)
    {
        [webView.scrollView.pinchGestureRecognizer setEnabled:NO];
    }

    NSString* urlRequest = [[[navigationAction request] URL] absoluteString];
    if([urlRequest containsString:@"https://scratch.mit.edu"])
    {
        decisionHandler(WKNavigationActionPolicyCancel);
        return;
    }
    decisionHandler(WKNavigationActionPolicyAllow);
    
}

//correction done to the inner website when loaded
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    //When loaded, cleaning the useless elements in the view
    NSString* removeNotFinishedFunctions = @"var myClasses = document.querySelectorAll('.menu-bar_coming-soon_3yU1L'),\n i = 0,\n l = myClasses.length; \n   for (i; i < l; i++) { \n   myClasses[i].style.display = 'none'; \n   } ; document.querySelector('.menu-bar_feedback-link_1BnAR').firstElementChild.style.display = 'none'; ";
    [webView evaluateJavaScript:removeNotFinishedFunctions completionHandler:nil];
    
    //Copy the document.title into top bar.
    NSString* titleObserver = @"var target = document.querySelector('title');var observer = new MutationObserver(function(mutations) {mutations.forEach(function(mutation) {document.querySelector('.menu-bar_main-menu_3wjWH').lastChild.replaceWith(document.title)});});var config = {childList: true,};observer.observe(target, config);";
    [webView evaluateJavaScript:titleObserver completionHandler:nil];
    
}


- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    qDebug() << QString([error.localizedDescription cStringUsingEncoding:0]) ;
}
- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation {
    NSString *javascript = @"var meta = document.createElement('meta');meta.setAttribute('name', 'viewport');meta.setAttribute('content', 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no');document.getElementsByTagName('head')[0].appendChild(meta);";
    [webView evaluateJavaScript:javascript completionHandler:nil];
}

- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView {
    return nil;
}

@end
#endif //Q_OS_IOS

namespace mobsya {
#ifdef Q_OS_IOS
void Launcher::OpenUrlInNativeWebView(const QUrl& qurl) {
    WKWebView* v = [LauncherDelegate createWebViewWithBaseURL:qurl.toNSURL()];
    if(v.superview == nil)
    {
        [[[[UIApplication sharedApplication] keyWindow]rootViewController].view addSubview:v];
    }
    
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
}

/// Does a best effort attempt to trigger the local network privacy alert.
///
/// It works by sending a UDP datagram to the discard service (port 9) of every
/// IP address associated with a broadcast-capable interface interface. This
/// should trigger the local network privacy alert, assuming the alert hasn’t
/// already been displayed for this app.
///
/// This code takes a ‘best effort’. It handles errors by ignoring them. As
/// such, there’s guarantee that it’ll actually trigger the alert.
///
/// - note: iOS devices don’t actually run the discard service. I’m using it
/// here because I need a port to send the UDP datagram to and port 9 is
/// always going to be safe (either the discard service is running, in which
/// case it will discard the datagram, or it’s not, in which case the TCP/IP
/// stack will discard it).
///
/// There should be a proper API for this (r. 69157424).
///
/// For more background on this, see [Triggering the Local Network Privacy Alert](https://developer.apple.com/forums/thread/663768).
extern void Launcher::triggerLocalNetworkPrivacyAlertObjC() const {
    int sock4 = socket(AF_INET, SOCK_DGRAM, 0);
    int sock6 = socket(AF_INET6, SOCK_DGRAM, 0);
    
    if ((sock4 >= 0) && (sock6 >= 0)) {
        char message = '!';
        NSArray<NSData *> * addresses = addressesOfDiscardServiceOnBroadcastCapableInterfaces();
        for (NSData * address in addresses) {
            int sock = ((const struct sockaddr *) address.bytes)->sa_family == AF_INET ? sock4 : sock6;
            //(void) sendto(sock, &message, sizeof(message), MSG_DONTWAIT, address.bytes, (socklen_t) address.length);
        }
    }
    
    // If we failed to open a socket, the descriptor will be -1 and it’s safe to
    // close that (it’s guaranteed to fail with `EBADF`).
    close(sock4);
    close(sock6);
}
#endif

auto QStringListToNSArray(const QStringList &list)
{
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:NSUInteger(list.size())];
    for (auto && str : list){
        [result addObject:str.toNSString()];
    }
    return result;
}

#ifdef Q_OS_OSX

bool Launcher::hasOsXBundle(const QString& name) const {
    const auto path = QDir(QCoreApplication::applicationDirPath() +
                           QStringLiteral("/../Applications/%1.app").arg(name)).absolutePath();

    auto* bundle = [NSBundle bundleWithPath:path.toNSString()];
    return bundle != nullptr;
}

bool Launcher::doLaunchPlaygroundBundle() const {
    const auto path = QDir(QCoreApplication::applicationDirPath() +
                           QStringLiteral("/../Applications/AsebaPlayground.app")).absolutePath();
    auto* bundle = [NSBundle bundleWithPath:path.toNSString()];
    if(!bundle) {
        NSLog(@"Unable to find the bundle");
        return false;
    }
    auto ws = [NSWorkspace sharedWorkspace];
    [ws launchApplicationAtURL:[bundle bundleURL]
        options:NSWorkspaceLaunchNewInstance
        configuration:@{} error:nil];
    return true;

}
    
bool Launcher::doLaunchOsXBundle(const QString& name, const QVariantMap &args) const {
    //Get the bundle path
    const auto path = QDir(QCoreApplication::applicationDirPath() +
                           QStringLiteral("/../Applications/%1.app").arg(name)).absolutePath();
    qDebug() << path;

    auto* bundle = [NSBundle bundleWithPath:path.toNSString()];
    if(!bundle) {
        NSLog(@"Unable to find the bundle");
        return false;
    }

    auto url = [bundle bundleURL];
    if(!url) {
        NSLog(@"Unable to find the bundle url");
        return false;
    }

    NSLog(@"Bundle url %@", url);

    QUrl appUrl(QStringLiteral("mobsya:connect-to-device?uuid=%1")
                .arg(args.value("uuid").toString()));


    auto urls = @[appUrl.toNSURL()];
    auto ws = [NSWorkspace sharedWorkspace];
    [ws openURLs:urls
                 withApplicationAtURL:url
                 options:NSWorkspaceLaunchNewInstance
                 configuration:@{} error:nil];    
    return true;

}
#endif //OSX
}
