# Microsoft Developer Studio Project File - Name="mbot" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=mbot - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mbot.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mbot.mak" CFG="mbot - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mbot - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mbot - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mbot - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /GX /O1 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x816 /d "NDEBUG"
# ADD RSC /l 0x816 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"..\mbot.exe"
# SUBTRACT LINK32 /map /debug

!ELSEIF  "$(CFG)" == "mbot - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mbot___Win32_Debug"
# PROP BASE Intermediate_Dir "mbot___Win32_Debug"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /GX /O1 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /GX /Zi /Od /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x816 /d "NDEBUG"
# ADD RSC /l 0x816 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"..\mbot.exe"
# SUBTRACT BASE LINK32 /map /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"..\mbot.exe"
# SUBTRACT LINK32 /map

!ENDIF 

# Begin Target

# Name "mbot - Win32 Release"
# Name "mbot - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\Bot.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Channel.cpp
# End Source File
# Begin Source File

SOURCE=..\src\List.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ListUsers.cpp
# End Source File
# Begin Source File

SOURCE=..\src\ListVars.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Log.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Module.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Net.cpp
# End Source File
# Begin Source File

SOURCE=..\src\NetDCC.cpp
# End Source File
# Begin Source File

SOURCE=..\src\NetServer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Script.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Services.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Strings.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Text.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mbot.cpp
# End Source File
# Begin Source File

SOURCE=..\src\missing.cpp
# End Source File
# Begin Source File

SOURCE=..\src\scriptcmd.cpp
# End Source File
# Begin Source File

SOURCE=..\src\utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\Bot.h
# End Source File
# Begin Source File

SOURCE=..\inc\Channel.h
# End Source File
# Begin Source File

SOURCE=..\inc\List.h
# End Source File
# Begin Source File

SOURCE=..\inc\ListUsers.h
# End Source File
# Begin Source File

SOURCE=..\inc\ListVars.h
# End Source File
# Begin Source File

SOURCE=..\inc\Log.h
# End Source File
# Begin Source File

SOURCE=..\inc\Module.h
# End Source File
# Begin Source File

SOURCE=..\inc\Net.h
# End Source File
# Begin Source File

SOURCE=..\inc\NetDCC.h
# End Source File
# Begin Source File

SOURCE=..\inc\NetServer.h
# End Source File
# Begin Source File

SOURCE=..\inc\Script.h
# End Source File
# Begin Source File

SOURCE=..\inc\Services.h
# End Source File
# Begin Source File

SOURCE=..\inc\Strings.h
# End Source File
# Begin Source File

SOURCE=..\inc\Text.h
# End Source File
# Begin Source File

SOURCE=..\inc\defines.h
# End Source File
# Begin Source File

SOURCE=..\inc\mbot.h
# End Source File
# Begin Source File

SOURCE=..\inc\missing.h
# End Source File
# Begin Source File

SOURCE=..\inc\scriptcmd.h
# End Source File
# Begin Source File

SOURCE=..\inc\system.h
# End Source File
# Begin Source File

SOURCE=..\inc\utils.h
# End Source File
# End Group
# End Target
# End Project
