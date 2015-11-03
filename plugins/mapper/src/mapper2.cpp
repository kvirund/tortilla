#include "stdafx.h"
#include "mapper.h"
#include "helpers.h"
//-------------------------------------------------------------------------------------
void Mapper::newZone(Room *room, RoomDir dir)
{   
    RoomHelper rh(room);
    if (rh.isCycled())
    {
        MessageBox(L"���������� ������� ����� ���� ��-�� �����!", L"������", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<Room*> rooms;
    rh.getSubZone(dir, &rooms);
    if (rooms.empty())
    {
        MessageBox(L"��� ������ ��� �������� ����� ����!", L"������", MB_OK | MB_ICONERROR);
        return;
    }

    int min_x = -1, max_x = 0, min_y = -1, max_y = 0, min_z = -1, max_z = 0;
    for (int i = 0, e = rooms.size(); i < e; ++i)
    {
        Room *r = rooms[i];
        if (min_x == -1 || min_x > r->x) min_x = r->x;
        if (max_x < r->x) max_x = r->x;
        if (min_y == -1 || min_y > r->y) min_y = r->y;
        if (max_y < r->y) max_y = r->y;
        int z = r->level->getLevel();
        if (min_z == -1 || min_z > z) min_z = z;
        if (max_z < z) max_z = z;
    }
    
    Zone *new_zone = addNewZone();
    int levels = max_z - min_z + 1;
    for (int i = 0; i < levels; ++i)
       new_zone->getLevel(i, true);       
    new_zone->resizeLevels(max_x - min_x, max_y - min_y);
    
    for (int i = 0, e = rooms.size(); i < e; ++i)
    {
        Room *r = rooms[i];
        int x = r->x - min_x;
        int y = r->y - min_y;
        int z = r->level->getLevel() - min_z;            
        r->level->detachRoom(r->x, r->y);
        RoomsLevel *level = new_zone->getLevel(z, false);            
        level->addRoom(r, x, y);
    }

    m_zones_control.addNewZone(new_zone);
    m_view.Invalidate();
}

void Mapper::saveMaps(lua_State *L)
{
    tstring dir;
    base::getPath(L, L"", &dir);

    std::vector<tstring> todelete;
    for (int i = 0, e = m_zones.size(); i < e; ++i)
    {
        Zone *zone = m_zones[i];
        if (!zone->isChanged())
            continue;        
        ZoneParams zp;
        zone->getParams(&zp);

        xml::node s("zone");
        //s.set("width", zone->width());
        //s.set("height", zone->height());
        //s.set("name", zp.name);
        for (int j = zp.minl; j <= zp.maxl; ++j)
        {
            RoomsLevel *level = zone->getLevel(j, false);
            if (level->isEmpty()) continue;
            xml::node l = s.createsubnode(L"level");
            l.set(L"z", j);
            for (int x = 0, xe = level->width(); x<xe; ++x) {
            for (int y = 0, ye = level->height(); y<ye; ++y) {
            Room *room = level->getRoom(x, y);
            if (!room) continue;
            xml::node r = l.createsubnode(L"room");
            r.set(L"name", room->roomdata.name);            
            r.set(L"exits", room->roomdata.exits);
            r.set(L"x", room->x);
            r.set(L"y", room->y);
            if (room->use_color)
            {
                wchar_t buffer[8]; COLORREF c = room->color;
                swprintf(buffer, L"%.2x%.2x%.2x", GetRValue(c), GetGValue(c), GetBValue(c));
                r.set(L"color", buffer);
            }
            if (room->icon > 0)
                r.set(L"icon", room->icon);
            r.set(L"descr", room->roomdata.descr);
            
            for (int dir = RD_NORTH; dir <= RD_DOWN; ++dir)
            {                
                RoomExit &exit = room->dirs[dir];
                if (!exit.exist)
                    continue;                
                
                xml::node e = r.createsubnode(L"exit");
                e.set(L"dir", RoomDirName[dir]);

                //u8string state;
                Room* next_room = exit.next_room;
                if (next_room)
                {
                    e.set(L"x", next_room->x);
                    e.set(L"y", next_room->y);
                    e.set(L"z", next_room->level->getLevel());
                    Zone* zone0 = next_room->level->getZone();
                    if (zone != zone0)
                    {
                        ZoneParams zp0;
                        zone0->getParams(&zp0);
                        e.set(L"zone", zp0.name);
                    }
                }
                if (exit.door)
                    e.set(L"door", 1);
            }
            }}
        }

        for (int j = 0, je = todelete.size(); j < je; ++j)
        {
            if (todelete[j] == zp.name)
                { todelete.erase(todelete.begin() + j); break; }
        }
        if (zp.name != zp.original_name)
            todelete.push_back(zp.original_name);

        tstring path(dir);
        path.append(zp.name);
        path.append(L".map");
        if ( !s.save( path.c_str()) )
        {
            tstring error(L"������ ������ ����� � �����:");
            error.append( zp.name);
            base::log(L, error.c_str());
        }
        s.deletenode();
    }

    for (int j = 0, je = todelete.size(); j < je; ++j)
    {
        // delete old zone files
        tstring file(dir);
        file.append(todelete[j]);
        file.append(L".map");
        DeleteFile(file.c_str());
    }
}

void Mapper::loadMaps(lua_State *L)
{
    tstring dir;
    base::getPath(L, L"", &dir);

    tstring mask(dir);
    mask.append(L"*.map");
    std::vector<tstring> zones;
    WIN32_FIND_DATA fd;
    memset(&fd, 0, sizeof(WIN32_FIND_DATA));
    HANDLE file = FindFirstFile(mask.c_str(), &fd);
    if (file != INVALID_HANDLE_VALUE)
    {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                zones.push_back(fd.cFileName);
        } while (::FindNextFile(file, &fd));
        ::FindClose(file);
    }
    
    for (int i = 0, e = zones.size(); i < e; ++i)
    {
        tstring file(zones[i]);
        int pos = file.rfind(L".");
        tstring name(file.substr(0, pos));
        tstring filepath(dir);
        filepath.append(file);

        xml::node zn;
        bool loaded = false;
        Zone *zone = new Zone(name);
        if (zn.load(filepath.c_str()))
        {
            loaded = true;
            xml::request levels(zn, L"level");
            for (int j = 0, je = levels.size(); j < je; ++j)
            {
                int z = 0;
                if (!levels[j].get(L"z", &z))
                    { loaded = false;  break; }
                RoomsLevel *level = zone->getLevel(z, true);
                xml::request rooms(levels[j], L"room");
                for (int r = 0, re = rooms.size(); r < re; ++r)
                {
                    RoomData rdata;
                    xml::node room = rooms[r];
                    int x = 0, y = 0;
                    if (!room.get(L"x", &x) || !room.get(L"y", &y) || 
                        !room.get(L"name", &rdata.name) || !room.get(L"descr", &rdata.descr) || !room.get(L"exits", &rdata.exits)
                       ) { continue; }                    
                    rdata.calcHash();
                    Room *new_room = createNewRoom(rdata);
                    int icon = 0;
                    if (room.get(L"icon", &icon) && icon > 0)
                        new_room->icon = icon;
                    tstring color;
                    if (room.get(L"color", &color))
                    {
                        wchar_t *p = NULL;
                        COLORREF n = wcstol(color.c_str(), &p, 16);
                        if (*p == 0)
                            { new_room->color = n; new_room->use_color = 1; }
                    }
                    if (!level->addRoom(new_room, x, y))                    
                        { delete new_room; loaded = false; break; }
                }
                if (!loaded) break;
            }
                        
            // load exits (after loading all rooms)
            if (loaded){
            m_zones.push_back(zone);
            m_zones_control.addNewZone(zone);
            for (int j = 0, je = levels.size(); j < je; ++j)
            {
                int z = 0;
                levels[j].get(L"z", &z);
                RoomsLevel *level = zone->getLevel(z, false);
                xml::request rooms(levels[j], L"room");
                for (int r = 0, re = rooms.size(); r < re; ++r)
                {
                    int x = 0, y = 0;
                    if (!rooms[r].get(L"x", &x) || !rooms[r].get(L"y", &y))
                        continue;
                    Room* room = level->getRoom(x, y);
                    xml::request exits(rooms[r], L"exit");
                    for (int e = 0, ee = exits.size(); e < ee; ++e)
                    {
                        xml::node exitnode = exits[e];
                        tstring exit_dir;
                        exitnode.get(L"dir", &exit_dir);
                        int dir = -1;
                        for (int d = RD_NORTH; d <= RD_DOWN; ++d) { if (!exit_dir.compare(RoomDirName[d])) { dir = d; break; }}
                        if (dir == -1)
                            continue;
                        RoomExit &exit = room->dirs[dir];
                        exit.exist = true;
                        int d = 0; if (exitnode.get(L"door", &d) && d == 1) exit.door = true;

                        if (!exitnode.get(L"x", &x) || !exitnode.get(L"y", &y) || !exitnode.get(L"z", &z))
                            continue;
                        RoomsLevel *next_level = NULL;
                        tstring zone_name;
                        if (exitnode.get(L"zone", &zone_name))
                        {
                            if (zone_name.empty()) continue;
                            int index = -1;
                            ZoneParams zp;
                            for (int zi = 0, ze = m_zones.size(); zi < ze; ++zi)
                            {
                                m_zones[zi]->getParams(&zp);
                                if (zp.name == zone_name) { index = zi; break; }
                            }
                            if (index == -1) continue;
                            Zone *new_zone = m_zones[index];
                            next_level = new_zone->getLevel(z, false);
                        }
                        else {
                            next_level = zone->getLevel(z, false);
                        }
                        if (!next_level) continue;
                        exit.next_room = next_level->getRoom(x, y);
                    }
                }
            }}
        }

        zn.deletenode();
        if (!loaded)
        {
            delete zone;
            tstring error(L"������ �������� ����:");
            error.append(filepath);
            base::log(L, error.c_str());
            continue;
        }             
    }

    m_viewpos.reset();
    if (!zones.empty())
    {
        Zone *zone = m_zones[0];
        m_viewpos.level = zone->getDefaultLevel();
    }
    redrawPosition();
}
