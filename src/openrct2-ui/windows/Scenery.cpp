/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <algorithm>
#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2-ui/interface/Viewport.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Context.h>
#include <openrct2/Input.h>
#include <openrct2/audio/audio.h>
#include <openrct2/core/Guard.hpp>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/management/Research.h>
#include <openrct2/network/network.h>
#include <openrct2/object/ObjectList.h>
#include <openrct2/sprites.h>
#include <openrct2/world/LargeScenery.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/Scenery.h>
#include <openrct2/world/SmallScenery.h>

static constexpr const rct_string_id WINDOW_TITLE = STR_NONE;
constexpr int32_t WINDOW_SCENERY_WIDTH = 634;
constexpr int32_t WINDOW_SCENERY_HEIGHT = 180;
constexpr int32_t SCENERY_BUTTON_WIDTH = 66;
constexpr int32_t SCENERY_BUTTON_HEIGHT = 80;
constexpr int32_t SCENERY_WINDOW_TABS = MAX_SCENERY_GROUP_OBJECTS + 1; // The + 1 is for the 'Miscellaneous' tab
constexpr uint16_t SCENERY_ENTRIES_PER_TAB = 1024;

// clang-format off
enum {
    WINDOW_SCENERY_TAB_1,
    WINDOW_SCENERY_TAB_2,
    WINDOW_SCENERY_TAB_3,
    WINDOW_SCENERY_TAB_4,
    WINDOW_SCENERY_TAB_5,
    WINDOW_SCENERY_TAB_6,
    WINDOW_SCENERY_TAB_7,
    WINDOW_SCENERY_TAB_8,
    WINDOW_SCENERY_TAB_9,
    WINDOW_SCENERY_TAB_10,
    WINDOW_SCENERY_TAB_11,
    WINDOW_SCENERY_TAB_12,
    WINDOW_SCENERY_TAB_13,
    WINDOW_SCENERY_TAB_14,
    WINDOW_SCENERY_TAB_15,
    WINDOW_SCENERY_TAB_16,
    WINDOW_SCENERY_TAB_17,
    WINDOW_SCENERY_TAB_18,
    WINDOW_SCENERY_TAB_19,
    WINDOW_SCENERY_TAB_20
};

ScenerySelection gWindowSceneryTabSelections[SCENERY_WINDOW_TABS];
uint8_t gWindowSceneryActiveTabIndex;
uint8_t gWindowSceneryPaintEnabled;
uint8_t gWindowSceneryRotation;
colour_t gWindowSceneryPrimaryColour;
colour_t gWindowScenerySecondaryColour;
colour_t gWindowSceneryTertiaryColour;
bool gWindowSceneryEyedropperEnabled = false;

static void window_scenery_close(rct_window *w);
static void window_scenery_mouseup(rct_window *w, rct_widgetindex widgetIndex);
static void window_scenery_resize(rct_window *w);
static void window_scenery_mousedown(rct_window *w, rct_widgetindex widgetIndex, rct_widget* widget);
static void window_scenery_dropdown(rct_window *w, rct_widgetindex widgetIndex, int32_t dropdownIndex);
static void window_scenery_update(rct_window *w);
static void window_scenery_periodic_update(rct_window *w);
static void window_scenery_scrollgetsize(rct_window *w, int32_t scrollIndex, int32_t *width, int32_t *height);
static void window_scenery_scrollmousedown(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void window_scenery_scrollmouseover(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void window_scenery_tooltip(rct_window* w, rct_widgetindex widgetIndex, rct_string_id *stringId);
static void window_scenery_invalidate(rct_window *w);
static void window_scenery_paint(rct_window *w, rct_drawpixelinfo *dpi);
static void window_scenery_scrollpaint(rct_window *w, rct_drawpixelinfo *dpi, int32_t scrollIndex);

static rct_window_event_list window_scenery_events = {
    window_scenery_close,
    window_scenery_mouseup,
    window_scenery_resize,
    window_scenery_mousedown,
    window_scenery_dropdown,
    nullptr,
    window_scenery_update,
    window_scenery_periodic_update,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_scenery_scrollgetsize,
    window_scenery_scrollmousedown,
    nullptr,
    window_scenery_scrollmouseover,
    nullptr,
    nullptr,
    nullptr,
    window_scenery_tooltip,
    nullptr,
    nullptr,
    window_scenery_invalidate,
    window_scenery_paint,
    window_scenery_scrollpaint,
};


enum WINDOW_SCENERY_LIST_WIDGET_IDX {
    WIDX_SCENERY_BACKGROUND,                // 1
    WIDX_SCENERY_TITLE,                     // 2
    WIDX_SCENERY_CLOSE,                     // 4
    WIDX_SCENERY_TAB_CONTENT_PANEL,         // 8
    WIDX_SCENERY_TAB_1,                     // 10
    WIDX_SCENERY_TAB_2,                     // 20
    WIDX_SCENERY_TAB_3,                     // 40
    WIDX_SCENERY_TAB_4,                     // 80
    WIDX_SCENERY_TAB_5,                     // 100
    WIDX_SCENERY_TAB_6,                     // 200
    WIDX_SCENERY_TAB_7,                     // 400
    WIDX_SCENERY_TAB_8,                     // 800
    WIDX_SCENERY_TAB_9,                     // 1000
    WIDX_SCENERY_TAB_10,                    // 2000
    WIDX_SCENERY_TAB_11,                    // 4000
    WIDX_SCENERY_TAB_12,                    // 8000
    WIDX_SCENERY_TAB_13,                    // 10000
    WIDX_SCENERY_TAB_14,                    // 20000
    WIDX_SCENERY_TAB_15,                    // 40000
    WIDX_SCENERY_TAB_16,                    // 80000
    WIDX_SCENERY_TAB_17,                    // 100000
    WIDX_SCENERY_TAB_18,                    // 200000
    WIDX_SCENERY_TAB_19,                    // 400000
    WIDX_SCENERY_TAB_20,                    // 800000
    WIDX_SCENERY_LIST,                      // 1000000
    WIDX_SCENERY_ROTATE_OBJECTS_BUTTON,     // 2000000
    WIDX_SCENERY_REPAINT_SCENERY_BUTTON,    // 4000000
    WIDX_SCENERY_PRIMARY_COLOUR_BUTTON,     // 8000000
    WIDX_SCENERY_SECONDARY_COLOUR_BUTTON,   // 10000000
    WIDX_SCENERY_TERTIARY_COLOUR_BUTTON,    // 20000000
    WIDX_SCENERY_EYEDROPPER_BUTTON,         // 40000000
    WIDX_SCENERY_BUILD_CLUSTER_BUTTON,      // 80000000
};

validate_global_widx(WC_SCENERY, WIDX_SCENERY_TAB_1);
validate_global_widx(WC_SCENERY, WIDX_SCENERY_ROTATE_OBJECTS_BUTTON);
validate_global_widx(WC_SCENERY, WIDX_SCENERY_EYEDROPPER_BUTTON);

static rct_widget window_scenery_widgets[] = {
    WINDOW_SHIM(WINDOW_TITLE, WINDOW_SCENERY_WIDTH, WINDOW_SCENERY_HEIGHT),
    { WWT_RESIZE, 1, 0, 633, 43, 141, 0xFFFFFFFF, STR_NONE },                           // 8                0x009DE2C8
    { WWT_TAB, 1, 3, 33, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                  // 10               0x009DE2D8
    { WWT_TAB, 1, 34, 64, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                 // 20               0x009DE2E8
    { WWT_TAB, 1, 65, 95, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                 // 40               0x009DE2F8
    { WWT_TAB, 1, 96, 126, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                    // 80               0x009DE308
    { WWT_TAB, 1, 127, 157, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 100              0x009DE318
    { WWT_TAB, 1, 158, 188, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 200              0x009DE328
    { WWT_TAB, 1, 189, 219, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 400              0x009DE338
    { WWT_TAB, 1, 220, 250, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 800              0x009DE348
    { WWT_TAB, 1, 251, 281, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 1000             0x009DE358
    { WWT_TAB, 1, 282, 312, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 2000             0x009DE368
    { WWT_TAB, 1, 313, 343, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 4000             0x009DE378
    { WWT_TAB, 1, 344, 374, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 8000             0x009DE388
    { WWT_TAB, 1, 375, 405, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 10000            0x009DE398
    { WWT_TAB, 1, 406, 436, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 20000            0x009DE3A8
    { WWT_TAB, 1, 437, 467, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 40000            0x009DE3B8
    { WWT_TAB, 1, 468, 498, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 80000            0x009DE3C8
    { WWT_TAB, 1, 468, 498, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 100000           0x009DE3D8
    { WWT_TAB, 1, 468, 498, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 200000           0x009DE3E8
    { WWT_TAB, 1, 468, 498, 17, 43, 0xFFFFFFFF, STR_STRING_DEFINED_TOOLTIP },                                   // 400000           0x009DE3F8
    { WWT_TAB, 1, 468, 498, 17, 43, IMAGE_TYPE_REMAP | SPR_TAB_QUESTION, STR_STRING_DEFINED_TOOLTIP },                                    // 800000           0x009DE408
    { WWT_SCROLL, 1, 2, 608, 47, 126, SCROLL_VERTICAL, STR_NONE },                                  // 1000000          0x009DE418
    { WWT_FLATBTN, 1, 609, 632, 44, 67, SPR_ROTATE_ARROW, STR_ROTATE_OBJECTS_90 },                  // 2000000          0x009DE428
    { WWT_FLATBTN, 1, 609, 632, 68, 91, SPR_PAINTBRUSH, STR_SCENERY_PAINTBRUSH_TIP },                                   // 4000000          0x009DE438
    { WWT_COLOURBTN, 1, 615, 626, 93, 104, 0xFFFFFFFF, STR_SELECT_COLOUR },             // 8000000          0x009DE448
    { WWT_COLOURBTN, 1, 615, 626, 105, 116, 0xFFFFFFFF, STR_SELECT_SECONDARY_COLOUR },  // 10000000         0x009DE458
    { WWT_COLOURBTN, 1, 615, 626, 117, 128, 0xFFFFFFFF, STR_SELECT_TERNARY_COLOUR },        // 20000000         0x009DE468
    { WWT_FLATBTN, 1, 609, 632, 130, 153, SPR_G2_EYEDROPPER, STR_SCENERY_EYEDROPPER_TIP },                                 // 40000000         0x009DE478
    { WWT_FLATBTN, 1, 609, 632, 154, 177, SPR_SCENERY_CLUSTER, STR_SCENERY_CLUSTER_TIP },                                   // 40000000         0x009DE478
    { WIDGETS_END },
};
// clang-format on

void window_scenery_update_scroll(rct_window* w);

// rct2: 0x00F64F2C
static ScenerySelection window_scenery_tab_entries[SCENERY_WINDOW_TABS][SCENERY_ENTRIES_PER_TAB + 1];

/**
 * Was part of 0x006DFA00
 * The same code repeated five times for every scenery entry type
 */
static void init_scenery_entry(
    rct_scenery_entry* sceneryEntry, const ScenerySelection& selection, ObjectEntryIndex sceneryTabId)
{
    Guard::ArgumentInRange<int32_t>(selection.EntryIndex, 0, WINDOW_SCENERY_TAB_SELECTION_UNDEFINED);
    if (scenery_is_invented(selection) || gCheatsIgnoreResearchStatus)
    {
        if (sceneryTabId < SCENERY_WINDOW_TABS)
        {
            for (int32_t i = 0; i < SCENERY_ENTRIES_PER_TAB; i++)
            {
                if (window_scenery_tab_entries[sceneryTabId][i].IsUndefined())
                {
                    window_scenery_tab_entries[sceneryTabId][i] = selection;
                    window_scenery_tab_entries[sceneryTabId][i + 1].SetUndefined();
                    return;
                }
            }
        }

        for (int32_t i = 0; i < SCENERY_WINDOW_TABS - 1; i++)
        {
            int32_t counter = 0;

            while (!window_scenery_tab_entries[i][counter].IsUndefined())
            {
                if (window_scenery_tab_entries[i][counter] == selection)
                {
                    return;
                }

                counter++;
            }
        }

        for (int32_t i = 0; i < SCENERY_ENTRIES_PER_TAB; i++)
        {
            if (window_scenery_tab_entries[SCENERY_WINDOW_TABS - 1][i].IsUndefined())
            {
                window_scenery_tab_entries[SCENERY_WINDOW_TABS - 1][i] = selection;
                window_scenery_tab_entries[SCENERY_WINDOW_TABS - 1][i + 1].SetUndefined();
                break;
            }
        }
    }
}

/**
 *
 *  rct2: 0x006DFA00
 */
void window_scenery_init()
{
    bool enabledScenerySets[SCENERY_WINDOW_TABS] = { false };

    for (int32_t scenerySetIndex = 0; scenerySetIndex < SCENERY_WINDOW_TABS; scenerySetIndex++)
    {
        window_scenery_tab_entries[scenerySetIndex][0].SetUndefined();
        if (scenerySetIndex == MAX_SCENERY_GROUP_OBJECTS)
            continue;

        rct_scenery_group_entry* sceneryGroupEntry = get_scenery_group_entry(scenerySetIndex);
        if (sceneryGroupEntry == nullptr)
            continue;

        int32_t sceneryTabEntryCount = 0;
        for (int32_t i = 0; i < sceneryGroupEntry->entry_count; i++)
        {
            auto sceneryEntry = sceneryGroupEntry->scenery_entries[i];
            if (scenery_is_invented(sceneryEntry) || gCheatsIgnoreResearchStatus)
            {
                window_scenery_tab_entries[scenerySetIndex][sceneryTabEntryCount] = sceneryEntry;
                window_scenery_tab_entries[scenerySetIndex][++sceneryTabEntryCount].SetUndefined();
            }
            else
            {
                enabledScenerySets[scenerySetIndex] = true;
            }
        }
    }

    // small scenery
    for (ObjectEntryIndex sceneryId = 0; sceneryId < MAX_SMALL_SCENERY_OBJECTS; sceneryId++)
    {
        rct_scenery_entry* sceneryEntry = get_small_scenery_entry(sceneryId);
        if (sceneryEntry == nullptr)
            continue;

        init_scenery_entry(sceneryEntry, { SCENERY_TYPE_SMALL, sceneryId }, sceneryEntry->small_scenery.scenery_tab_id);
    }

    // large scenery
    for (ObjectEntryIndex sceneryId = 0; sceneryId < MAX_LARGE_SCENERY_OBJECTS; sceneryId++)
    {
        rct_scenery_entry* sceneryEntry = get_large_scenery_entry(sceneryId);
        if (sceneryEntry == nullptr)
            continue;

        init_scenery_entry(sceneryEntry, { SCENERY_TYPE_LARGE, sceneryId }, sceneryEntry->large_scenery.scenery_tab_id);
    }

    // walls
    for (ObjectEntryIndex sceneryId = 0; sceneryId < MAX_WALL_SCENERY_OBJECTS; sceneryId++)
    {
        rct_scenery_entry* sceneryEntry = get_wall_entry(sceneryId);
        if (sceneryEntry == nullptr)
            continue;

        init_scenery_entry(sceneryEntry, { SCENERY_TYPE_WALL, sceneryId }, sceneryEntry->wall.scenery_tab_id);
    }

    // banners
    for (ObjectEntryIndex sceneryId = 0; sceneryId < MAX_BANNER_OBJECTS; sceneryId++)
    {
        rct_scenery_entry* sceneryEntry = get_banner_entry(sceneryId);
        if (sceneryEntry == nullptr)
            continue;

        init_scenery_entry(sceneryEntry, { SCENERY_TYPE_BANNER, sceneryId }, sceneryEntry->banner.scenery_tab_id);
    }

    // path bits
    for (ObjectEntryIndex sceneryId = 0; sceneryId < MAX_PATH_ADDITION_OBJECTS; sceneryId++)
    {
        rct_scenery_entry* sceneryEntry = get_footpath_item_entry(sceneryId);
        if (sceneryEntry == nullptr)
            continue;

        init_scenery_entry(sceneryEntry, { SCENERY_TYPE_PATH_ITEM, sceneryId }, sceneryEntry->path_bit.scenery_tab_id);
    }

    for (rct_widgetindex widgetIndex = WIDX_SCENERY_TAB_1; widgetIndex < WIDX_SCENERY_LIST; widgetIndex++)
        window_scenery_widgets[widgetIndex].type = WWT_EMPTY;

    uint8_t tabIndexes[SCENERY_WINDOW_TABS];
    uint8_t order[SCENERY_WINDOW_TABS];
    int32_t usedValues = 0;

    for (int32_t scenerySetId = 0; scenerySetId < MAX_SCENERY_GROUP_OBJECTS; scenerySetId++)
    {
        rct_scenery_group_entry* sceneryEntry = get_scenery_group_entry(scenerySetId);
        if (sceneryEntry == nullptr)
            continue;

        tabIndexes[usedValues] = scenerySetId;
        order[usedValues] = sceneryEntry->priority;

        usedValues++;
    }

    while (true)
    {
        bool finished = true;
        for (int32_t i = 1; i < usedValues; i++)
        {
            if (order[i - 1] > order[i])
            {
                uint8_t tmp = tabIndexes[i - 1];
                tabIndexes[i - 1] = tabIndexes[i];
                tabIndexes[i] = tmp;
                tmp = order[i - 1];
                order[i - 1] = order[i];
                order[i] = tmp;
                finished = false;
            }
        }

        if (finished)
            break;
    }

    tabIndexes[usedValues] = SCENERY_WINDOW_TABS - 1;
    usedValues++;

    uint16_t left = 3;
    for (int32_t i = 0; i < usedValues; i++)
    {
        uint32_t tabIndex = tabIndexes[i];
        rct_widget* tabWidget = &window_scenery_widgets[tabIndex + WIDX_SCENERY_TAB_1];

        if (left != 3 || tabIndex != SCENERY_WINDOW_TABS - 1)
        {
            if (window_scenery_tab_entries[tabIndex][0].IsUndefined())
                continue;

            if (enabledScenerySets[tabIndex])
                continue;
        }

        tabWidget->type = WWT_TAB;
        tabWidget->left = left;
        tabWidget->right = left + 0x1E;
        left += 0x1F;

        if (tabIndex >= SCENERY_WINDOW_TABS - 1)
            continue;

        tabWidget->image = get_scenery_group_entry(tabIndex)->image | IMAGE_TYPE_REMAP;
    }

    window_invalidate_by_class(WC_SCENERY);
}

/**
 *
 *  rct2: 0x006DFEE4
 */
void window_scenery_set_default_placement_configuration()
{
    gWindowSceneryRotation = 3;
    gWindowSceneryPrimaryColour = COLOUR_BORDEAUX_RED;
    gWindowScenerySecondaryColour = COLOUR_YELLOW;
    gWindowSceneryTertiaryColour = COLOUR_DARK_BROWN;
    window_scenery_init();

    for (int32_t i = 0; i < SCENERY_WINDOW_TABS; i++)
        gWindowSceneryTabSelections[i] = ScenerySelection::CreateUndefined();

    for (int32_t i = 0; i < SCENERY_WINDOW_TABS; i++)
    {
        if (!window_scenery_tab_entries[i][0].IsUndefined())
        {
            gWindowSceneryActiveTabIndex = i;
            return;
        }
    }

    for (int32_t i = 0; i < 16; i++)
    {
        rct_widget* tabWidget = &window_scenery_widgets[WIDX_SCENERY_TAB_1 + i];
        if (tabWidget->type != WWT_EMPTY)
        {
            gWindowSceneryActiveTabIndex = i;
            return;
        }
    }
}

/**
 *
 *  rct2: 0x006E0FEF
 */
rct_window* window_scenery_open()
{
    rct_window* window;

    // Check if window is already open
    window = window_bring_to_front_by_class(WC_SCENERY);
    if (window != nullptr)
        return window;

    window_scenery_init();

    window = window_create(
        ScreenCoordsXY(context_get_width() - WINDOW_SCENERY_WIDTH, 0x1D), WINDOW_SCENERY_WIDTH, WINDOW_SCENERY_HEIGHT,
        &window_scenery_events, WC_SCENERY, WF_NO_SCROLLING);
    window->widgets = window_scenery_widgets;

    window->enabled_widgets = (1 << WIDX_SCENERY_CLOSE) | (1 << WIDX_SCENERY_ROTATE_OBJECTS_BUTTON) | (1 << WIDX_SCENERY_TAB_1)
        | (1 << WIDX_SCENERY_TAB_2) | (1 << WIDX_SCENERY_TAB_3) | (1 << WIDX_SCENERY_TAB_4) | (1 << WIDX_SCENERY_TAB_5)
        | (1 << WIDX_SCENERY_TAB_6) | (1 << WIDX_SCENERY_TAB_7) | (1 << WIDX_SCENERY_TAB_8) | (1 << WIDX_SCENERY_TAB_9)
        | (1 << WIDX_SCENERY_TAB_10) | (1 << WIDX_SCENERY_TAB_11) | (1 << WIDX_SCENERY_TAB_12) | (1 << WIDX_SCENERY_TAB_13)
        | (1 << WIDX_SCENERY_TAB_14) | (1 << WIDX_SCENERY_TAB_15) | (1 << WIDX_SCENERY_TAB_16) | (1 << WIDX_SCENERY_TAB_17)
        | (1 << WIDX_SCENERY_TAB_18) | (1 << WIDX_SCENERY_TAB_19) | (1 << WIDX_SCENERY_TAB_20)
        | (1 << WIDX_SCENERY_PRIMARY_COLOUR_BUTTON) | (1 << WIDX_SCENERY_SECONDARY_COLOUR_BUTTON)
        | (1 << WIDX_SCENERY_REPAINT_SCENERY_BUTTON) | (1 << WIDX_SCENERY_TERTIARY_COLOUR_BUTTON)
        | (1 << WIDX_SCENERY_EYEDROPPER_BUTTON) | (1ULL << WIDX_SCENERY_BUILD_CLUSTER_BUTTON);

    window_init_scroll_widgets(window);
    window_scenery_update_scroll(window);
    show_gridlines();
    gWindowSceneryRotation = 3;
    gSceneryCtrlPressed = false;
    gSceneryShiftPressed = false;
    window->scenery.SelectedScenery = ScenerySelection::CreateUndefined();
    window->scenery.hover_counter = 0;
    window_push_others_below(window);
    gSceneryGhostType = 0;
    gSceneryPlaceCost = MONEY32_UNDEFINED;
    gSceneryPlaceRotation = 0;
    gWindowSceneryPaintEnabled = 0; // repaint coloured scenery tool state
    gWindowSceneryEyedropperEnabled = false;

    window->min_width = WINDOW_SCENERY_WIDTH;
    window->max_width = WINDOW_SCENERY_WIDTH;
    window->min_height = WINDOW_SCENERY_HEIGHT;
    window->max_height = WINDOW_SCENERY_HEIGHT;

    return window;
}

/**
 *
 *  rct2: 0x006E1A73
 */
void window_scenery_close(rct_window* w)
{
    scenery_remove_ghost_tool_placement();
    hide_gridlines();
    viewport_set_visibility(0);

    if (gWindowSceneryScatterEnabled)
        window_close_by_class(WC_SCENERY_SCATTER);

    if (scenery_tool_is_active())
        tool_cancel();
}

static int32_t count_rows(int32_t items)
{
    int32_t rows = items / 9;

    return rows;
}

struct scenery_item
{
    int32_t allRows;
    int32_t selected_item;
    ScenerySelection scenerySelection;
};

static scenery_item window_scenery_count_rows_with_selected_item(int32_t tabIndex)
{
    scenery_item sceneryItem = { 0, 0, ScenerySelection::CreateUndefined() };
    int32_t totalItems = 0;
    ScenerySelection currentEntry = { 0, 0 };
    ScenerySelection scenerySelection = gWindowSceneryTabSelections[tabIndex];

    while ((currentEntry = window_scenery_tab_entries[tabIndex][totalItems]), !currentEntry.IsUndefined())
    {
        if (currentEntry == scenerySelection)
        {
            sceneryItem.selected_item = totalItems;
            sceneryItem.scenerySelection = scenerySelection;
        }
        totalItems++;
    }
    sceneryItem.allRows = count_rows(totalItems + 8);
    return sceneryItem;
}

static int32_t window_scenery_count_rows()
{
    int32_t tabIndex = gWindowSceneryActiveTabIndex;
    int32_t totalItems = 0;

    while (!window_scenery_tab_entries[tabIndex][totalItems].IsUndefined())
    {
        totalItems++;
    }

    int32_t rows = count_rows(totalItems + 8);
    return rows;
}

static int32_t window_scenery_rows_height(int32_t rows)
{
    return rows * SCENERY_BUTTON_HEIGHT;
}

/**
 *
 *  rct2: 0x006BD94C
 */
static void window_scenery_mouseup(rct_window* w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex)
    {
        case WIDX_SCENERY_CLOSE:
            if (gWindowSceneryScatterEnabled)
                window_close_by_class(WC_SCENERY_SCATTER);
            window_close(w);
            break;
        case WIDX_SCENERY_ROTATE_OBJECTS_BUTTON:
            gWindowSceneryRotation++;
            gWindowSceneryRotation = gWindowSceneryRotation % 4;
            scenery_remove_ghost_tool_placement();
            w->Invalidate();
            break;
        case WIDX_SCENERY_REPAINT_SCENERY_BUTTON:
            gWindowSceneryPaintEnabled ^= 1;
            gWindowSceneryEyedropperEnabled = false;
            if (gWindowSceneryScatterEnabled)
                window_close_by_class(WC_SCENERY_SCATTER);
            w->Invalidate();
            break;
        case WIDX_SCENERY_EYEDROPPER_BUTTON:
            gWindowSceneryPaintEnabled = 0;
            gWindowSceneryEyedropperEnabled = !gWindowSceneryEyedropperEnabled;
            if (gWindowSceneryScatterEnabled)
                window_close_by_class(WC_SCENERY_SCATTER);
            scenery_remove_ghost_tool_placement();
            w->Invalidate();
            break;
        case WIDX_SCENERY_BUILD_CLUSTER_BUTTON:
            gWindowSceneryPaintEnabled = 0;
            gWindowSceneryEyedropperEnabled = false;
            if (gWindowSceneryScatterEnabled)
                window_close_by_class(WC_SCENERY_SCATTER);
            else if (
                network_get_mode() != NETWORK_MODE_CLIENT
                || network_can_perform_command(network_get_current_player_group_index(), -2))
            {
                window_scenery_scatter_open();
            }
            else
            {
                context_show_error(STR_CANT_DO_THIS, STR_PERMISSION_DENIED);
            }
            w->Invalidate();
            break;
    }
}

/**
 *
 *  rct2: 0x006E1EB4
 */
void window_scenery_update_scroll(rct_window* w)
{
    int32_t tabIndex = gWindowSceneryActiveTabIndex;
    int32_t listHeight = w->height - 14 - window_scenery_widgets[WIDX_SCENERY_LIST].top - 1;

    scenery_item sceneryItem = window_scenery_count_rows_with_selected_item(tabIndex);
    w->scrolls[0].v_bottom = window_scenery_rows_height(sceneryItem.allRows) + 1;

    int32_t maxTop = std::max(0, w->scrolls[0].v_bottom - listHeight);
    int32_t rowSelected = count_rows(sceneryItem.selected_item);
    if (sceneryItem.scenerySelection.IsUndefined())
    {
        rowSelected = 0;
        ScenerySelection scenery = window_scenery_tab_entries[tabIndex][0];
        if (!scenery.IsUndefined())
            gWindowSceneryTabSelections[tabIndex] = scenery;
    }

    w->scrolls[0].v_top = window_scenery_rows_height(rowSelected);
    w->scrolls[0].v_top = std::min<int32_t>(maxTop, w->scrolls[0].v_top);

    widget_scroll_update_thumbs(w, WIDX_SCENERY_LIST);
}

/**
 *
 *  rct2: 0x006E1E48
 */
static void window_scenery_resize(rct_window* w)
{
    if (w->width < w->min_width)
    {
        w->Invalidate();
        w->width = w->min_width;
        w->Invalidate();
    }

    if (w->width > w->max_width)
    {
        w->Invalidate();
        w->width = w->max_width;
        w->Invalidate();
    }

    if (w->height < w->min_height)
    {
        w->Invalidate();
        w->height = w->min_height;
        w->Invalidate();
        // HACK: For some reason invalidate has not been called
        window_event_invalidate_call(w);
        window_scenery_update_scroll(w);
    }

    if (w->height > w->max_height)
    {
        w->Invalidate();
        w->height = w->max_height;
        w->Invalidate();
        // HACK: For some reason invalidate has not been called
        window_event_invalidate_call(w);
        window_scenery_update_scroll(w);
    }
}

/**
 *
 *  rct2: 0x006E1A25
 */
static void window_scenery_mousedown(rct_window* w, rct_widgetindex widgetIndex, rct_widget* widget)
{
    switch (widgetIndex)
    {
        case WIDX_SCENERY_PRIMARY_COLOUR_BUTTON:
            window_dropdown_show_colour(w, widget, w->colours[1], gWindowSceneryPrimaryColour);
            break;
        case WIDX_SCENERY_SECONDARY_COLOUR_BUTTON:
            window_dropdown_show_colour(w, widget, w->colours[1], gWindowScenerySecondaryColour);
            break;
        case WIDX_SCENERY_TERTIARY_COLOUR_BUTTON:
            window_dropdown_show_colour(w, widget, w->colours[1], gWindowSceneryTertiaryColour);
            break;
    }

    if (widgetIndex >= WIDX_SCENERY_TAB_1 && widgetIndex <= WIDX_SCENERY_TAB_20)
    {
        gWindowSceneryActiveTabIndex = widgetIndex - WIDX_SCENERY_TAB_1;
        w->Invalidate();
        gSceneryPlaceCost = MONEY32_UNDEFINED;

        window_scenery_update_scroll(w);
    }
}

/**
 *
 *  rct2: 0x006E1A54
 */
static void window_scenery_dropdown(rct_window* w, rct_widgetindex widgetIndex, int32_t dropdownIndex)
{
    if (dropdownIndex == -1)
        return;

    if (widgetIndex == WIDX_SCENERY_PRIMARY_COLOUR_BUTTON)
    {
        gWindowSceneryPrimaryColour = static_cast<colour_t>(dropdownIndex);
    }
    else if (widgetIndex == WIDX_SCENERY_SECONDARY_COLOUR_BUTTON)
    {
        gWindowScenerySecondaryColour = static_cast<colour_t>(dropdownIndex);
    }
    else if (widgetIndex == WIDX_SCENERY_TERTIARY_COLOUR_BUTTON)
    {
        gWindowSceneryTertiaryColour = static_cast<colour_t>(dropdownIndex);
    }

    w->Invalidate();
}

/**
 *
 *  rct2: 0x006E1B9F
 */
static void window_scenery_periodic_update(rct_window* w)
{
    if (!w->scenery.SelectedScenery.IsUndefined())
    {
        w->scenery.SelectedScenery = ScenerySelection::CreateUndefined();
    }
}

/**
 *
 *  rct2: 0x006E1CD3
 */
static void window_scenery_update(rct_window* w)
{
    const CursorState* state = context_get_cursor_state();
    rct_window* other = window_find_from_point(state->position);
    if (other == w)
    {
        ScreenCoordsXY window = state->position - ScreenCoordsXY{ w->windowPos.x - 26, w->windowPos.y };

        if (window.y < 44 || window.x <= w->width)
        {
            rct_widgetindex widgetIndex = window_find_widget_from_point(w, state->position);
            if (widgetIndex >= WIDX_SCENERY_TAB_CONTENT_PANEL)
            {
                w->scenery.hover_counter++;
                if (w->scenery.hover_counter < 8)
                {
                    if (input_get_state() != INPUT_STATE_SCROLL_LEFT)
                    {
                        w->min_width = WINDOW_SCENERY_WIDTH;
                        w->max_width = WINDOW_SCENERY_WIDTH;
                        w->min_height = WINDOW_SCENERY_HEIGHT;
                        w->max_height = WINDOW_SCENERY_HEIGHT;
                    }
                }
                else
                {
                    int32_t windowHeight = std::min(463, w->scrolls[0].v_bottom + 62);
                    if (context_get_height() < 600)
                        windowHeight = std::min(374, windowHeight);
                    windowHeight = std::max(WINDOW_SCENERY_HEIGHT, windowHeight);

                    w->min_width = WINDOW_SCENERY_WIDTH;
                    w->max_width = WINDOW_SCENERY_WIDTH;
                    w->min_height = windowHeight;
                    w->max_height = windowHeight;
                }
            }
        }
    }
    else
    {
        w->scenery.hover_counter = 0;
        if (input_get_state() != INPUT_STATE_SCROLL_LEFT)
        {
            w->min_width = WINDOW_SCENERY_WIDTH;
            w->max_width = WINDOW_SCENERY_WIDTH;
            w->min_height = WINDOW_SCENERY_HEIGHT;
            w->max_height = WINDOW_SCENERY_HEIGHT;
        }
    }

    w->Invalidate();

    if (!scenery_tool_is_active())
    {
        window_close(w);
        return;
    }

    if (gWindowSceneryEyedropperEnabled)
    {
        gCurrentToolId = TOOL_CROSSHAIR;
    }
    else if (gWindowSceneryPaintEnabled == 1)
    {
        gCurrentToolId = TOOL_PAINT_DOWN;
    }
    else
    {
        uint16_t tabIndex = gWindowSceneryActiveTabIndex;
        ScenerySelection tabSelectedScenery = gWindowSceneryTabSelections[tabIndex];

        if (!tabSelectedScenery.IsUndefined())
        {
            if (tabSelectedScenery.SceneryType == SCENERY_TYPE_BANNER)
            {
                gCurrentToolId = TOOL_ENTRANCE_DOWN;
            }
            else if (tabSelectedScenery.SceneryType == SCENERY_TYPE_LARGE)
            {
                gCurrentToolId = static_cast<TOOL_IDX>(
                    get_large_scenery_entry(tabSelectedScenery.EntryIndex)->large_scenery.tool_id);
            }
            else if (tabSelectedScenery.SceneryType == SCENERY_TYPE_WALL)
            {
                gCurrentToolId = static_cast<TOOL_IDX>(get_wall_entry(tabSelectedScenery.EntryIndex)->wall.tool_id);
            }
            else if (tabSelectedScenery.SceneryType == SCENERY_TYPE_PATH_ITEM)
            { // path bit
                gCurrentToolId = static_cast<TOOL_IDX>(
                    get_footpath_item_entry(tabSelectedScenery.EntryIndex)->path_bit.tool_id);
            }
            else
            { // small scenery
                gCurrentToolId = static_cast<TOOL_IDX>(
                    get_small_scenery_entry(tabSelectedScenery.EntryIndex)->small_scenery.tool_id);
            }
        }
    }
}

/**
 *
 *  rct2: 0x006E1A91
 */
void window_scenery_scrollgetsize(rct_window* w, int32_t scrollIndex, int32_t* width, int32_t* height)
{
    int32_t rows = window_scenery_count_rows();
    *height = window_scenery_rows_height(rows);
}

static ScenerySelection get_scenery_id_by_cursor_pos(const ScreenCoordsXY& screenCoords)
{
    int32_t tabSceneryIndex = screenCoords.x / SCENERY_BUTTON_WIDTH + (screenCoords.y / SCENERY_BUTTON_HEIGHT) * 9;
    uint8_t tabIndex = gWindowSceneryActiveTabIndex;

    int32_t itemCounter = 0;
    ScenerySelection scenery = ScenerySelection::CreateUndefined();
    while (itemCounter <= tabSceneryIndex)
    {
        scenery = window_scenery_tab_entries[tabIndex][itemCounter];
        if (scenery.IsUndefined())
            return ScenerySelection::CreateUndefined();

        itemCounter++;
    }

    return scenery;
}

/**
 *
 *  rct2: 0x006E1C4A
 */
void window_scenery_scrollmousedown(rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    ScenerySelection scenery = get_scenery_id_by_cursor_pos(screenCoords);
    if (scenery.IsUndefined())
        return;

    uint8_t tabIndex = gWindowSceneryActiveTabIndex;
    gWindowSceneryTabSelections[tabIndex] = scenery;

    gWindowSceneryPaintEnabled &= 0xFE;
    gWindowSceneryEyedropperEnabled = false;
    audio_play_sound(SoundId::Click1, 0, w->windowPos.x + (w->width / 2));
    w->scenery.hover_counter = -16;
    gSceneryPlaceCost = MONEY32_UNDEFINED;
    w->Invalidate();
}

/**
 *
 *  rct2: 0x006E1BB8
 */
void window_scenery_scrollmouseover(rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    ScenerySelection scenery = get_scenery_id_by_cursor_pos(screenCoords);
    if (!scenery.IsUndefined())
    {
        w->scenery.SelectedScenery = scenery;
        w->Invalidate();
    }
}

/**
 *
 *  rct2: 0x006E1C05
 */
void window_scenery_tooltip(rct_window* w, rct_widgetindex widgetIndex, rct_string_id* stringId)
{
    auto ft = Formatter::Common();

    switch (widgetIndex)
    {
        case WIDX_SCENERY_TAB_1:
        case WIDX_SCENERY_TAB_2:
        case WIDX_SCENERY_TAB_3:
        case WIDX_SCENERY_TAB_4:
        case WIDX_SCENERY_TAB_5:
        case WIDX_SCENERY_TAB_6:
        case WIDX_SCENERY_TAB_7:
        case WIDX_SCENERY_TAB_8:
        case WIDX_SCENERY_TAB_9:
        case WIDX_SCENERY_TAB_10:
        case WIDX_SCENERY_TAB_11:
        case WIDX_SCENERY_TAB_12:
        case WIDX_SCENERY_TAB_13:
        case WIDX_SCENERY_TAB_14:
        case WIDX_SCENERY_TAB_15:
        case WIDX_SCENERY_TAB_16:
        case WIDX_SCENERY_TAB_17:
        case WIDX_SCENERY_TAB_18:
        case WIDX_SCENERY_TAB_19:
            ft.Add<rct_string_id>(get_scenery_group_entry(widgetIndex - WIDX_SCENERY_TAB_1)->name);
            break;
        case WIDX_SCENERY_TAB_20:
            ft.Add<rct_string_id>(STR_MISCELLANEOUS);
            break;
    }
}

/**
 *
 *  rct2: 0x006E118B
 */
void window_scenery_invalidate(rct_window* w)
{
    uint16_t tabIndex = gWindowSceneryActiveTabIndex;
    uint32_t titleStringId = STR_MISCELLANEOUS;
    if (tabIndex < SCENERY_WINDOW_TABS - 1)
    {
        rct_scenery_group_entry* sgEntry = get_scenery_group_entry(tabIndex);
        if (sgEntry != nullptr)
        {
            titleStringId = sgEntry->name;
        }
    }

    window_scenery_widgets[WIDX_SCENERY_TITLE].text = titleStringId;

    w->pressed_widgets = 0;
    w->pressed_widgets |= 1ULL << (tabIndex + WIDX_SCENERY_TAB_1);
    if (gWindowSceneryPaintEnabled == 1)
        w->pressed_widgets |= (1 << WIDX_SCENERY_REPAINT_SCENERY_BUTTON);
    if (gWindowSceneryEyedropperEnabled)
        w->pressed_widgets |= (1 << WIDX_SCENERY_EYEDROPPER_BUTTON);
    if (gWindowSceneryScatterEnabled)
        w->pressed_widgets |= (1ULL << WIDX_SCENERY_BUILD_CLUSTER_BUTTON);

    window_scenery_widgets[WIDX_SCENERY_ROTATE_OBJECTS_BUTTON].type = WWT_EMPTY;
    window_scenery_widgets[WIDX_SCENERY_EYEDROPPER_BUTTON].type = WWT_EMPTY;
    window_scenery_widgets[WIDX_SCENERY_BUILD_CLUSTER_BUTTON].type = WWT_EMPTY;

    if (!(gWindowSceneryPaintEnabled & 1))
    {
        window_scenery_widgets[WIDX_SCENERY_EYEDROPPER_BUTTON].type = WWT_FLATBTN;
    }

    ScenerySelection tabSelectedScenery = gWindowSceneryTabSelections[tabIndex];
    if (!tabSelectedScenery.IsUndefined())
    {
        if (tabSelectedScenery.SceneryType == SCENERY_TYPE_SMALL)
        {
            if (!(gWindowSceneryPaintEnabled & 1))
            {
                window_scenery_widgets[WIDX_SCENERY_BUILD_CLUSTER_BUTTON].type = WWT_FLATBTN;
            }

            rct_scenery_entry* sceneryEntry = get_small_scenery_entry(tabSelectedScenery.EntryIndex);
            if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_ROTATABLE))
            {
                window_scenery_widgets[WIDX_SCENERY_ROTATE_OBJECTS_BUTTON].type = WWT_FLATBTN;
            }
        }
        else if (tabSelectedScenery.SceneryType >= SCENERY_TYPE_LARGE)
        {
            window_scenery_widgets[WIDX_SCENERY_ROTATE_OBJECTS_BUTTON].type = WWT_FLATBTN;
        }
    }

    window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].image = SPRITE_ID_PALETTE_COLOUR_1(gWindowSceneryPrimaryColour)
        | IMAGE_TYPE_TRANSPARENT | SPR_PALETTE_BTN;
    window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].image = SPRITE_ID_PALETTE_COLOUR_1(
                                                                             gWindowScenerySecondaryColour)
        | IMAGE_TYPE_TRANSPARENT | SPR_PALETTE_BTN;
    window_scenery_widgets[WIDX_SCENERY_TERTIARY_COLOUR_BUTTON].image = SPRITE_ID_PALETTE_COLOUR_1(gWindowSceneryTertiaryColour)
        | IMAGE_TYPE_TRANSPARENT | SPR_PALETTE_BTN;

    window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_EMPTY;
    window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].type = WWT_EMPTY;
    window_scenery_widgets[WIDX_SCENERY_TERTIARY_COLOUR_BUTTON].type = WWT_EMPTY;

    if (gWindowSceneryPaintEnabled & 1)
    { // repaint coloured scenery tool is on
        window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
        window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
        window_scenery_widgets[WIDX_SCENERY_TERTIARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
        window_scenery_widgets[WIDX_SCENERY_ROTATE_OBJECTS_BUTTON].type = WWT_EMPTY;
    }
    else if (!tabSelectedScenery.IsUndefined())
    {
        rct_scenery_entry* sceneryEntry = nullptr;

        if (tabSelectedScenery.SceneryType == SCENERY_TYPE_BANNER)
        {
            sceneryEntry = get_banner_entry(tabSelectedScenery.EntryIndex);
            if (sceneryEntry->banner.flags & BANNER_ENTRY_FLAG_HAS_PRIMARY_COLOUR)
            {
                window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
            }
        }
        else if (tabSelectedScenery.SceneryType == SCENERY_TYPE_LARGE)
        {
            sceneryEntry = get_large_scenery_entry(tabSelectedScenery.EntryIndex);

            if (sceneryEntry->large_scenery.flags & LARGE_SCENERY_FLAG_HAS_PRIMARY_COLOUR)
                window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
            if (sceneryEntry->large_scenery.flags & LARGE_SCENERY_FLAG_HAS_SECONDARY_COLOUR)
                window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
        }
        else if (tabSelectedScenery.SceneryType == SCENERY_TYPE_WALL)
        {
            sceneryEntry = get_wall_entry(tabSelectedScenery.EntryIndex);
            if (sceneryEntry->wall.flags & (WALL_SCENERY_HAS_PRIMARY_COLOUR | WALL_SCENERY_HAS_GLASS))
            {
                window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_COLOURBTN;

                if (sceneryEntry->wall.flags & WALL_SCENERY_HAS_SECONDARY_COLOUR)
                {
                    window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].type = WWT_COLOURBTN;

                    if (sceneryEntry->wall.flags2 & WALL_SCENERY_2_NO_SELECT_PRIMARY_COLOUR)
                        window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_EMPTY;
                    if (sceneryEntry->wall.flags & WALL_SCENERY_HAS_TERNARY_COLOUR)
                        window_scenery_widgets[WIDX_SCENERY_TERTIARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
                }
            }
        }
        else if (tabSelectedScenery.SceneryType == SCENERY_TYPE_SMALL)
        {
            sceneryEntry = get_small_scenery_entry(tabSelectedScenery.EntryIndex);

            if (scenery_small_entry_has_flag(
                    sceneryEntry, SMALL_SCENERY_FLAG_HAS_PRIMARY_COLOUR | SMALL_SCENERY_FLAG_HAS_GLASS))
            {
                window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].type = WWT_COLOURBTN;

                if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_HAS_SECONDARY_COLOUR))
                    window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].type = WWT_COLOURBTN;
            }
        }
    }

    window_scenery_widgets[WIDX_SCENERY_BACKGROUND].right = w->width - 1;
    window_scenery_widgets[WIDX_SCENERY_BACKGROUND].bottom = w->height - 1;
    window_scenery_widgets[WIDX_SCENERY_TAB_CONTENT_PANEL].right = w->width - 1;
    window_scenery_widgets[WIDX_SCENERY_TAB_CONTENT_PANEL].bottom = w->height - 1;
    window_scenery_widgets[WIDX_SCENERY_TITLE].right = w->width - 2;
    window_scenery_widgets[WIDX_SCENERY_CLOSE].left = w->width - 13;
    window_scenery_widgets[WIDX_SCENERY_CLOSE].right = window_scenery_widgets[WIDX_SCENERY_CLOSE].left + 10;
    window_scenery_widgets[WIDX_SCENERY_LIST].right = w->width - 26;
    window_scenery_widgets[WIDX_SCENERY_LIST].bottom = w->height - 14;

    window_scenery_widgets[WIDX_SCENERY_ROTATE_OBJECTS_BUTTON].left = w->width - 25;
    window_scenery_widgets[WIDX_SCENERY_REPAINT_SCENERY_BUTTON].left = w->width - 25;
    window_scenery_widgets[WIDX_SCENERY_EYEDROPPER_BUTTON].left = w->width - 25;
    window_scenery_widgets[WIDX_SCENERY_BUILD_CLUSTER_BUTTON].left = w->width - 25;
    window_scenery_widgets[WIDX_SCENERY_ROTATE_OBJECTS_BUTTON].right = w->width - 2;
    window_scenery_widgets[WIDX_SCENERY_REPAINT_SCENERY_BUTTON].right = w->width - 2;
    window_scenery_widgets[WIDX_SCENERY_EYEDROPPER_BUTTON].right = w->width - 2;
    window_scenery_widgets[WIDX_SCENERY_BUILD_CLUSTER_BUTTON].right = w->width - 2;

    window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].left = w->width - 19;
    window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].left = w->width - 19;
    window_scenery_widgets[WIDX_SCENERY_TERTIARY_COLOUR_BUTTON].left = w->width - 19;
    window_scenery_widgets[WIDX_SCENERY_PRIMARY_COLOUR_BUTTON].right = w->width - 8;
    window_scenery_widgets[WIDX_SCENERY_SECONDARY_COLOUR_BUTTON].right = w->width - 8;
    window_scenery_widgets[WIDX_SCENERY_TERTIARY_COLOUR_BUTTON].right = w->width - 8;
}

/**
 *
 *  rct2: 0x006E1462
 */
void window_scenery_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);

    uint16_t tabIndex = gWindowSceneryActiveTabIndex;
    uint16_t selectedWidgetId = tabIndex + 4;
    uint32_t imageId = ((w->colours[1] << 19) | window_scenery_widgets[selectedWidgetId].image) + 1ul;

    gfx_draw_sprite(
        dpi, imageId, w->windowPos.x + window_scenery_widgets[selectedWidgetId].left,
        w->windowPos.y + window_scenery_widgets[selectedWidgetId].top, selectedWidgetId);

    ScenerySelection selectedSceneryEntry = w->scenery.SelectedScenery;
    if (selectedSceneryEntry.IsUndefined())
    {
        if (gWindowSceneryPaintEnabled & 1) // repaint coloured scenery tool is on
            return;
        if (gWindowSceneryEyedropperEnabled)
            return;

        selectedSceneryEntry = gWindowSceneryTabSelections[tabIndex];

        if (selectedSceneryEntry.IsUndefined())
            return;
    }

    uint32_t price = 0;

    rct_scenery_entry* sceneryEntry = nullptr;
    switch (selectedSceneryEntry.SceneryType)
    {
        case SCENERY_TYPE_SMALL:
            sceneryEntry = get_small_scenery_entry(selectedSceneryEntry.EntryIndex);
            price = sceneryEntry->small_scenery.price * 10;
            break;
        case SCENERY_TYPE_PATH_ITEM:
            sceneryEntry = get_footpath_item_entry(selectedSceneryEntry.EntryIndex);
            price = sceneryEntry->path_bit.price;
            break;
        case SCENERY_TYPE_WALL:
            sceneryEntry = get_wall_entry(selectedSceneryEntry.EntryIndex);
            price = sceneryEntry->wall.price;
            break;
        case SCENERY_TYPE_LARGE:
            sceneryEntry = get_large_scenery_entry(selectedSceneryEntry.EntryIndex);
            price = sceneryEntry->large_scenery.price * 10;
            break;
        case SCENERY_TYPE_BANNER:
            sceneryEntry = get_banner_entry(selectedSceneryEntry.EntryIndex);
            price = sceneryEntry->banner.price;
            break;
    }

    if (w->scenery.SelectedScenery.IsUndefined() && gSceneryPlaceCost != MONEY32_UNDEFINED)
    {
        price = gSceneryPlaceCost;
    }

    if (!(gParkFlags & PARK_FLAGS_NO_MONEY))
    {
        auto ft = Formatter::Common();
        ft.Add<uint32_t>(price);

        // -14
        gfx_draw_string_right(
            dpi, STR_COST_LABEL, gCommonFormatArgs, COLOUR_BLACK,
            w->windowPos + ScreenCoordsXY{ w->width - 0x1A, w->height - 13 });
    }

    auto ft = Formatter::Common();
    ft.Add<rct_string_id>(sceneryEntry != nullptr ? sceneryEntry->name : static_cast<rct_string_id>(STR_UNKNOWN_OBJECT_TYPE));
    gfx_draw_string_left_clipped(
        dpi, STR_BLACK_STRING, gCommonFormatArgs, COLOUR_BLACK, { w->windowPos.x + 3, w->windowPos.y + w->height - 13 },
        w->width - 19);
}

/**
 *
 *  rct2: 0x006E15ED
 */
void window_scenery_scrollpaint(rct_window* w, rct_drawpixelinfo* dpi, int32_t scrollIndex)
{
    gfx_clear(dpi, ColourMapA[w->colours[1]].mid_light);

    uint8_t tabIndex = gWindowSceneryActiveTabIndex;

    int32_t sceneryTabItemIndex = 0;
    ScenerySelection currentSceneryGlobal = ScenerySelection::CreateUndefined();
    int16_t left = 0, top = 0;

    while ((currentSceneryGlobal = window_scenery_tab_entries[tabIndex][sceneryTabItemIndex]),
           !currentSceneryGlobal.IsUndefined())
    {
        ScenerySelection tabSelectedScenery = gWindowSceneryTabSelections[tabIndex];

        if (gWindowSceneryPaintEnabled == 1 || gWindowSceneryEyedropperEnabled)
        {
            if (w->scenery.SelectedScenery == currentSceneryGlobal)
            {
                gfx_fill_rect_inset(
                    dpi, left, top, left + SCENERY_BUTTON_WIDTH - 1, top + SCENERY_BUTTON_HEIGHT - 1, w->colours[1],
                    INSET_RECT_FLAG_FILL_MID_LIGHT);
            }
        }
        else
        {
            if (tabSelectedScenery == currentSceneryGlobal)
            {
                gfx_fill_rect_inset(
                    dpi, left, top, left + SCENERY_BUTTON_WIDTH - 1, top + SCENERY_BUTTON_HEIGHT - 1, w->colours[1],
                    (INSET_RECT_FLAG_BORDER_INSET | INSET_RECT_FLAG_FILL_MID_LIGHT));
            }
            else if (w->scenery.SelectedScenery == currentSceneryGlobal)
            {
                gfx_fill_rect_inset(
                    dpi, left, top, left + SCENERY_BUTTON_WIDTH - 1, top + SCENERY_BUTTON_HEIGHT - 1, w->colours[1],
                    INSET_RECT_FLAG_FILL_MID_LIGHT);
            }
        }

        rct_scenery_entry* sceneryEntry;
        rct_drawpixelinfo clipdpi;
        if (clip_drawpixelinfo(&clipdpi, dpi, left + 1, top + 1, SCENERY_BUTTON_WIDTH - 2, SCENERY_BUTTON_HEIGHT - 2))
        {
            if (currentSceneryGlobal.SceneryType == SCENERY_TYPE_BANNER)
            {
                sceneryEntry = get_banner_entry(currentSceneryGlobal.EntryIndex);
                uint32_t imageId = sceneryEntry->image + gWindowSceneryRotation * 2;
                imageId |= (gWindowSceneryPrimaryColour << 19) | IMAGE_TYPE_REMAP;

                gfx_draw_sprite(&clipdpi, imageId, 0x21, 0x28, w->colours[1]);
                gfx_draw_sprite(&clipdpi, imageId + 1, 0x21, 0x28, w->colours[1]);
            }
            else if (currentSceneryGlobal.SceneryType == SCENERY_TYPE_LARGE)
            {
                sceneryEntry = get_large_scenery_entry(currentSceneryGlobal.EntryIndex);
                uint32_t imageId = sceneryEntry->image + gWindowSceneryRotation;
                imageId |= (gWindowSceneryPrimaryColour << 19) | IMAGE_TYPE_REMAP;
                imageId |= (gWindowScenerySecondaryColour << 24) | IMAGE_TYPE_REMAP_2_PLUS;

                gfx_draw_sprite(&clipdpi, imageId, 0x21, 0, w->colours[1]);
            }
            else if (currentSceneryGlobal.SceneryType == SCENERY_TYPE_WALL)
            {
                sceneryEntry = get_wall_entry(currentSceneryGlobal.EntryIndex);
                uint32_t imageId = sceneryEntry->image;
                uint8_t tertiaryColour = w->colours[1];
                uint16_t spriteTop = (sceneryEntry->wall.height * 2) + 0x32;

                if (sceneryEntry->wall.flags & WALL_SCENERY_HAS_GLASS)
                {
                    imageId |= (gWindowSceneryPrimaryColour << 19) | IMAGE_TYPE_REMAP;

                    if (sceneryEntry->wall.flags & WALL_SCENERY_HAS_SECONDARY_COLOUR)
                    {
                        imageId |= (gWindowScenerySecondaryColour << 24) | IMAGE_TYPE_REMAP_2_PLUS;
                    }
                    gfx_draw_sprite(&clipdpi, imageId, 0x2F, spriteTop, tertiaryColour);

                    imageId = (sceneryEntry->image + 0x40000006) | (GlassPaletteIds[gWindowSceneryPrimaryColour] << 19);
                    gfx_draw_sprite(&clipdpi, imageId, 0x2F, spriteTop, tertiaryColour);
                }
                else
                {
                    imageId |= (gWindowSceneryPrimaryColour << 19) | IMAGE_TYPE_REMAP;

                    if (sceneryEntry->wall.flags & WALL_SCENERY_HAS_SECONDARY_COLOUR)
                    {
                        imageId |= (gWindowScenerySecondaryColour << 24) | IMAGE_TYPE_REMAP_2_PLUS;

                        if (sceneryEntry->wall.flags & WALL_SCENERY_HAS_TERNARY_COLOUR)
                        {
                            imageId &= 0xDFFFFFFF;
                            tertiaryColour = gWindowSceneryTertiaryColour;
                        }
                    }
                    gfx_draw_sprite(&clipdpi, imageId, 0x2F, spriteTop, tertiaryColour);

                    if (sceneryEntry->wall.flags & WALL_SCENERY_IS_DOOR)
                    {
                        gfx_draw_sprite(&clipdpi, imageId + 1, 0x2F, spriteTop, tertiaryColour);
                    }
                }
            }
            else if (currentSceneryGlobal.SceneryType == SCENERY_TYPE_PATH_ITEM)
            {
                sceneryEntry = get_footpath_item_entry(currentSceneryGlobal.EntryIndex);
                uint32_t imageId = sceneryEntry->image;

                gfx_draw_sprite(&clipdpi, imageId, 0x0B, 0x10, w->colours[1]);
            }
            else
            {
                sceneryEntry = get_small_scenery_entry(currentSceneryGlobal.EntryIndex);
                uint32_t imageId = sceneryEntry->image + gWindowSceneryRotation;

                if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_HAS_PRIMARY_COLOUR))
                {
                    imageId |= (gWindowSceneryPrimaryColour << 19) | IMAGE_TYPE_REMAP;

                    if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_HAS_SECONDARY_COLOUR))
                    {
                        imageId |= (gWindowScenerySecondaryColour << 24) | IMAGE_TYPE_REMAP_2_PLUS;
                    }
                }

                uint16_t spriteTop = (sceneryEntry->small_scenery.height / 4) + 0x2B;

                if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_FULL_TILE)
                    && scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_VOFFSET_CENTRE))
                {
                    spriteTop -= 0x0C;
                }

                gfx_draw_sprite(&clipdpi, imageId, 0x20, spriteTop, w->colours[1]);

                if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_HAS_GLASS))
                {
                    imageId = ((sceneryEntry->image + gWindowSceneryRotation) + 0x40000004)
                        + (GlassPaletteIds[gWindowSceneryPrimaryColour] << 19);

                    gfx_draw_sprite(&clipdpi, imageId, 0x20, spriteTop, w->colours[1]);
                }

                if (scenery_small_entry_has_flag(sceneryEntry, SMALL_SCENERY_FLAG_ANIMATED_FG))
                {
                    imageId = (sceneryEntry->image + gWindowSceneryRotation) + 4;
                    gfx_draw_sprite(&clipdpi, imageId, 0x20, spriteTop, w->colours[1]);
                }
            }
        }

        left += SCENERY_BUTTON_WIDTH;
        if (left >= 594)
        {
            top += SCENERY_BUTTON_HEIGHT;
            left = 0;
        }
        sceneryTabItemIndex++;
    }
}

static int32_t window_scenery_find_tab_with_scenery(const ScenerySelection& scenery)
{
    for (int32_t i = 0; i < SCENERY_WINDOW_TABS; i++)
    {
        for (int32_t j = 0; j < SCENERY_ENTRIES_PER_TAB; j++)
        {
            ScenerySelection entry = window_scenery_tab_entries[i][j];
            if (entry.IsUndefined())
                break;
            if (entry == scenery)
                return i;
        }
    }
    return -1;
}

bool window_scenery_set_selected_item(const ScenerySelection& scenery)
{
    bool result = false;
    rct_window* w = window_bring_to_front_by_class(WC_SCENERY);
    if (w != nullptr)
    {
        int32_t tabIndex = window_scenery_find_tab_with_scenery(scenery);
        if (tabIndex != -1)
        {
            gWindowSceneryActiveTabIndex = tabIndex;
            gWindowSceneryTabSelections[tabIndex] = scenery;

            audio_play_sound(SoundId::Click1, 0, context_get_width() / 2);
            w->scenery.hover_counter = -16;
            gSceneryPlaceCost = MONEY32_UNDEFINED;
            w->Invalidate();
            result = true;
        }
    }
    return result;
}

// Used after removing objects, in order to avoid crashes.
void window_scenery_reset_selected_scenery_items()
{
    for (size_t i = 0; i < SCENERY_WINDOW_TABS; i++)
    {
        gWindowSceneryTabSelections[i].SetUndefined();
    }
}
