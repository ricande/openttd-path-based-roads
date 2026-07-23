/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file freehand_gui.h Thin GUI hooks for freehand road / rail / canal tools. */

#ifndef FREEHAND_GUI_H
#define FREEHAND_GUI_H

#include "core/geometry_type.hpp"
#include "rail_type.h"
#include "road_type.h"
#include "strings_type.h"
#include "tile_type.h"
#include "viewport_type.h"

/**
 * Unified freehand preview highlight for viewport painting.
 * @return True if \a tile is part of an active freehand stroke.
 */
bool GetFreehandPreviewHighlight(TileIndex tile, bool &draw_red);

/** Whether freehand infrastructure tools are enabled in settings. */
bool FreehandInfrastructureEnabled();

/* Road */
void FreehandRoadPlaceStart(TileIndex tile, RoadType rt, bool remove);
bool FreehandRoadPlaceDrag(ViewportDragDropSelectionProcess select_proc, Point pt, bool remove);
bool FreehandRoadPlaceMouseUp(ViewportDragDropSelectionProcess select_proc, bool remove, RoadType rt, StringID err_build, StringID err_remove);
void FreehandRoadPlaceAbort();

/* Rail */
void FreehandRailPlaceStart(TileIndex tile, RailType rt, bool remove);
bool FreehandRailPlaceDrag(ViewportDragDropSelectionProcess select_proc, Point pt, bool remove);
bool FreehandRailPlaceMouseUp(ViewportDragDropSelectionProcess select_proc, bool remove, RailType rt);
void FreehandRailPlaceAbort();

/* Canal */
void FreehandCanalPlaceStart(TileIndex tile, bool remove);
bool FreehandCanalPlaceDrag(ViewportDragDropSelectionProcess select_proc, Point pt, bool remove);
bool FreehandCanalPlaceMouseUp(ViewportDragDropSelectionProcess select_proc, bool remove);
void FreehandCanalPlaceAbort();

#endif /* FREEHAND_GUI_H */
