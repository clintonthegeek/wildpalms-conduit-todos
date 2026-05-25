#include "tododomainextension.h"

#include "palmtovtodotransformation.h"
#include "propertycatalogue.h"
#include "transformationregistry.h"

using namespace Kalburator::Shape;

namespace WildPalms::TodoPlugin {

namespace {

PropertyCatalogue makePalmCatalogue()
{
    PropertyCatalogue cat;
    // Palm ToDoDB native fields; used by loss-profile UI to describe a
    // palm-shape record.
    cat.addProperty({ PropertyId{"summary"},        PropertyKind::String,  QStringLiteral("Summary") });
    cat.addProperty({ PropertyId{"description"},    PropertyKind::String,  QStringLiteral("Description") });
    cat.addProperty({ PropertyId{"due"},            PropertyKind::Json,    QStringLiteral("Due") });
    cat.addProperty({ PropertyId{"priority"},       PropertyKind::Integer, QStringLiteral("Priority") });
    cat.addProperty({ PropertyId{"status"},         PropertyKind::String,  QStringLiteral("Status") });
    cat.addProperty({ PropertyId{"classification"}, PropertyKind::String,  QStringLiteral("Classification") });
    cat.addProperty({ PropertyId{"category"},       PropertyKind::Integer, QStringLiteral("Category Slot") });
    return cat;
}

} // namespace

void TodoDomainExtension::registerWith(TransformationRegistry &registry)
{
    const Shape palm { DomainId{"todo"}, EncodingId{"palm"} };
    const Shape vtodo{ DomainId{"todo"}, EncodingId{"ical-vtodo"} };

    // Defensive: libkalburator's todo domain plugin registers the ical-vtodo
    // peer shape at PluginManager load time (via registerStockPlugins). If that
    // static-init registrar didn't run in this address space (e.g. a unit test
    // that skips full plugin init), the vtodo shape is absent and registerEdge
    // below would assert "to-shape not registered". Register a minimal
    // placeholder; registerShape is idempotent, so libkalburator's real
    // catalogue replaces it under the same key when it runs.
    if (registry.catalogueFor(vtodo) == nullptr) {
        registry.registerShape(vtodo, {});
    }
    registry.registerShape(palm, makePalmCatalogue());

    // palm -> ical-vtodo (lossless; identity X- stamps preserved by vtodo<->canon)
    registry.registerEdge(TransformationEdge{
        palm, vtodo, palmToVTodoLoss(), std::make_shared<PalmToVTodoStage>() });

    // ical-vtodo -> palm (lossy; Palm ToDoDB holds a subset of VTODO)
    registry.registerEdge(TransformationEdge{
        vtodo, palm, vtodoToPalmLoss(), std::make_shared<VTodoToPalmStage>() });
}

} // namespace WildPalms::TodoPlugin
