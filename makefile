parser:
	gcc parser.c -g -std=gnu99 -o parser

clean:
	rm -rf parser.o parser
