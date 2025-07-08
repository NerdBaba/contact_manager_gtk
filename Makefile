CC=gcc
CFLAGS=-I.
LDLIBS=-lreadline
GTK_CFLAGS=$(shell pkg-config --cflags gtk4 libadwaita-1)
GTK_LIBS=$(shell pkg-config --libs gtk4 libadwaita-1)

SRCS_CONTACT_MANAGER_CLI=src/contact_manager_cli.c src/config.c src/database.c
OBJS_CONTACT_MANAGER_CLI=$(SRCS_CONTACT_MANAGER_CLI:.c=.o)

SRCS_GUI=src/gui.c src/config.c src/database.c src/contact_object.c
OBJS_GUI=$(SRCS_GUI:.c=.o)

all: contact_manager_cli contact_manager_gtk

contact_manager_cli: $(OBJS_CONTACT_MANAGER_CLI)
	$(CC) -o contact_manager_cli $(OBJS_CONTACT_MANAGER_CLI) $(LDLIBS)

contact_manager_gtk: $(OBJS_GUI)
	$(CC) -o contact_manager_gtk $(OBJS_GUI) $(GTK_LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $(GTK_CFLAGS) $< -o $@

clean:
	rm -f contact_manager_cli contact_manager_gtk $(OBJS_CONTACT_MANAGER_CLI) $(OBJS_GUI)
