#include "tododomainextension.h"

#include "palmtovtodotransformation.h"
#include <propertycatalogue.h>

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

DomainId TodoPalmShapes::targetDomain() const
{
    return DomainId{QStringLiteral("todo")};
}

QList<std::pair<Shape, PropertyCatalogue>> TodoPalmShapes::peerShapes() const
{
    const Shape palm{ DomainId{"todo"}, EncodingId{"palm"} };
    return { { palm, makePalmCatalogue() } };
}

QList<TransformationEdge> TodoPalmShapes::edges() const
{
    const Shape palm { DomainId{"todo"}, EncodingId{"palm"} };
    const Shape vtodo{ DomainId{"todo"}, EncodingId{"ical-vtodo"} };
    // palm -> ical-vtodo (lossless; identity X- stamps preserved by vtodo<->canon).
    // ical-vtodo -> palm (lossy; Palm ToDoDB holds a subset of VTODO).
    // The ical-vtodo endpoint is registered by libkalburator's TodoStockShapes,
    // which loads earlier in the same PluginManager batch.
    return {
        TransformationEdge{ palm, vtodo, palmToVTodoLoss(), std::make_shared<PalmToVTodoStage>() },
        TransformationEdge{ vtodo, palm, vtodoToPalmLoss(), std::make_shared<VTodoToPalmStage>() },
    };
}

} // namespace WildPalms::TodoPlugin
