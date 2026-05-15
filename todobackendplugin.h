#ifndef WILDPALMS_TODO_TODOBACKENDPLUGIN_H
#define WILDPALMS_TODO_TODOBACKENDPLUGIN_H

#include <memory>

#include "plugin.h"

namespace Kalburator::Conflict { struct RecordSnapshot; class ConflictHandler; }
namespace Kalburator::Sync { class SyncBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; }

namespace WildPalms::TodoPlugin {

/**
 * @brief Todo plugin (K.8b): inherits Kalburator::Plugin (K.7 surface).
 *
 * No longer a KCoreAddons MODULE plugin. Linked STATIC and loaded
 * in-process by PalmRuntime::registerPalmPlugins() (Task 6).
 *
 * Provides:
 *   - TodoBlobBackend (as SyncBackend) via createPalmBackend() — called
 *     directly by PalmRuntime; not routed through BackendContributions.
 *   - TodoConflictHandler (todo-aware overlays + Palm delegation).
 *   - TaskView as a main-window tab.
 */
class TodoBackendPlugin : public Kalburator::Plugin
{
public:
    TodoBackendPlugin();
    ~TodoBackendPlugin() override;

    // Kalburator::Plugin — all return {} (Palm plugins don't contribute
    // to the libkalburator BackendContribution system)
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("todo"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const { return {QStringLiteral("ToDoDB")}; }

    // Palm backend — called directly by PalmRuntime (Task 6)
    std::unique_ptr<Kalburator::Sync::SyncBackend>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device);

    // Conflict handler
    Kalburator::Conflict::ConflictHandler *createConflictHandler();

    // Main view
    bool     hasMainView()   const;
    QWidget *createMainView(QWidget *parent) const;
    QString  mainViewName()  const;
    QIcon    mainViewIcon()  const;

    // Conflict presentation (called by conflict UI layer)
    void    enrichConflictSnapshot(
        Kalburator::Conflict::RecordSnapshot &snapshot,
        bool isSourceSide) const;
    QString formatConflictRecordHtml(
        const Kalburator::Conflict::RecordSnapshot &snapshot) const;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    std::unique_ptr<WildPalms::PalmSync::PalmBackend>              m_palmBackend;
    WildPalms::Runtime::PalmDeviceAccess *m_device = nullptr; // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOBACKENDPLUGIN_H
