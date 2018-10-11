-- run premake command to generate a solution file in the temp folder
--  premake5 vs2017
--  premake5 gmake2
--  premake5 xcode4

function os.winSdkVersion()
    -- fix for vs2017 incorrectly selecting 8.1 SDK when the SDK is not installed.
    -- This is patched in latest premake:master branch but not released on the premake website.
    -- In the patched version of premake, use systemversion("latest")
    -- https://github.com/premake/premake-core/issues/935
    
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version end
end

-- CONFIGURATION VARIABLES (this is where we want the generated solution to put its files, or look for source code)
binaries_folder         = "binaries/"
includes_folder         = "include/"
source_folder           = "source/"                         
libs_folder             = "libs/"

workspace "Monte Carlo Ray Tracer"
    location("temp/") -- temporary files (sln, proj, obj, pdb, ilk, etc)
    language "C++"
    cppdialect "gnu++14"  -- flag needed for gcc/clang, visual studio 2017 does not need it

    configurations { "Debug", "Release" }

    if os.host() == "windows" then
        systemversion(os.winSdkVersion() .. ".0")
        system      "windows"
        platforms { "win64" }
        defines   { "OS_WINDOWS" }        
        
    else -- MACOSX
        system      "macosx"
        platforms { "macosx64" }        
        defines   { "OS_MACOSX" }
    end

    filter { "platforms:*64"} architecture "x64"

    if os.host() == "windows" then
        entrypoint "mainCRTStartup"     -- force Windows-executables to use main instead of WinMain as entry point   
        symbolspath '$(TargetName).pdb'

        debugdir(binaries_folder)
        includedirs { includes_folder }
        libdirs     { libs_folder }
        links       { "opengl32", "SDL2" }

    else -- MACOSX
        debugdir(binaries_folder)
        includedirs { includes_folder }
        libdirs     { libs_folder }
        links { "OpenGL.framework", "SDL2.framework"}

        -- These two lines must be added, otherwise xcode won't tell clang how to find and link user added frameworks
        buildoptions { "-F /Library/Frameworks" }
        linkoptions  { "-F /Library/Frameworks" }
    end

    filter { "configurations:Debug" }
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"
        
    filter { "configurations:Release" }
        defines { "NDEBUG" }
        symbols "Off"        
        optimize "On"
        
    filter{}


project "Main Application"
    kind "ConsoleApp"
    targetdir(binaries_folder)
    targetname("MonteCarloRayTracer")
    files ({source_folder .. "**.h", source_folder .. "**.c", source_folder .. "**.cpp"})
    removefiles{ source_folder .. "main*.cpp"}
    files ({source_folder .. "main.cpp"})
    
   project "Obj Loader"
   	kind "ConsoleApp"
    	targetdir(binaries_folder)
    	targetname("MonteCarloRayTracer")
    	files ({source_folder .. "**.h", source_folder .. "**.c", source_folder .. "**.cpp"})
	 removefiles{ source_folder .. "main*.cpp"}
   	 files ({source_folder .. "main_Obj.cpp"})
	