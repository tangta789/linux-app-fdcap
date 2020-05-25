all:fdcap.c fdshow.c
	$(CC) -o fdcap fdcap.c
	$(CC) -o fdshow fdshow.c
clean:
	rm -rf fdcap fdshow
install:all
	install -d $(DESTDIR)/usr/bin/
	install -m 0755 fdcap $(DESTDIR)/usr/bin/
	install -m 0755 fdshow $(DESTDIR)/usr/bin/
