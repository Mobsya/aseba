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

namespace mobsya {

auto QStringListToNSArray(const QStringList &list)
{
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:NSUInteger(list.size())];
    for (auto && str : list){
        [result addObject:str.toNSString()];
    }
    return result;
}
#ifdef Q_OS_OSX
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
