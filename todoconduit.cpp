#include "todoconduit.h"
#include "todomapper.h"
#include "taskview.h"
#include "palm/pilotrecord.h"
#include "palm/categoryinfo.h"
#include "sync/localfilebackend.h"
#include "sync/qsynccore/conflictrecord.h"

#include <QDebug>

namespace Sync {

TodoConduit::TodoConduit(QObject *parent)
    : SyncConduitBase(parent)
{
}

BackendRecord* TodoConduit::palmToBackend(PilotRecord *palmRecord,
                                           SyncContext *context)
{
    if (!palmRecord) return nullptr;

    // Unpack Palm todo
    TodoMapper::Todo todo = TodoMapper::unpackTodo(palmRecord);

    // Convert to iCalendar VTODO
    QString catName = categoryName(todo.category);
    qDebug() << "[TodoConduit::palmToBackend] Palm ID:" << palmRecord->id()
             << "category index:" << todo.category
             << "-> name:" << catName
             << "m_categories valid:" << (m_categories != nullptr);
    QString ical = TodoMapper::todoToICal(todo, catName);

    // Create backend record
    BackendRecord *record = new BackendRecord();
    record->data = ical.toUtf8();
    record->type = "todo";
    record->contentHash = LocalFileBackend::calculateHash(record->data);
    record->lastModified = QDateTime::currentDateTime();

    // Set display name from task description
    QString displayName = todo.description;
    if (displayName.isEmpty()) {
        displayName = "Task";
    }
    if (displayName.length() > 50) {
        displayName = displayName.left(50);
    }
    record->displayName = displayName;

    return record;
}

PilotRecord* TodoConduit::backendToPalm(BackendRecord *backendRecord,
                                         SyncContext *context)
{
    if (!backendRecord) return nullptr;

    // Parse iCalendar content
    QString content = QString::fromUtf8(backendRecord->data);
    qDebug() << "[TodoConduit] Parsing iCal content, length:" << content.length();

    TodoMapper::Todo todo = TodoMapper::iCalToTodo(content);
    qDebug() << "[TodoConduit] Parsed todo - description:" << todo.description
             << "priority:" << todo.priority
             << "complete:" << todo.isComplete
             << "hasDue:" << !todo.hasIndefiniteDue;

    // Look up or create category from name
    if (!todo.categoryName.isEmpty() && m_categories) {
        todo.category = m_categories->getOrCreateCategory(todo.categoryName);
        qDebug() << "[TodoConduit] Category" << todo.categoryName << "-> index" << todo.category;
    }

    // Pack to Palm record
    PilotRecord *record = TodoMapper::packTodo(todo);
    if (!record) {
        qWarning() << "[TodoConduit] packTodo() returned null!";
    }

    return record;
}

bool TodoConduit::recordsEqual(PilotRecord *palm, BackendRecord *backend,
                                const SyncContext *context) const
{
    Q_UNUSED(context);
    if (!palm || !backend) return false;

    // Unpack Palm todo
    TodoMapper::Todo palmTodo = TodoMapper::unpackTodo(palm);

    // Parse backend content
    QString backendContent = QString::fromUtf8(backend->data);
    TodoMapper::Todo backendTodo = TodoMapper::iCalToTodo(backendContent);

    // Compare key fields
    if (palmTodo.description != backendTodo.description) return false;
    if (palmTodo.isComplete != backendTodo.isComplete) return false;
    if (palmTodo.priority != backendTodo.priority) {
        return false;
    }

    // Compare due date (if both have one)
    if (!palmTodo.hasIndefiniteDue && !backendTodo.hasIndefiniteDue) {
        if (palmTodo.due.date() != backendTodo.due.date()) return false;
    } else if (palmTodo.hasIndefiniteDue != backendTodo.hasIndefiniteDue) {
        return false;
    }

    // Compare categories
    QString palmCategoryName = categoryName(palmTodo.category);

    // Normalize: "Unfiled" (index 0) and empty string are equivalent
    QString normalizedPalmCat = palmCategoryName;
    QString normalizedBackendCat = backendTodo.categoryName;

    if (normalizedPalmCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedPalmCat.clear();
    }
    if (normalizedBackendCat.compare("Unfiled", Qt::CaseInsensitive) == 0) {
        normalizedBackendCat.clear();
    }

    if (normalizedPalmCat.compare(normalizedBackendCat, Qt::CaseInsensitive) != 0) {
        return false;
    }

    return true;
}

QString TodoConduit::palmRecordDescription(PilotRecord *record,
                                            const SyncContext *context) const
{
    Q_UNUSED(context);
    if (!record) return QString();

    TodoMapper::Todo todo = TodoMapper::unpackTodo(record);

    QString desc = todo.description;
    if (desc.isEmpty()) {
        desc = "<Untitled Task>";
    }

    // Add completion status
    if (todo.isComplete) {
        desc = "[x] " + desc;
    } else {
        desc = "[ ] " + desc;
    }

    return desc;
}

void TodoConduit::enrichConflictSnapshot(QSyncCore::RecordSnapshot &snapshot,
                                          bool isSourceSide) const
{
    if (snapshot.content.isEmpty()) return;

    TodoMapper::Todo todo;

    if (isSourceSide) {
        // Source: Palm binary — unpack via mapper, convert to iCal VTODO text
        PilotRecord tempRecord(0, 0, 0, snapshot.content);
        todo = TodoMapper::unpackTodo(&tempRecord);
        QString catName = categoryName(todo.category);
        snapshot.content = TodoMapper::todoToICal(todo, catName).toUtf8();
    } else {
        // Target: already iCal VTODO text — parse for metadata
        todo = TodoMapper::iCalToTodo(QString::fromUtf8(snapshot.content));
    }

    // Populate metadata
    if (!todo.description.isEmpty())
        snapshot.metadata[QStringLiteral("description")] = todo.description;
    snapshot.metadata[QStringLiteral("priority")] = todo.priority;
    if (!todo.hasIndefiniteDue && todo.due.isValid())
        snapshot.metadata[QStringLiteral("due_date")] = todo.due.toString(QStringLiteral("yyyy-MM-dd"));
    snapshot.metadata[QStringLiteral("completed")] = todo.isComplete;
    if (!todo.note.isEmpty())
        snapshot.metadata[QStringLiteral("notes")] = todo.note;

    snapshot.contentType = QStringLiteral("text/calendar");
}

QString TodoConduit::formatConflictRecordHtml(const QSyncCore::RecordSnapshot &snapshot) const
{
    QString html;
    const QVariantMap &m = snapshot.metadata;

    QString desc = m.value(QStringLiteral("description")).toString();
    if (!desc.isEmpty())
        html += QStringLiteral("<h3>%1</h3>").arg(desc.toHtmlEscaped());

    html += QStringLiteral("<table cellpadding='4'>");

    // Priority badge
    int priority = m.value(QStringLiteral("priority"), 0).toInt();
    if (priority > 0) {
        QString color = (priority <= 2) ? QStringLiteral("#d32f2f") :
                         (priority <= 3) ? QStringLiteral("#f57c00") :
                                           QStringLiteral("#388e3c");
        html += QStringLiteral("<tr><td><b>Priority:</b></td><td>"
                "<span style='background-color:%1; color:white; padding:2px 6px; border-radius:3px;'>%2</span>"
                "</td></tr>").arg(color).arg(priority);
    }

    auto addRow = [&html](const QString &label, const QString &value) {
        if (!value.isEmpty())
            html += QStringLiteral("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                .arg(label.toHtmlEscaped(), value.toHtmlEscaped());
    };

    addRow(QStringLiteral("Due Date"), m.value(QStringLiteral("due_date")).toString());

    bool completed = m.value(QStringLiteral("completed"), false).toBool();
    html += QStringLiteral("<tr><td><b>Status:</b></td><td>%1</td></tr>")
        .arg(completed ? QStringLiteral("<span style='color:green;'>Completed</span>")
                       : QStringLiteral("<span style='color:orange;'>In Progress</span>"));

    addRow(QStringLiteral("Notes"), m.value(QStringLiteral("notes")).toString());

    html += QStringLiteral("</table>");

    return html;
}

QWidget *TodoConduit::createView(QWidget *parent)
{
    return new TaskView(parent);
}

} // namespace Sync

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(TodoConduitFactory, "todos-conduit.json",
                           registerPlugin<Sync::TodoConduit>();)

#include "todoconduit.moc"
