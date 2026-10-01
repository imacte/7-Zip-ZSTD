PROG = 7zipUninstall.exe
RESOURCE_IMAGE_DEPS = 7zipUninstall.ico
MY_FIXED = 1

!IFDEF Z7_64BIT_INSTALLER
CFLAGS = $(CFLAGS) -DZ7_64BIT_INSTALLER
!ENDIF

MAIN_OBJS = \
  $O\7zipUninstall.obj \

OBJS = \
  $(MAIN_OBJS) \
  $O\resource.res

!include "../../../CPP/Build.mak"

$(MAIN_OBJS): $(*B).c
	$(COMPL_O1)
