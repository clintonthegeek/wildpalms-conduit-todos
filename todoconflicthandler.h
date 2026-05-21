#ifndef WILDPALMS_TODO_TODOCONFLICTHANDLER_H
#define WILDPALMS_TODO_TODOCONFLICTHANDLER_H

#include "conflictpolicy.h"   // brings in Kalburator::Conflict::ConflictHandler

#include <memory>

#include <QString>

namespace WildPalms::PalmSync { class IPalmDatabaseAccess; }
namespace WildPalms::PalmConflict {
class PalmConflictHandler;
struct PalmBackendConfig;
}

namespace WildPalms::TodoPlugin {

/**
 * @brief ConflictHandler with one ToDo overlay, delegating to PalmConflictHandler.
 *
 * Resolution order:
 *   1. Decode both sides as PalmRecord -> Todo POD via the transcoder.
 *      If either side fails to decode -> delegate to PalmConflictHandler.
 *   2. Detect "completion-asymmetric merge": exactly one side has
 *      isComplete=true and the other has isComplete=false, AND the
 *      non-completion fields (description, note, due, priority, the
 *      record's secret bit) differ between sides. Return
 *      ConflictDecision::Merge with mergedContent = re-serialised
 *      VTODO holding {isComplete=true, peer's text/priority/due/
 *      secret-bit}.
 *   3. Otherwise -> delegate to PalmConflictHandler::handleConflict.
 *
 * Owns its inner PalmConflictHandler (constructed from the (device,
 * config) pair the plugin passes through).
 *
 * Lifetime: does NOT own device or config. Both must outlive the
 * handler.
 */
class TodoConflictHandler : public Kalburator::Conflict::ConflictHandler
{
public:
    TodoConflictHandler(WildPalms::PalmSync::IPalmDatabaseAccess *device,
                        const WildPalms::PalmConflict::PalmBackendConfig *config);
    ~TodoConflictHandler() override;

    Kalburator::Conflict::ConflictDecision handleConflict(
        Kalburator::Conflict::ConflictRecord &conflict,
        const Kalburator::Conflict::ConflictPolicy &policy) override;

    bool canPrompt() const override { return false; }

    /// Test hook: which path was last taken.
    /// Values: "" (uninitialised), "completion-asymmetric", "delegated".
    const QString &lastOverlay() const { return m_lastOverlay; }

private:
    std::unique_ptr<WildPalms::PalmConflict::PalmConflictHandler> m_palm;
    QString m_lastOverlay;
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOCONFLICTHANDLER_H
