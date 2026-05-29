#include "hubtodoreader.h"

#include <syncbackendbase.h>
#include <backendrecord.h>

namespace {

// See hubcalendarreader.cpp for the rationale; GenericSqliteBackend
// returns records with ids of the form `<collectionId>\x01<origId>`.
QString stripCollectionPrefix(const QString &recordId,
                              const QString &collectionId)
{
    const QString prefix = collectionId + QLatin1Char('\x01');
    if (recordId.startsWith(prefix))
        return recordId.mid(prefix.size());
    return recordId;
}

} // namespace

namespace WildPalms::TodoPlugin {

HubTodoReader::HubTodoReader(Kalburator::Sync::SyncBackendBase *hub,
                             QString collectionId)
    : m_hub(hub)
    , m_collectionId(std::move(collectionId))
{
    Q_ASSERT(m_hub);
}

QStringList HubTodoReader::listRecordIds() const
{
    if (!m_hub) return {};
    QStringList ids;
    const auto records = m_hub->loadRecords(m_collectionId);
    ids.reserve(records.size());
    for (const auto &r : records) {
        if (!r.isDeleted)
            ids.append(stripCollectionPrefix(r.id, m_collectionId));
    }
    return ids;
}

QByteArray HubTodoReader::recordBytes(const QString &id) const
{
    if (!m_hub) return {};
    const auto records = m_hub->loadRecords(m_collectionId);
    for (const auto &r : records) {
        if (r.isDeleted) continue;
        if (stripCollectionPrefix(r.id, m_collectionId) == id)
            return r.data;
    }
    return {};
}

} // namespace WildPalms::TodoPlugin
