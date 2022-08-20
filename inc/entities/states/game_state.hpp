#include "../../managers/state_manager.hpp"

class GameState {
 public:
  GameState(StateManager* context);
  ~GameState();
  virtual void init() = 0;
  virtual void update() = 0;
  virtual void renderer() = 0;
};