#include "palmtovtodotransformation.h"

#include "todoicstranscoder.h"
#include "palm/sync/palmrecord.h"

using namespace Kalburator::Shape;

namespace WildPalms::TodoPlugin {

QByteArray PalmToVTodoStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    const auto pr = WildPalms::PalmSync::PalmRecord::fromWireBytes(sourceBytes);
    return encodePalmToIcs(pr);
}

QByteArray VTodoToPalmStage::transform(const QByteArray &sourceBytes) const
{
    if (sourceBytes.isEmpty()) return {};
    // slotHint = -1: the category field of this stage's output is NOT
    // authoritative. The backend (createRecord/updateRecord) sets the real
    // slot from collection context on write; X-WP-PALM-CATEGORY-SLOT rides on
    // the VTODO but the codec does not read it back. Treat the wire bytes'
    // category as undefined after this stage.
    const auto prOpt = decodeIcsToPalm(sourceBytes, /*slotHint*/ -1);
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
    p.affected.insert(PropertyId{QStringLiteral("categories")},      LossKind::Dropped);
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
