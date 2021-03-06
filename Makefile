BUILDROOT=.

build: chess2

include makefile.mk

.PHONY: chess2
chess2: $(obj)
	@cd src && make
	$(CC) $(CFLAGS) -o $@ $^ src/*.o

.PHONY: clean
clean:
	@cd src && make clean
	rm -f $(obj) chess2
