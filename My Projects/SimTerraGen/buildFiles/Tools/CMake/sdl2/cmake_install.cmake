# Install script for directory: D:/Repositorios/Torque3D/Engine/lib/sdl

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/temp")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/Debug/SDL2.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/Release/SDL2.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/MinSizeRel/SDL2.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/RelWithDebInfo/SDL2.lib")
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/game/SDL2.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/game/SDL2.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/game/SDL2.dll")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/game/SDL2.dll")
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/Debug/SDL2main.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/Release/SDL2main.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/MinSizeRel/SDL2main.lib")
  elseif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/RelWithDebInfo/SDL2main.lib")
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/SDL2" TYPE FILE FILES
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_assert.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_atomic.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_audio.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_bits.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_blendmode.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_clipboard.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_android.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_iphoneos.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_macosx.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_minimal.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_pandora.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_psp.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_windows.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_winrt.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_config_wiz.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_copying.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_cpuinfo.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_egl.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_endian.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_error.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_events.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_filesystem.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_gamecontroller.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_gesture.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_haptic.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_hints.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_joystick.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_keyboard.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_keycode.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_loadso.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_log.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_main.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_messagebox.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_mouse.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_mutex.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_name.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengl.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengl_glext.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengles.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengles2.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengles2_gl2.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengles2_gl2ext.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengles2_gl2platform.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_opengles2_khrplatform.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_pixels.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_platform.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_power.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_quit.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_rect.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_render.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_revision.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_rwops.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_scancode.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_shape.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_stdinc.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_surface.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_system.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_syswm.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_assert.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_common.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_compare.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_crc32.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_font.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_fuzzer.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_harness.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_images.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_log.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_md5.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_test_random.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_thread.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_timer.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_touch.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_types.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_version.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/SDL_video.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/begin_code.h"
    "D:/Repositorios/Torque3D/Engine/lib/sdl/include/close_code.h"
    "D:/Repositorios/Torque3D/My Projects/SimTerraGen/buildFiles/Tools/CMake/sdl2/include/SDL_config.h"
    )
endif()

