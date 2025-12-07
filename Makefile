all: notepad

notepad: notepad.cpp
    g++ notepad.cpp -o notepad `pkg-config --cflags --libs gtkmm-3.0`

clean:
    rm -f notepad
