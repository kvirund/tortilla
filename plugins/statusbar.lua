﻿-- statusbar
-- Плагин для Tortilla mud client

-- Включите режим га/автозавершения в клиенте, либо настройте распознавание prompt-строки!
-- подробнее #help plugin_statusbar
-- FAQ: Если необновляются бары, например в бою, проверьте режим га/автозавершения!

-- Местоположение в окне клиента "top" или "bottom"
local position = "bottom"

-- Цвета баров hp(здоровья),mv(энергия),mn(мана),exp(опыт)
local colors = {
hp1 = {r=240},
hp2 = {r=128},
mv1 = {r=240,g=240},
mv2 = {r=128,g=128},
mn1 = {g=128,b=240},
mn2 = {g=64,b=128},
exp1 = {r=240,g=240,b=240},
exp2 = {r=128,g=128,b=128}
}

statusbar = {}
function statusbar.name() 
    return 'Гистограммы здоровья, маны, энергии, опыта'
end

function statusbar.description()
return 'Плагин отображает информацию о здоровье, мане, энергии и опыте в виде полосок\r\n\z
на отдельной панели клиента. Требует для работы режим га/автозавершения в маде.\r\n\z
Требует также настройки. Про настройку читайте в справке к клиенту (#help plugin_statusbar).\r\n\z
В пакете с клиентом уже есть конфигурационные файлы для основных существующих мадов.'
end

function statusbar.version()
    return '1.04'
end

local objs = {}
local regs = {}
local bars = 0
local connect = false
local reinit = false
local tegs = { 'hp','mn','mv','xp','dsu'}
local tegs1 = { 'hp','mn','mv'}

local r, values, cfg
local round = math.floor

function statusbar.render()
  if not cfg or not connect or bars == 0 then
    return
  end
  local showmsg = false
  if values.hp and values.mv then
    if not values.maxhp or not values.maxmv then
      showmsg = true
    else
      reinit = false
      if values.hp > values.maxhp or values.mv > values.maxmv then
        reinit = true
      end
      if values.mn and values.maxmn and values.mn > values.maxmn then
        reinit = true
      end
    end
  end
  if showmsg then
    statusbar.print(4, "Выполните команду 'счет' для настройки плагина.")
    return
  end
  statusbar.drawbars()
end

function statusbar.drawbar(t, pos)
  local val = tonumber(t.val)
  local maxval = tonumber(t.maxval)
  if not val or not maxval then return false end

  local percents = 0
  if val > 0 and maxval > 0 then
    if val < maxval then 
    percents = val * 100 / maxval
    else
      percents = 100
    end
  end
  local textlen = 0
  if t.text then
    r:textColor(t.color)
    textlen = r:print(pos.x, pos.y, t.text) + 2
  end
  local barlen = pos.width - textlen
  local v = round( (barlen * percents) / 100 )
  r:select(t.brush1)
  r:solidRect{x=pos.x+textlen, y=pos.y, width=v-1, height = pos.height}
  r:select(t.brush2)
  r:solidRect{x=pos.x+textlen+v, y=pos.y, width=barlen-v, height = pos.height}
  return true
end

function statusbar.drawbars()
  local delta_bars = 10
  local w = round( ((r:width()/10*6) - delta_bars*(bars-1)) / bars )
  local h = r:fontHeight()
  local pos = { x=4, y=(r:height()-h)/2, width=w, height=h }

  local hpbar = {val=values.hp,maxval=values.maxhp,text="HP:",brush1=objs.hpbrush1,brush2=objs.hpbrush2,color=colors.hp1}
  if statusbar.drawbar(hpbar, pos) then
    pos.x = pos.x + pos.width + delta_bars
  end

  local mnbar = {val=values.mn, maxval=values.maxmn,text="MA:",brush1=objs.mnbrush1,brush2=objs.mnbrush2,color=colors.mn1}
  if statusbar.drawbar(mnbar, pos) then
    pos.x = pos.x + pos.width + delta_bars
  end

  local mvbar = {val=values.mv,maxval=values.maxmv,text="MV:",brush1=objs.mvbrush1,brush2=objs.mvbrush2,color=colors.mv1}
  if statusbar.drawbar(mvbar, pos) then
    pos.x = pos.x + pos.width + delta_bars
  end

  local val, maxval
  if values.xp and values.dsu then
    val = values.xp
    maxval = values.xp + values.dsu
  elseif values.xp and values.summ then
    val = values.xp
    maxval = values.summ
  elseif values.dsu and values.summ then
    val = values.summ - values.dsu
    maxval = values.summ
  elseif values.maxdsu and values.maxxp then
    val = values.maxxp
    maxval = values.summ
  end
  local expbar = {val=val,maxval=maxval,text="XP:",brush1=objs.expbrush1,brush2=objs.expbrush2,color=colors.exp1}
  if statusbar.drawbar(expbar, pos) then
    pos.x = pos.x + pos.width + delta_bars
  end

-- hp > maxhp or mv > maxmv or mn > maxmn (level up, affects? - неверной значение max параметров)
  if reinit then
    statusbar.print(pos.x, "(сч)")
  end
end

function statusbar.print(x, msg)
  r:textColor(props.paletteColor(7))
  local y = (r:height()-r:fontHeight()) / 2
  r:print(x, y, msg)
end

function statusbar.before(window, v)
  if window ~= 0 or not cfg then return end
  local update = false 
  for i=1,v:size() do
    v:select(i)
    if v:isPrompt() then
      local tmp,count = {},0
      local prompt = v:getPrompt()
      for _,teg in pairs(tegs) do
         local c = cfg[teg]
         if c and c.prompt and c.prompt:find(prompt) then
           tmp[teg] = tonumber(c.prompt:get(1))
           count = count + 1
         end
      end
      if count > 1 then
        update = true
        for k,v in pairs(tmp) do values[k] = v end
      end
    end
  end
  for id,regexp in pairs(regs) do
    if v:find(regexp) then
      for _,teg in pairs(tegs1) do
        local c = cfg[teg]
        if c and c.regid == id then
          values['max'..teg] = tonumber(regexp:get(c.regindex))
          update = true
        end
      end
    end
  end

  if values.xp or values.dsu then

  end
  --[[local mxp = tonumber(values.maxxp)
  local mdsu = tonumber(values.maxdsu)
  if mxp and mdsu then
    values.summ = mxp + mdsu
  end]]

  if update then
    r:update()
  end
end

function statusbar.connect()
  connect = true
end

function statusbar.disconnect()
  connect = false
  values = {}
  r:update()
end

local function initprompt(teg)
  local prompt_letter = cfg.baseparams[teg]
  if prompt_letter then
    local c = cfg[teg] or {}
    c.prompt = createPcre("([0-9]+)"..prompt_letter)
    if not c.prompt then
      return false, 'Ошибка в параметре baseparams/'..teg
    end
    cfg[teg] = c
    return true
  end
  return false
end

local function initmax(teg)
  local score_index = tonumber(cfg.maxparams[teg])
  if score_index then
    local id = cfg.maxparams[teg..'id']
    if not id then
      return false, 'Не указан индекс регулярного выражения maxparams/'..teg..'id'
    end
    if not cfg.regexp[id] then
      return false, 'Нет регулярного выражения regexp/'..id
    end
    if not regs[id] then
       regs[id] = createPcre(cfg.regexp[id])
       if not regs[id] then
         return false, 'Ошибка в регулярном выражении regexp/'..id
       end
    end
    local c = cfg[teg] or {}
    c.regindex = score_index
    c.regid = id
    cfg[teg] = c
    return true
  end
  return false
end

function statusbar.init()
  local file = loadTable('config.xml')
  if not file then
    cfg = nil
    return terminate("Нет файла с настройками: "..getPath('config.xml'))
  end

  cfg = file
  bars = 0
  regs = {}
  local msgs = {}
  for _,v in pairs(tegs) do
    local res,msg = initprompt(v)
    if not res and msg then
      msgs[#msgs+1] = msg
    end
    for _,v2 in pairs(tegs1) do
      if v == v2 then
         res, msg = initmax(v)
         if not res and msg then
           msgs[#msgs+1] = msg
         end
         break
      end
    end
    if res then
      bars = bars + 1
    end
  end

  if #msgs ~= 0 then
    log('Ошибки в файле настрек: '..getPath('config.xml'))
    for _,v in ipairs(msgs) do
      log(v)
    end
    return terminate("Продолжение работы невозможно")
  end

  local p = createPanel(position, 28)
  r = p:setRender(statusbar.render)
  r:setBackground(props.backgroundColor())
  r:textColor(props.paletteColor(7))
  r:select(props.currentFont())

  objs.hpbrush1 = r:createBrush{color=colors.hp1}
  objs.hpbrush2 = r:createBrush{color=colors.hp2}
  objs.mvbrush1 = r:createBrush{color=colors.mv1}
  objs.mvbrush2 = r:createBrush{color=colors.mv2}
  objs.mnbrush1 = r:createBrush{color=colors.mn1}
  objs.mnbrush2 = r:createBrush{color=colors.mn2}
  objs.expbrush1 = r:createBrush{color=colors.exp1}
  objs.expbrush2 = r:createBrush{color=colors.exp2}

  values = {}
  connect = props.connected()
end
