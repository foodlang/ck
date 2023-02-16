workspace "ck"
	configurations { "Debug", "Release" }
	platforms { "x86_32", "x86_64" }
	startproject "ck"

	-- Default target: x86_64
	defaultplatform "x86_64"

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

	filter "platforms:x86_32"
		architecture "x86"

	filter "platforms:x86_64"
		architecture "x86_64"

	filter "system:Windows"
		systemversion "latest"
		toolset "clang" -- No MSVC
		defines { "_CRT_SECURE_NO_WARNINGS" }

	filter "system:Linux"
		toolset "gcc"
		cdialect "gnu11"
		links { "m" }

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

	files {
		"src/**.c",
		"include/**.h"
	}

	includedirs { "include/", "vendor/cwalk/", "vendor/cJSON/", "vendor/lua/src/" }

	links { "cJSON", "cwalk", "lua" }