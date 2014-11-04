solution "Bitable"
	configurations { "DebugLib", "ReleaseLib", "DebugDLL", "ReleaseDLL" }
	platforms      { "x32", "x64" }
	includedirs    { "include" }
	flags          { "NoPCH" }
	location       ( _ACTION )

	project "bitable"
		language "C"
		files { "bitable/*.c", "bitable/*.h", "include/*.h" }
		objdir ( _ACTION .. "/bitable" )
				
		configuration "*Lib"
			kind "StaticLib"

		configuration "*DLL"
			kind "SharedLib"
			defines { "BITABLE_DLL", "BITABLE_DLL_EXPORT" }

		configuration "Debug*"
			flags { "Symbols" }
			
		configuration "Release*"
			flags { "OptimizeSpeed" }
			
		configuration "windows"
			excludes { "bitable/*.posix.c"}
			defines	"_CRT_SECURE_NO_WARNINGS"

		configuration "linux"
			excludes { "bitable/*.win32.c"}
			buildoptions { "-fvisibility=hidden" }
			
		configuration { "x64", "DebugLib" }
			targetdir "bin/64/debug_lib"

		configuration { "x64", "ReleaseLib" }
			targetdir "bin/64/release_lib"

		configuration { "x64", "DebugDLL" }
			targetdir "bin/64/debug_dll"

		configuration { "x64", "ReleaseDLL" }
			targetdir "bin/64/release_dll"
			
		configuration { "x32", "DebugLib" }
			targetdir "bin/32/debug_lib"

		configuration { "x32", "ReleaseLib" }
			targetdir "bin/32/release_lib"

		configuration { "x32", "DebugDLL" }
			targetdir "bin/32/debug_dll"

		configuration { "x32", "ReleaseDLL" }
			targetdir "bin/32/release_dll"

	project "example"
		language "C++"
		kind "ConsoleApp"
		files { "example/*.cpp", "example/*.c", "example/*.h" }
		links { "bitable" }

		configuration "Debug*"
			flags { "Symbols" }
			
		configuration "Release*"
			flags { "OptimizeSpeed" }

		configuration "*DLL"
			defines { "BITABLE_DLL" }
			if os.is( "linux" ) then
				if _ACTION == "gmake" then
					linkoptions { "-Wl,-rpath,'$$ORIGIN'" } 
				elseif _ACTION == "codeblocks" then
					linkoptions { "-Wl,-R\\\\$$$ORIGIN" }
				end
			end
	

		configuration { "x64", "DebugLib" }
			targetdir "bin/64/debug_lib"

		configuration { "x64", "ReleaseLib" }
			targetdir "bin/64/release_lib"

		configuration { "x64", "DebugDLL" }
			targetdir "bin/64/debug_dll"

		configuration { "x64", "ReleaseDLL" }
			targetdir "bin/64/release_dll"
			
		configuration { "x32", "DebugLib" }
			targetdir "bin/32/debug_lib"

		configuration { "x32", "ReleaseLib" }
			targetdir "bin/32/release_lib"

		configuration { "x32", "DebugDLL" }
			targetdir "bin/32/debug_dll"

		configuration { "x32", "ReleaseDLL" }
			targetdir "bin/32/release_dll"
		