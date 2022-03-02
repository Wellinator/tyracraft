EE_BIN = tyracraft.elf

# TYRA_DIR = ./../../engine #Reads from ENV: $TYRA_DIR

EE_OBJS =											\
	managers/terrain_manager.o						\
	objects/World.o									\
	objects/Block.o									\
	objects/chunck.o								\
	objects/player.o								\
	utils.o											\
	camera.o										\
	start.o											\
	main.o

EE_LIBS = -ltyra 

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
	rm $(EE_OBJS)

rebuild-engine: 
	cd $(TYRA_DIR) && make clean && make

clean:
	rm -f $(EE_OBJS)

rebuild:
	make clean && make all	

run: $(EE_BIN)
	killall -v ps2client || true
	ps2client reset
	ps2client reset
	$(EE_STRIP) --strip-all $(EE_BIN)
	rm $(EE_OBJS)
	cd bin/ && ps2client execee host:$(EE_BIN)

include $(TYRA_DIR)/Makefile.pref
