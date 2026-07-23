/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file freehand_gui.cpp Thin GUI hooks for freehand road / rail / canal tools. */

#include "stdafx.h"

#include "freehand_gui.h"

#include "canal_freehand.h"
#include "command_func.h"
#include "dock_cmd.h"
#include "error.h"
#include "landscape_cmd.h"
#include "map_func.h"
#include "rail_cmd.h"
#include "rail_freehand.h"
#include "road_cmd.h"
#include "road_freehand.h"
#include "settings_type.h"
#include "strings_func.h"
#include "table/strings.h"
#include "tile_map.h"
#include "tilehighlight_func.h"
#include "town_type.h"
#include "track_func.h"
#include "viewport_func.h"
#include "water_cmd.h"
#include "water_map.h"

#include "safeguards.h"

static FreehandRoadPath _freehand_road_path;
static FreehandRailPath _freehand_rail_path;
static FreehandCanalPath _freehand_canal_path;

static std::vector<TileIndex> _freehand_preview_dirty;
static bool _freehand_road_remove = false;
static bool _freehand_rail_remove = false;
static bool _freehand_canal_remove = false;
static RoadType _freehand_roadtype = INVALID_ROADTYPE;
static RailType _freehand_railtype = INVALID_RAILTYPE;

bool FreehandInfrastructureEnabled()
{
	return _settings_client.gui.freehand_infrastructure_tools;
}

static void DirtyPreviewTiles(const std::vector<TileIndex> &tiles)
{
	for (TileIndex tile : _freehand_preview_dirty) {
		if (IsValidTile(tile)) MarkTileDirtyByTile(tile);
	}
	_freehand_preview_dirty = tiles;
	for (TileIndex tile : _freehand_preview_dirty) {
		if (IsValidTile(tile)) MarkTileDirtyByTile(tile);
	}
}

static void RefreshRoadPreview()
{
	DirtyPreviewTiles(_freehand_road_path.tiles);
	SetSelectionRed(_freehand_road_remove || _freehand_road_path.ShouldHighlightInvalid(_freehand_roadtype, _freehand_road_remove));
}

static void RefreshRailPreview()
{
	DirtyPreviewTiles(_freehand_rail_path.tiles);
	SetSelectionRed(_freehand_rail_remove || _freehand_rail_path.ShouldHighlightInvalid(_freehand_railtype, _freehand_rail_remove));
}

static void RefreshCanalPreview()
{
	DirtyPreviewTiles(_freehand_canal_path.tiles);
	SetSelectionRed(_freehand_canal_remove || _freehand_canal_path.ShouldHighlightInvalid(WaterClass::Canal, _freehand_canal_remove));
}

bool GetFreehandPreviewHighlight(TileIndex tile, bool &draw_red)
{
	if (!_freehand_road_path.IsEmpty() && _freehand_road_path.Contains(tile)) {
		draw_red = _freehand_road_remove || _freehand_road_path.ShouldHighlightInvalid(_freehand_roadtype, _freehand_road_remove);
		return true;
	}
	if (!_freehand_rail_path.IsEmpty() && _freehand_rail_path.Contains(tile)) {
		draw_red = _freehand_rail_remove || _freehand_rail_path.ShouldHighlightInvalid(_freehand_railtype, _freehand_rail_remove);
		return true;
	}
	if (!_freehand_canal_path.IsEmpty() && _freehand_canal_path.Contains(tile)) {
		draw_red = _freehand_canal_remove || _freehand_canal_path.ShouldHighlightInvalid(WaterClass::Canal, _freehand_canal_remove);
		return true;
	}
	return false;
}

void FreehandRoadPlaceStart(TileIndex tile, RoadType rt, bool remove)
{
	_freehand_road_remove = remove;
	_freehand_roadtype = rt;
	_freehand_road_path.Start(tile);
	RefreshRoadPreview();
	VpStartDragging(DDSP_PLACE_FREEHAND_ROAD);
}

bool FreehandRoadPlaceDrag(ViewportDragDropSelectionProcess select_proc, Point pt, bool remove)
{
	if (select_proc != DDSP_PLACE_FREEHAND_ROAD) return false;
	_freehand_road_remove = remove;
	_freehand_road_path.ExtendTo(TileVirtXY(pt.x, pt.y));
	RefreshRoadPreview();
	return true;
}

bool FreehandRoadPlaceMouseUp(ViewportDragDropSelectionProcess select_proc, bool remove, RoadType rt, StringID err_build, StringID err_remove)
{
	if (select_proc != DDSP_PLACE_FREEHAND_ROAD) return false;

	_freehand_road_remove = remove;
	_freehand_roadtype = rt;
	const CommandCost test = _freehand_road_path.Test(rt, remove);
	if (test.Succeeded()) {
		if (remove) {
			std::vector<std::tuple<TileIndex, TileIndex, Axis>> segments;
			_freehand_road_path.ForEachRemoveSegment([&](TileIndex start, TileIndex end, Axis axis) {
				segments.emplace_back(start, end, axis);
			});
			for (size_t i = 0; i < segments.size(); i++) {
				const auto &[start, end, axis] = segments[i];
				if (i + 1 == segments.size()) {
					Command<Commands::RemoveRoadLong>::Post(err_remove, CcPlaySound_CONSTRUCTION_OTHER, end, start, rt, axis, false, false);
				} else {
					Command<Commands::RemoveRoadLong>::Post(end, start, rt, axis, false, false);
				}
			}
		} else {
			const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRoad>());
			std::vector<std::pair<TileIndex, RoadBits>> to_build;
			const size_t n = _freehand_road_path.Length();
			for (size_t i = 0; i < n; i++) {
				const RoadBits bits = _freehand_road_path.BitsForIndex(i);
				if (bits.None()) continue;
				const TileIndex tile = _freehand_road_path.tiles[i];
				CommandCost ret = Command<Commands::BuildRoad>::Do(flags, tile, bits, rt, {}, TownID::Invalid());
				if (ret.Failed()) {
					if (ret.GetErrorMessage() == STR_ERROR_ALREADY_BUILT) continue;
					break;
				}
				to_build.emplace_back(tile, bits);
			}
			for (size_t i = 0; i < to_build.size(); i++) {
				const auto &[tile, bits] = to_build[i];
				if (i + 1 == to_build.size()) {
					Command<Commands::BuildRoad>::Post(err_build, CcPlaySound_CONSTRUCTION_OTHER, tile, bits, rt, {}, TownID::Invalid());
				} else {
					Command<Commands::BuildRoad>::Post(tile, bits, rt, {}, TownID::Invalid());
				}
			}
		}
	} else if (!_freehand_road_path.IsEmpty()) {
		CommandCost error = test;
		ShowErrorMessage(GetEncodedString(remove ? err_remove : err_build), 0, 0, error);
	}

	_freehand_road_path.Clear();
	RefreshRoadPreview();
	_freehand_preview_dirty.clear();
	SetSelectionRed(remove);
	return true;
}

void FreehandRoadPlaceAbort()
{
	_freehand_road_path.Clear();
	RefreshRoadPreview();
	_freehand_preview_dirty.clear();
	_freehand_road_remove = false;
	SetSelectionRed(false);
}

void FreehandRailPlaceStart(TileIndex tile, RailType rt, bool remove)
{
	_freehand_rail_remove = remove;
	_freehand_railtype = rt;
	_freehand_rail_path.Start(tile);
	RefreshRailPreview();
	VpStartDragging(DDSP_PLACE_FREEHAND_RAIL);
}

bool FreehandRailPlaceDrag(ViewportDragDropSelectionProcess select_proc, Point pt, bool remove)
{
	if (select_proc != DDSP_PLACE_FREEHAND_RAIL) return false;
	_freehand_rail_remove = remove;
	_freehand_rail_path.ExtendTo(TileVirtXY(pt.x, pt.y));
	RefreshRailPreview();
	return true;
}

bool FreehandRailPlaceMouseUp(ViewportDragDropSelectionProcess select_proc, bool remove, RailType rt)
{
	if (select_proc != DDSP_PLACE_FREEHAND_RAIL) return false;

	_freehand_rail_remove = remove;
	_freehand_railtype = rt;
	const CommandCost test = _freehand_rail_path.Test(rt, remove);
	if (test.Succeeded()) {
		if (remove) {
			std::vector<std::pair<TileIndex, Track>> to_remove;
			const size_t n = _freehand_rail_path.Length();
			for (size_t i = 0; i < n; i++) {
				const Track track = _freehand_rail_path.TrackForIndex(i);
				if (!IsValidTrack(track)) continue;
				to_remove.emplace_back(_freehand_rail_path.tiles[i], track);
			}
			for (size_t i = 0; i < to_remove.size(); i++) {
				const auto &[tile, track] = to_remove[i];
				if (i + 1 == to_remove.size()) {
					Command<Commands::RemoveRail>::Post(STR_ERROR_CAN_T_REMOVE_RAILROAD_TRACK, CcPlaySound_CONSTRUCTION_RAIL, tile, track);
				} else {
					Command<Commands::RemoveRail>::Post(tile, track);
				}
			}
		} else {
			const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildRail>());
			const bool auto_remove_signals = _settings_client.gui.auto_remove_signals;
			std::vector<std::pair<TileIndex, Track>> to_build;
			const size_t n = _freehand_rail_path.Length();
			for (size_t i = 0; i < n; i++) {
				const Track track = _freehand_rail_path.TrackForIndex(i);
				if (!IsValidTrack(track)) continue;
				const TileIndex tile = _freehand_rail_path.tiles[i];
				CommandCost ret = Command<Commands::BuildRail>::Do(flags, tile, rt, track, auto_remove_signals);
				if (ret.Failed()) {
					if (ret.GetErrorMessage() == STR_ERROR_ALREADY_BUILT) continue;
					break;
				}
				to_build.emplace_back(tile, track);
			}
			for (size_t i = 0; i < to_build.size(); i++) {
				const auto &[tile, track] = to_build[i];
				if (i + 1 == to_build.size()) {
					Command<Commands::BuildRail>::Post(STR_ERROR_CAN_T_BUILD_RAILROAD_TRACK, CcPlaySound_CONSTRUCTION_RAIL, tile, rt, track, auto_remove_signals);
				} else {
					Command<Commands::BuildRail>::Post(tile, rt, track, auto_remove_signals);
				}
			}
		}
	} else if (!_freehand_rail_path.IsEmpty()) {
		CommandCost error = test;
		ShowErrorMessage(GetEncodedString(remove ? STR_ERROR_CAN_T_REMOVE_RAILROAD_TRACK : STR_ERROR_CAN_T_BUILD_RAILROAD_TRACK), 0, 0, error);
	}

	_freehand_rail_path.Clear();
	RefreshRailPreview();
	_freehand_preview_dirty.clear();
	SetSelectionRed(remove);
	return true;
}

void FreehandRailPlaceAbort()
{
	_freehand_rail_path.Clear();
	RefreshRailPreview();
	_freehand_preview_dirty.clear();
	_freehand_rail_remove = false;
	SetSelectionRed(false);
}

void FreehandCanalPlaceStart(TileIndex tile, bool remove)
{
	_freehand_canal_remove = remove;
	_freehand_canal_path.Start(tile);
	RefreshCanalPreview();
	VpStartDragging(DDSP_PLACE_FREEHAND_CANAL);
}

bool FreehandCanalPlaceDrag(ViewportDragDropSelectionProcess select_proc, Point pt, bool remove)
{
	if (select_proc != DDSP_PLACE_FREEHAND_CANAL) return false;
	_freehand_canal_remove = remove;
	_freehand_canal_path.ExtendTo(TileVirtXY(pt.x, pt.y));
	RefreshCanalPreview();
	return true;
}

bool FreehandCanalPlaceMouseUp(ViewportDragDropSelectionProcess select_proc, bool remove)
{
	if (select_proc != DDSP_PLACE_FREEHAND_CANAL) return false;

	_freehand_canal_remove = remove;
	const WaterClass wc = WaterClass::Canal;
	const CommandCost test = _freehand_canal_path.Test(wc, remove);
	if (test.Succeeded()) {
		if (remove) {
			std::vector<TileIndex> to_clear;
			for (TileIndex tile : _freehand_canal_path.tiles) {
				if (!IsWaterTile(tile) || !IsCanal(tile)) continue;
				to_clear.push_back(tile);
			}
			for (size_t i = 0; i < to_clear.size(); i++) {
				if (i + 1 == to_clear.size()) {
					Command<Commands::LandscapeClear>::Post(STR_ERROR_CAN_T_CLEAR_THIS_AREA, CcPlaySound_CONSTRUCTION_WATER, to_clear[i]);
				} else {
					Command<Commands::LandscapeClear>::Post(to_clear[i]);
				}
			}
		} else {
			const DoCommandFlags flags = CommandFlagsToDCFlags(GetCommandFlags<Commands::BuildCanal>());
			std::vector<TileIndex> to_build;
			for (TileIndex tile : _freehand_canal_path.tiles) {
				CommandCost ret = Command<Commands::BuildCanal>::Do(flags, tile, tile, wc, false);
				if (ret.Failed()) {
					if (ret.GetErrorMessage() == STR_ERROR_ALREADY_BUILT) continue;
					break;
				}
				to_build.push_back(tile);
			}
			for (size_t i = 0; i < to_build.size(); i++) {
				if (i + 1 == to_build.size()) {
					Command<Commands::BuildCanal>::Post(STR_ERROR_CAN_T_BUILD_CANALS, CcPlaySound_CONSTRUCTION_WATER, to_build[i], to_build[i], wc, false);
				} else {
					Command<Commands::BuildCanal>::Post(to_build[i], to_build[i], wc, false);
				}
			}
		}
	} else if (!_freehand_canal_path.IsEmpty()) {
		CommandCost error = test;
		ShowErrorMessage(GetEncodedString(remove ? STR_ERROR_CAN_T_CLEAR_THIS_AREA : STR_ERROR_CAN_T_BUILD_CANALS), 0, 0, error);
	}

	_freehand_canal_path.Clear();
	RefreshCanalPreview();
	_freehand_preview_dirty.clear();
	_freehand_canal_remove = false;
	SetSelectionRed(false);
	return true;
}

void FreehandCanalPlaceAbort()
{
	_freehand_canal_path.Clear();
	RefreshCanalPreview();
	_freehand_preview_dirty.clear();
	_freehand_canal_remove = false;
	SetSelectionRed(false);
}
