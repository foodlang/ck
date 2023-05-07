workspace "ck"
	configurations { "Debug", "Release" }
	platforms { "x86", "x86_64", "ARM", "ARM64" }
	startproject "ck"

	floatingpoint "Strict"
	intrinsics "On"
	stringpooling "On"
	staticruntime "On"

	filter "configurations:Debug"
		symbols "On"
		defines { "_DEBUG" }
		optimize "Debug"
		sanitize { "Address" }

	filter "configurations:Release"
		optimize "Full"
		omitframepointer "On"
		flags { "LinktimeOptimization", "NoBufferSecurityCheck", "NoRuntimeChecks" }

	filter "system:Windows"
		systemversion "latest"
		toolset "clang" -- No MSVC
		defines { "_CRT_SECURE_NO_WARNINGS" }

	filter "system:Linux"
		toolset "gcc"
		cdialect "gnu11"
		links { "m" }
	
	filter "configurations:*_x86_32"
		architecture "x86"
	
	filter "configurations:*_x86_64"
		architecture "x86_64"
	
	filter "configurations:*_ARM"
		architecture "ARM"
	
	filter "configurations:*_ARM64"
		architecture "ARM64"

	filter {}

local output_target = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
local vendorsrc = "vendor/%{prj.name}/"

project "cwalk"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		( vendorsrc .. "**.h" ),
		( vendorsrc .. "**.c" )
	}

project "cJSON"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		( vendorsrc .. "**.h" ),
		( vendorsrc .. "**.c" )
	}

project "lua"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		(vendorsrc .. "src/**.h"),
		(vendorsrc .. "src/**.c")
	}

project "ck"
	language "C"
	kind "ConsoleApp"
	targetdir( "bin/" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	linkoptions { "`llvm-config --ldflags --system-libs --libs core`" }
	buildoptions { "`llvm-config --cflags`" }

	files {
		"src/**.c",
		"include/**.h"
	}

	includedirs { "include/", "vendor/cwalk/", "vendor/cJSON/", "vendor/lua/src/" }

	links { "cJSON", "cwalk", "lua" }

	-- Copying Lua helper code to bin directory
	postbuildcommands { "{COPYDIR} lapi bin/" }