all:fdcap.c
	$(CC) -o fdcap fdcap.c
clean:
	rm -rf fdcap
install:all
	$(INSTALL) -D $(DESTDIR)/usr/bin/
	$(INSTALL) 0755 fdcap $(DESTDIR)/usr/bin/
