#ifndef WILDPALMS_TODO_TODOBACKENDPLUGIN_H
#define WILDPALMS_TODO_TODOBACKENDPLUGIN_H

#include <memory>

#include "plugins/pimplugin.h"

namespace Kalburator::Conflict { struct RecordSnapshot; class ConflictHandler; }
namespace Kalburator::Sync { class SyncBackend; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; class PalmRuntime; }
namespace WildPalms::TodoPlugin { class HubTodoReader; }

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
class TodoBackendPlugin : public WildPalms::Plugins::PimPlugin
{
public:
    TodoBackendPlugin();
    ~TodoBackendPlugin() override;

    // Kalburator::Plugin — no backend contributions (Palm backends are created
    // directly by PalmRuntime, not via the BackendContribution system).
    QList<std::shared_ptr<Kalburator::Sync::BackendContribution>>
        backendContributions() const override { return {}; }

    // O7: contribute the (todo, palm) peer shape + palm<->ical-vtodo edges via
    // the shape-graph contribution system (PluginManager registers them into
    // the injected ShapeRegistries). Replaces the old ctor-time registerWith().
    QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
        shapeContributions() const override;

    // Plugin identity
    QString     pluginId()         const { return QStringLiteral("todo"); }
    QString     displayName()      const;
    QIcon       icon()             const;
    QString     description()      const;
    QString     version()          const;
    QStringList claimedDatabases() const { return {QStringLiteral("ToDoDB")}; }

    // F.3: Category slot snapshot — used by PalmRuntime::finishConnect to
    // write the snapshot into Profile after createPalmBackend populates
    // m_categoryStore from the live AppInfo block. Returns empty list if
    // the store hasn't been populated yet.
    QString     primaryDbName()       const { return QStringLiteral("ToDoDB"); }
    QStringList categorySlotNames()   const;

    // Task 3: borrowed accessor for hub<->remote routing translation.
    WildPalms::PalmCalendar::CategoryMappingStore *categoryStore() const;

    // Sub-project D: PimPlugin lifecycle hooks.
    void setHub(Kalburator::Sync::SyncBackend *hub) override;
    void setRuntime(WildPalms::Runtime::PalmRuntime *runtime) override;

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

    // Sub-project D: per-domain reader over the canonical hub; constructed
    // in setHub, fed to TaskView in createMainView.
    std::unique_ptr<WildPalms::TodoPlugin::HubTodoReader> m_hubReader;
    WildPalms::Runtime::PalmRuntime *m_runtime = nullptr;       // borrowed
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOBACKENDPLUGIN_H
