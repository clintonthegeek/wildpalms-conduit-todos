#include "todoconflicthandler.h"

#include "todoicstranscoder.h"

#include "palm/codecs/todocodec.h"
#include "palm/conflict/palmconflicthandler.h"
#include "palm/conflict/palmbackendconfig.h"
#include "palm/sync/palmrecord.h"

#include "conflictrecord.h"

namespace WildPalms::TodoPlugin {

namespace {

using WildPalms::PalmCodecs::Todo;
using WildPalms::PalmCodecs::encodeTodo;
using WildPalms::PalmCodecs::decodeTodo;
using WildPalms::PalmSync::PalmRecord;

struct DecodedSide {
    PalmRecord record;
    Todo       todo;
    bool       valid = false;
};

DecodedSide decodeSide(const QByteArray &icsBytes, int slotHint)
{
    DecodedSide out;
    auto pr = WildPalms::TodoPlugin::decodeIcsToPalm(icsBytes, slotHint);
    if (!pr.has_value()) return out;
    auto t = decodeTodo(QByteArrayView(pr->data));
    if (!t.has_value()) return out;
    out.record = *pr;
    out.todo   = *t;
    out.valid  = true;
    return out;
}

// Compares non-completion fields only. Privacy comes from the
// PalmRecord::AttrSecret bit, NOT the POD's isPrivate field — the
// POD's isPrivate is not round-tripped through pisock's pack_ToDo.
bool nonCompletionFieldsEqual(const DecodedSide &a, const DecodedSide &b)
{
    return a.todo.description == b.todo.description
        && a.todo.note == b.todo.note
        && a.todo.hasIndefiniteDue == b.todo.hasIndefiniteDue
        && a.todo.due == b.todo.due
        && a.todo.priority == b.todo.priority
        && a.record.isSecret() == b.record.isSecret();
}

// Build merged VTODO bytes: take peer's non-completion fields (and the
// peer record's secret bit, recordId, category) and force isComplete=true.
QByteArray buildMergedIcs(const PalmRecord &peer, const Todo &peerTodo)
{
    Todo merged = peerTodo;
    merged.isComplete = true;
    PalmRecord pr = peer;
    pr.data = encodeTodo(merged);
    return WildPalms::TodoPlugin::encodePalmToIcs(pr);
}

} // namespace

TodoConflictHandler::TodoConflictHandler(
    WildPalms::PalmSync::IPalmDatabaseAccess *device,
    const WildPalms::PalmConflict::PalmBackendConfig *config)
    : m_palm(std::make_unique<WildPalms::PalmConflict::PalmConflictHandler>(device, config))
{
}

TodoConflictHandler::~TodoConflictHandler() = default;

Kalburator::Sync::QSyncCore::ConflictDecision TodoConflictHandler::handleConflict(
    Kalburator::Sync::QSyncCore::ConflictRecord &conflict,
    const Kalburator::Sync::QSyncCore::ConflictPolicy &policy)
{
    // Same-slot decode (slot doesn't matter for the overlay; defaults
    // to 0 — we re-stamp the merged record with peer's slot below).
    DecodedSide source = decodeSide(conflict.source.content, 0);
    DecodedSide target = decodeSide(conflict.target.content, 0);

    if (!source.valid || !target.valid) {
        m_lastOverlay = QStringLiteral("delegated");
        return m_palm->handleConflict(conflict, policy);
    }

    const bool sourceFlipped = source.todo.isComplete  && !target.todo.isComplete;
    const bool targetFlipped = target.todo.isComplete  && !source.todo.isComplete;

    // Overlay rule: fire iff exactly one side is complete, the other
    // isn't, AND non-completion fields differ between sides. Take the
    // not-yet-complete side as the authoritative source of non-completion
    // content (the flipper only intended to mark complete).
    //
    // Without per-side baselines accessible at the handler layer, we
    // can't strictly distinguish "flipper edited only completion" from
    // "flipper also edited text back to the baseline". The rule above
    // matches the failure mode the overlay exists to fix and degrades
    // gracefully: if both sides edited text, falling through to
    // PalmConflictHandler's lastModified tie-break is no worse than
    // the legacy conduit's behaviour today.
    if ((sourceFlipped || targetFlipped) &&
        !nonCompletionFieldsEqual(source, target)) {
        const PalmRecord &peerRecord = sourceFlipped ? target.record : source.record;
        const Todo       &peerTodo   = sourceFlipped ? target.todo   : source.todo;
        conflict.mergedContent = buildMergedIcs(peerRecord, peerTodo);
        m_lastOverlay = QStringLiteral("completion-asymmetric");
        return Kalburator::Sync::QSyncCore::ConflictDecision::Merge;
    }

    m_lastOverlay = QStringLiteral("delegated");
    return m_palm->handleConflict(conflict, policy);
}

} // namespace WildPalms::TodoPlugin
