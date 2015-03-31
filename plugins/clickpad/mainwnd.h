#include "resource.h"
#include "padbutton.h"
#include "settingsDlg.h"

class ClickpadMainWnd : public CWindowImpl < ClickpadMainWnd >
{
public:
    ClickpadMainWnd();
    ~ClickpadMainWnd();

    void setButtonsNet(int rows, int columns);

    void switchEditMode();
    void beginEditMode();
    void endEditMode();

    void initDefault();
    void save(xml::node& node);
    void load(xml::node& node);

private:
    BEGIN_MSG_MAP(ClickpadMainWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_USER, OnParentSetFocus)
        //MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    END_MSG_MAP()

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&) { onCreate(); return 0; }
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&) { m_hWnd = NULL; return 0; }
    LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&){ return 1; }
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&){ onSize();  return 0; }
    LRESULT OnParentSetFocus(UINT, WPARAM, LPARAM, BOOL&) { onSetParentFocus(); return 0; }    
    void onCreate();
    void onSize();
    void onSetParentFocus();
private:
    void createNewRows(int count);
    void createNewColumns(int count);
    void createButton(int x, int y);
    int  getRows() const;
    int  getColumns() const;

private:
    SettingsDlg m_settings_wnd;    
    bool m_editmode;

    int m_button_width;
    int m_button_height;

    std::vector<std::vector<PadButton*>> m_buttons;
};
