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
set_target_properties(wildpalms_todos_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(wildpalms_todos_static
    PUBLIC
        Kalburator::Sync
    PRIVATE
        # K.8b T6: WildPalmsCore removed to break CMake autogen cycle.
        # Symbols resolved when this static is pulled into WildPalmsCore.so.
        WildPalmsPalmSync
        WildPalmsPalmCalendar           # CategoryMappingStore + parseCategoryAppInfo
        WildPalmsPalmCodecs             # decodeTodo / encodeTodo
        WildPalmsPalmConflict           # PalmConflictHandler
        KF6::CalendarCore
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
)
