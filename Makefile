all: process.c mmu.c master.c scheduler.c
	gcc master.c -o master -lm
	gcc scheduler.c -o scheduler -lm
	gcc mmu.c -o mmu -lm
	gcc process.c -o process -lm

clean:
	rm process master scheduler mmu