#
## mdesk makefile
#
## © 2020 Shackledragger LLC. All Rights Reserved.
#

EXE=mdesk.exe
DLL=mdesk.dll
LIB=libmdesk.lib

CC=x86_64-w64-mingw32-gcc
CFLAGS=-W -Wall -Wextra -Wnarrowing -pedantic
LDFLAGS=-lgdi32 -Wl,-subsystem,windows

$(EXE): mdesk.c $(DLL)
	$(CC) $(CFLAGS) mdesk.c -o $(EXE) $(LDFLAGS) $(LIB)

$(DLL): mdeskdll.c mdeskdll.h
	$(CC) $(CFLAGS) mdeskdll.c -o $(DLL) -shared -Wl,--out-implib,$(LIB)

tilde:
	find ./ -name \*~ | xargs rm -f

clean: tilde
	rm -f *.o $(EXE) $(DLL) $(LIB)

commit: clean
	git add . 
	git commit
	git push

dollars:
	sloccount .

