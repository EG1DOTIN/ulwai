; ==============================================================================
; ULWAI AI Agent - Professional One-Click Windows Setup (NSIS Script)
; Generates a clean, standalone, unsigned/signed setup executable.
; ==============================================================================

!include "MUI2.nsh"
!include "FileFunc.nsh"

; General Configuration
Name "ULWAI AI Agent"
OutFile "..\dist\ULWAI-v2.0.0-windows-setup.exe"
InstallDir "$LOCALAPPDATA\Programs\ULWAI"
InstallDirRegKey HKCU "Software\ULWAI" "InstallDir"
RequestExecutionLevel user

; Compression Settings
SetCompressor /SOLID lzma

; UI Settings
!define MUI_ICON "..\..\resources\icons\ulwai.ico"
!define MUI_UNICON "..\..\resources\icons\ulwai.ico"
!define MUI_ABORTWARNING

; Installer Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\ulw-ai-agent.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ULWAI AI Agent now"
!insertmacro MUI_PAGE_FINISH

; Uninstaller Pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

; Installation Section
Section "Install Core Application" SecCore
    SetOutPath "$INSTDIR"

    ; Copy all packaged distribution files (binaries, DLLs, plugins, resources)
    File /r "..\..\build\windows-stage\*.*"

    ; Write Start Menu and Desktop Shortcuts
    CreateDirectory "$SMPROGRAMS\ULWAI AI Agent"
    CreateShortcut "$SMPROGRAMS\ULWAI AI Agent\ULWAI AI Agent.lnk" "$INSTDIR\ulw-ai-agent.exe" "" "$INSTDIR\ulw-ai-agent.exe" 0
    CreateShortcut "$SMPROGRAMS\ULWAI AI Agent\ULWAI Terminal CLI.lnk" "$INSTDIR\ulwai-cli.exe" "" "$INSTDIR\ulwai-cli.exe" 0
    CreateShortcut "$SMPROGRAMS\ULWAI AI Agent\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0
    CreateShortcut "$DESKTOP\ULWAI AI Agent.lnk" "$INSTDIR\ulw-ai-agent.exe" "" "$INSTDIR\ulw-ai-agent.exe" 0

    ; Write Uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; Registry Information for Windows Add/Remove Programs
    WriteRegStr HKCU "Software\ULWAI" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "DisplayName" "ULWAI AI Agent"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "DisplayVersion" "2.0.0"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "Publisher" "EG1 (Gautam Jangid)"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "DisplayIcon" "$INSTDIR\ulw-ai-agent.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "URLInfoAbout" "https://eg1.in"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI" "NoRepair" 1
SectionEnd

; Uninstallation Section
Section "Uninstall"
    ; Remove Shortcuts
    Delete "$DESKTOP\ULWAI AI Agent.lnk"
    Delete "$SMPROGRAMS\ULWAI AI Agent\*.*"
    RMDir "$SMPROGRAMS\ULWAI AI Agent"

    ; Remove Files & Directory
    RMDir /r "$INSTDIR"

    ; Remove Registry Keys
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ULWAI"
    DeleteRegKey HKCU "Software\ULWAI"
SectionEnd
