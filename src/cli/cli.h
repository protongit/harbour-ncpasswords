#ifndef CLI_H
#define CLI_H

#include <QStringList>

namespace Cli {

int run(const QStringList &arguments);
int selfTest();

} // namespace Cli

#endif // CLI_H
