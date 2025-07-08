# contact_manager_gtk

`contact_manager_gtk` is a project that contains two main components:

1.  `contact_manager_cli`: A command-line key-value store.
2.  `contact_manager_gtk`: A GTK+3-based graphical user interface for managing contacts.

## Dependencies

To build and run the `contact_manager_gtk` application, you will need the following dependency:

*   GTK+3

On Debian-based systems, you can install it with:

```bash
sudo apt-get install libgtk-3-dev
```

## Building the Project

To build both the command-line tool and the GUI application, you can use the provided `Makefile`:

```bash
make
```

This will create two executables in the root directory: `contact_manager_cli` and `contact_manager_gtk`.

To build only the GUI application, you can run:

```bash
make contact_manager_gtk
```

## Running the Applications

### contact_manager_cli (Command-Line)

```bash
./contact_manager_cli
```

### contact_manager_gtk (Graphical User Interface)

To run the contact manager GUI:

```bash
./contact_manager_gtk
```

## Cleaning Up

To remove the compiled object files and executables:

```bash
make clean
```