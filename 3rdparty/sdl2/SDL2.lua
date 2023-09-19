project "SDL2"
	filter "system:windows"               -- SDL needs to be a DLL on windows for
		kind          "SharedLib"         -- some reason :)
		staticruntime "off"

	filter "system:macosx"
		kind          "StaticLib"
		staticruntime "on"

	filter "system:linux"
		kind          "StaticLib"
		staticruntime "on"
	filter {}

	language          "C++"              -- Some files are C++ files, although they
	cppdialect        "C++17"            -- are not needed on normal Windows.
                            
	systemversion     "latest"

	flags {
		              "NoRuntimeChecks", -- Only used on Visual Studio.
		              "NoBufferSecurityCheck"
	}

	vectorextensions  "SSE"               -- Necessary to run x32.

	location          "Intermediate/ProjectFiles/%{_ACTION}"

	targetdir         "Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	objdir            "Intermediate/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"

	includedirs      {"include"}

	filter "system:windows"
		links {
			         "setupapi",
			         "winmm",
			         "imm32",
			         "version",
		}
	filter {}

	files {
		-- All platforms.
		-- Header files.                                    -- C files.
		"include/*.h",									    
														    
		"src/audio/disk/*.h",                               "src/atomic/*.c",
		"src/audio/dummy/*.h",						        "src/audio/disk/*.c",
		"src/audio/*.h",								    "src/audio/dummy/*.c",
		"src/dynapi/*.h",								    "src/audio/*.c",
		"src/events/blank_cursor.h",					    "src/cpuinfo/*.c",
		"src/events/default_cursor.h",				        "src/dynapi/*.c",
		"src/events/SDL_clipboardevents_c.h",			    "src/events/*.c",
		"src/events/SDL_displayevents_c.h",			        "src/file/*.c",
		"src/events/SDL_dropevents_c.h",				    "src/haptic/*.c",
		"src/events/SDL_events_c.h",					    "src/joystick/hidapi/*.c",
		"src/events/SDL_gesture_c.h",					    "src/joystick/*.c",
		"src/events/SDL_keyboard_c.h",				        "src/libm/*.c",
		"src/events/SDL_mouse_c.h",					        "src/power/*.c",
		"src/events/SDL_sysevents.h",					    "src/render/opengl/*.c",
		"src/events/SDL_touch_c.h",					        "src/render/opengles2/*.c",
		"src/events/SDL_windowevents_c.h",			        "src/render/SDL_render.c",
		"src/haptic/SDL_syshaptic.h",					    "src/render/SDL_yuv_sw.c",
		"src/joystick/hidapi/*.h",					        "src/render/software/*.c",
		--[["src/joystick/hidapi/SDL_hidapijoystick_c.h",]] "src/*.c",
		"src/joystick/SDL_hidapijoystick_c.h",		        "src/sensor/dummy/SDL_dummysensor.c",
		"src/joystick/SDL_joystick_c.h",				    "src/sensor/SDL_sensor.c",
		"src/joystick/SDL_sysjoystick.h",				    "src/stdlib/*.c",
		"src/libm/*.h",								        "src/thread/generic/SDL_syscond.c",
		"src/render/opengl/*.h",						    "src/thread/*.c",
		"src/render/opengles/*.h",					        "src/thread/windows/SDL_sysmutex.c",
		"src/render/SDL_yuv_sw_c.h",					    "src/thread/windows/SDL_syssem.c",
		"src/render/software/*.h",					        "src/thread/windows/SDL_systhread.c",
		"src/render/SDL_sysrender.h",					    "src/thread/windows/SDL_systls.c",
		"src/SDL_dataqueue.h",						        "src/timer/*.c",
		"src/SDL_error_c.h",							    "src/timer/windows/SDL_systimer.c",
		"src/sensor/dummy/*.h",						        "src/video/dummy/*.c",
		"src/sensor/*.h",								    "src/video/*.c",
		"src/thread/*.h",								    "src/video/yuv2rgb/*.c",
		"src/timer/*.h",
		"src/video/dummy/*.h",
		"src/video/*.h",
	}

	filter "system:windows"
		files {
			-- Windows specific files.
			-- Header files.                                -- C files.
			"include/SDL_config_windows.h",				    
														    
			"src/audio/directsound/*.h",                    "src/audio/directsound/*.c",
			"src/audio/wasapi/*.h",						    "src/audio/winmm/*.c",
			"src/audio/winmm/*.h",						    "src/audio/wasapi/*.c",
			--[["src/windows/SDL_directx.h",]]  		    "src/core/windows/*.c",
			"src/core/windows/*.h",						    "src/filesystem/windows/*.c",
			"src/haptic/windows/*.h",					    "src/haptic/windows/*.c",
			"src/joystick/windows/*.h",					    "src/joystick/windows/*.c",
			"src/render/direct3d11/SDL_shaders_d3d11.h",    "src/hidapi/windows/*.c",
			"src/render/direct3d/*.h",					    "src/loadso/windows/*.c",
			"src/render/SDL_d3dmath.h",					    "src/power/windows/*.c",
			"src/thread/windows/*.h",					    "src/render/direct3d11/*.c",
			"src/video/windows/SDL_vkeys.h",			    "src/render/direct3d/*.c",
			"src/video/windows/SDL_windowsclipboard.h",	    "src/render/SDL_d3dmath.c",
			"src/video/windows/SDL_windowsevents.h",	    "src/video/windows/*.c",
			"src/video/windows/SDL_windowsframebuffer.h",
			"src/video/windows/SDL_windowskeyboard.h",
			"src/video/windows/SDL_windowsmessagebox.h",
			"src/video/windows/SDL_windowsmodes.h",
			"src/video/windows/SDL_windowsmouse.h",
			"src/video/windows/SDL_windowsopengl.h",
			"src/video/windows/SDL_windowsshape.h",
			"src/video/windows/SDL_windowsvideo.h",
			"src/video/windows/SDL_windowsvulkan.h",
			"src/video/windows/SDL_windowswindow.h",
			"src/video/windows/wmmsg.h",
		}

		
	inlining          "Explicit"             -- General optimisation options.
	intrinsics        "Off"

	filter "system:windows"
		systemversion "latest"
		defines {
			          "_WINDOWS"
		}

	filter "configurations:Debug"
		defines {
			          "_DEBUG"
		}
		runtime       "Debug"
		symbols       "On"

	filter "configurations:Release"
		defines {
			          "NDEBUG"
		}
		runtime       "Release"
		optimize      "Speed"


	filter "configurations:Development"     -- These are the configurations I tend to
		defines {                           -- use in my projects, but I have added 
			          "NDEBUG"              -- the default ones anyway.
		}
		runtime       "Release"
		optimize      "On"

	filter "configurations:Ship"
		defines {
			          "NDEBUG"
		}
		runtime       "Release"
		optimize      "Speed"
