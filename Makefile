all:
	gcc -o programaTrab2 main.c escreverTela.c -g
run:
	./programaTrab2
debug:
	gdb programaTrab2
val:
	valgrind --leak-check=full --track-origins=yes ./programaTrab2
zip:
	zip trab2.zip main.c escreverTela.c escreverTela.h Makefile
backup:
	cp main.c main_backup.c
bin7:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-7.bin
	make all
case1:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-1.bin
	make all
	make run < 1.in > 1meu.out
	diff 1meu.out ./Casos/1.out
case11:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-11.bin
	make all
	make run < 11.in > 11meu.out
	diff 11meu.out ./Casos/11.out
make meld:
	hexdump -Cv binario-1.bin > 1.txt
	hexdump -Cv binario-1-depois.bin > 2.txt
	meld 1.txt 2.txt
