#ifndef WILDPALMS_TODO_TODOICSTRANSCODER_H
#define WILDPALMS_TODO_TODOICSTRANSCODER_H

#include <optional>

#include <QByteArray>

#include "palm/sync/palmrecord.h"

namespace WildPalms::TodoPlugin {

/**
 * @brief Encode a Palm ToDo record into iCalendar VCALENDAR/VTODO bytes.
 *
 * Composes WildPalms::PalmCodecs::decodeTodo (Palm bytes -> Todo POD)
 * with a Todo -> KCalendarCore::Todo mapper, then serialises the
 * single-VTODO calendar via KCalendarCore::ICalFormat. Stamps the
 * Palm slot via X-WP-PALM-CATEGORY-SLOT and the Palm recordId via
 * X-WP-PALM-RECORDID for round-trip.
 *
 * Returns empty QByteArray on decode failure or empty input.
 */
QByteArray encodePalmToIcs(const WildPalms::PalmSync::PalmRecord &record);

/**
 * @brief Decode VCALENDAR bytes containing a single VTODO into a PalmRecord.
 *
 * `slotHint` populates `PalmRecord::category` (overriding any
 * X-WP-PALM-CATEGORY-SLOT in the body — collection-id wins). The
 * X-WP-PALM-RECORDID property, if present, populates
 * `PalmRecord::recordId`; otherwise recordId stays 0 and the device
 * assigns on write.
 *
 * Returns std::nullopt if `icsBytes` doesn't parse as a single VTODO,
 * or if encoding to Palm bytes fails.
 */
std::optional<WildPalms::PalmSync::PalmRecord>
decodeIcsToPalm(const QByteArray &icsBytes, int slotHint);

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOICSTRANSCODER_H
