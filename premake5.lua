-- premake5.lua
workspace "LemonNote"
   configurations { "Debug", "Release" }

project "LemonNote"
   kind "ConsoleApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files { "**.h", "**.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

include "3rdparty/sdl2/SDL2.lua"
include "3rdparty/sdl2/SDL2main.lua"
