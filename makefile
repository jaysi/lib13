DEBUGFLAGS13 = -O -g
#DEBUGFLAGS13 = -O2 -DNDEBUG
INCLUDE13 = ./include
CFLAGS = -Winline -Wall -c -fPIC -I$(INCLUDE13)
OBJ = acc13.o \
		aes.o \
		arg13.o \
		base64.o \
		const13.o \
		crypt13.o \
		day13.o \
		db13.o \
		des.o \
		error13.o \
		hash13.o \
		io13.o \
		io13i.o \
		lib13.o \
		lock13.o \
		mem13.o \
		num2text.o \
		obj13.o \
		pack13.o \
		path13.o \
		rand13.o \
		rc4.o \
		rr13.o \
		sha256.o \
		str13.o \
		xtea.o

SRC = *.c

CFLAGS += $(DEBUGFLAGS13)
LIB = lib13.so
IF =
EXTERN = ../sqlite/sqlite3.o ../libntru/libntru.a
TOLINK = -lpthread

all: dynamic static

dynamic: $(OBJ)
	$(CC) --shared $(TOLINK) -o $(LIB) $(OBJ) $(EXTERN)

static: $(OBJ)
	ar -crs lib13.a $(OBJ) $(EXTERN)	

cleanall: clean cleanbak
	@rm -f $(LIB)

clean:
	@rm -f $(OBJ)	

cleanbak:
	@rm -f *.c~
	@rm -f Makefile~ makefile~

backup:
	@tar -cf lib13.tar *.c makefile include/
	@bzip2 lib13.tar

