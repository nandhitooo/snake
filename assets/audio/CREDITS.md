# Audio credits

All audio shipped in this folder is **CC0 1.0 (public domain)**. License was
checked on the source page before downloading; nothing here is converted or
edited, files are used exactly as downloaded.

## Music (`assets/audio/music/`)

| File | Original | Source (CC0) | Author |
|---|---|---|---|
| `menu.ogg` | `menumusicloop-tiggo.ogg` | https://opengameart.org/node/174263 — "Two Simple Game Music Loops" | qubodup |
| `gameplay.ogg` | `empacotatron_loop.ogg` | https://opengameart.org/node/180427 — "Empacotatron" | Fupi |

## Sound effects (`assets/audio/sfx/`)

Interface sounds are WAV from Kenney's *Interface Sounds* pack
(https://github.com/Calinou/kenney-interface-sounds, CC0).
Digital sounds are from Kenney's *63 Digital Sound Effects* pack
(https://opengameart.org/content/63-digital-sound-effects-lasers-phasers-space-etc, CC0).

| File | Original | Pack | Author | License |
|---|---|---|---|---|
| `menu_hover.wav` | `select_002.wav` | Interface Sounds | Kenney | CC0 |
| `menu_click.wav` | `click_001.wav` | Interface Sounds | Kenney | CC0 |
| `menu_back.wav` | `back_001.wav` | Interface Sounds | Kenney | CC0 |
| `obstacle_warning.wav` | `bong_001.wav` | Interface Sounds | Kenney | CC0 |
| `pause.wav` | `toggle_001.wav` | Interface Sounds | Kenney | CC0 |
| `resume.wav` | `toggle_002.wav` | Interface Sounds | Kenney | CC0 |
| `eat_food.mp3` | `pepSound1.mp3` | 63 Digital SFX | Kenney | CC0 |
| `special_good.mp3` | `pepSound4.mp3` | 63 Digital SFX | Kenney | CC0 |
| `special_bad.mp3` | `lowDown.mp3` | 63 Digital SFX | Kenney | CC0 |
| `boost_start.mp3` | `phaserUp2.mp3` | 63 Digital SFX | Kenney | CC0 |
| `boost_end.mp3` | `phaserDown2.mp3` | 63 Digital SFX | Kenney | CC0 |
| `obstacle_spawn.mp3` | `lowRandom.mp3` | 63 Digital SFX | Kenney | CC0 |
| `ai_spawn.mp3` | `phaseJump1.mp3` | 63 Digital SFX | Kenney | CC0 |
| `eat_ai_snake.mp3` | `pepSound5.mp3` | 63 Digital SFX | Kenney | CC0 |
| `ai_die.mp3` | `zap1.mp3` | 63 Digital SFX | Kenney | CC0 |
| `game_over.mp3` | `lowThreeTone.mp3` | 63 Digital SFX | Kenney | CC0 |

## Notes

- The two GitHub mirrors for Kenney's digital/impact packs listed in the task
  return 404, so the digital sounds come from the documented OpenGameArt
  fallback. That fallback only ships MP3; conversion was explicitly out of
  scope, so those files stay MP3 (raylib streams both WAV and MP3).
- No impact-sound pack was available, so `obstacle_spawn.mp3` uses a short,
  low digital hit instead of an impact sample.
