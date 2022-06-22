EE_BIN = bin/tyracraft.elf

TYRA_DIR = $(TYRA)

EE_OBJS =											\
	managers/terrain_manager.o						\
	managers/block_manager.o						\
	managers/state_manager.o						\
	managers/menu_manager.o							\
	managers/items_repository.o						\
	managers/collision_manager.o					\
	objects/World.o									\
	objects/Block.o									\
	objects/chunck.o								\
	objects/player.o								\
	objects/item.o									\
	utils.o											\
	ui.o											\
	camera.o										\
	splash_screen.o									\
	start.o											\
	main.o

EE_LIBS = -ltyra

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
#	mv $(EE_BIN) bin/$(EE_BIN) 
	rm $(EE_OBJS)

sync-assets:
	cp -a assets/* bin/

rebuild-engine:
	cd $(TYRA_DIR) && make clean && make

clean:
	rm -f $(EE_OBJS)

rebuild:
	make clean && make all && make sync-assets

run: $(EE_BIN)
	killall -v ps2client || true
	ps2client reset
	ps2client reset
	$(EE_STRIP) --strip-all $(EE_BIN)
#	mv $(EE_BIN) bin/$(EE_BIN)
	rm $(EE_OBJS)
	cd bin/ && ps2client execee host:$(EE_BIN)

include $(TYRA)/src/engine/Makefile.pref

