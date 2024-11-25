// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

enum GameMode { GAME_MODE_GAME, GAME_MODE_EDITOR, GAME_MODE_EDITOR_AUTO, GAME_MODE_END };

/** Global game state (singleton). */
class GameState {
 public:
  /** Get instance of the object. */
  static GameState *getInstance();

  /** Get current game mode. */
  GameMode getMode();

  /** Set editor mode. */
  void setEditorMode(bool autoLoad);

  /** Check if we are in the editor mode. */
  bool isEditorMode();

 private:
  GameMode m_mode;

  /** Private constructor to avoid instantiation. */
  GameState();

  GameState(GameState const &);
  void operator=(GameState const &);
};
