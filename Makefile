build = bin
src = src
obj = $(build)/obj

exec = $(build)/halfpint

source = $(shell find $(src) -name *.c)
objects = $(patsubst $(src)/%.c, $(obj)/%.o, $(source))

cflags =
lflags = -g -fsanitize=address

$(exec): $(objects)
	gcc $(lflags) -o $@ $^

$(obj)/%.o: $(src)/%.c mkdirs
	gcc -c $(cflags) -o $@ $<

mkdirs:
	-mkdir -p $(build)
	-mkdir -p $(obj)

clean:
	rm -rf $(build)

