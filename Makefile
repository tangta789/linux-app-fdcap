all:fdcap.c
	$(CC) -o fdcap fdcap.c
clean:
	rm -rf fdcap
install:all
	install -d $(DESTDIR)/usr/bin/
	install -m 0755 fdcap $(DESTDIR)/usr/bin/
