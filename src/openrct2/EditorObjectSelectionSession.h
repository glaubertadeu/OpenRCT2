/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "common.h"
#include "object/Object.h"

#include <vector>

enum EDITOR_INPUT_FLAGS
{
    INPUT_FLAG_EDITOR_OBJECT_1 = (1 << 1),
    INPUT_FLAG_EDITOR_OBJECT_2 = (1 << 2),
    INPUT_FLAG_EDITOR_OBJECT_ALWAYS_REQUIRED = (1 << 3)
};

extern bool _maxObjectsWasHit;
extern std::vector<uint8_t> _objectSelectionFlags;
extern int32_t _numSelectedObjectsForType[OBJECT_TYPE_COUNT];

bool editor_check_object_group_at_least_one_selected(int32_t objectType);
void editor_object_flags_free();
void unload_unselected_objects();
void sub_6AB211();
void reset_selected_object_count_and_size();
void finish_object_selection();
int32_t window_editor_object_selection_select_object(uint8_t bh, int32_t flags, const rct_object_entry* entry);

/**
 * Removes all unused objects from the object selection.
 * @return The number of removed objects.
 */
int32_t editor_remove_unused_objects();
