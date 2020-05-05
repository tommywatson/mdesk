#
## mdesk makefile
#
## © 2020 Shackledragger LLC. All Rights Reserved.
#

EXE=mdesk.exe

CC=x86_64-w64-mingw32-gcc
CFLAGS=-W -Wall -Wextra -Wnarrowing -pedantic
LDFLAGS=-lgdi32 -Wl,-subsystem,windows


$(EXE): mdesk.c
	$(CC) $(CFLAGS) mdesk.c -o $(EXE) $(LDFLAGS)

tilde:
	find ./ -name \*~ | xargs rm -f

clean: tilde
	rm -f *.o mdesk.exe

commit: clean
	git add . 
	git commit
	git push

