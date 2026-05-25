#ifndef WILDPALMS_TODO_PALMTOVTODOTRANSFORMATION_H
#define WILDPALMS_TODO_PALMTOVTODOTRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::TodoPlugin {

// (todo, palm) -> (todo, ical-vtodo): wraps the Palm ToDo codec via
// todoicstranscoder. Lossless: a Palm ToDo is a subset of a VTODO (identity
// X- stamps are preserved downstream by libkalburator's vtodo<->canon stage).
class PalmToVTodoStage : public Kalburator::Shape::TransformationStage {
public:
    QByteArray transform(const QByteArray &sourceBytes) const override;
};

// (todo, ical-vtodo) -> (todo, palm): lossy; Palm ToDoDB cannot hold most
// VTODO fields.
class VTodoToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    QByteArray transform(const QByteArray &sourceBytes) const override;
};

Kalburator::Shape::LossProfile palmToVTodoLoss();
Kalburator::Shape::LossProfile vtodoToPalmLoss();

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_PALMTOVTODOTRANSFORMATION_H
