# Implementation Plan: Rendering Layers & Transparency Fix

## Goal Description
Resolve a graphical bug where transparent regions of solid wall tiles were erasing the floor layers beneath them, causing the map to render black voids in the corners of rooms.

## Proposed Changes

### `src/Core/Manager/LevelManager.cpp`
- [MODIFY] Refactor the map rendering loops in `DrawLevel()`.
- [MODIFY] Separate the drawing process into distinct Z-index layers: background/floor layer (Layer 0), interactive entities (Layer 1), and foreground walls/decorations (Layer 2).
- [MODIFY] Ensure that ID 0 (Void space) properly renders the background texture before any wall tiles with alpha-transparency are rendered on top of it.

## Verification Plan
1. **Manual Verification:** Walk into the corners of the Hub map and the Battle map. Verify that the transparent edges of the "Wall Left" and "Wall Right" tiles correctly display the underlying floor tiles instead of cutting through to the background clear color.
