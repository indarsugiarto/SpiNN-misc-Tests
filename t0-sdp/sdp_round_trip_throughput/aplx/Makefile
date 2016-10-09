# What if we compile both spin3 and spin5 version here?
#MKFILES = Makefile.1 Makefile.2 

RM := \rm -rf
LS := \ls -l
MK := \make -f

all: $(APPS)
	@$(MK) Makefile.1
	@$(MK) Makefile.2

clean: $(APPS)
	@$(MK) Makefile.1 clean
	@$(MK) Makefile.2 clean
