#ifndef WILDPALMS_TODO_TODOBLOBBACKEND_H
#define WILDPALMS_TODO_TODOBLOBBACKEND_H

#include "iblobbackend.h"

#include <QObject>

namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::TodoPlugin {

/**
 * @brief Transcoding IBlobBackend wrapping PalmBackend's "ToDoDB".
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
class TodoBlobBackend : public QObject, public Kalburator::Sync::IBlobBackend
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

    // --- Identity ---
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
