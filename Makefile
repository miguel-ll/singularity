default:
	gcc -o singularity --std=c99 main.c stroke.c -lraylib
install:
	gcc -o /usr/local/bin/singularity --std=c99 main.c stroke.c -lraylib
uninstall:
	rm /usr/local/bin/singularity
