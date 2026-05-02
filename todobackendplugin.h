#ifndef WILDPALMS_TODO_TODOBACKENDPLUGIN_H
#define WILDPALMS_TODO_TODOBACKENDPLUGIN_H

#include <memory>

#include <QObject>

#include "core/ibackendplugin_v2.h"

namespace Kalburator::Sync::QSyncCore { struct RecordSnapshot; }
namespace WildPalms::PalmCalendar { class CategoryMappingStore; }
namespace WildPalms::PalmConflict { struct PalmBackendConfig; }
namespace WildPalms::PalmSync { class PalmBackend; }
namespace WildPalms::Runtime { class PalmDeviceAccess; }

namespace WildPalms::TodoPlugin {

class TodoBackendPlugin : public QObject, public WildPalms::IBackendPluginV2
{
    Q_OBJECT
    Q_INTERFACES(WildPalms::IBackendPluginV2)
public:
    explicit TodoBackendPlugin(QObject *parent = nullptr);
    ~TodoBackendPlugin() override;

    // IPlugin
    QString pluginId()    const override;
    QString displayName() const override;
    QIcon   icon()        const override;
    QString description() const override;
    QString version()     const override;

    // IBackendPluginV2
    QStringList claimedDatabases() const override;
    std::unique_ptr<Kalburator::Sync::IBlobBackend>
        createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device) override;

    // IBackendPluginV2 — conflict handler
    Kalburator::Sync::QSyncCore::ConflictHandler *createConflictHandler() override;

    // IBackendPluginV2 — main view
    bool     hasMainView()   const override;
    QWidget *createMainView(QWidget *parent) const override;
    QString  mainViewName()  const override;
    QIcon    mainViewIcon()  const override;

    // Conflict presentation (called by conflict UI layer; not virtual in v2)
    void    enrichConflictSnapshot(
        Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
        bool isSourceSide) const;
    QString formatConflictRecordHtml(
        const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const;

private:
    std::unique_ptr<WildPalms::PalmCalendar::CategoryMappingStore> m_categoryStore;
    std::unique_ptr<WildPalms::PalmConflict::PalmBackendConfig>    m_palmConfig;
    std::unique_ptr<WildPalms::PalmSync::PalmBackend>              m_palmBackend;
    WildPalms::Runtime::PalmDeviceAccess *m_device = nullptr; // borrowed; cached for createConflictHandler
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOBACKENDPLUGIN_H
