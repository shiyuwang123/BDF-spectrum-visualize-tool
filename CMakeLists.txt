cmake_minimum_required(VERSION 3.10)
project(VTKCSVPlot)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Python development libraries
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

find_package(VTK REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    FiltersSources
    FiltersGeneral
    FiltersStatistics
    IOXML
    RenderingContextOpenGL2
    RenderingCore
    RenderingFreeType
    RenderingContext2D
    InteractionStyle
    ChartsCore
    InfovisCore              # for vtkColorSeries
    RenderingGL2PSOpenGL2    # for vtkGL2PSExporter
    RenderingOpenGL2 # Use RenderingOpenGL if OpenGL2 is not available/desired
    ViewsContext2D
    IOExportGL2PS
)

if (NOT VTK_FOUND)
  message(FATAL_ERROR "Axes: Unable to find the VTK build folder.")
endif()

add_executable(plotspec src/main.cpp)
target_link_libraries(plotspec PRIVATE ${VTK_LIBRARIES} Python3::Python)
target_include_directories(plotspec PRIVATE ${Python3_INCLUDE_DIRS})
install(TARGETS plotspec DESTINATION bin)

vtk_module_autoinit(
  TARGETS plotspec
  MODULES ${VTK_LIBRARIES}
)
