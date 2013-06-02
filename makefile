all:
	gcc hw7.c -o hw7User -lpthread `pkg-config --libs --cflags gtk+-2.0`

clean:
	rm hw7User
