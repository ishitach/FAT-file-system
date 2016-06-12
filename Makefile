A1 = gcc
A2 = -lpthread
c_file = main.c filesys.c disk.c
all: $(c_file)
	$(A1) $(c_file) $(A2) -o filesys

$(c_file):
	$(A1) -c $(c_file)
