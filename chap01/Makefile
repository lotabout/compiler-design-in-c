LIBS = lex.o name.o
MAIN = main.o
PLAIN = plain.o
IMPROVED = improved.o
RETVAL = retval.o
EXES = plain improved retval

all: plain improved retval

%.o:%.c
	gcc -c $<

plain: ${LIBS} ${PLAIN} ${MAIN}
	gcc -o $@ $^

improved: ${LIBS} ${MAIN} ${IMPROVED}
	gcc -o $@ $^

retval: ${LIBS} ${MAIN} ${RETVAL}
	gcc -o $@ $^

.PHONY: clean
clean:
	rm ${LIBS} ${MAIN} ${IMPROVED} ${RETVAL} ${PLAIN}

.PHONY: clean-exes
clean-exes:
	rm ${EXES}
