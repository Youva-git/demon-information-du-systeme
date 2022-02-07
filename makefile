LIBDIR=Lib
CLDIR=Client
FIFODIR= Fifo
SERDIR=Serveur
CFLAGS = -std=c11 -pthread -c  -D_XOPEN_SOURCE=600 -Wpedantic -Wwrite-strings -Wall -Wextra -Werror -fstack-protector-all -fpie -D_FORTIFY_SOURCE=2 -O -g -I$(FIFODIR) -lrt
LDFLAGS = -LLib -Wl,-z,relro,-z,now -pie -pthread   -lfifo  -lrt


all :  client serveur  

client : $(CLDIR)/client.o $(LIBDIR)/libfifo.a
	$(CC) $< -o $@ $(LDFLAGS)

serveur : $(SERDIR)/serveur.o $(LIBDIR)/libfifo.a
	$(CC) $< -o $@ $(LDFLAGS)

$(LIBDIR)/libfifo.a : $(FIFODIR)/fifo.o 
	$(AR) -r $@ $<

$(SERDIR)/serveur.o: $(SERDIR)/serveur.c
	$(CC) $(CFLAGS) $^  -o $@ 
$(CLDIR)/client.o: $(CLDIR)/client.c 
	$(CC) $(CFLAGS) $^  -o $@ 
$(FIFODIR)/fifo.o: $(FIFODIR)/fifo.c
	$(CC) $(CFLAGS) $^  -o $@ 

clean:
	$(RM) $(SERDIR)/*.o $(FIFODIR)/*.o $(CLDIR)/*.o client serveur $(LIBDIR)/* [0-9]*
