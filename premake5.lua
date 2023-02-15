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
	flags { "FatalWarnings" }

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
		buildoptions { "-Wno-warn_unused_result" }
		links { "m" }

	filter {}

-- The output target
local output_target = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "cwalk"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/**.c"
	}

project "cJSON"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/**.c"
	}

project "lua"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.c"
	}

project "ckmem"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/src/**.c"
	}

	includedirs { "%{prj.name}/", "." }

project "fflib"
	language "C"
	kind "StaticLib"

	targetdir( "sbin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		"%{prj.name}/**.h",
		"%{prj.name}/src/**.c"
	}

	includedirs { "%{prj.name}/", "." }

project "ck"
	language "C"
	kind "ConsoleApp"

	targetdir( "bin/" .. output_target .. "/%{prj.name}" )
	objdir( "obj/" .. output_target .. "/%{prj.name}" )

	files {
		"%{prj.name}/include/**.h",
		"%{prj.name}/src/**.c"
	}

	includedirs { "%{prj.name}/", ".", "cwalk/", "cJSON/", "lua/src/" }

	links { "cJSON", "cwalk", "lua", "ckmem", "fflib" }