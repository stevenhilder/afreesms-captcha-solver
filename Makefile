APP = solver
OBJS = solver.o main.o

.DEFAULT_GOAL = all
.PHONY: all
all: $(APP)

$(APP): $(OBJS)
	$(CC) -o '$@' $$(pkg-config --libs MagickCore-7.Q16HDRI) $^

%.o: src/%.c
	$(CC) -std=c99 -O3 -o '$@' -I include $$(pkg-config --cflags MagickCore-7.Q16HDRI) -c '$<'

.PHONY: clean
clean:
	rm -f *.o '$(APP)'

.PHONY: test
test: tests/test.sh | all
	@'$<'
