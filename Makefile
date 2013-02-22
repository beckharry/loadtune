all: loadtune.c
	gcc -o loadtune-daemon -g loadtune.c -lpthread
install:
	@cp loadtune-daemon /bin/
	@cp loadtune /bin/
	@chmod +x /bin/loadtune
	@echo "Install loadtune successfully."
