workspace "Lab3"
    configurations { 
        "Debug",
        "Release"
    }  
    configurations { "Debug", "Release" }
      
    platforms { "x64" }

    startproject "Lab3"

TRAGET_DIR = "%{wks.location}/build/bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"
OBJ_DIR = "%{wks.location}/build/obj/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"

INCLUDE_DIRS = {}
LINK_DIRS = {}

INCLUDE_DIRS["CLI11"] = "%{wks.location}/vendor/CLI11/include"
INCLUDE_DIRS["spdlog"] = "%{wks.location}/vendor/spdlog/include"

-- HDF5 library location
INCLUDE_DIRS["hdf5"] = "%{wks.location}/../libs/HDF5/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/include"
LINK_DIRS["hdf5"] =  "%{wks.location}/../libs/HDF5/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/lib"
-- SystemC library location
INCLUDE_DIRS["systemc"] = "%{wks.location}/../libs/SystemC/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/include"
LINK_DIRS["systemc"] = "%{wks.location}/../libs/SystemC/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/lib"

project "Lab3"
    kind "ConsoleApp"
    language "C++"
    cdialect "C17"
    cppdialect "C++20"
    characterset ("Unicode")
    
    flags { "MultiProcessorCompile" }
            
    includedirs {
        "./src/",
        "%{INCLUDE_DIRS.CLI11}",
        "%{INCLUDE_DIRS.spdlog}",
        "%{INCLUDE_DIRS.hdf5}",
        "%{INCLUDE_DIRS.systemc}",
    }
    
    libdirs{
        "%{LINK_DIRS.hdf5}",
        "%{LINK_DIRS.systemc}",
    }
    
    links {
        "libhdf5.lib",
        "libhdf5_hl.lib",
        "libhdf5_cpp.lib",
        "libhdf5_hl_cpp.lib",
        "libz.lib",
        "libszaec.lib",
        "libaec.lib",
        "systemc.lib",
    }
    
    files { 
        "./src/**.h",
        "./src/**.cpp",
    }
    
    defines{
        "LOCAL_MEMORY_SIZE=1024000",
        "SHARED_MEMORY_SIZE=1024000",
    }

    targetdir(TRAGET_DIR)
    objdir(OBJ_DIR)

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"

    filter "action:vs*"       
        buildoptions  {"/vmg"}
