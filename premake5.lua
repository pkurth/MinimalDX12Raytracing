
-- Premake extension to include files at solution-scope. From https://github.com/premake/premake-core/issues/1061#issuecomment-441417853

require('vstudio')
premake.api.register {
	name = "workspacefiles",
	scope = "workspace",
	kind = "list:string",
}

premake.override(premake.vstudio.sln2005, "projects", function(base, wks)
	if wks.workspacefiles and #wks.workspacefiles > 0 then
		premake.push('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Solution Items", "Solution Items", "{' .. os.uuid("Solution Items:"..wks.name) .. '}"')
		premake.push("ProjectSection(SolutionItems) = preProject")
		for _, file in ipairs(wks.workspacefiles) do
			file = path.rebase(file, ".", wks.location)
			premake.w(file.." = "..file)
		end
		premake.pop("EndProjectSection")
		premake.pop("EndProject")
	end
	base(wks)
end)


-----------------------------------------
-- GENERATE SOLUTION
-----------------------------------------

workspace "MinimalDX12Raytracing"
	architecture "x64"
	startproject "MinimalDX12Raytracing"

	configurations {
		"Debug",
		"Release"
	}

	flags {
		"MultiProcessorCompile"
	}
	
	workspacefiles {
        "premake5.lua",
    }


outputdir = "%{cfg.buildcfg}_%{cfg.architecture}"
shaderoutputdir = "shaders/bin/%{cfg.buildcfg}/"


group "Dependencies"
group ""





-----------------------------------------
-- GENERATE PROJECT
-----------------------------------------

project "MinimalDX12Raytracing"
	--location "bin/MinimalDX12Raytracing"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"

	targetdir ("./bin/" .. outputdir)
	objdir ("./bin_int/" .. outputdir ..  "/%{prj.name}")

	debugenvs {
		"PATH=ext/bin;%PATH%;"
	}
	debugdir "."

	files {
		"src/**.h",
		"src/**.cpp",
		"shaders/**.hlsl*",
	}

	vpaths {
		["Headers/*"] = { "src/**.h" },
		["Sources/*"] = { "src/**.cpp" },
		["Shaders/*"] = { "shaders/**.hlsl" },
	}

	libdirs {
		"ext/lib",
	}

	links {
		"d3d12",
		"D3Dcompiler",
		"DXGI",
		"dxguid",
		"dxcompiler",
	}

	includedirs {
		"src",
	}

	sysincludedirs {
		"ext",
	}

	prebuildcommands {
		"ECHO Compiling shaders...",
	}

	--vectorextensions "AVX2"
	floatingpoint "Fast"

	filter "configurations:Debug"
        runtime "Debug"
		symbols "On"
		
	filter "configurations:Release"
        runtime "Release"
		optimize "On"
		inlining "Auto"

	filter "system:windows"
		systemversion (sdk_version_string)

		defines {
			"_UNICODE",
			"UNICODE",
			"_CRT_SECURE_NO_WARNINGS",
		}

		defines { "SHADER_BIN_DIR=L\"" .. shaderoutputdir .. "\"" }

	filter "files:**.hlsl"
		shadermodel "6.5"
		shaderdefines {
			"SHADERMODEL=65"
		}

		flags "ExcludeFromBuild"
		shaderobjectfileoutput(shaderoutputdir .. "%{file.basename}.cso")
		shaderincludedirs {
			"shaders/rs",
			"shaders/common"
		}
		shaderdefines {
			"HLSL",
			"mat4=float4x4",
			"mat4x3=float4x3",
			"mat3x4=float3x4",
			"vec2=float2",
			"vec3=float3",
			"vec4=float4",
			"int32=int",
			"uint32=uint",
			"uint64=uint64_t",
		}
	
		shaderoptions {
			"/WX",
			"/all_resources_bound",
		}
	
		filter { "configurations:Debug", "files:**.hlsl" }
			shaderoptions {
				"/Qembed_debug",
			}
		
 	
	filter("files:**_vs.hlsl")
		removeflags("ExcludeFromBuild")
		shadertype("Vertex")
 	
	filter("files:**_gs.hlsl")
		removeflags("ExcludeFromBuild")
		shadertype("Geometry")
 	
	filter("files:**_hs.hlsl")
		removeflags("ExcludeFromBuild")
		shadertype("Hull")
 	
	filter("files:**_ds.hlsl")
		removeflags("ExcludeFromBuild")
		shadertype("Domain")
	
	filter("files:**_ps.hlsl")
		removeflags("ExcludeFromBuild")
		shadertype("Pixel")
 	
	filter("files:**_cs.hlsl")
		removeflags("ExcludeFromBuild")
		shadertype("Compute")
		