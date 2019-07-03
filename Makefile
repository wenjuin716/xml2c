CURR_DIR=$(shell pwd)
EXPAT_DIR=$(CURR_DIR)/expat-2.2.6

OBJS = xml_translate.o main.o
EXEC = xml2c
CFLAGS = -I$(EXPAT_DIR)/INSTALL/include/ 
LDFALGS = -L$(EXPAT_DIR)/INSTALL/lib/ -static -lexpat
TOOLS_DIR = $(HOME)/bin

all: pre_config $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(CFLAGS) $(LDFALGS)

install:
	install $(CURR_DIR)/$(EXEC) $(TOOLS_DIR)

pre_config:
	if [ ! -d $(EXPAT_DIR) ]; then \
		tar -jxf expat-2.2.6.tar.bz2; \
		cd $(EXPAT_DIR) && ./configure --prefix=$(EXPAT_DIR)/INSTALL --exec-prefix=$(EXPAT_DIR)/INSTALL && make && make install; \
	fi

clean:
	rm -rf $(OBJS) $(EXEC)

distclean: clean
	rm -rf $(EXPAT_DIR)
