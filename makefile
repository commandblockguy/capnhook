all: lib test_program

lib:
	$(MAKE) -C src

install:
	$(MAKE) -C src install

test_program:
	$(MAKE) -C test
