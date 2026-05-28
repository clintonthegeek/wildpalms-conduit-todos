#ifndef WILDPALMS_TODO_TODODOMAINEXTENSION_H
#define WILDPALMS_TODO_TODODOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::TodoPlugin {

// O7: contributes the (todo, palm) peer shape and palm<->ical-vtodo edges to
// the shape graph. The ical-vtodo<->canon hop is libkalburator's (TodoStockShapes).
// PluginManager registers this into the injected ShapeRegistries.
//
// Borrows a (nullable) CategoryMappingStore, threaded into the palm<->vtodo
// stages so the Palm category slot rides as the canonical `categories` field.
// The store is owned by the plugin and outlives this contribution.
class TodoPalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    explicit TodoPalmShapes(
        const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);

    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODODOMAINEXTENSION_H
