#include "stdafx.h"
#include "mapperObjects.h"
#include "mapperObjects2.h"

const wchar_t* RoomDirName[] = { L"north", L"south", L"west", L"east", L"up", L"down" };
RoomDir RoomDirHelper::revertDir(RoomDir dir)
{
    if (dir == RD_NORTH)
        return RD_SOUTH;
    if (dir == RD_SOUTH)
        return RD_NORTH;
    if (dir == RD_WEST)
        return RD_EAST;
    if (dir == RD_EAST)
        return RD_WEST;
    if (dir == RD_UP)
        return RD_DOWN;
    if (dir == RD_DOWN)
        return RD_UP;
    assert(false);
    return RD_UNKNOWN;
}
const wchar_t* RoomDirHelper::getDirName(RoomDir dir)
{
    int index = static_cast<int>(dir);
    if (index >= 0 && index <= 5)
        return RoomDirName[index];
    return NULL;
}
RoomDir RoomDirHelper::getDirByName(const wchar_t* dirname)
{
    tstring name(dirname);
    for (int index=0;index<=5;++index)
    {
        if (!name.compare(RoomDirName[index]))
            return static_cast<RoomDir>(index);    
    }
    return RD_UNKNOWN;
}

RoomCursor::RoomCursor(Room* current_room) : 
m_current_room(current_room), x(0), y(0), level(0)
{
    assert(m_current_room && m_current_room->level && m_current_room->level->getZone());
}

Room* RoomCursor::getRoom(RoomDir dir)
{
    if (!move(dir))
        return NULL;
    if (!m_current_room || !m_current_room->level) {
        assert(false);
        return NULL; 
    }
    Zone *zone = m_current_room->level->getZone();
    if (!zone) { 
        assert(false);
        return NULL;
    }
    RoomsLevel *rl = zone->getLevel(level, false);
    if (!rl)
        return NULL;
    return rl->getRoom(x, y);
}

bool RoomCursor::addRoom(RoomDir dir, Room* room)
{
    if (!move(dir))
        return false;
    if (!m_current_room || !m_current_room->level) {
        assert(false);
        return false;
    }
    Zone *zone = m_current_room->level->getZone();
    RoomsLevel *rl = zone->getLevel(level, true);
    bool result = rl->addRoom(room, x, y);
    if (result)
    {
        result = addLink(dir, room);
        if (!result)
        {
            rl->detachRoom(x, y);
            assert(false);
        }
    }
    return result;
}

bool RoomCursor::addLink(RoomDir dir, Room *room)
{
    Room *next = m_current_room->dirs[dir].next_room;
    if (!next)
    {
        m_current_room->dirs[dir].next_room = room;
        return true;
    }
    return (next == room);
}

bool RoomCursor::move(RoomDir dir)
{
    x = m_current_room->x;
    y = m_current_room->y;
    level = m_current_room->level->getLevel();
    switch (dir)
    {
    case RD_NORTH: y -= 1; break;
    case RD_SOUTH: y += 1; break;
    case RD_WEST:  x -= 1; break;
    case RD_EAST:  x += 1; break;
    case RD_UP: level += 1; break;
    case RD_DOWN: level -= 1; break;
    default:
        assert(false);
        return false;
    }
    return true;
}

bool RoomCursor::isExplored(RoomDir dir)
{
    Room *r = m_current_room->dirs[dir].next_room;
    if (r && m_current_room->level->getZone() == r->level->getZone())      
       return true;
    return false;
}

Zone* RoomCursorNewZone::createNewZone(const tstring& name, Room* room)
{
    Zone *new_zone = new Zone(name);
    RoomsLevel *level = new_zone->getLevel(0, true);
    level->addRoom(room, 0, 0);
    return new_zone;
}

RoomDir MapperDirCommand::check(const tstring& cmd) const
{
    int size = cmd.size();
    if (size < main_size) return RD_UNKNOWN;
    if (size == main_size)
        return (cmd == main) ? dir : RD_UNKNOWN;
    tstring main_part(cmd.substr(0, main_size));
    if (main_part != main) return RD_UNKNOWN;
    if (rel.empty()) return RD_UNKNOWN;
    tstring rel_part(cmd.substr(main_size));
    int rel_part_size = rel_part.size();
    if (rel_part_size > rel_size) return RD_UNKNOWN;
    return rel.find(rel_part) == 0 ? dir : RD_UNKNOWN;
}