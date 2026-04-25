#include "todobackendplugin.h"

#include "todoblobbackend.h"
#include "todoconflicthandler.h"
#include "todoicstranscoder.h"
#include "taskview.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/palmdeviceconnection.h"
#include "palm/sync/palmbackend.h"
#include "palm/codecs/todocodec.h"

#include "conflictrecord.h"

#include <KCalendarCore/Todo>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include <QIcon>
#include <QLoggingCategory>
#include <QString>
#include <QWidget>

namespace {
Q_LOGGING_CATEGORY(WP_TODO_PLUGIN, "wildpalms.todo.plugin")
}

namespace WildPalms::TodoPlugin {

TodoBackendPlugin::TodoBackendPlugin(QObject *parent)
    : QObject(parent)
    , m_categoryStore(std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
    , m_palmConfig(std::make_unique<WildPalms::PalmConflict::PalmBackendConfig>())
{
}

TodoBackendPlugin::~TodoBackendPlugin() = default;

QString TodoBackendPlugin::pluginId()    const { return QStringLiteral("todo"); }
QString TodoBackendPlugin::displayName() const { return QStringLiteral("Tasks"); }
QIcon   TodoBackendPlugin::icon()        const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-tasks"));
}
QString TodoBackendPlugin::description() const
{
    return QStringLiteral(
        "Synchronizes Palm ToDoDB with iCalendar VTODO files via virtual category sub-collections");
}
QString TodoBackendPlugin::version()     const { return QStringLiteral("2.0"); }

QStringList TodoBackendPlugin::claimedDatabases() const
{
    return { QStringLiteral("ToDoDB") };
}

WildPalms::IBackendPlugin::ProvidedBackends
TodoBackendPlugin::createBackends(Kalburator::Sync::ISyncHost *host,
                                  PalmDeviceConnection         *device)
{
    Q_UNUSED(host)
    ProvidedBackends out;
    if (!device) return out;

    // Cached for createConflictHandler. Re-entry overwrites: the
    // IBackendPlugin contract is once-per-session per device, so a
    // second call implies a new session and is intentional.
    m_device = device;

    auto *palmBackend = device->palmBackend();
    if (palmBackend) {
        // Populate the category store from AppInfo. Failure is non-fatal:
        // the backend still surfaces palm:todo/0 ("Unfiled").
        WildPalms::PalmCalendar::populateFromAppInfo(
            *m_categoryStore,
            QStringLiteral("ToDoDB"),
            palmBackend->readAppBlock(QStringLiteral("ToDoDB")));
        out.blob = new TodoBlobBackend(palmBackend, m_categoryStore.get());
    }

    // No typed SyncBackend: libkalburator has no typed-todo upstream
    // layer. out.calendar stays null.
    return out;
}

Kalburator::Sync::QSyncCore::ConflictHandler *
TodoBackendPlugin::createConflictHandler()
{
    if (!m_device || !m_device->device()) {
        qCWarning(WP_TODO_PLUGIN)
            << "createConflictHandler called before createBackends — "
               "manager must invoke createBackends first to wire the device.";
        return nullptr;
    }
    return new TodoConflictHandler(m_device->device(), m_palmConfig.get());
}

bool TodoBackendPlugin::hasMainView() const { return true; }

QWidget *TodoBackendPlugin::createMainView(QWidget *parent) const
{
    return new TaskView(parent);
}

QString TodoBackendPlugin::mainViewName() const { return QStringLiteral("Tasks"); }

QIcon TodoBackendPlugin::mainViewIcon() const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-tasks"));
}

void TodoBackendPlugin::enrichConflictSnapshot(
    Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot,
    bool /*isSourceSide*/) const
{
    if (snapshot.content.isEmpty()) return;

    KCalendarCore::ICalFormat fmt;
    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!fmt.fromString(cal, QString::fromUtf8(snapshot.content))) return;
    auto todos = cal->todos();
    if (todos.isEmpty()) return;
    auto todo = todos.first();
    if (!todo) return;

    snapshot.metadata[QStringLiteral("title")] = todo->summary();
    snapshot.metadata[QStringLiteral("complete")] =
        todo->isCompleted() ? QStringLiteral("yes") : QStringLiteral("no");
    snapshot.contentType = QStringLiteral("text/calendar");
}

QString TodoBackendPlugin::formatConflictRecordHtml(
    const Kalburator::Sync::QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    const QString title    = snapshot.metadata.value(QStringLiteral("title")).toString();
    const QString complete = snapshot.metadata.value(QStringLiteral("complete")).toString();
    if (!title.isEmpty()) {
        html += QStringLiteral("<h3>%1</h3>").arg(title.toHtmlEscaped());
    }
    if (!complete.isEmpty()) {
        html += QStringLiteral("<p><b>Complete:</b> %1</p>").arg(complete);
    }
    html += QStringLiteral("<pre>%1</pre>")
        .arg(QString::fromUtf8(snapshot.content).toHtmlEscaped());
    return html;
}

} // namespace WildPalms::TodoPlugin

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(TodoBackendPluginFactory,
                           "todo-backend-plugin.json",
                           registerPlugin<WildPalms::TodoPlugin::TodoBackendPlugin>();)

#include "todobackendplugin.moc"
