#include "palmtovtodotransformation.h"

#include "todoicstranscoder.h"
#include "palm/sync/palmrecord.h"

using namespace Kalburator::Shape;

namespace WildPalms::TodoPlugin {

PalmToVTodoStage::PalmToVTodoStage(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray PalmToVTodoStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    const auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(sourceBytes);
    // "ToDoDB" is the only Palm ToDo database name — hardcoded here so the
    // stage is self-contained and matches the AppInfo reader in the plugin.
    return encodePalmToIcs(pr, m_cats, QStringLiteral("ToDoDB"));
}

VTodoToPalmStage::VTodoToPalmStage(
    const WildPalms::PalmCalendar::CategoryMappingStore *cats)
    : m_cats(cats)
{
}

QByteArray VTodoToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    // The category slot is derived from the iCalendar CATEGORIES property via
    // the borrowed CategoryMappingStore (name -> slot). With no store / no
    // categories the slot is 0 (Unfiled).
    const auto prOpt = decodeIcsToPalm(sourceBytes, m_cats, QStringLiteral("ToDoDB"));
    if (!prOpt) return {};
    return prOpt->toWireBytes();
}

LossProfile palmToVTodoLoss()
{
    // lossless: every Palm ToDoDB field maps directly onto a VTODO field
    // (priority widens 1..5 -> 1..9 injectively). Identity X- stamps are
    // preserved downstream by libkalburator's vtodo<->canon stage.
    return {};
}

LossProfile vtodoToPalmLoss()
{
    LossProfile p;
    // Palm ToDoDB has no field for these (canon todo property vocabulary):
    p.affected.insert(PropertyId{QStringLiteral("descriptionHtml")}, LossKind::Dropped);
    // categories is NO LONGER dropped: the Palm category slot carries the
    // canonical `categories` field (name-based) via CategoryMappingStore.
    p.affected.insert(PropertyId{QStringLiteral("start")},           LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("completed")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("recurrence")},      LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("alarms")},          LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("location")},        LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("geo")},             LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("sortOrder")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("relatedTo")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("parentUid")},       LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("checklistItems")},  LossKind::Dropped);
    p.affected.insert(PropertyId{QStringLiteral("linkedResources")}, LossKind::Dropped);
    // Survive in reduced form: Palm holds a complete/incomplete boolean (no
    // granular percent, no NEEDS-ACTION/IN-PROCESS/CANCELLED distinction)...
    p.affected.insert(PropertyId{QStringLiteral("status")},          LossKind::Simplified);
    p.affected.insert(PropertyId{QStringLiteral("percentComplete")}, LossKind::Simplified);
    // ...and a coarse 1..5 priority mapped many-to-one from the 1..9 vocab.
    p.affected.insert(PropertyId{QStringLiteral("priority")},        LossKind::Degraded);
    return p;
}

} // namespace WildPalms::TodoPlugin
