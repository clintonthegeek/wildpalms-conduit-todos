#ifndef WILDPALMS_TODO_TODOICSTRANSCODER_H
#define WILDPALMS_TODO_TODOICSTRANSCODER_H

#include <optional>

#include <QByteArray>

#include <QString>

#include "palm/sync/palmrecord.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

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
 * If `cats` is non-null and `record.category != 0`, the slot's display
 * name (via CategoryMappingStore::slotName(dbName, slot)) is written to
 * the Todo's CATEGORIES property — libkalburator's vtodo<->canon stage
 * then carries it into canon `categories`. A null `cats` emits no
 * categories (degrades gracefully).
 *
 * Returns empty QByteArray on decode failure or empty input.
 */
QByteArray encodePalmToIcs(const WildPalms::PalmSync::PalmRecord &record,
                           const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                           const QString &dbName);

/**
 * @brief Decode VCALENDAR bytes containing a single VTODO into a PalmRecord.
 *
 * The category slot is derived from the VTODO's CATEGORIES property: if
 * `cats` is non-null and the todo carries at least one category, the
 * first category name is mapped back to a slot via
 * CategoryMappingStore::slotForName(dbName, name). A null `cats` (or no
 * categories) yields slot 0 (Unfiled). The X-WP-PALM-RECORDID property,
 * if present, populates `PalmRecord::recordId`; otherwise recordId stays
 * 0 and the device assigns on write.
 *
 * Returns std::nullopt if `icsBytes` doesn't parse as a single VTODO,
 * or if encoding to Palm bytes fails.
 */
std::optional<WildPalms::PalmSync::PalmRecord>
decodeIcsToPalm(const QByteArray &icsBytes,
                const WildPalms::PalmCalendar::CategoryMappingStore *cats,
                const QString &dbName);

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODOICSTRANSCODER_H
