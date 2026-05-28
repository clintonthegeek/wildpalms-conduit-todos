#include "todoicstranscoder.h"

#include "palm/codecs/todocodec.h"
#include "palm/calendar/categorymappingstore.h"

#include <KCalendarCore/Todo>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

#include <QDateTime>
#include <QString>
#include <QTimeZone>

namespace WildPalms::TodoPlugin {

namespace {

constexpr const char *kCategorySlotProp = "X-WP-PALM-CATEGORY-SLOT";
constexpr const char *kRecordIdProp     = "X-WP-PALM-RECORDID";

// Palm priority (1..5, 1=highest) -> KCal priority (1..9, 1=highest).
int palmPriorityToKCal(int p)
{
    switch (p) {
        case 1: return 1;
        case 2: return 3;
        case 3: return 5;
        case 4: return 7;
        case 5: return 9;
        default: return 5;
    }
}

// KCal priority (1..9) -> Palm (1..5). Lossy on even input; lossless
// for any value the Palm could have authored.
int kcalPriorityToPalm(int p)
{
    if (p <= 1) return 1;
    if (p <= 3) return 2;
    if (p <= 5) return 3;
    if (p <= 7) return 4;
    return 5;
}

// Map Palm Todo POD -> KCalendarCore::Todo. The `isPrivate` argument is
// the record-header secret bit (PalmRecord::AttrSecret); the Todo POD's
// own isPrivate field is not packed into the data blob, so the
// authoritative source is the record attributes.
KCalendarCore::Todo::Ptr toKCalTodo(const WildPalms::PalmCodecs::Todo &t,
                                    int slot,
                                    std::uint32_t recordId,
                                    bool isPrivate)
{
    KCalendarCore::Todo::Ptr todo(new KCalendarCore::Todo);
    todo->setSummary(t.description);
    if (!t.note.isEmpty()) {
        todo->setDescription(t.note);
    }
    if (!t.hasIndefiniteDue && t.due.isValid()) {
        todo->setDtStart(t.due);
        todo->setDtDue(t.due);
        todo->setAllDay(true);
    }
    todo->setPriority(palmPriorityToKCal(t.priority));
    if (t.isComplete) {
        todo->setStatus(KCalendarCore::Incidence::StatusCompleted);
        todo->setCompleted(QDateTime::currentDateTimeUtc());
    }
    if (isPrivate) {
        todo->setSecrecy(KCalendarCore::Incidence::SecrecyPrivate);
    } else {
        todo->setSecrecy(KCalendarCore::Incidence::SecrecyPublic);
    }
    todo->setNonKDECustomProperty(kCategorySlotProp,
                                  QString::number(slot));
    todo->setNonKDECustomProperty(kRecordIdProp,
                                  QString::number(recordId));
    return todo;
}

// Map KCalendarCore::Todo -> Palm Todo POD.
WildPalms::PalmCodecs::Todo fromKCalTodo(const KCalendarCore::Todo::Ptr &todo)
{
    WildPalms::PalmCodecs::Todo t;
    t.description = todo->summary();
    t.note        = todo->description();
    if (todo->hasDueDate()) {
        t.hasIndefiniteDue = false;
        t.due = todo->dtDue();
    } else {
        t.hasIndefiniteDue = true;
    }
    t.priority   = kcalPriorityToPalm(todo->priority());
    t.isComplete = todo->isCompleted();
    t.isPrivate  = todo->secrecy() == KCalendarCore::Incidence::SecrecyPrivate;
    return t;
}

} // namespace

QByteArray encodePalmToIcs(const WildPalms::PalmSync::PalmRecord &record,
                           const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                           const QString &dbName)
{
    if (record.data.isEmpty()) return {};
    auto decoded = WildPalms::PalmCodecs::decodeTodo(QByteArrayView(record.data));
    if (!decoded.has_value()) return {};

    auto todo = toKCalTodo(*decoded,
                           static_cast<int>(record.category),
                           record.recordId,
                           record.isSecret());

    // Carry the Palm category slot as the iCalendar CATEGORIES property
    // (name-based). libkalburator's vtodo<->canon stage lifts it into
    // canon `categories`.
    if (cats && record.category != 0) {
        const QString nm = cats->slotName(dbName, record.category);
        if (!nm.isEmpty()) todo->setCategories(QStringList{nm});
    }

    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!cal->addTodo(todo)) return {};

    KCalendarCore::ICalFormat fmt;
    return fmt.toString(cal).toUtf8();
}

std::optional<WildPalms::PalmSync::PalmRecord>
decodeIcsToPalm(const QByteArray &icsBytes,
                const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                const QString &dbName)
{
    if (icsBytes.isEmpty()) return std::nullopt;

    KCalendarCore::ICalFormat fmt;
    auto cal = KCalendarCore::MemoryCalendar::Ptr(
        new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!fmt.fromString(cal, QString::fromUtf8(icsBytes))) {
        return std::nullopt;
    }

    auto todos = cal->todos();
    if (todos.isEmpty()) return std::nullopt;
    auto todo = todos.first();
    if (!todo) return std::nullopt;

    auto pod = fromKCalTodo(todo);
    QByteArray bytes = WildPalms::PalmCodecs::encodeTodo(pod);
    if (bytes.isEmpty()) return std::nullopt;

    // Map the first CATEGORIES name back to a Palm slot. No store or no
    // categories => slot 0 (Unfiled).
    const int slot = (cats && !todo->categories().isEmpty())
        ? cats->slotForName(dbName, todo->categories().constFirst())
        : 0;

    WildPalms::PalmSync::PalmRecord pr;
    pr.data     = bytes;
    pr.category = static_cast<std::uint8_t>(slot);
    if (todo->secrecy() == KCalendarCore::Incidence::SecrecyPrivate) {
        pr.attributes |= WildPalms::PalmSync::PalmRecord::AttrSecret;
    }
    bool ok = false;
    const QString rid = todo->nonKDECustomProperty(kRecordIdProp);
    if (!rid.isEmpty()) {
        const std::uint32_t parsed = rid.toUInt(&ok);
        if (ok) pr.recordId = parsed;
    }
    return pr;
}

} // namespace WildPalms::TodoPlugin
