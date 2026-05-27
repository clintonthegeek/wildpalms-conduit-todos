#ifndef WILDPALMS_TODO_TODODOMAINEXTENSION_H
#define WILDPALMS_TODO_TODODOMAINEXTENSION_H

#include <shapecontribution.h>

namespace WildPalms::TodoPlugin {

// O7: contributes the (todo, palm) peer shape and palm<->ical-vtodo edges to
// the shape graph. The ical-vtodo<->canon hop is libkalburator's (TodoStockShapes).
// PluginManager registers this into the injected ShapeRegistries.
class TodoPalmShapes : public Kalburator::Shape::ShapeContribution {
public:
    Kalburator::Shape::DomainId targetDomain() const override;
    QList<std::pair<Kalburator::Shape::Shape, Kalburator::Shape::PropertyCatalogue>>
        peerShapes() const override;
    QList<Kalburator::Shape::TransformationEdge> edges() const override;
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODODOMAINEXTENSION_H
