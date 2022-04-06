EE_BIN = tyracraft.elf

EE_OBJS =											\
	3libs/SimplexNoise.o							\
	managers/terrain_manager.o						\
	managers/block_manager.o						\
	objects/World.o									\
	objects/Block.o									\
	objects/chunck.o								\
	objects/player.o								\
	utils.o											\
	ui.o											\
	camera.o										\
	start.o											\
	main.o

EE_LIBS = -ltyra

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
	mkdir bin/
	mv $(EE_BIN) bin/$(EE_BIN) 
	cp -a assets/* bin/ 

rebuild-engine:
	cd $(TYRA) && make all

clean:
	rm -fr bin/ $(EE_OBJS)

rebuild: rebuild-engine all 

run: $(EE_BIN)
	killall -v ps2client || true
	ps2client reset
	ps2client reset
	$(EE_STRIP) --strip-all $(EE_BIN)
	mkdir -p bin/
	mv $(EE_BIN) bin/$(EE_BIN)
	cp -a assets/* bin/
	rm -fr $(EE_OBJS) 
	cd bin/ && ps2client execee host:$(EE_BIN)

include $(TYRA)/src/engine/Makefile.pref

