# 3-Week Sprint Roadmap (Weeks 7-9)

Based on our recent implementations (Parry & Hitstop, Team Switching, Punchy Gun Mechanics, UI/HUD Refactor, CSV Tilemaps), this roadmap outlines the trajectory for the next three weeks of development to bring the vertical slice to completion.

## Week 7: Content Completion & Boss Logic
**Focus:** Fleshing out the missing core components of the ZZZ/Voltron combat loop.
- **[Feature] The 3rd Paladin:** Replace `PlaceholderPaladin` with a fully realized 3rd team member (e.g., Pidge or Shiro). Implement their unique weapon static sprite, attack strategy, and stats.
- **[Feature] Ultimate (Decibel) System:** We have the TeamManager tracking a global team ultimate pool. We need to implement the visual UI for the Decibel meter and a devastating Ultimate Attack state that pauses the game and unleashes a massive AoE when triggered.
- **[Feature] Multi-Phase Boss:** Build upon the AI to create a definitive end-of-wave Boss that utilizes telegraphs, requires parrying to create openings (synergizing with the hitstop mechanic), and has multiple phases.

## Week 8: Polish, Juice, & Flow
**Focus:** Enhancing game feel (Juice) and connecting the isolated systems into a seamless experience.
- **[System] Particle Engine:** Implement a lightweight particle manager for dash trails, parry sparks, and projectile impacts to complement the hitstop camera zoom.
- **[System] Menu & Flow:** Implement `GameState::MAIN_MENU`, `GameState::PAUSE`, and `GameState::GAME_OVER`. Add UI buttons and transitions to allow the player to start, pause, restart, or quit without closing the executable.
- **[Audio] Settings & Polish:** Hook up the UI sliders to adjust `AudioManager` music and SFX volumes dynamically. Add specialized SFX for the newly added parry and ultimate systems.

## Week 9: Meta-Progression & Expansion
**Focus:** Content scaling and replayability.
- **[Content] Level Expansion:** Utilize the existing CSV Tilemap parser to construct Level 2 and Level 3 maps.
- **[System] Roguelite Upgrades:** Implement a simple end-of-wave shop or upgrade prompt where players can spend currency (or accumulated score) to increase `maxHealth`, `maxExEnergy`, or `speed` for the team.
- **[QA] Balancing & Bug Squashing:** Extensive playtesting to balance enemy HP, parry windows, and weapon damage across the 3 Paladins. Fix lingering memory leaks or rendering layer bugs.

## User Review Required
> [!IMPORTANT]
> - Which Paladin do you want to prioritize replacing the `PlaceholderPaladin` with in Week 7?
> - For the Boss Battle, do you want a Galra Commander (humanoid, uses weapons) or a Robeast (giant monster, AoE attacks)?
