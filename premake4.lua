
solution "FractalsDemo"
	configurations {"Debug", "Release"}
	location "build"

	includedirs {
		"../limbus/include",
		"../limbus/generated/cpp",
		"../pingo/include",
		"../pingo/generated/cpp",
		"./include",
	}
	
	project "fractals"
		language "C++"
		location "build"
		targetdir "./bin"

		files {"source/main.cpp"}

		links {
			"limbus",
			"limbus_cpp",
			"pingo",
			"pingo_cpp",
			"opengl32"
		}
		libdirs {
			"../limbus/lib/Debug",
			"../limbus/lib/Release",
			"../pingo/lib",
		}

		configuration "Debug"
			includedirs {
				"../boost_1_47_0_d/",
			}
			libdirs {
				"../boost_1_47_0_d/stage/lib"
			}
			flags {"Symbols", "ExtraWarnings"}
			kind "ConsoleApp"
			defines {"DEBUG"}
		
		configuration "Release"
			includedirs {
				"../boost_1_47_0/",
			}
			libdirs {
				"../boost_1_47_0/stage/lib"
			}
			flags {"OptimizeSpeed"}
			kind "WindowedApp"
			defines {"NDEBUG"}
