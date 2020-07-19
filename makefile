all: lib test

lib:
	$(MAKE) -C src

test:
	$(MAKE) -C test
