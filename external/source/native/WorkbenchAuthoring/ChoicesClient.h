#pragma once
#include <QObject>
#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include <functional>

namespace gc {
struct EntityChoice {
    QString id;
    QString name;
    QString label;
};
enum class ChoiceKind { Values, Entities, Context };
struct ChoicesReply {
    QString state;
    QString world;
    QStringList values;
    QString error;
    QStringList duplicateValues;
    QVector<EntityChoice> entities;
};
struct EntityEditReply {
    QString state;
    QString name;
    QString reason;
    QString error;
};

// Deferred, in-process calls to the fixed GC dropdown script handlers.
// No listener, helper process, retry, or alternate executor.
class ChoicesClient
{
public:
    using Completed = std::function<void(ChoicesReply)>;
    using EditCompleted = std::function<void(EntityEditReply)>;
    // Destroy the returned object to cancel observation. The handler is read-only.
    static QObject *fetch(QObject *lifetime, const QString &adapterBuild,
        const QString &provider, Completed completed, ChoiceKind kind = ChoiceKind::Values);
    // prepare_scalar is read-only; commit_scalar may write and is never retried.
    // Destroying the returned object cancels observation, not a dispatched edit.
    // Caller supplies one token shared by preparation and its single commit.
    static QObject *edit(QObject *lifetime, const QString &adapterBuild,
        QJsonObject request, EditCompleted completed);
};
}
