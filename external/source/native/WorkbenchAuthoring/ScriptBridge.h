#pragma once
#include <QByteArray>
#include <QString>

namespace gc {
// Fixed dropdown handlers only. Runs on the Workbench application thread.
// No socket, settings changes, external executor, or retained VM objects.
QByteArray dispatchChoices(const QByteArray &request, QString &error);
}
