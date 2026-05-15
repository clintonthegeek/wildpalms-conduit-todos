#ifndef WILDPALMS_TODO_TODOBLOBBACKEND_H
#define WILDPALMS_TODO_TODOBLOBBACKEND_H

#include "syncbackend.h"

#include <QObject>

namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::TodoPlugin {

/**
 * @brief Transcoding SyncBackend wrapping PalmBackend's "ToDoDB".
 *
 * Surfaces one collection per populated category slot:
 *   - "palm:todo/0"   "Unfiled" (always present)
 *   - "palm:todo/<N>" 1..15, present iff
 *     `categoryStore->slotName("ToDoDB", N)` is non-empty.
 *
 * Records route to/from these collections by PalmRecord::category.
 * loadRecords transcodes wire bytes -> VTODO bytes via TodoIcsTranscoder;
 * createRecord/updateRecord transcode VTODO -> wire and forward to
 * PalmBackend's category-aware createPalmRecord/updatePalmRecord.
 *
 * Lifetime: does NOT own palmBackend or categoryStore. Caller retains
 * ownership; both must outlive the backend.
 */
class TodoBlobBackend final : public Kalburator::Sync::SyncBackend
{
    Q_OBJECT
public:
    static constexpr const char *BackendId        = "palm-todo";
    static constexpr const char *PalmDbName       = "ToDoDB";
    static constexpr const char *CollectionPrefix = "palm:todo/";

    explicit TodoBlobBackend(
        WildPalms::PalmSync::PalmBackend *palmBackend,
        const WildPalms::PalmCalendar::CategoryMappingStore *categoryStore,
        QObject *parent = nullptr);
    ~TodoBlobBackend() override;

    // --- SyncBackend identity ---
    QString backendType() const override { return QStringLiteral("palm-todo"); }
    // K.8b T6 fix: use blob/raw to match the pre-T3 BlobBackendAdapter
    // wrapping. loadRecords() returns pre-transcoded ICS bytes; the engine
    // copies them verbatim via the identity blob/raw pipeline.
    QList<Kalburator::Shape::Shape> nativeShapes() const override {
        return { { Kalburator::Shape::DomainId{QStringLiteral("blob")},
                   Kalburator::Shape::EncodingId{QStringLiteral("raw")} } };
    }

    // --- IBlobBackend identity ---
    QString backendId()   const override;
    QString displayName() const override;
    bool    isAvailable() const override;

    // --- Collections ---
    QList<Kalburator::Sync::CollectionInfo> availableCollections() override;
    Kalburator::Sync::CollectionInfo collectionInfo(const QString &collectionId) override;
    QString createCollection(const Kalburator::Sync::CollectionInfo &info) override;

    // --- Records ---
    QList<Kalburator::Sync::BackendRecord> loadRecords(const QString &collectionId) override;
    std::optional<Kalburator::Sync::BackendRecord> loadRecord(const QString &recordId) override;
    QString createRecord(const QString &collectionId,
                         const Kalburator::Sync::BackendRecord &record) override;
    bool    updateRecord(const Kalburator::Sync::BackendRecord &record) override;
    bool    deleteRecord(const QString &recordId) override;

    // --- Change detection ---
    QList<Kalburator::Sync::BackendRecord> modifiedSince(
        const QString &collectionId, const QDateTime &since) override;
    QStringList deletedSince(const QString &collectionId, const QDateTime &since) override;
    bool        supportsDeleteTracking() const override;

    // --- Helpers (exposed for tests) ---
    /// Parse "palm:todo/<N>" -> N. Returns -1 on bad input.
    static int slotFromCollectionId(const QString &collectionId);
    /// Produce "palm:todo/<N>".
    static QString collectionIdForSlot(int slot);

    // --- SyncBackend calendar pure-virtuals — stubs; dispatchBlobSync never calls these ---
    void loadCalendars(const QString &) override {}
    void storeCalendars(const QString &,
                        const QList<KCalendarCore::MemoryCalendar *> &) override {}
    void startSync(const QString &,
                   KCalendarCore::MemoryCalendar *,
                   const QList<KCalendarCore::Incidence::Ptr> &,
                   const QList<KCalendarCore::Incidence::Ptr> &,
                   const QMap<QString, QString> &,
                   const Kalburator::Sync::TranscodingPlan &) override {}
    void removeItem(const QString &, const QString &) override {}
    Kalburator::Sync::PushOperation *pushItems(
        const QString &,
        const QList<KCalendarCore::Incidence::Ptr> &,
        const Kalburator::Sync::TranscodingPlan &) override { return nullptr; }

Q_SIGNALS:
    void recordCreated(const QString &recordId);
    void recordUpdated(const QString &recordId);
    void recordDeleted(const QString &recordId);
    void errorOccurred(const QString &error);
    void progressUpdated(int current, int total, const QString &message);

private:
    WildPalms::PalmSync::PalmBackend                     *m_palmBackend = nullptr;
    const WildPalms::PalmCalendar::CategoryMappingStore  *m_categoryStore = nullptr;
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOBLOBBACKEND_H
