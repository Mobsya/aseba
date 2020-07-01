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
        [b addTarget:self  action:@selector(closeCurrentWebView) forControlEvents:UIControlEventTouchUpInside];
      
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
