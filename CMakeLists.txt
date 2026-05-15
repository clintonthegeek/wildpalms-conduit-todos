add_library(wildpalms_todos_static STATIC
    todobackendplugin.cpp     todobackendplugin.h
    todoblobbackend.cpp       todoblobbackend.h
    todoconflicthandler.cpp   todoconflicthandler.h
    todoicstranscoder.cpp     todoicstranscoder.h
    taskview.cpp              taskview.h
)
target_include_directories(wildpalms_todos_static
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
# See calendar plugin's CMakeLists for why this BEFORE include is
# required (Kalburator::Sync ordering vs WildPalmsCore's legacy
# ::Sync include).
target_include_directories(wildpalms_todos_static BEFORE
    PRIVATE
        $<TARGET_PROPERTY:Kalburator::Sync,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(wildpalms_todos_static
    PUBLIC
        Kalburator::Sync
    PRIVATE
        WildPalmsCore
        WildPalmsPalmSync
        WildPalmsPalmCalendar           # CategoryMappingStore + parseCategoryAppInfo
        WildPalmsPalmCodecs             # decodeTodo / encodeTodo
        WildPalmsPalmConflict           # PalmConflictHandler
        KF6::CalendarCore
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
)
