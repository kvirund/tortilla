#pragma once

struct PropertiesData;
struct MudViewString;
class Palette256;
class LogsFormatter
{
public:
    enum PrepareMode { PM_NEW = 0, PM_APPEND };
    LogsFormatter(PropertiesData *pd);
    virtual ~LogsFormatter();
    bool open(const tstring& filename, PrepareMode pmode);
    const tstring& getFilename() const { return m_filename; }
    virtual void close() = 0;
    virtual void prepare() = 0;
    virtual void writeString(const MudViewString* str);
    virtual void convertString(const MudViewString* str, std::string* out) = 0;
protected:
    void getHeader(std::string* out);
    void write(const std::string &data);
    PropertiesData* m_propData;
    HANDLE hfile;
    WideToUtf8 m_converter;
    tstring m_filename;
    PrepareMode m_mode;
};

class LogsFormatterHtml : public LogsFormatter
{
    MemoryBuffer m_buffer;
    MemoryBuffer m_color_buffer;
    MemoryBuffer m_color_buffer2;
    Palette256* m_palette;
public:
    LogsFormatterHtml(PropertiesData *pd);
    ~LogsFormatterHtml();
    void prepare();
    void convertString(const MudViewString* str, std::string* out);
    void close();
private:    
    const char* color(COLORREF c);
    const char* color2(COLORREF c);
};

class LogsFormatterTxt : public LogsFormatter
{
    WideToUtf8 m_converter;
public:
    LogsFormatterTxt(PropertiesData *pd);
    ~LogsFormatterTxt();
    void prepare();
    void convertString(const MudViewString* str, std::string* out);
    void close();
};
