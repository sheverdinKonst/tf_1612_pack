all: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)  -Wall -lmbedtls -lmbedcrypto -luci -o dmhash $^ $(LDLIBS) -lrt
