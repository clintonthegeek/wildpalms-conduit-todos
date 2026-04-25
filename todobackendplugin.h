#ifndef WILDPALMS_TODO_TODOBACKENDPLUGIN_H
#define WILDPALMS_TODO_TODOBACKENDPLUGIN_H

#include <memory>

#include <QObject>

#include "core/ibackendplugin.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
class PalmDeviceConnection;

namespace WildPalms::TodoPlugin {

/**
 * @brief Third new-ABI WildPalms plugin (after Memo E.9, Calendar E.10).
 *
 * Provides:
 *   - TodoBlobBackend wrapping the shared PalmBackend (one
 *     collection per populated category slot under "ToDoDB").
 *   - No typed SyncBackend; libkalburator has no typed-todo upper
 *     layer (extract-on-second-consumer per parent spec).
 *   - TodoConflictHandler (completion-asymmetric overlay + Palm
 *     delegation).
 *
 * Owns the per-session CategoryMappingStore, populated from the
 * ToDoDB AppInfo block at createBackends() time.
 *
 * Surfaces TaskView as a main-window tab (reused unchanged from
 * the legacy TodoConduit).
 */
class TodoBackendPlugin : public QObject, public WildPalms::IBackendPlugin
{
    Q_OBJECT
    Q_INTERFACES(WildPalms::IBackendPlugin)
public:
    explicit TodoBackendPlugin(QObject *parent = nullptr);
    ~TodoBackendPlugin() override;

    // IPlugin
    QString pluginId()    const override;
    QString displayName() const override;
    QIcon   icon()        const override;
    QString description() const override;
    QString version()     const override;

    // IBackendPlugin
    QStringList      claimedDatabases() const override;
    ProvidedBackends createBackends(Kalburator::Sync::ISyncHost *host,
                                    PalmDeviceConnection         *device) override;

    // IBackendPlugin — conflict handler
    Kalburator::Sync::QSyncCore::ConflictHandler *createConflictHandler() override;

    // IBackendPlugin — main view
    bool     hasMainView()   const override;
    QWidget *createMainView(QWidget *parent) const override;
    QString  mainViewName()  const override;
    QIcon    mainViewIcon()  const override;

    // IBackendPlugin — conflict presentation
    void    enrichConflictSnapshot(
        Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
        bool isSourceSide) const override;
    QString formatConflictRecordHtml(
        const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const override;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    PalmDeviceConnection *m_device = nullptr;   // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOBACKENDPLUGIN_H
