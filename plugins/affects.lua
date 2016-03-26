﻿-- affects
-- Плагин для Tortilla mud client

local affects = {}
local colors = { header = 80 }
local working = false

function affects.name()
  return 'Аффекты на персонаже'
end

function affects.description()
  return 'Плагин отображает аффекты, которые висят на персонаже.'
end

function affects.version()
  return '1.0'
end

local function setTextColor(color)
  r:textColor(props.paletteColor(color))
end

function affects.render()
  local x, y = 4, 4
  local h = r:fontHeight()
  if not working then
    setTextColor(colors.header)
	r:print(x, y, 'Аффекты на персонаже')
	y = y + h
    r:print(x, y, 'Ошибка в настройках')
    return
  end    
  setTextColor(colors.header)
  r:print(x, y, 'Аффекты:')
end

function affects.init()
  local p = createPanel("right", 200)
  r = p:setRender(affects.render)
  r:setBackground(props.backgroundColor())
  r:select(props.currentFont())
end

return affects