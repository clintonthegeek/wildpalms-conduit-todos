#ifndef WILDPALMS_TODOS_HUBTODOREADER_H
#define WILDPALMS_TODOS_HUBTODOREADER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Kalburator::Sync { class SyncBackendBase; }

namespace WildPalms::TodoPlugin {

/**
 * @brief Thin per-domain facade over the canonical hub
 *        ("wp-hub" GenericSqliteBackend, collection "palm:todo").
 *
 * Read-only. The view (TaskView) talks to this facade -- never
 * to Kalburator::Sync::SyncBackendBase directly. The reader owns no Qt
 * signals; refresh is driven by PalmRuntime::syncCompleted.
 *
 * Lifetime: the hub pointer is borrowed and outlives the reader.
 * The reader is owned by TodoBackendPlugin; the plugin outlives
 * the view that borrows the reader pointer.
 */
class HubTodoReader {
public:
    HubTodoReader(Kalburator::Sync::SyncBackendBase *hub,
                  QString collectionId);

    QStringList listRecordIds() const;
    QByteArray  recordBytes(const QString &id) const;
    QString     collectionId() const { return m_collectionId; }

private:
    Kalburator::Sync::SyncBackendBase *m_hub;
    QString m_collectionId;
};

} // namespace WildPalms::TodoPlugin

#endif
