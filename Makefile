cashe: cashe.c
	cc -std=c11 -O3 -flto -Wall -Wextra -pedantic -o $@ $<

install: cashe
	mv cashe /usr/local/bin/cashe
