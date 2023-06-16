INCLUDE_PATH = $(HOME)/prj/lib13/include
LIB_PATH = 
CFLAGS = -Winline -Wall -fPIC -I$(INCLUDE_PATH)
debug: CFLAGS += -O -g
release: CFLAGS += -O2 -DNDEBUG
OUTDIR = debug/
release: OUTDIR = release/

SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)

EXTERNLIBS = $(HOME)/prj/sqlite/sqlite3.o $(HOME)/prj/libntru/libntru.a
LINKLIBS = -lpthread

MV = mv
RM = rm
MKDIR = mkdir
AR = ar -crs
TAR = tar
ZIP = bzip2

all: debug

release: $(OBJ)
	$(CC) --shared $(CFLAGS) $(LINKSLIBS) -o lib13.so $< $(EXTERNLIBS)
	$(AR) lib13.a $< $(EXTERNLIBS)
	@$(MV) lib13.so lib13.a $(OUTDIR)
	@$(MV) *.o obj/
	$(info *** put libs in release/ ; 'make clean' before 'make debug' ***)

debug: $(OBJ)
	$(CC) --shared $(CFLAGS) $(LINKSLIBS) -o lib13.so $< $(EXTERNLIBS)
	$(AR) lib13.a $< $(EXTERNLIBS)
	@$(MV) lib13.so lib13.a $(OUTDIR)
	@$(MV) *.o obj/
	$(info *** put libs in debug/ ; 'make clean' before 'make release' ***)
	
cleanall: clean
	@$(RM) release/* debug/*

clean:
	@$(RM) -f *.o obj/* *.*~ *~

backup:
	@$(TAR) -cf lib13.tar *.c makefile include/
	@$(ZIP) lib13.tar

init:
	$(MKDIR) release debug obj
