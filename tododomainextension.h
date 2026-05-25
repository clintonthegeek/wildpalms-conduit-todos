#ifndef WILDPALMS_TODO_TODODOMAINEXTENSION_H
#define WILDPALMS_TODO_TODODOMAINEXTENSION_H

namespace Kalburator::Shape { class TransformationRegistry; }

namespace WildPalms::TodoPlugin {

// Registers the (todo, palm) peer shape and palm<->ical-vtodo edges with the
// shape graph. The ical-vtodo<->canon hop is libkalburator's (TodoStockShapes).
class TodoDomainExtension {
public:
    static void registerWith(Kalburator::Shape::TransformationRegistry &registry);
};

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_TODODOMAINEXTENSION_H
