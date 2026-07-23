# Freehand Infrastructure Tools — File Change Map

This document describes every file touched by the freehand road / rail / canal feature in this OpenTTD fork, why it was changed, and how the new modules interact.

**Scope (current working tree vs upstream `master`):**

| Category | Count |
|----------|------:|
| Modified existing OpenTTD files | 15 |
| New source files | 10 |
| Lines added in existing files | +131 |
| Lines removed in existing files | −16 |
| Approximate size of new modules | ~1035 lines |

The design goal is **minimal impact on vanilla GUI files**: existing toolbars keep thin hooks that call into `freehand_gui.*`, while geometry, bit/track derivation, and test/build logic live in dedicated modules.

---

## Architecture overview

```text
┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│   road_gui.cpp   │   │   rail_gui.cpp   │   │   dock_gui.cpp   │
│ (thin hooks)     │   │ (thin hooks)     │   │ (thin hooks)     │
└────────┬─────────┘   └────────┬─────────┘   └────────┬─────────┘
         │                      │                      │
         └──────────────────────┼──────────────────────┘
                                ▼
                       ┌─────────────────┐
                       │ freehand_gui.*  │  stroke state, preview,
                       │                 │  mouse start/drag/up/abort,
                       │                 │  Command::Post commit
                       └────────┬────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         ▼                      ▼                      ▼
 ┌───────────────┐     ┌───────────────┐     ┌────────────────┐
 │road_freehand.*│     │rail_freehand.*│     │canal_freehand.*│
 │ RoadBits +    │     │ Track +       │     │ Canal tile     │
 │ TestBuild /   │     │ TestBuild /   │     │ TestBuild /    │
 │ TestRemove    │     │ TestRemove    │     │ TestRemove     │
 └───────┬───────┘     └───────┬───────┘     └────────┬───────┘
         │                     │                      │
         └─────────────────────┼──────────────────────┘
                               ▼
                      ┌─────────────────┐
                      │ freehand_path.* │  orthogonal path geometry,
                      │                 │  backtrack, axis segments
                      └─────────────────┘

 ┌─────────────────┐
 │  viewport.cpp   │──► GetFreehandPreviewHighlight() from freehand_gui
 └─────────────────┘
```

**Data flow for one stroke**

1. Toolbar (`road_gui` / `rail_gui` / `dock_gui`) detects place / drag / mouse-up / abort and calls the matching `Freehand*Place*` helper.
2. `freehand_gui` owns the active path instance and remove-mode flags; it updates geometry via `Start` / `ExtendTo`.
3. Type helpers (`road_freehand`, `rail_freehand`, `canal_freehand`) derive pieces and run dry-run `Command::Do` tests for preview validity.
4. On mouse-up, `freehand_gui` posts real construction/removal commands (`BuildRoad`, `RemoveRoadLong`, `BuildRail`, `RemoveRail`, `BuildCanal`, `LandscapeClear`).
5. `viewport.cpp` paints highlight rectangles for tiles reported by `GetFreehandPreviewHighlight`.

---

## New files

### Shared geometry

| File | Role |
|------|------|
| [`src/freehand_path.h`](src/freehand_path.h) | Declares `FreehandTilePath`: ordered orthogonal tile list, obstruction flag, `Start` / `ExtendTo` / `Contains` / `ForEachAxisSegment`. |
| [`src/freehand_path.cpp`](src/freehand_path.cpp) | Implements path editing: insert orthogonal intermediates on cursor jumps; **backtrack** when the cursor returns onto a tile already in the path; coalesce consecutive tiles into axis-aligned segments. |

No transport knowledge lives here — only geometry.

### Type-specific helpers (GUI-independent)

| File | Role |
|------|------|
| [`src/road_freehand.h`](src/road_freehand.h) / [`.cpp`](src/road_freehand.cpp) | `FreehandRoadPath`: derive `RoadBits` per index (straight, corner, end stub); `TestBuild` via `Commands::BuildRoad`; `TestRemove` via coalesced `Commands::RemoveRoadLong` segments. |
| [`src/rail_freehand.h`](src/rail_freehand.h) / [`.cpp`](src/rail_freehand.cpp) | `FreehandRailPath`: derive `Track` per index; `TestBuild` / `TestRemove` via `Commands::BuildRail` / `Commands::RemoveRail`. |
| [`src/canal_freehand.h`](src/canal_freehand.h) / [`.cpp`](src/canal_freehand.cpp) | `FreehandCanalPath`: `TestBuild` via `Commands::BuildCanal`; `TestRemove` via `Commands::LandscapeClear` on canal tiles. |

These modules are intentionally free of window / toolbar code so they can be reused or tested without the GUI.

### GUI façade

| File | Role |
|------|------|
| [`src/freehand_gui.h`](src/freehand_gui.h) | Public API used by toolbars and the viewport: `FreehandInfrastructureEnabled`, `GetFreehandPreviewHighlight`, and `FreehandRoad/Rail/CanalPlace{Start,Drag,MouseUp,Abort}`. |
| [`src/freehand_gui.cpp`](src/freehand_gui.cpp) | Holds static stroke state; dirties preview tiles; starts viewport drag with `DDSP_PLACE_FREEHAND_*`; commits on mouse-up; shows errors via `ShowErrorMessage` + `GetEncodedString`. |

**Interaction summary**

- Toolbars → **only** `freehand_gui.h`.
- `freehand_gui` → `road_freehand` / `rail_freehand` / `canal_freehand` + existing command headers.
- Type helpers → `freehand_path` + existing `*_cmd` APIs.
- Viewport → `GetFreehandPreviewHighlight` only.

---

## Modified existing files

### Build system

| File | Why |
|------|-----|
| [`src/CMakeLists.txt`](src/CMakeLists.txt) | Registers the ten new freehand sources so they compile into `openttd_lib`. |

### Settings (toggle)

| File | Why |
|------|-----|
| [`src/table/settings/gui_settings.ini`](src/table/settings/gui_settings.ini) | Defines client setting `gui.freehand_infrastructure_tools` (default **on**, not saved / not network-synced). Post-callback resets the current place tool and refreshes build toolbars. |
| [`src/settings_type.h`](src/settings_type.h) | Adds `GUISettings::freehand_infrastructure_tools`. |
| [`src/settings_table.cpp`](src/settings_table.cpp) | Regenerated / linked settings table entry for the new INI setting (one-line touch as part of settings plumbing). |
| [`src/settingentry_gui.cpp`](src/settingentry_gui.cpp) | Exposes the setting under **Interface → Construction** in the settings UI. |

When the setting is **off**: Autoroad / Autorail use classic behaviour; the freehand canal button is hidden.

### Viewport / selection process IDs

| File | Why |
|------|-----|
| [`src/viewport_type.h`](src/viewport_type.h) | Adds `DDSP_PLACE_FREEHAND_ROAD`, `DDSP_PLACE_FREEHAND_RAIL`, `DDSP_PLACE_FREEHAND_CANAL` so drag handling can distinguish freehand strokes from classic Autoroad / Autorail / area canal. |
| [`src/viewport.cpp`](src/viewport.cpp) | During tile painting, draws a selection rectangle for tiles that belong to the active freehand stroke (normal or red palette). Calls `GetFreehandPreviewHighlight` from `freehand_gui.h`. |

### Road toolbar

| File | Why |
|------|-----|
| [`src/road_gui.cpp`](src/road_gui.cpp) | Thin hooks: when freehand is enabled and Autoroad is used for **road** (not tram), start/drag/mouseup/abort call `FreehandRoadPlace*`. Tooltip switches to the freehand string. Tram Autoroad remains classic Autotram. |
| [`src/road_gui.h`](src/road_gui.h) | Minor related declaration / include touch for the toolbar integration. |
| [`src/widgets/road_widget.h`](src/widgets/road_widget.h) | Comment update on `WID_ROT_AUTOROAD` (freehand for road / Autotram for tram). Widget ID unchanged so layout impact stays small. |

### Rail toolbar

| File | Why |
|------|-----|
| [`src/rail_gui.cpp`](src/rail_gui.cpp) | Thin hooks on Autorail when freehand is enabled: `FreehandRailPlace*`, highlight type `HT_RECT` instead of `HT_RAIL`, freehand tooltip. |

### Waterways / docks toolbar

| File | Why |
|------|-----|
| [`src/dock_gui.cpp`](src/dock_gui.cpp) | Adds freehand canal button wiring: show/hide via selection plane from `FreehandInfrastructureEnabled()`; place/drag/mouseup/abort call `FreehandCanalPlace*`. Classic area canal tool is kept. |
| [`src/widgets/dock_widget.h`](src/widgets/dock_widget.h) | Adds `WID_DT_SEL_FREEHAND_CANAL` and `WID_DT_FREEHAND_CANAL` widget IDs (and renumbers subsequent comments for clarity). |

### Strings

| File | Why |
|------|-----|
| [`src/lang/english.txt`](src/lang/english.txt) | Setting name/helptext; tooltips for freehand road, rail, and canal. |
| [`src/lang/swedish.txt`](src/lang/swedish.txt) | Matching Swedish translations for the same strings. |

---

## Behaviour notes (for reviewers)

- **Road / rail**: reuses the existing Autoroad / Autorail toolbar buttons when the setting is on. No new road/rail toolbar buttons.
- **Canal**: adds a dedicated freehand button next to the classic area canal tool; hidden when freehand tools are disabled.
- **Ctrl**: remove mode along the freehand path (road/rail/canal), consistent with other construction tools.
- **Backtracking**: dragging back onto a previous path tile shortens the stroke.
- **Obstruction**: if the path cannot be extended cleanly, the stroke is treated as blocked (red preview / cancel semantics as implemented in the helpers).
- **Commands**: commit uses the same map commands as vanilla tools. Town authority / rating rules for removing town roads therefore behave like classic `RemoveRoadLong` (rating drains over many tiles before refusal).

---

## File checklist

### New (10)

- `src/freehand_path.h`
- `src/freehand_path.cpp`
- `src/freehand_gui.h`
- `src/freehand_gui.cpp`
- `src/road_freehand.h`
- `src/road_freehand.cpp`
- `src/rail_freehand.h`
- `src/rail_freehand.cpp`
- `src/canal_freehand.h`
- `src/canal_freehand.cpp`

### Modified (15)

- `src/CMakeLists.txt`
- `src/dock_gui.cpp`
- `src/lang/english.txt`
- `src/lang/swedish.txt`
- `src/rail_gui.cpp`
- `src/road_gui.cpp`
- `src/road_gui.h`
- `src/settingentry_gui.cpp`
- `src/settings_table.cpp`
- `src/settings_type.h`
- `src/table/settings/gui_settings.ini`
- `src/viewport.cpp`
- `src/viewport_type.h`
- `src/widgets/dock_widget.h`
- `src/widgets/road_widget.h`

---

*Describes the freehand infrastructure feature as committed in this experimental fork.*
