PROG = 7zFM.exe

CFLAGS = $(CFLAGS) -DZ7_EXTERNAL_CODECS

!include "../Format7zF/Arc.mak"

!include "../../UI/FileManager/FM.mak"

COMMON_OBJS = $(COMMON_OBJS) \
  $O\CommandLineParser.obj \
  $O\Lang.obj \
  $O\ListFileUtils.obj \
  $O\Random.obj \

WIN_OBJS = $(WIN_OBJS) \
  $O\Clipboard.obj \
  $O\CommonDialog.obj \
  $O\DLL.obj \
  $O\ErrorMsg.obj \
  $O\FileLink.obj \
  $O\MemoryGlobal.obj \
  $O\MemoryLock.obj \
  $O\Menu.obj \
  $O\ProcessUtils.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\SystemInfo.obj \
  $O\Shell.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\ComboBox.obj \
  $O\Dialog.obj \
  $O\ListView.obj \
  $O\PropertyPage.obj \
  $O\Window2.obj \

7ZIP_COMMON_OBJS = $(7ZIP_COMMON_OBJS) \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\MultiOutStream.obj \

UI_COMMON_OBJS = \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveName.obj \
  $O\ArchiveOpenCallback.obj \
  $O\Bench.obj \
  $O\CompressCall.obj \
  $O\CompressCall2.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\HashCalc.obj \
  $O\LoadCodecs.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \
  $O\SetProperties.obj \
  $O\SortUtils.obj \
  $O\TempFiles.obj \
  $O\Update.obj \
  $O\UpdateAction.obj \
  $O\UpdateCallback.obj \
  $O\UpdatePair.obj \
  $O\UpdateProduce.obj \
  $O\WorkDir.obj \
  $O\ZipRegistry.obj \

EXPLORER_OBJS = \
  $O\ContextMenu.obj \
  $O\MyMessages.obj \
  $O\RegistryContextMenu.obj \

GUI_OBJS = \
  $O\BenchmarkDialog.obj \
  $O\CompressDialog.obj \
  $O\ExtractDialog.obj \
  $O\ExtractGUI.obj \
  $O\HashGUI.obj \
  $O\UpdateCallbackGUI.obj \
  $O\UpdateCallbackGUI2.obj \
  $O\UpdateGUI.obj \


!include "../../7zip.mak"

# - 7-Zip ZS -
#
# The options page ("7-Zip ZS") registers the sparse package of the Windows 11
# context menu through the PackageManager API. ShellIntegrationModern.cpp is the
# only C++/WinRT translation unit, so it gets its own compile flags: C++17 (which
# C++/WinRT requires) and the cppwinrt include directory of the Windows SDK (the
# projections ship with the SDK, no NuGet package is needed). The WinRT API
# umbrella library is linked in addition.
#
# Everything else of the project stays on its own settings.
LIBS = $(LIBS) windowsapp.lib

# The C++/WinRT headers ship with the Windows SDK. nmake does not see the
# WindowsSdkDir environment variable that vcvars64 sets in every setup, so the
# path is written by cmd.exe and then included. It can also be given directly:
#   nmake PLATFORM=x64 CPPWINRT_INCLUDE=C:\...\Include\<ver>\cppwinrt
!IFNDEF CPPWINRT_INCLUDE
!IF [cmd /c "echo CPPWINRT_INCLUDE=%WindowsSdkDir%Include\%WindowsSDKVersion%cppwinrt> cppwinrt_path.mak"]
!ENDIF
!INCLUDE cppwinrt_path.mak
!ENDIF

# (no -Yu/-Fp here: the precompiled header is built with the project standard and
#  MSVC rejects /std:c++17 together with a PCH of another standard, so this file
#  includes StdAfx.h as a normal header)
$O\ShellIntegrationModern.obj: ..\..\UI\FileManager\ShellIntegrationModern.cpp
	$(CC) $(CFLAGS_O1) -DZ7_ENABLE_MODERN_SHELL_INTEGRATION -std:c++17 -W3 -EHsc -I"$(CPPWINRT_INCLUDE)" ..\..\UI\FileManager\ShellIntegrationModern.cpp

