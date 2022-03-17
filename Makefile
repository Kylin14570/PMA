#obj = build/main.o build/cache.o build/mmap.o build/OffsetPointerConvert.o build/PMalloc.o build/root.o build/sizeclass.o build/SuperBlock.o build/thread.o
#main: $(obj)
#	gcc $(obj) -o build/main -lpmem
#build/%.o: src/%.c src/def.h
#	gcc -c $< -o $@
#clean:
#	rm -rf $(obj) build/main
#run:
#	make
#	build/main

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c, obj/%.o, $(src))
main: $(obj)
	gcc $(obj) -o main -lpmem -lpthread
obj/%.o: src/%.c
	gcc -c $< -o $@
clean:
	rm -rf $(obj) main
#run:
#	make
#	./main 16 100 100