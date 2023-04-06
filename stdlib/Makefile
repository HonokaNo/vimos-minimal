include ../env.mk

STDLIB_OBJ := isdigit.o islower.o isspace.o isupper.o memcmp.o memset.o sprintf.o strcat.o strchr.o strcmp.o strcpy.o strlen.o strncpy.o strrchr.o tolower.o toupper.o vsprintf.o

.PHONY: all
all:
	$(MAKE) stdlib.a

stdlib.a: $(STDLIB_OBJ)
	$(AR) r $@ $(STDLIB_OBJ)

.c.o:
	$(COMPILER) $(DEFAULT_COMPILE_OPT) $(STDLIB_OPT) -o $@ -c $<
.s.o:
	$(COMPILER) $(DEFAULT_COMPILE_OPT) --target=x86_64-elf -o $@ -c $<

clean:
	$(REMOVE) $(STDLIB).o
	$(REMOVE) stdlib.a
