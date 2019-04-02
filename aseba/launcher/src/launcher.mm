#include "launcher.h"
#include <Foundation/Foundation.h>
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

bool Launcher::doLaunchOsXBundle(const QString& name, const QStringList & args) const {
    //Get the bundle path
    const auto path = QDir(QCoreApplication::applicationDirPath()+ QStringLiteral("/../Applications/%1.app/Contents/MacOS/%1").arg(name)).absolutePath();
    qDebug() << path;

    NSTask*  app = [[NSTask alloc] init];
    [app setLaunchPath: path.toNSString()];
    [app setArguments: QStringListToNSArray(args)];
    [app launch];
    [app autorelease];

}

}
