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
case7:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-7.bin
	make all
	make run < 7.in > 7meu.out
	diff 7meu.out ./Casos/7.out
case7out:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-7.bin
	make all
	make run < 7.in
comp:
	hexdump -Cv binario-7.bin > 1.txt
	hexdump -Cv binario-7-gaba.bin > 2.txt
	meld 1.txt 2.txt
