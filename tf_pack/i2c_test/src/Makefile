all: i2c_test 

i2c_test: i2c_test.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -Wall -o i2c_test $^ $(LDLIBS) -lrt

