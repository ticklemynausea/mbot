# Microsoft Developer Studio Project File - Name="watch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=watch - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "watch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "watch.mak" CFG="watch - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "watch - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "watch___Win32_Release"
# PROP BASE Intermediate_Dir "watch___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WATCH_EXPORTS" /YX /FD /c
# ADD CPP /nologo /W1 /GX /O1 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WATCH_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x816 /d "NDEBUG"
# ADD RSC /l 0x816 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\mbot.lib /nologo /dll /machine:I386 /out:"..\mod\watch.dll"
# Begin Target

# Name "watch - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\mod\watch.cpp
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
