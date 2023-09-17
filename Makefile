
default: mac

# Debian / Ubuntu
linux:
	gcc main.c -o game -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Raspberry PI
rpi:
	gcc main.c -o game -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -latomic

# Mac OS
mac:
	clang -Iinclude -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL libraylib.a main.c -o game 
