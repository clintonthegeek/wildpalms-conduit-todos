#include "todobackendplugin.h"

#include "hubtodoreader.h"
#include "todoblobbackend.h"
#include "todoconflicthandler.h"
#include "tododomainextension.h"
#include "todoicstranscoder.h"
#include "taskview.h"

#include "palm/calendar/categoryappinforeader.h"
#include "palm/calendar/categorymappingstore.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/sync/palmbackend.h"
#include "runtime/palmdeviceaccess.h"
#include "runtime/palmruntime.h"
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

TodoBackendPlugin::TodoBackendPlugin()
    : m_categoryStore(std::make_unique<WildPalms::PalmCalendar::CategoryMappingStore>())
    , m_palmConfig(std::make_unique<WildPalms::PalmConflict::PalmBackendConfig>())
{
    // O7: shape registration moved out of the ctor into shapeContributions();
    // PluginManager registers the contribution into the injected ShapeRegistries.
}

QList<std::shared_ptr<Kalburator::Shape::ShapeContribution>>
TodoBackendPlugin::shapeContributions() const
{
    // Timing contract: this runs at PluginManager load time (PalmRuntime ctor),
    // BEFORE createPalmBackend() populates m_categoryStore from the AppInfo block
    // at device-connect. The contribution/stages keep a borrowed POINTER (not a
    // snapshot), so by the time any transform runs (post-connect) the store is
    // populated. Do not cache the store by value, and do not assume it is
    // populated here.
    return { std::make_shared<TodoPalmShapes>(m_categoryStore.get()) };
}

TodoBackendPlugin::~TodoBackendPlugin() = default;

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

std::unique_ptr<Kalburator::Sync::SyncBackend>
TodoBackendPlugin::createPalmBackend(WildPalms::Runtime::PalmDeviceAccess *device)
{
    if (!device) return nullptr;

    m_device = device;
    m_palmBackend = std::make_unique<WildPalms::PalmSync::PalmBackend>(device);

    WildPalms::PalmCalendar::populateFromAppInfo(
        *m_categoryStore,
        QStringLiteral("ToDoDB"),
        m_palmBackend->readAppBlock(QStringLiteral("ToDoDB")));

    return std::make_unique<TodoBlobBackend>(m_palmBackend.get(), m_categoryStore.get());
}

Kalburator::Conflict::ConflictHandler *
TodoBackendPlugin::createConflictHandler()
{
    if (!m_device) {
        qCWarning(WP_TODO_PLUGIN)
            << "createConflictHandler called before createPalmBackend — "
               "runtime must invoke createPalmBackend first to wire the device.";
        return nullptr;
    }
    // PalmDeviceAccess IS-A IPalmDatabaseAccess; no cast needed.
    return new TodoConflictHandler(m_device, m_palmConfig.get());
}

bool TodoBackendPlugin::hasMainView() const { return true; }

QWidget *TodoBackendPlugin::createMainView(QWidget *parent) const
{
    auto *v = new TaskView(parent);
    v->setHubReader(m_hubReader.get());
    if (m_runtime) {
        QObject::connect(m_runtime,
                         &WildPalms::Runtime::PalmRuntime::syncCompleted,
                         v, &TaskView::refresh);
    }
    return v;
}

void TodoBackendPlugin::setHub(Kalburator::Sync::SyncBackend *hub)
{
    Q_ASSERT(hub);
    m_hubReader = std::make_unique<WildPalms::TodoPlugin::HubTodoReader>(
        hub, QStringLiteral("palm:todo"));
}

void TodoBackendPlugin::setRuntime(WildPalms::Runtime::PalmRuntime *runtime)
{
    m_runtime = runtime;
}

QString TodoBackendPlugin::mainViewName() const { return QStringLiteral("Tasks"); }

QIcon TodoBackendPlugin::mainViewIcon() const
{
    return QIcon::fromTheme(QStringLiteral("view-pim-tasks"));
}

void TodoBackendPlugin::enrichConflictSnapshot(
    Kalburator::Conflict::RecordSnapshot &snapshot,
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
    const Kalburator::Conflict::RecordSnapshot &snapshot) const
{
    QString html;
    const QString title    = snapshot.metadata.value(QStringLiteral("title")).toString();
    const QString complete = snapshot.metadata.value(QStringLiteral("complete")).toString();
    if (!title.isEmpty()) {
        html += QStringLiteral("<h3>%1</h3>").arg(title.toHtmlEscaped());
    }
    if (!complete.isEmpty()) {
        html += QStringLiteral("<p><b>Complete:</b> %1</p>").arg(complete.toHtmlEscaped());
    }
    html += QStringLiteral("<pre>%1</pre>")
        .arg(QString::fromUtf8(snapshot.content).toHtmlEscaped());
    return html;
}

QStringList TodoBackendPlugin::categorySlotNames() const
{
    if (!m_categoryStore) return {};
    return m_categoryStore->sixteenSlotNames(primaryDbName());
}

WildPalms::PalmCalendar::CategoryMappingStore *
TodoBackendPlugin::categoryStore() const
{
    return m_categoryStore.get();
}

} // namespace WildPalms::TodoPlugin
