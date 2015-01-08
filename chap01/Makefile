LIBS = lex.o name.o
MAIN = main.o
PLAIN = plain.o
IMPROVED = improved.o
RETVAL = retval.o
ARGS = args.o
EXES = plain improved retval args

all: plain improved retval args

%.o:%.c
	gcc -c $<

plain: ${LIBS} ${PLAIN} ${MAIN}
	gcc -o $@ $^

improved: ${LIBS} ${MAIN} ${IMPROVED}
	gcc -o $@ $^

retval: ${LIBS} ${MAIN} ${RETVAL}
	gcc -o $@ $^

args: ${LIBS} ${MAIN} ${ARGS}
	gcc -o $@ $^

.PHONY: clean
clean:
	rm ${LIBS} ${MAIN} ${IMPROVED} ${RETVAL} ${PLAIN} ${ARGS}

.PHONY: clean-exes
clean-exes:
	rm ${EXES}
