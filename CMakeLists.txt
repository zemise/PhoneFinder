cmake_minimum_required(VERSION 3.16)
project(PhoneFinder VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PHONEFINDER_CORE_SOURCES
  src/csv_loader.cpp
  src/pinyin.cpp
  src/search.cpp
  src/hotkey_rules.cpp
  src/settings.cpp
  src/service.cpp
)

add_executable(phonefinder_cpp
  src/main.cpp
  ${PHONEFINDER_CORE_SOURCES}
)
target_include_directories(phonefinder_cpp PRIVATE include)
set_target_properties(phonefinder_cpp PROPERTIES OUTPUT_NAME "PhoneFinderCLI")

find_package(Qt5Core CONFIG QUIET)
find_package(Qt5Gui CONFIG QUIET)
find_package(Qt5Widgets CONFIG QUIET)
if(Qt5Core_FOUND AND Qt5Gui_FOUND AND Qt5Widgets_FOUND)
  if(WIN32)
    add_executable(phonefinder_qt WIN32
      src/qt_main.cpp
      src/qt_mainwindow.cpp
      include/phonefinder/qt_mainwindow.hpp
      ${PHONEFINDER_CORE_SOURCES}
    )
  else()
    add_executable(phonefinder_qt
      src/qt_main.cpp
      src/qt_mainwindow.cpp
      include/phonefinder/qt_mainwindow.hpp
      ${PHONEFINDER_CORE_SOURCES}
    )
  endif()
  target_include_directories(phonefinder_qt PRIVATE include)
  target_link_libraries(phonefinder_qt PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
  set_target_properties(phonefinder_qt PROPERTIES OUTPUT_NAME "PhoneFinder")
  if(WIN32)
    target_sources(phonefinder_qt PRIVATE src/qt_app.rc)
  endif()
  if(APPLE)
    target_link_libraries(phonefinder_qt PRIVATE "-framework Carbon")
  endif()
endif()

if(MSVC)
  target_compile_options(phonefinder_cpp PRIVATE /W4 /permissive-)
  if(TARGET phonefinder_qt)
    target_compile_options(phonefinder_qt PRIVATE /W4 /permissive-)
  endif()
else()
  target_compile_options(phonefinder_cpp PRIVATE -Wall -Wextra -Wpedantic)
  if(TARGET phonefinder_qt)
    target_compile_options(phonefinder_qt PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()
