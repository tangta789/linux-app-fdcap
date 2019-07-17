all:fdcap.c
	$(CC) -o fdcap fdcap.c
clean:
	rm -rf fdcap
install:all
	install -D $(DESTDIR)/usr/bin/
	install 0755 fdcap $(DESTDIR)/usr/bin/
