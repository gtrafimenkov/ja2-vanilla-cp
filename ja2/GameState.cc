// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "GameState.h"

/** Get instance of the object. */
GameState *GameState::getInstance() {
  static GameState instance;
  return &instance;
}

/** Get current game mode. */
GameMode GameState::getMode() { return m_mode; }

/** Check if we are in the editor mode. */
bool GameState::isEditorMode() {
  return (m_mode == GAME_MODE_EDITOR) || (m_mode == GAME_MODE_EDITOR_AUTO);
}

/** Set editor mode. */
void GameState::setEditorMode(bool autoLoad) {
  m_mode = autoLoad ? GAME_MODE_EDITOR_AUTO : GAME_MODE_EDITOR;
}

/** Private constructor to avoid instantiation. */
GameState::GameState() : m_mode(GAME_MODE_GAME) {}
