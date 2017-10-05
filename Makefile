VERSION := 0.5.0-SNAPSHOT
PYTHON := python
MODNAME := glpk
ARCHIVE := py$(MODNAME)-$(VERSION)
CURDIR := $(shell pwd)

.PHONY : all install clean cleaner archive syncto syncfrom valgrinder docs

glpk.so: all

all:
	$(PYTHON) setup.py build
	rm -f $(MODNAME).so
	ln -s build/lib.*/$(MODNAME).so

install:
	$(PYTHON) setup.py install

clean:
	@rm -rf build
	@rm -f glpk.so

cleaner: clean
	@rm -rf .tox
	find . -name "*~" -delete
	find . -name "*.pyc" -delete

$(ARCHIVE).tar.bz2: cleaner
	mkdir -p $(ARCHIVE)
	cp -rp *.txt Makefile examples html setup.py src tests $(ARCHIVE)
	tar -cjf $@ $(ARCHIVE)
	rm -rf $(ARCHIVE)

archive: $(ARCHIVE).tar.bz2

# Make the documents.

docs: install
	make -C docs singlehtml

# Functions for remote synchronization of this project, mostly for
# backup purposes.

REMOTE := tomf@kodiak.cs.cornell.edu:glpk/

syncto:
	rsync -rvtu --exclude 'build' --exclude '*~' --exclude "locals" --exclude "glpk" * "$(REMOTE)"
syncfrom:
	rsync -rvtu "$(REMOTE)[^b]*" .

valgrinder2:
	valgrind --tool=memcheck --leak-check=yes --db-attach=yes --show-reachable=yes --suppressions=valgrind-python.supp $(PYTHON) -i test2.py

valgrinder:
	valgrind --tool=memcheck --leak-check=yes --db-attach=yes --suppressions=valgrind-python.supp $(PYTHON)

# Builds for the locally built glpk.

glpk/glpk-4.%: glpk/glpk-4.%.tar.gz
	cd glpk; tar -zxf ../$<

locals/%: glpk/glpk-%
	cd glpk/glpk-$* ; ./configure --prefix=$(CURDIR)/locals/$* ; make -j 8 CFLAGS=-m32 LDFLAGS=-m32 install

local% : locals/4.%
	$(PYTHON) setup.py build glpver=$*
	rm -f $(MODNAME).so
	ln -s build/lib.*/$(MODNAME).so

testlocal% :
	make clean
	make local$*
	make test
