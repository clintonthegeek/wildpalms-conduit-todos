#ifndef WILDPALMS_TODO_PALMTOVTODOTRANSFORMATION_H
#define WILDPALMS_TODO_PALMTOVTODOTRANSFORMATION_H

#include "transformationedge.h"

namespace WildPalms::PalmCalendar { class CategoryMappingStore; }

namespace WildPalms::TodoPlugin {

// (todo, palm) -> (todo, ical-vtodo): wraps the Palm ToDo codec via
// todoicstranscoder. Lossless: a Palm ToDo is a subset of a VTODO (identity
// X- stamps are preserved downstream by libkalburator's vtodo<->canon stage).
// Borrows a (nullable) CategoryMappingStore to carry the Palm category slot
// into the iCalendar CATEGORIES property. The store must outlive the stage.
class PalmToVTodoStage : public Kalburator::Shape::TransformationStage {
public:
    explicit PalmToVTodoStage(
        const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

// (todo, ical-vtodo) -> (todo, palm): lossy; Palm ToDoDB cannot hold most
// VTODO fields. Borrows a (nullable) CategoryMappingStore to map the
// iCalendar CATEGORIES name back to a Palm category slot.
class VTodoToPalmStage : public Kalburator::Shape::TransformationStage {
public:
    explicit VTodoToPalmStage(
        const WildPalms::PalmCalendar::CategoryMappingStore *cats = nullptr);
    QByteArray transform(const QByteArray &sourceBytes) const override;

private:
    const WildPalms::PalmCalendar::CategoryMappingStore *m_cats = nullptr;
};

Kalburator::Shape::LossProfile palmToVTodoLoss();
Kalburator::Shape::LossProfile vtodoToPalmLoss();

} // namespace WildPalms::TodoPlugin

#endif // WILDPALMS_TODO_PALMTOVTODOTRANSFORMATION_H
