/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Sprite.h"

#include "../Cheats.h"
#include "../Game.h"
#include "../OpenRCT2.h"
#include "../audio/audio.h"
#include "../core/Crypt.h"
#include "../core/Guard.hpp"
#include "../interface/Viewport.h"
#include "../localisation/Date.h"
#include "../localisation/Localisation.h"
#include "../scenario/Scenario.h"
#include "Fountain.h"

#include <algorithm>
#include <cmath>
#include <iterator>

uint16_t gSpriteListHead[SPRITE_LIST_COUNT];
uint16_t gSpriteListCount[SPRITE_LIST_COUNT];
static rct_sprite _spriteList[MAX_SPRITES];

static bool _spriteFlashingList[MAX_SPRITES];

uint16_t gSpriteSpatialIndex[SPATIAL_INDEX_SIZE];

const rct_string_id litterNames[12] = { STR_LITTER_VOMIT,
                                        STR_LITTER_VOMIT,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_CAN,
                                        STR_SHOP_ITEM_SINGULAR_RUBBISH,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_BURGER_BOX,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_CUP,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_BOX,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_BOTTLE,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_BOWL_RED,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_DRINK_CARTON,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_JUICE_CUP,
                                        STR_SHOP_ITEM_SINGULAR_EMPTY_BOWL_BLUE };

static CoordsXYZ _spritelocations1[MAX_SPRITES];
static CoordsXYZ _spritelocations2[MAX_SPRITES];

static size_t GetSpatialIndexOffset(int32_t x, int32_t y);
static void move_sprite_to_list(SpriteBase* sprite, SPRITE_LIST newListIndex);

// Required for GetEntity to return a default
template<> bool SpriteBase::Is<SpriteBase>() const
{
    return true;
}

template<> bool SpriteBase::Is<Litter>() const
{
    return sprite_identifier == SPRITE_IDENTIFIER_LITTER;
}

template<> bool SpriteBase::Is<SteamParticle>() const
{
    return sprite_identifier == SPRITE_IDENTIFIER_MISC && type == SPRITE_MISC_STEAM_PARTICLE;
}

template<> bool SpriteBase::Is<ExplosionFlare>() const
{
    return sprite_identifier == SPRITE_IDENTIFIER_MISC && type == SPRITE_MISC_EXPLOSION_FLARE;
}

template<> bool SpriteBase::Is<ExplosionCloud>() const
{
    return sprite_identifier == SPRITE_IDENTIFIER_MISC && type == SPRITE_MISC_EXPLOSION_CLOUD;
}

std::string rct_sprite_checksum::ToString() const
{
    std::string result;

    result.reserve(raw.size() * 2);
    for (auto b : raw)
    {
        char buf[3];
        snprintf(buf, 3, "%02x", b);
        result.append(buf);
    }

    return result;
}

rct_sprite* try_get_sprite(size_t spriteIndex)
{
    rct_sprite* sprite = nullptr;
    if (spriteIndex < MAX_SPRITES)
    {
        sprite = &_spriteList[spriteIndex];
    }
    return sprite;
}

rct_sprite* get_sprite(size_t sprite_idx)
{
    if (sprite_idx == SPRITE_INDEX_NULL)
    {
        return nullptr;
    }
    openrct2_assert(sprite_idx < MAX_SPRITES, "Tried getting sprite %u", sprite_idx);
    if (sprite_idx >= MAX_SPRITES)
    {
        return nullptr;
    }
    return &_spriteList[sprite_idx];
}

SpriteBase* GetEntity(size_t sprite_idx)
{
    return GetEntity<SpriteBase>(sprite_idx);
}

uint16_t sprite_get_first_in_quadrant(int32_t x, int32_t y)
{
    return gSpriteSpatialIndex[GetSpatialIndexOffset(x, y)];
}

static void invalidate_sprite_max_zoom(SpriteBase* sprite, int32_t maxZoom)
{
    if (sprite->sprite_left == LOCATION_NULL)
        return;

    for (int32_t i = 0; i < MAX_VIEWPORT_COUNT; i++)
    {
        rct_viewport* viewport = &g_viewport_list[i];
        if (viewport->width != 0 && viewport->zoom <= maxZoom)
        {
            viewport_invalidate(viewport, sprite->sprite_left, sprite->sprite_top, sprite->sprite_right, sprite->sprite_bottom);
        }
    }
}

/**
 * Invalidate the sprite if at closest zoom.
 *  rct2: 0x006EC60B
 */
void SpriteBase::Invalidate0()
{
    invalidate_sprite_max_zoom(this, 0);
}

/**
 * Invalidate sprite if at closest zoom or next zoom up from closest.
 *  rct2: 0x006EC53F
 */
void SpriteBase::Invalidate1()
{
    invalidate_sprite_max_zoom(this, 1);
}

/**
 * Invalidate sprite if not at furthest zoom.
 *  rct2: 0x006EC473
 */
void SpriteBase::Invalidate2()
{
    invalidate_sprite_max_zoom(this, 2);
}

/**
 *
 *  rct2: 0x0069EB13
 */
void reset_sprite_list()
{
    gSavedAge = 0;
    std::memset(static_cast<void*>(_spriteList), 0, sizeof(_spriteList));

    for (int32_t i = 0; i < SPRITE_LIST_COUNT; i++)
    {
        gSpriteListHead[i] = SPRITE_INDEX_NULL;
        gSpriteListCount[i] = 0;
        _spriteFlashingList[i] = false;
    }

    SpriteBase* previous_spr = nullptr;

    for (int32_t i = 0; i < MAX_SPRITES; ++i)
    {
        auto* spr = GetEntity(i);
        spr->sprite_identifier = SPRITE_IDENTIFIER_NULL;
        spr->sprite_index = i;
        spr->next = SPRITE_INDEX_NULL;
        spr->linked_list_index = SPRITE_LIST_FREE;

        if (previous_spr != nullptr)
        {
            spr->previous = previous_spr->sprite_index;
            previous_spr->next = i;
        }
        else
        {
            spr->previous = SPRITE_INDEX_NULL;
            gSpriteListHead[SPRITE_LIST_FREE] = i;
        }
        _spriteFlashingList[i] = false;
        previous_spr = spr;
    }

    gSpriteListCount[SPRITE_LIST_FREE] = MAX_SPRITES;

    reset_sprite_spatial_index();
}

/**
 *
 *  rct2: 0x0069EBE4
 * This function looks as though it sets some sort of order for sprites.
 * Sprites can share their position if this is the case.
 */
void reset_sprite_spatial_index()
{
    std::fill_n(gSpriteSpatialIndex, std::size(gSpriteSpatialIndex), SPRITE_INDEX_NULL);
    for (size_t i = 0; i < MAX_SPRITES; i++)
    {
        auto* spr = GetEntity(i);
        if (spr->sprite_identifier != SPRITE_IDENTIFIER_NULL)
        {
            size_t index = GetSpatialIndexOffset(spr->x, spr->y);
            uint32_t nextSpriteId = gSpriteSpatialIndex[index];
            gSpriteSpatialIndex[index] = spr->sprite_index;
            spr->next_in_quadrant = nextSpriteId;
        }
    }
}

static size_t GetSpatialIndexOffset(int32_t x, int32_t y)
{
    size_t index = SPATIAL_INDEX_LOCATION_NULL;
    if (x != LOCATION_NULL)
    {
        x = std::clamp(x, 0, 0xFFFF);
        y = std::clamp(y, 0, 0xFFFF);

        int16_t flooredX = floor2(x, 32);
        uint8_t tileY = y >> 5;
        index = (flooredX << 3) | tileY;
    }

    if (index >= sizeof(gSpriteSpatialIndex))
    {
        return SPATIAL_INDEX_LOCATION_NULL;
    }
    return index;
}

#ifndef DISABLE_NETWORK

rct_sprite_checksum sprite_checksum()
{
    using namespace Crypt;

    // TODO Remove statics, should be one of these per sprite manager / OpenRCT2 context.
    //      Alternatively, make a new class for this functionality.
    static std::unique_ptr<HashAlgorithm<20>> _spriteHashAlg;

    rct_sprite_checksum checksum;

    try
    {
        if (_spriteHashAlg == nullptr)
        {
            _spriteHashAlg = CreateSHA1();
        }

        _spriteHashAlg->Clear();
        for (size_t i = 0; i < MAX_SPRITES; i++)
        {
            // TODO create a way to copy only the specific type
            auto sprite = get_sprite(i);
            if (sprite->generic.sprite_identifier != SPRITE_IDENTIFIER_NULL
                && sprite->generic.sprite_identifier != SPRITE_IDENTIFIER_MISC)
            {
                auto copy = *sprite;

                // Only required for rendering/invalidation, has no meaning to the game state.
                copy.generic.sprite_left = copy.generic.sprite_right = copy.generic.sprite_top = copy.generic.sprite_bottom = 0;
                copy.generic.sprite_width = copy.generic.sprite_height_negative = copy.generic.sprite_height_positive = 0;

                // Next in quadrant might be a misc sprite, set first non-misc sprite in quadrant.
                while (auto* nextSprite = GetEntity(copy.generic.next_in_quadrant))
                {
                    if (nextSprite->sprite_identifier == SPRITE_IDENTIFIER_MISC)
                        copy.generic.next_in_quadrant = nextSprite->next_in_quadrant;
                    else
                        break;
                }

                if (copy.generic.Is<Peep>())
                {
                    // Name is pointer and will not be the same across clients
                    copy.peep.Name = {};

                    // We set this to 0 because as soon the client selects a guest the window will remove the
                    // invalidation flags causing the sprite checksum to be different than on server, the flag does not affect
                    // game state.
                    copy.peep.WindowInvalidateFlags = 0;
                }

                _spriteHashAlg->Update(&copy, sizeof(copy));
            }
        }

        checksum.raw = _spriteHashAlg->Finish();
    }
    catch (std::exception& e)
    {
        log_error("sprite_checksum failed: %s", e.what());
        throw;
    }

    return checksum;
}
#else

rct_sprite_checksum sprite_checksum()
{
    return rct_sprite_checksum{};
}

#endif // DISABLE_NETWORK

static void sprite_reset(SpriteBase* sprite)
{
    // Need to retain how the sprite is linked in lists
    uint8_t llto = sprite->linked_list_index;
    uint16_t next = sprite->next;
    uint16_t next_in_quadrant = sprite->next_in_quadrant;
    uint16_t prev = sprite->previous;
    uint16_t sprite_index = sprite->sprite_index;
    _spriteFlashingList[sprite_index] = false;

    std::memset(sprite, 0, sizeof(rct_sprite));

    sprite->linked_list_index = llto;
    sprite->next = next;
    sprite->next_in_quadrant = next_in_quadrant;
    sprite->previous = prev;
    sprite->sprite_index = sprite_index;
    sprite->sprite_identifier = SPRITE_IDENTIFIER_NULL;
}

/**
 * Clears all the unused sprite memory to zero. Probably so that it can be compressed better when saving.
 *  rct2: 0x0069EBA4
 */
void sprite_clear_all_unused()
{
    for (auto sprite : EntityList(SPRITE_LIST_FREE))
    {
        sprite_reset(sprite);
        sprite->linked_list_index = SPRITE_LIST_FREE;

        // sprite->next_in_quadrant will only end up as zero owing to corruption
        // most likely due to previous builds not preserving it when resetting sprites
        // We reset it to SPRITE_INDEX_NULL to prevent cycles in the sprite lists
        if (sprite->next_in_quadrant == 0)
        {
            sprite->next_in_quadrant = SPRITE_INDEX_NULL;
        }
        _spriteFlashingList[sprite->sprite_index] = false;
    }
}

static void SpriteSpatialInsert(SpriteBase* sprite, const CoordsXY& newLoc);

static constexpr uint16_t MAX_MISC_SPRITES = 300;

rct_sprite* create_sprite(SPRITE_IDENTIFIER spriteIdentifier, SPRITE_LIST linkedListIndex)
{
    if (gSpriteListCount[SPRITE_LIST_FREE] == 0)
    {
        // No free sprites.
        return nullptr;
    }

    if (linkedListIndex == SPRITE_LIST_MISC)
    {
        // Misc sprites are commonly used for effects, if there are less than MAX_MISC_SPRITES
        // free it will fail to keep slots for more relevant sprites.
        // Also there can't be more than MAX_MISC_SPRITES sprites in this list.
        uint16_t miscSlotsRemaining = MAX_MISC_SPRITES - gSpriteListCount[SPRITE_LIST_MISC];
        if (miscSlotsRemaining >= gSpriteListCount[SPRITE_LIST_FREE])
        {
            return nullptr;
        }
    }

    auto* sprite = GetEntity(gSpriteListHead[SPRITE_LIST_FREE]);

    move_sprite_to_list(sprite, linkedListIndex);

    // Need to reset all sprite data, as the uninitialised values
    // may contain garbage and cause a desync later on.
    sprite_reset(sprite);

    sprite->x = LOCATION_NULL;
    sprite->y = LOCATION_NULL;
    sprite->z = 0;
    sprite->sprite_width = 0x10;
    sprite->sprite_height_negative = 0x14;
    sprite->sprite_height_positive = 0x8;
    sprite->flags = 0;
    sprite->sprite_left = LOCATION_NULL;

    SpriteSpatialInsert(sprite, { LOCATION_NULL, 0 });

    return reinterpret_cast<rct_sprite*>(sprite);
}

rct_sprite* create_sprite(SPRITE_IDENTIFIER spriteIdentifier)
{
    SPRITE_LIST linkedListIndex = SPRITE_LIST_FREE;
    switch (spriteIdentifier)
    {
        case SPRITE_IDENTIFIER_VEHICLE:
            linkedListIndex = SPRITE_LIST_VEHICLE;
            break;
        case SPRITE_IDENTIFIER_PEEP:
            linkedListIndex = SPRITE_LIST_PEEP;
            break;
        case SPRITE_IDENTIFIER_MISC:
            linkedListIndex = SPRITE_LIST_MISC;
            break;
        case SPRITE_IDENTIFIER_LITTER:
            linkedListIndex = SPRITE_LIST_LITTER;
            break;
        default:
            Guard::Assert(false, "Invalid sprite identifier: 0x%02X", spriteIdentifier);
            return nullptr;
    }
    return create_sprite(spriteIdentifier, linkedListIndex);
}

/*
 * rct2: 0x0069ED0B
 * This function moves a sprite to the specified sprite linked list.
 * The game uses this list to categorise sprites by type.
 */
static void move_sprite_to_list(SpriteBase* sprite, SPRITE_LIST newListIndex)
{
    int32_t oldListIndex = sprite->linked_list_index;

    // No need to move if the sprite is already in the desired list
    if (oldListIndex == newListIndex)
    {
        return;
    }

    // If the sprite is currently the head of the list, the
    // sprite following this one becomes the new head of the list.
    if (sprite->previous == SPRITE_INDEX_NULL)
    {
        gSpriteListHead[oldListIndex] = sprite->next;
    }
    else
    {
        // Hook up sprite->previous->next to sprite->next, removing the sprite from its old list
        GetEntity(sprite->previous)->next = sprite->next;
    }

    // Similarly, hook up sprite->next->previous to sprite->previous
    if (sprite->next != SPRITE_INDEX_NULL)
    {
        GetEntity(sprite->next)->previous = sprite->previous;
    }

    sprite->previous = SPRITE_INDEX_NULL; // We become the new head of the target list, so there's no previous sprite
    sprite->linked_list_index = newListIndex;

    sprite->next = gSpriteListHead[newListIndex];         // This sprite's next sprite is the old head, since we're the new head
    gSpriteListHead[newListIndex] = sprite->sprite_index; // Store this sprite's index as head of its new list

    if (sprite->next != SPRITE_INDEX_NULL)
    {
        // Fix the chain by settings sprite->next->previous to sprite_index
        GetEntity(sprite->next)->previous = sprite->sprite_index;
    }

    // These globals are probably counters for each sprite list?
    // Decrement old list counter, increment new list counter.
    gSpriteListCount[oldListIndex]--;
    gSpriteListCount[newListIndex]++;
}

/**
 *
 *  rct2: 0x00673200
 */
static void sprite_steam_particle_update(SteamParticle* steam)
{
    steam->Invalidate2();

    // Move up 1 z every 3 ticks (Starts after 4 ticks)
    steam->time_to_move++;
    if (steam->time_to_move >= 4)
    {
        steam->time_to_move = 1;
        steam->MoveTo({ steam->x, steam->y, steam->z + 1 });
    }
    steam->frame += 64;
    if (steam->frame >= (56 * 64))
    {
        sprite_remove(steam);
    }
}

/**
 *
 *  rct2: 0x0067363D
 */
void sprite_misc_explosion_cloud_create(int32_t x, int32_t y, int32_t z)
{
    SpriteGeneric* sprite = &create_sprite(SPRITE_IDENTIFIER_MISC)->generic;
    if (sprite != nullptr)
    {
        sprite->sprite_width = 44;
        sprite->sprite_height_negative = 32;
        sprite->sprite_height_positive = 34;
        sprite->sprite_identifier = SPRITE_IDENTIFIER_MISC;
        sprite->MoveTo({ x, y, z + 4 });
        sprite->type = SPRITE_MISC_EXPLOSION_CLOUD;
        sprite->frame = 0;
    }
}

/**
 *
 *  rct2: 0x00673385
 */
static void sprite_misc_explosion_cloud_update(rct_sprite* sprite)
{
    sprite->generic.Invalidate2();
    sprite->generic.frame += 128;
    if (sprite->generic.frame >= (36 * 128))
    {
        sprite_remove(&sprite->generic);
    }
}

/**
 *
 *  rct2: 0x0067366B
 */
void sprite_misc_explosion_flare_create(int32_t x, int32_t y, int32_t z)
{
    SpriteGeneric* sprite = &create_sprite(SPRITE_IDENTIFIER_MISC)->generic;
    if (sprite != nullptr)
    {
        sprite->sprite_width = 25;
        sprite->sprite_height_negative = 85;
        sprite->sprite_height_positive = 8;
        sprite->sprite_identifier = SPRITE_IDENTIFIER_MISC;
        sprite->MoveTo({ x, y, z + 4 });
        sprite->type = SPRITE_MISC_EXPLOSION_FLARE;
        sprite->frame = 0;
    }
}

/**
 *
 *  rct2: 0x006733B4
 */
static void sprite_misc_explosion_flare_update(rct_sprite* sprite)
{
    sprite->generic.Invalidate2();
    sprite->generic.frame += 64;
    if (sprite->generic.frame >= (124 * 64))
    {
        sprite_remove(&sprite->generic);
    }
}

/**
 *
 *  rct2: 0x006731CD
 */
static void sprite_misc_update(rct_sprite* sprite)
{
    switch (sprite->generic.type)
    {
        case SPRITE_MISC_STEAM_PARTICLE:
            sprite_steam_particle_update(reinterpret_cast<SteamParticle*>(sprite));
            break;
        case SPRITE_MISC_MONEY_EFFECT:
            sprite->money_effect.Update();
            break;
        case SPRITE_MISC_CRASHED_VEHICLE_PARTICLE:
            crashed_vehicle_particle_update(reinterpret_cast<VehicleCrashParticle*>(sprite));
            break;
        case SPRITE_MISC_EXPLOSION_CLOUD:
            sprite_misc_explosion_cloud_update(sprite);
            break;
        case SPRITE_MISC_CRASH_SPLASH:
            crash_splash_update(reinterpret_cast<CrashSplashParticle*>(sprite));
            break;
        case SPRITE_MISC_EXPLOSION_FLARE:
            sprite_misc_explosion_flare_update(sprite);
            break;
        case SPRITE_MISC_JUMPING_FOUNTAIN_WATER:
        case SPRITE_MISC_JUMPING_FOUNTAIN_SNOW:
            sprite->jumping_fountain.Update();
            break;
        case SPRITE_MISC_BALLOON:
            balloon_update(&sprite->balloon);
            break;
        case SPRITE_MISC_DUCK:
            duck_update(&sprite->duck);
            break;
    }
}

/**
 *
 *  rct2: 0x00672AA4
 */
void sprite_misc_update_all()
{
    for (auto entity : EntityList(SPRITE_LIST_MISC))
    {
        // TODO: Use more specific Sprite class
        sprite_misc_update(reinterpret_cast<rct_sprite*>(entity));
    }
}

// Performs a search to ensure that insert keeps next_in_quadrant in sprite_index order
static void SpriteSpatialInsert(SpriteBase* sprite, const CoordsXY& newLoc)
{
    size_t newIndex = GetSpatialIndexOffset(newLoc.x, newLoc.y);

    auto* next = &gSpriteSpatialIndex[newIndex];
    while (sprite->sprite_index < *next && *next != SPRITE_INDEX_NULL)
    {
        auto sprite2 = GetEntity(*next);
        next = &sprite2->next_in_quadrant;
    }

    sprite->next_in_quadrant = *next;
    *next = sprite->sprite_index;
}

static void SpriteSpatialRemove(SpriteBase* sprite)
{
    size_t currentIndex = GetSpatialIndexOffset(sprite->x, sprite->y);
    auto* index = &gSpriteSpatialIndex[currentIndex];

    // This indicates that the spatial index data is incorrect.
    if (*index == SPRITE_INDEX_NULL)
    {
        log_warning("Bad sprite spatial index. Rebuilding the spatial index...");
        reset_sprite_spatial_index();
    }

    auto* sprite2 = GetEntity(*index);
    while (sprite != sprite2)
    {
        index = &sprite2->next_in_quadrant;
        if (*index == SPRITE_INDEX_NULL)
        {
            break;
        }
        sprite2 = GetEntity(*index);
    }
    *index = sprite->next_in_quadrant;
}

static void SpriteSpatialMove(SpriteBase* sprite, const CoordsXY& newLoc)
{
    size_t newIndex = GetSpatialIndexOffset(newLoc.x, newLoc.y);
    size_t currentIndex = GetSpatialIndexOffset(sprite->x, sprite->y);
    if (newIndex == currentIndex)
        return;

    SpriteSpatialRemove(sprite);
    SpriteSpatialInsert(sprite, newLoc);
}

/**
 * Moves a sprite to a new location.
 *  rct2: 0x0069E9D3
 *
 * @param x (ax)
 * @param y (cx)
 * @param z (dx)
 * @param sprite (esi)
 */
void SpriteBase::MoveTo(const CoordsXYZ& newLocation)
{
    auto loc = newLocation;
    if (!map_is_location_valid(loc))
    {
        loc.x = LOCATION_NULL;
    }

    SpriteSpatialMove(this, loc);

    if (loc.x == LOCATION_NULL)
    {
        sprite_left = LOCATION_NULL;
        x = loc.x;
        y = loc.y;
        z = loc.z;
    }
    else
    {
        sprite_set_coordinates(loc.x, loc.y, loc.z, this);
    }
}

void sprite_set_coordinates(int16_t x, int16_t y, int16_t z, SpriteBase* sprite)
{
    CoordsXYZ coords3d = { x, y, z };
    auto screenCoords = translate_3d_to_2d_with_z(get_current_rotation(), coords3d);

    sprite->sprite_left = screenCoords.x - sprite->sprite_width;
    sprite->sprite_right = screenCoords.x + sprite->sprite_width;
    sprite->sprite_top = screenCoords.y - sprite->sprite_height_negative;
    sprite->sprite_bottom = screenCoords.y + sprite->sprite_height_positive;
    sprite->x = x;
    sprite->y = y;
    sprite->z = z;
}

/**
 *
 *  rct2: 0x0069EDB6
 */
void sprite_remove(SpriteBase* sprite)
{
    auto peep = sprite->As<Peep>();
    if (peep != nullptr)
    {
        peep->SetName({});
    }

    move_sprite_to_list(sprite, SPRITE_LIST_FREE);
    sprite->sprite_identifier = SPRITE_IDENTIFIER_NULL;
    _spriteFlashingList[sprite->sprite_index] = false;

    size_t quadrantIndex = GetSpatialIndexOffset(sprite->x, sprite->y);
    uint16_t* spriteIndex = &gSpriteSpatialIndex[quadrantIndex];
    SpriteBase* quadrantSprite;
    while (*spriteIndex != SPRITE_INDEX_NULL && (quadrantSprite = GetEntity(*spriteIndex)) != sprite)
    {
        spriteIndex = &quadrantSprite->next_in_quadrant;
    }
    *spriteIndex = sprite->next_in_quadrant;
}

static bool litter_can_be_at(int32_t x, int32_t y, int32_t z)
{
    TileElement* tileElement;

    if (!map_is_location_owned({ x, y, z }))
        return false;

    tileElement = map_get_first_element_at({ x, y });
    if (tileElement == nullptr)
        return false;
    do
    {
        if (tileElement->GetType() != TILE_ELEMENT_TYPE_PATH)
            continue;

        int32_t pathZ = tileElement->GetBaseZ();
        if (pathZ < z || pathZ >= z + 32)
            continue;

        return !tile_element_is_underground(tileElement);
    } while (!(tileElement++)->IsLastForTile());
    return false;
}

/**
 *
 *  rct2: 0x0067375D
 */
void litter_create(int32_t x, int32_t y, int32_t z, int32_t direction, int32_t type)
{
    if (gCheatsDisableLittering)
        return;

    x += CoordsDirectionDelta[direction >> 3].x / 8;
    y += CoordsDirectionDelta[direction >> 3].y / 8;

    if (!litter_can_be_at(x, y, z))
        return;

    if (gSpriteListCount[SPRITE_LIST_LITTER] >= 500)
    {
        Litter* newestLitter = nullptr;
        uint32_t newestLitterCreationTick = 0;
        for (auto litter : EntityList<Litter>(SPRITE_LIST_LITTER))
        {
            if (newestLitterCreationTick <= litter->creationTick)
            {
                newestLitterCreationTick = litter->creationTick;
                newestLitter = litter;
            }
        }

        if (newestLitter != nullptr)
        {
            newestLitter->Invalidate0();
            sprite_remove(newestLitter);
        }
    }

    Litter* litter = reinterpret_cast<Litter*>(create_sprite(SPRITE_IDENTIFIER_LITTER));
    if (litter == nullptr)
        return;

    litter->sprite_direction = direction;
    litter->sprite_width = 6;
    litter->sprite_height_negative = 6;
    litter->sprite_height_positive = 3;
    litter->sprite_identifier = SPRITE_IDENTIFIER_LITTER;
    litter->type = type;
    litter->MoveTo({ x, y, z });
    litter->Invalidate0();
    litter->creationTick = gScenarioTicks;
}

/**
 *
 *  rct2: 0x006738E1
 */
void litter_remove_at(int32_t x, int32_t y, int32_t z)
{
    for (auto litter : EntityTileList<Litter>({ x, y }))
    {
        if (abs(litter->z - z) <= 16)
        {
            if (abs(litter->x - x) <= 8 && abs(litter->y - y) <= 8)
            {
                litter->Invalidate0();
                sprite_remove(litter);
            }
        }
    }
}

/**
 * Loops through all sprites, finds floating objects and removes them.
 * Returns the amount of removed objects as feedback.
 */
uint16_t remove_floating_sprites()
{
    uint16_t removed = 0;
    for (uint16_t i = 0; i < MAX_SPRITES; i++)
    {
        auto* entity = GetEntity(i);
        if (entity->Is<Balloon>())
        {
            sprite_remove(entity);
            removed++;
        }
        else if (entity->Is<Duck>())
        {
            auto* duck = entity->As<Duck>();
            if (duck->IsFlying())
            {
                duck->Remove();
                removed++;
            }
        }
        else if (entity->Is<MoneyEffect>())
        {
            sprite_remove(entity);
            removed++;
        }
    }
    return removed;
}

/**
 * Determines whether it's worth tweening a sprite or not when frame smoothing is on.
 */
static bool sprite_should_tween(SpriteBase* sprite)
{
    switch (sprite->sprite_identifier)
    {
        case SPRITE_IDENTIFIER_PEEP:
        case SPRITE_IDENTIFIER_VEHICLE:
            return true;
    }
    return false;
}

static void store_sprite_locations(CoordsXYZ* sprite_locations)
{
    for (uint16_t i = 0; i < MAX_SPRITES; i++)
    {
        // skip going through `get_sprite` to not get stalled on assert,
        // this can get very expensive for busy parks with uncap FPS option on
        const rct_sprite* sprite = &_spriteList[i];
        sprite_locations[i].x = sprite->generic.x;
        sprite_locations[i].y = sprite->generic.y;
        sprite_locations[i].z = sprite->generic.z;
    }
}

void sprite_position_tween_store_a()
{
    store_sprite_locations(_spritelocations1);
}

void sprite_position_tween_store_b()
{
    store_sprite_locations(_spritelocations2);
}

void sprite_position_tween_all(float alpha)
{
    const float inv = (1.0f - alpha);

    for (uint16_t i = 0; i < MAX_SPRITES; i++)
    {
        auto* sprite = GetEntity(i);
        if (sprite_should_tween(sprite))
        {
            auto posA = _spritelocations1[i];
            auto posB = _spritelocations2[i];
            if (posA == posB)
            {
                continue;
            }
            sprite_set_coordinates(
                std::round(posB.x * alpha + posA.x * inv), std::round(posB.y * alpha + posA.y * inv),
                std::round(posB.z * alpha + posA.z * inv), sprite);
            sprite->Invalidate2();
        }
    }
}

/**
 * Restore the real positions of the sprites so they aren't left at the mid-tween positions
 */
void sprite_position_tween_restore()
{
    for (uint16_t i = 0; i < MAX_SPRITES; i++)
    {
        auto* sprite = GetEntity(i);
        if (sprite_should_tween(sprite))
        {
            sprite->Invalidate2();

            auto pos = _spritelocations2[i];
            sprite_set_coordinates(pos.x, pos.y, pos.z, sprite);
        }
    }
}

void sprite_position_tween_reset()
{
    for (uint16_t i = 0; i < MAX_SPRITES; i++)
    {
        auto* sprite = GetEntity(i);
        _spritelocations1[i].x = _spritelocations2[i].x = sprite->x;
        _spritelocations1[i].y = _spritelocations2[i].y = sprite->y;
        _spritelocations1[i].z = _spritelocations2[i].z = sprite->z;
    }
}

void sprite_set_flashing(SpriteBase* sprite, bool flashing)
{
    assert(sprite->sprite_index < MAX_SPRITES);
    _spriteFlashingList[sprite->sprite_index] = flashing;
}

bool sprite_get_flashing(SpriteBase* sprite)
{
    assert(sprite->sprite_index < MAX_SPRITES);
    return _spriteFlashingList[sprite->sprite_index];
}

static SpriteBase* find_sprite_list_cycle(uint16_t sprite_idx)
{
    if (sprite_idx == SPRITE_INDEX_NULL)
    {
        return nullptr;
    }
    const SpriteBase* fast = GetEntity(sprite_idx);
    const SpriteBase* slow = fast;
    bool increment_slow = false;
    SpriteBase* cycle_start = nullptr;
    while (fast->sprite_index != SPRITE_INDEX_NULL)
    {
        // increment fast every time, unless reached the end
        if (fast->next == SPRITE_INDEX_NULL)
        {
            break;
        }
        else
        {
            fast = GetEntity(fast->next);
        }
        // increment slow only every second iteration
        if (increment_slow)
        {
            slow = GetEntity(slow->next);
        }
        increment_slow = !increment_slow;
        if (fast == slow)
        {
            cycle_start = GetEntity(slow->sprite_index);
            break;
        }
    }
    return cycle_start;
}

static SpriteBase* find_sprite_quadrant_cycle(uint16_t sprite_idx)
{
    if (sprite_idx == SPRITE_INDEX_NULL)
    {
        return nullptr;
    }
    const SpriteBase* fast = GetEntity(sprite_idx);
    const SpriteBase* slow = fast;
    bool increment_slow = false;
    SpriteBase* cycle_start = nullptr;
    while (fast->sprite_index != SPRITE_INDEX_NULL)
    {
        // increment fast every time, unless reached the end
        if (fast->next_in_quadrant == SPRITE_INDEX_NULL)
        {
            break;
        }
        else
        {
            fast = GetEntity(fast->next_in_quadrant);
        }
        // increment slow only every second iteration
        if (increment_slow)
        {
            slow = GetEntity(slow->next_in_quadrant);
        }
        increment_slow = !increment_slow;
        if (fast == slow)
        {
            cycle_start = GetEntity(slow->sprite_index);
            break;
        }
    }
    return cycle_start;
}

static bool index_is_in_list(uint16_t index, enum SPRITE_LIST sl)
{
    for (auto entity : EntityList(sl))
    {
        if (entity->sprite_index == index)
        {
            return true;
        }
    }
    return false;
}

int32_t check_for_sprite_list_cycles(bool fix)
{
    for (int32_t i = 0; i < SPRITE_LIST_COUNT; i++)
    {
        auto* cycle_start = find_sprite_list_cycle(gSpriteListHead[i]);
        if (cycle_start != nullptr)
        {
            if (fix)
            {
                // Fix head list, but only in reverse order
                // This is likely not needed, but just in case
                GetEntity(gSpriteListHead[i])->previous = SPRITE_INDEX_NULL;

                // Store the leftover part of cycle to be fixed
                uint16_t cycle_next = cycle_start->next;

                // Break the cycle
                cycle_start->next = SPRITE_INDEX_NULL;

                // Now re-add remainder of the cycle back to list, safely.
                // Add each sprite to the list until we encounter one that is already part of the list.
                while (!index_is_in_list(cycle_next, static_cast<SPRITE_LIST>(i)))
                {
                    auto* spr = GetEntity(cycle_next);

                    cycle_start->next = cycle_next;
                    spr->previous = cycle_start->sprite_index;
                    cycle_next = spr->next;
                    spr->next = SPRITE_INDEX_NULL;
                    cycle_start = spr;
                }
            }
            return i;
        }
    }
    return -1;
}

/**
 * Finds and fixes null sprites that are not reachable via SPRITE_LIST_FREE list.
 *
 * @return count of disjoint sprites found
 */
int32_t fix_disjoint_sprites()
{
    // Find reachable sprites
    bool reachable[MAX_SPRITES] = { false };

    SpriteBase* null_list_tail = nullptr;
    for (uint16_t sprite_idx = gSpriteListHead[SPRITE_LIST_FREE]; sprite_idx != SPRITE_INDEX_NULL;)
    {
        reachable[sprite_idx] = true;
        // cache the tail, so we don't have to walk the list twice
        null_list_tail = GetEntity(sprite_idx);
        sprite_idx = null_list_tail->next;
    }

    int32_t count = 0;

    // Find all null sprites
    for (uint16_t sprite_idx = 0; sprite_idx < MAX_SPRITES; sprite_idx++)
    {
        auto* spr = GetEntity(sprite_idx);
        if (spr->sprite_identifier == SPRITE_IDENTIFIER_NULL)
        {
            openrct2_assert(null_list_tail != nullptr, "Null list is empty, yet found null sprites");
            spr->sprite_index = sprite_idx;
            if (!reachable[sprite_idx])
            {
                // Add the sprite directly to the list
                null_list_tail->next = sprite_idx;
                spr->next = SPRITE_INDEX_NULL;
                spr->previous = null_list_tail->sprite_index;
                null_list_tail = spr;
                count++;
                reachable[sprite_idx] = true;
            }
        }
    }
    return count;
}

int32_t check_for_spatial_index_cycles(bool fix)
{
    for (uint32_t i = 0; i < SPATIAL_INDEX_LOCATION_NULL; i++)
    {
        auto* cycle_start = find_sprite_quadrant_cycle(gSpriteSpatialIndex[i]);
        if (cycle_start != nullptr)
        {
            if (fix)
            {
                // Store the leftover part of cycle to be fixed
                uint16_t cycle_next = cycle_start->next_in_quadrant;

                // Break the cycle
                cycle_start->next_in_quadrant = SPRITE_INDEX_NULL;

                // Now re-add remainder of the cycle back to list, safely.
                // Add each sprite to the list until we encounter one that is already part of the list.
                while (!index_is_in_list(cycle_next, static_cast<SPRITE_LIST>(i)))
                {
                    auto* spr = GetEntity(cycle_next);

                    cycle_start->next_in_quadrant = cycle_next;
                    cycle_next = spr->next_in_quadrant;
                    spr->next_in_quadrant = SPRITE_INDEX_NULL;
                    cycle_start = spr;
                }
            }
            return i;
        }
    }
    return -1;
}
