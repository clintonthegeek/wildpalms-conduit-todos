#ifndef TODOCONDUIT_H
#define TODOCONDUIT_H

#include "sync/conduit.h"

namespace Sync {

/**
 * @brief Conduit for Palm ToDos <-> iCalendar VTODO files
 *
 * Syncs:
 *   - Palm ToDoDB (binary format)
 *   - Local .ics files (iCalendar VTODO format)
 *
 * Uses TodoMapper for format conversion.
 * Supports bidirectional category sync.
 */
class TodoConduit : public SyncConduitBase
{
    Q_OBJECT

public:
    explicit TodoConduit(QObject *parent = nullptr);

    // ========== Conduit Identity ==========

    QString conduitId() const override { return "todos"; }
    QString displayName() const override { return "Tasks"; }
    QStringList palmDatabaseNames() const override { return {"ToDoDB"}; }
    QString fileExtension() const override { return ".ics"; }

    // ========== Record Conversion ==========

    BackendRecord* palmToBackend(PilotRecord *palmRecord,
                                  SyncContext *context) override;

    PilotRecord* backendToPalm(BackendRecord *backendRecord,
                                SyncContext *context) override;

    bool recordsEqual(PilotRecord *palm, BackendRecord *backend,
                       const SyncContext *context) const override;

    QString palmRecordDescription(PilotRecord *record,
                                   const SyncContext *context) const override;

    // ========== UI Contribution ==========
    QIcon icon() const override {
        return QIcon::fromTheme(QStringLiteral("view-pim-tasks"));
    }
    QString description() const override {
        return QStringLiteral("Synchronizes Palm ToDoDB with iCalendar VTODO files");
    }
    bool hasView() const override { return true; }
    QWidget *createView(QWidget *parent) override;
    QString viewName() const override { return QStringLiteral("Tasks"); }
    QIcon viewIcon() const override {
        return QIcon::fromTheme(QStringLiteral("view-pim-tasks"));
    }

    // ========== Conflict Display ==========

    void enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                 bool isSourceSide) const override;
    QString formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const override;
};

} // namespace Sync

#endif // TODOCONDUIT_H
