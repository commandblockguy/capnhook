all: lib test_program

lib:
	$(MAKE) -C src

test_program:
	$(MAKE) -C test
