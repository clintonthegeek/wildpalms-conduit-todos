kcoreaddons_add_plugin(wildpalms_todos
    SOURCES
        todoconduit.cpp
        todoconduit.h
        todomapper.cpp
        todomapper.h
        taskview.cpp
        taskview.h
    INSTALL_NAMESPACE "wildpalms/conduits"
)

target_link_libraries(wildpalms_todos
    WildPalmsCore
    KF6::CoreAddons
    KF6::CalendarCore
    KF6::I18n
    KF6::WidgetsAddons
    Qt::Widgets
)
