#include "stdafx.h"
#include "accessors.h"
#include "mudViewParser.h"
#include "logicHelper.h"

LogicHelper::LogicHelper()
{
     m_if_regexp.setRegExp(L"^('.*'|\".*\"|{.*}|[^ =~!<>]+) *(=|!=|<|>|<=|>=) *('.*'|\".*\"|{.*}|[^ =~!<>]+)$", true);
     m_math_regexp.setRegExp(L"^('.*'|\".*\"|{.*}|[^ */+-]+) *([*/+-]) *('.*'|\".*\"|{.*}|[^ */+-]+)$", true);
}

bool LogicHelper::processAliases(const InputCommand* cmd, InputCommands* newcmds)
{
    for (int i=0,e=m_aliases.size(); i<e; ++i)
    {
        if (m_aliases[i]->processing(cmd, newcmds))
           return true;
    }
    return false;
}

bool LogicHelper::processHotkeys(const tstring& key, InputCommands* newcmds)
{
    for (int i=0,e=m_hotkeys.size(); i<e; ++i)
    {
        if (m_hotkeys[i]->processing(key, newcmds))
            return true;
    }
    return false;
}

void LogicHelper::processActions(parseData *parse_data, PluginsTriggersHandler* plugins_triggers, LogicPipelineElement *pe)
{
    for (int j=0,je=parse_data->strings.size()-1; j<=je; ++j)
    {
        MudViewString *s = parse_data->strings[j];
        bool incomplstr = (j==je && !parse_data->last_finished);

        bool processed = false; //todo plugins_triggers->processTriggers(s, incomplstr);
        if (!processed)
        {
            for (int i=0, e=m_actions.size(); i<e; ++i)
            {
              CompareData cd(s);
              if (m_actions[i]->processing(cd, incomplstr, &pe->commands))
              {
                  processed = true;
                  break;
              }
            }
        }

        if (processed)
        {
            s->system = true; //����� ������� ����� ������������ ����� ����� ������� �� ������� �������� �������
            parseData &not_processed = pe->data;
            not_processed.last_finished = parse_data->last_finished;
            parse_data->last_finished = true;
            not_processed.update_prev_string = false;
            int from = j+1;
            not_processed.strings.assign(parse_data->strings.begin() + from, parse_data->strings.end());
            parse_data->strings.resize(from);
            break;
        }
    }
}

void LogicHelper::processSubs(parseData *parse_data)
{
    for (int j=0,je=parse_data->strings.size()-1; j<=je; ++j)
    {
        MudViewString *s = parse_data->strings[j];
        //if (s->gamecmd || s->system) continue;
        bool incomplstr = (j==je && !parse_data->last_finished);
        if (incomplstr) continue;
        for (int i=0,e=m_subs.size(); i<e; ++i)
        {
            CompareData cd(s);
            while (m_subs[i]->processing(cd))
                cd.reinit();
        }
    }
}

void LogicHelper::processAntiSubs(parseData *parse_data)
{
    for (int j=0,je=parse_data->strings.size()-1; j<=je; ++j)
    {
        MudViewString *s = parse_data->strings[j];
        //if (s->gamecmd || s->system) continue;
        bool incomplstr = (j == je && !parse_data->last_finished);
        if (incomplstr) continue;
        for (int i=0,e=m_antisubs.size(); i<e; ++i)
        {
            CompareData cd(s);
            while (m_antisubs[i]->processing(cd))
                cd.reinit();
        }
    }
}

void LogicHelper::processGags(parseData *parse_data)
{
    for (int j=0,je=parse_data->strings.size()-1; j<=je; ++j)
    {
        MudViewString *s = parse_data->strings[j];
        //if (s->gamecmd || s->system) continue;
        bool incomplstr = (j == je && !parse_data->last_finished);
        if (incomplstr) continue;
        for (int i=0,e=m_gags.size(); i<e; ++i)
        {
            CompareData cd(s);
            while (m_gags[i]->processing(cd))
                cd.fullinit();
        }
    }
}

void LogicHelper::processHighlights(parseData *parse_data)
{
    for (int j=0,je=parse_data->strings.size()-1; j<=je; ++j)
    {
        for (int i=0,e=m_highlights.size(); i<e; ++i)
        {
            CompareData cd(parse_data->strings[j]);
            while (m_highlights[i]->processing(cd))
                cd.reinit();  // restart highlight
        }
    }
}

void LogicHelper::processTimers(InputCommands* newcmds)
{
    int dt = m_ticker.getDiff();
    m_ticker.sync();
    for (int i=0,e=m_timers.size(); i<e; ++i)
    {
        if (m_timers[i]->tick(dt))
            m_timers[i]->makeCommands(newcmds);
    }
}

void LogicHelper::resetTimers()
{
    for (int i=0,e=m_timers.size(); i<e; ++i)
        m_timers[i]->reset();
    m_ticker.sync();
}

LogicHelper::IfResult LogicHelper::compareIF(const tstring& param)
{
     m_if_regexp.find(param);
     if (m_if_regexp.getSize() != 4)
         return LogicHelper::IF_ERROR;

     tstring p1, p2, cond;
     m_if_regexp.getString(1, &p1);  //1st parameter
     m_if_regexp.getString(3, &p2);  //2nd parameter
     m_if_regexp.getString(2, &cond);//condition

     if (tortilla::getVars()->processVarsStrong(&p1) && tortilla::getVars()->processVarsStrong(&p2))
     {
         if (isInt(p1) && isInt(p2))
         {
             int n1 = _wtoi(p1.c_str());
             int n2 = _wtoi(p2.c_str());
             if (n1 == n2 && (cond == L"=" || cond == L"<=" || cond == L">="))
                 return LogicHelper::IF_SUCCESS;
             if (n1 < n2 && (cond == L"<" || cond == L"<=" || cond == L"!="))
                 return LogicHelper::IF_SUCCESS;
             if (n1 > n2 && (cond == L">" || cond == L">=" || cond == L"!="))
                 return LogicHelper::IF_SUCCESS;
          }
          else
          {
             int result = wcscmp(p1.c_str(), p2.c_str());
             if (result == 0 && (cond == L"=" || cond == L"<=" || cond == L">="))
                 return LogicHelper::IF_SUCCESS;
             if (result < 0 && (cond == L"<" || cond == L"<=" || cond == L"!="))
                 return LogicHelper::IF_SUCCESS;
             if (result > 0 && (cond == L">" || cond == L">=" || cond == L"!="))
                 return LogicHelper::IF_SUCCESS;
           }
     }
     return LogicHelper::IF_FAIL;
}

LogicHelper::MathResult LogicHelper::mathOp(const tstring& expr, tstring* result)
{
    m_math_regexp.find(expr);
    if (m_math_regexp.getSize() != 4)
         return LogicHelper::MATH_ERROR;

     tstring p1, p2, op;
     m_math_regexp.getString(1, &p1);  //1st parameter
     m_math_regexp.getString(3, &p2);  //2nd parameter
     m_math_regexp.getString(2, &op);  //operator

     if (tortilla::getVars()->processVarsStrong(&p1) && tortilla::getVars()->processVarsStrong(&p2))
     {
         if (isInt(p1) && isInt(p2))
         {
             int r = 0;
             int n1 = _wtoi(p1.c_str());
             int n2 = _wtoi(p2.c_str());
             if (op == L"*")
                 r = n1 * n2;
             else if (op == L"/")
                 r = (n2 != 0) ? n1 / n2 : 0;
             else if (op == L"+")
                 r = n1 + n2;
             else if (op == L"-")
                 r = n1 - n2;
             tchar buffer[16]; _itow(r, buffer, 10);
             result->assign(buffer);
             return LogicHelper::MATH_SUCCESS;
         }
         return LogicHelper::MATH_ERROR;
     }
     return LogicHelper::MATH_VARNOTEXIST;
}

void LogicHelper::updateProps(int what)
{
    PropertiesData *pdata = tortilla::getProperties();
    std::vector<tstring> active_groups;
    for (int i=0,e=pdata->groups.size(); i<e; i++)
    {
        const property_value& v = pdata->groups.get(i);
        if (v.value == L"1")
            active_groups.push_back(v.key);
    }

    InputTemplateParameters p;
    p.separator = pdata->cmd_separator;
    p.prefix = pdata->cmd_prefix;

    if (what == UPDATE_ALL || what == UPDATE_ALIASES)
        m_aliases.init(p, &pdata->aliases, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_ACTIONS)
        m_actions.init(p, &pdata->actions, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_SUBS)
        m_subs.init(&pdata->subs, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_HOTKEYS)
        m_hotkeys.init(p, &pdata->hotkeys, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_ANTISUBS)
        m_antisubs.init(&pdata->antisubs, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_GAGS)
        m_gags.init(&pdata->gags, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_HIGHLIGHTS)
        m_highlights.init(&pdata->highlights, active_groups);
    if (what == UPDATE_ALL || what == UPDATE_TIMERS)
        m_timers.init(&pdata->timers, active_groups, p);
}
