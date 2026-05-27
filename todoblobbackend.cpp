#include "todoblobbackend.h"

#include "palm/calendar/categorymappingstore.h"
#include "palm/sync/palmbackend.h"
#include "palm/sync/palmrecord.h"

#include "backendrecord.h"
#include "collectioninfo.h"

#include <QDateTime>
#include <QStringList>

namespace WildPalms::TodoPlugin {

namespace {

QString idForPalmRecord(std::uint32_t recordId)
{
    return WildPalms::PalmSync::PalmBackend::encodeRecordId(
        QStringLiteral("ToDoDB"), recordId);
}

bool decodeId(const QString &id, std::uint32_t *outRecordId)
{
    // PalmBackend::decodeRecordId reconstructs the dbName by uppercasing
    // only the first letter ("palm:todo:N" -> "TodoDB"), which doesn't
    // match the canonical "ToDoDB" exactly. Compare case-insensitively.
    QString dbName;
    return WildPalms::PalmSync::PalmBackend::decodeRecordId(id, &dbName, outRecordId)
        && dbName.compare(QLatin1String("ToDoDB"), Qt::CaseInsensitive) == 0;
}

} // namespace

TodoBlobBackend::TodoBlobBackend(
    WildPalms::PalmSync::PalmBackend *palmBackend,
    const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
    QObject *parent)
    : Kalburator::Sync::SyncBackend(parent)
    , m_palmBackend(palmBackend)
    , m_categoryStore(categoryStore)
{
}

TodoBlobBackend::~TodoBlobBackend() = default;

QString TodoBlobBackend::backendId()   const { return QStringLiteral("palm-todo"); }
QString TodoBlobBackend::displayName() const { return QStringLiteral("Palm ToDo"); }
bool    TodoBlobBackend::isAvailable() const
{
    return m_palmBackend != nullptr && m_palmBackend->isAvailable();
}

QList<Kalburator::Sync::CollectionInfo> TodoBlobBackend::availableCollections()
{
    QList<Kalburator::Sync::CollectionInfo> out;

    // Domain-level collection: returns all records regardless of category slot.
    Kalburator::Sync::CollectionInfo domain;
    domain.id   = QStringLiteral("palm:todo");
    domain.name = QStringLiteral("ToDo");
    domain.type = QStringLiteral("todo");
    out.append(domain);

    Kalburator::Sync::CollectionInfo unfiled;
    unfiled.id   = collectionIdForSlot(0);
    unfiled.name = QStringLiteral("Unfiled");
    unfiled.type = QStringLiteral("calendar");   // VTODO is calendar-typed
    out.append(unfiled);

    if (!m_categoryStore) return out;

    const QList<int> populated = m_categoryStore->populatedSlots(
        QStringLiteral("ToDoDB"));
    for (int slot : populated) {
        Kalburator::Sync::CollectionInfo info;
        info.id   = collectionIdForSlot(slot);
        info.name = m_categoryStore->slotName(
            QStringLiteral("ToDoDB"), slot);
        info.type = QStringLiteral("calendar");
        out.append(info);
    }
    return out;
}

Kalburator::Sync::CollectionInfo TodoBlobBackend::collectionInfo(
    const QString &collectionId)
{
    for (const auto &c : availableCollections()) {
        if (c.id == collectionId) return c;
    }
    return {};
}

QString TodoBlobBackend::createCollection(
    const Kalburator::Sync::CollectionInfo &)
{
    // Slots are governed by the device's AppInfo block — plugin
    // doesn't create new ones. Returning empty signals "not supported".
    return {};
}

QList<Kalburator::Sync::BackendRecord> TodoBlobBackend::loadRecords(
    const QString &collectionId)
{
    QList<Kalburator::Sync::BackendRecord> out;

    // Domain-level collection: return ALL records unfiltered.
    if (collectionId == QStringLiteral("palm:todo")) {
        if (!m_palmBackend) return out;
        for (const auto &pr : m_palmBackend->loadPalmRecords(QStringLiteral("ToDoDB"))) {
            if (pr.isDeleted()) continue;
            Kalburator::Sync::BackendRecord br;
            br.id           = idForPalmRecord(pr.recordId);
            br.data         = pr.toWireBytes();
            br.type         = QStringLiteral("todo");
            br.lastModified = pr.lastModified;
            br.contentHash  = pr.contentHash();
            out.append(br);
        }
        return out;
    }

    const int slot = slotFromCollectionId(collectionId);
    if (slot < 0 || !m_palmBackend) return out;

    const auto records = m_palmBackend->loadPalmRecords(QStringLiteral("ToDoDB"));
    for (const auto &pr : records) {
        if (static_cast<int>(pr.category) != slot) continue;
        if (pr.isDeleted()) continue;

        Kalburator::Sync::BackendRecord br;
        br.id           = idForPalmRecord(pr.recordId);
        br.data         = pr.toWireBytes();
        br.type         = QStringLiteral("todo");
        br.lastModified = pr.lastModified;
        br.contentHash  = pr.contentHash();
        out.append(br);
    }
    return out;
}

std::optional<Kalburator::Sync::BackendRecord>
TodoBlobBackend::loadRecord(const QString &recordId)
{
    std::uint32_t rid = 0;
    if (!decodeId(recordId, &rid) || !m_palmBackend) return std::nullopt;
    auto pr = m_palmBackend->loadPalmRecord(QStringLiteral("ToDoDB"), rid);
    if (!pr) return std::nullopt;

    Kalburator::Sync::BackendRecord br;
    br.id           = recordId;
    br.data         = pr->toWireBytes();
    br.type         = QStringLiteral("todo");
    br.lastModified = pr->lastModified;
    br.contentHash  = pr->contentHash();
    return br;
}

QString TodoBlobBackend::createRecord(
    const QString &collectionId,
    const Kalburator::Sync::BackendRecord &record)
{
    if (!m_palmBackend) return {};
    if (record.data.isEmpty()) return {};
    // For the domain-level collection, slot comes from the record's wire bytes.
    if (collectionId == QStringLiteral("palm:todo")) {
        auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
        pr.recordId     = 0;   // device assigns
        pr.lastModified = record.lastModified.isValid()
            ? record.lastModified
            : QDateTime::currentDateTimeUtc();
        const auto newId = m_palmBackend->createPalmRecord(
            QStringLiteral("ToDoDB"), pr);
        if (newId == 0) return {};
        return idForPalmRecord(newId);
    }
    const int slot = slotFromCollectionId(collectionId);
    if (slot < 0) return {};

    auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
    pr.category     = static_cast<std::uint8_t>(slot);
    pr.recordId     = 0;   // device assigns
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    const auto newId = m_palmBackend->createPalmRecord(
        QStringLiteral("ToDoDB"), pr);
    if (newId == 0) return {};
    return idForPalmRecord(newId);
}

bool TodoBlobBackend::updateRecord(
    const Kalburator::Sync::BackendRecord &record)
{
    std::uint32_t rid = 0;
    if (!decodeId(record.id, &rid) || !m_palmBackend) return false;
    if (record.data.isEmpty()) return false;

    // Look up the existing record to recover its slot (the
    // BackendRecord's id alone doesn't carry the slot).
    auto existing = m_palmBackend->loadPalmRecord(
        QStringLiteral("ToDoDB"), rid);
    if (!existing) return false;
    const int slot = static_cast<int>(existing->category);

    auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(record.data);
    pr.recordId     = rid;
    pr.category     = static_cast<std::uint8_t>(slot);
    pr.lastModified = record.lastModified.isValid()
        ? record.lastModified
        : QDateTime::currentDateTimeUtc();

    return m_palmBackend->updatePalmRecord(QStringLiteral("ToDoDB"), pr);
}

bool TodoBlobBackend::deleteRecord(const QString &recordId)
{
    if (!m_palmBackend) return false;
    std::uint32_t rid = 0;
    if (!decodeId(recordId, &rid)) return false;
    // Use the dbName-aware delete so the canonical "ToDoDB" name is
    // used (PalmBackend::decodeRecordId would yield "TodoDB", which the
    // device wouldn't recognise).
    return m_palmBackend->deletePalmRecord(QStringLiteral("ToDoDB"), rid);
}

QList<Kalburator::Sync::BackendRecord>
TodoBlobBackend::modifiedSince(const QString &collectionId,
                               const QDateTime &since)
{
    const int slot = slotFromCollectionId(collectionId);
    QList<Kalburator::Sync::BackendRecord> out;
    if (slot < 0 || !m_palmBackend) return out;

    // Forward to PalmBackend's underlying list, then filter+transcode.
    const auto records = m_palmBackend->loadPalmRecords(QStringLiteral("ToDoDB"));
    for (const auto &pr : records) {
        if (static_cast<int>(pr.category) != slot) continue;
        if (since.isValid() && pr.lastModified <= since) continue;

        Kalburator::Sync::BackendRecord br;
        br.id           = idForPalmRecord(pr.recordId);
        br.data         = pr.toWireBytes();
        br.type         = QStringLiteral("todo");
        br.lastModified = pr.lastModified;
        br.contentHash  = pr.contentHash();
        out.append(br);
    }
    return out;
}

QStringList TodoBlobBackend::deletedSince(const QString & /*collectionId*/,
                                          const QDateTime &since)
{
    if (!m_palmBackend) return {};
    // KNOWN LIMITATION (mirrors CalendarBlobBackend): returns deletions
    // from ALL category slots, not just the requested collection. The
    // record's category byte is lost when the record is deleted.
    // BlobSyncEngine tolerates over-broad returns. Tighten when
    // PalmBackend grows a slot-aware deletedSince variant (post-E.15).
    const QString sourceCollection =
        WildPalms::PalmSync::PalmBackend::encodeCollectionId(
            QStringLiteral("ToDoDB"));
    return m_palmBackend->deletedSince(sourceCollection, since);
}

bool TodoBlobBackend::supportsDeleteTracking() const
{
    return m_palmBackend && m_palmBackend->supportsDeleteTracking();
}

int TodoBlobBackend::slotFromCollectionId(const QString &collectionId)
{
    static constexpr QLatin1String prefix(CollectionPrefix);
    if (!collectionId.startsWith(prefix)) return -1;
    bool ok = false;
    const int slot = collectionId.mid(prefix.size()).toInt(&ok);
    if (!ok || slot < 0 || slot > 15) return -1;
    return slot;
}

QString TodoBlobBackend::collectionIdForSlot(int slot)
{
    return QString::fromLatin1(CollectionPrefix) + QString::number(slot);
}

} // namespace WildPalms::TodoPlugin
