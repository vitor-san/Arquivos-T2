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
bin8:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-8.bin
	make all
case8:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-8.bin
	make all
	make run < 8.in > 8meu.out
	diff 8meu.out ./Casos/8.out
case8out:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-8.bin
	make all
	make run < 8.in
bin9:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-9.bin
	make all
case9:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-9.bin
	make all
	make run < 9.in > 9meu.out
	diff 9meu.out ./Casos/9.out
case9out:
	cp -t /home/vitorsan/Desktop/Trabalhos-USP/Files/Trabalho2 ./Arquivos-binarios/binario-9.bin
	make all
	make run < 9.in
