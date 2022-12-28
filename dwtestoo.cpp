/*
 * Simple C++ Dynamic Windows Example
 */
#include "dw.hpp"

/* Select a fixed width font for our platform */
#ifdef __OS2__
#define FIXEDFONT "5.System VIO"
#define PLATFORMFOLDER "os2\\"
#elif defined(__WIN32__)
#define FIXEDFONT "10.Lucida Console"
#define PLATFORMFOLDER "win\\"
#elif defined(__MAC__)
#define FIXEDFONT "9.Monaco"
#define PLATFORMFOLDER "mac/"
#elif defined(__IOS__)
#define FIXEDFONT "9.Monaco"
#elif defined(__ANDROID__)
#define FIXEDFONT "10.Monospace"
#elif GTK_MAJOR_VERSION > 1
#define FIXEDFONT "10.monospace"
#define PLATFORMFOLDER "gtk/"
#else
#define FIXEDFONT "fixed"
#endif

#define SHAPES_DOUBLE_BUFFERED  0
#define SHAPES_DIRECT           1
#define DRAW_FILE               2

class DWTest : public DW::Window
{
private:
    const char *ResolveKeyName(int vk) {
        const char *keyname;
        switch(vk) {
            case  VK_LBUTTON : keyname =  "VK_LBUTTON"; break;
            case  VK_RBUTTON : keyname =  "VK_RBUTTON"; break;
            case  VK_CANCEL  : keyname =  "VK_CANCEL"; break;
            case  VK_MBUTTON : keyname =  "VK_MBUTTON"; break;
            case  VK_TAB     : keyname =  "VK_TAB"; break;
            case  VK_CLEAR   : keyname =  "VK_CLEAR"; break;
            case  VK_RETURN  : keyname =  "VK_RETURN"; break;
            case  VK_PAUSE   : keyname =  "VK_PAUSE"; break;
            case  VK_CAPITAL : keyname =  "VK_CAPITAL"; break;
            case  VK_ESCAPE  : keyname =  "VK_ESCAPE"; break;
            case  VK_SPACE   : keyname =  "VK_SPACE"; break;
            case  VK_PRIOR   : keyname =  "VK_PRIOR"; break;
            case  VK_NEXT    : keyname =  "VK_NEXT"; break;
            case  VK_END     : keyname =  "VK_END"; break;
            case  VK_HOME    : keyname =  "VK_HOME"; break;
            case  VK_LEFT    : keyname =  "VK_LEFT"; break;
            case  VK_UP      : keyname =  "VK_UP"; break;
            case  VK_RIGHT   : keyname =  "VK_RIGHT"; break;
            case  VK_DOWN    : keyname =  "VK_DOWN"; break;
            case  VK_SELECT  : keyname =  "VK_SELECT"; break;
            case  VK_PRINT   : keyname =  "VK_PRINT"; break;
            case  VK_EXECUTE : keyname =  "VK_EXECUTE"; break;
            case  VK_SNAPSHOT: keyname =  "VK_SNAPSHOT"; break;
            case  VK_INSERT  : keyname =  "VK_INSERT"; break;
            case  VK_DELETE  : keyname =  "VK_DELETE"; break;
            case  VK_HELP    : keyname =  "VK_HELP"; break;
            case  VK_LWIN    : keyname =  "VK_LWIN"; break;
            case  VK_RWIN    : keyname =  "VK_RWIN"; break;
            case  VK_NUMPAD0 : keyname =  "VK_NUMPAD0"; break;
            case  VK_NUMPAD1 : keyname =  "VK_NUMPAD1"; break;
            case  VK_NUMPAD2 : keyname =  "VK_NUMPAD2"; break;
            case  VK_NUMPAD3 : keyname =  "VK_NUMPAD3"; break;
            case  VK_NUMPAD4 : keyname =  "VK_NUMPAD4"; break;
            case  VK_NUMPAD5 : keyname =  "VK_NUMPAD5"; break;
            case  VK_NUMPAD6 : keyname =  "VK_NUMPAD6"; break;
            case  VK_NUMPAD7 : keyname =  "VK_NUMPAD7"; break;
            case  VK_NUMPAD8 : keyname =  "VK_NUMPAD8"; break;
            case  VK_NUMPAD9 : keyname =  "VK_NUMPAD9"; break;
            case  VK_MULTIPLY: keyname =  "VK_MULTIPLY"; break;
            case  VK_ADD     : keyname =  "VK_ADD"; break;
            case  VK_SEPARATOR: keyname = "VK_SEPARATOR"; break;
            case  VK_SUBTRACT: keyname =  "VK_SUBTRACT"; break;
            case  VK_DECIMAL : keyname =  "VK_DECIMAL"; break;
            case  VK_DIVIDE  : keyname =  "VK_DIVIDE"; break;
            case  VK_F1      : keyname =  "VK_F1"; break;
            case  VK_F2      : keyname =  "VK_F2"; break;
            case  VK_F3      : keyname =  "VK_F3"; break;
            case  VK_F4      : keyname =  "VK_F4"; break;
            case  VK_F5      : keyname =  "VK_F5"; break;
            case  VK_F6      : keyname =  "VK_F6"; break;
            case  VK_F7      : keyname =  "VK_F7"; break;
            case  VK_F8      : keyname =  "VK_F8"; break;
            case  VK_F9      : keyname =  "VK_F9"; break;
            case  VK_F10     : keyname =  "VK_F10"; break;
            case  VK_F11     : keyname =  "VK_F11"; break;
            case  VK_F12     : keyname =  "VK_F12"; break;
            case  VK_F13     : keyname =  "VK_F13"; break;
            case  VK_F14     : keyname =  "VK_F14"; break;
            case  VK_F15     : keyname =  "VK_F15"; break;
            case  VK_F16     : keyname =  "VK_F16"; break;
            case  VK_F17     : keyname =  "VK_F17"; break;
            case  VK_F18     : keyname =  "VK_F18"; break;
            case  VK_F19     : keyname =  "VK_F19"; break;
            case  VK_F20     : keyname =  "VK_F20"; break;
            case  VK_F21     : keyname =  "VK_F21"; break;
            case  VK_F22     : keyname =  "VK_F22"; break;
            case  VK_F23     : keyname =  "VK_F23"; break;
            case  VK_F24     : keyname =  "VK_F24"; break;
            case  VK_NUMLOCK : keyname =  "VK_NUMLOCK"; break;
            case  VK_SCROLL  : keyname =  "VK_SCROLL"; break;
            case  VK_LSHIFT  : keyname =  "VK_LSHIFT"; break;
            case  VK_RSHIFT  : keyname =  "VK_RSHIFT"; break;
            case  VK_LCONTROL: keyname =  "VK_LCONTROL"; break;
            case  VK_RCONTROL: keyname =  "VK_RCONTROL"; break;
            default: keyname = "<unknown>"; break;
        }
        return keyname;
    }

    const char *ResolveKeyModifiers(int mask) {
        if((mask & KC_CTRL) && (mask & KC_SHIFT) && (mask & KC_ALT))
            return "KC_CTRL KC_SHIFT KC_ALT";
        else if((mask & KC_CTRL) && (mask & KC_SHIFT))
            return "KC_CTRL KC_SHIFT";
        else if((mask & KC_CTRL) && (mask & KC_ALT))
            return "KC_CTRL KC_ALT";
        else if((mask & KC_SHIFT) && (mask & KC_ALT))
            return "KC_SHIFT KC_ALT";
        else if((mask & KC_SHIFT))
            return "KC_SHIFT";
        else if((mask & KC_CTRL))
            return "KC_CTRL";
        else if((mask & KC_ALT))
            return "KC_ALT";
        else return "none";
    }

    // Add the menus to the window
    void CreateMenus() {
        // Setup the menu
        DW::MenuBar *menubar = this->MenuBarNew();
        
        // add menus to the menubar
        DW::Menu *menu = new DW::Menu();
        DW::MenuItem *menuitem = menu->AppendItem("~Quit");
        menuitem->ConnectClicked([this] () -> int 
            { 
                if(this->app->MessageBox("dwtestoo", DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit?") != 0) {
                    this->app->MainQuit();
                }
                return TRUE;
            });

        // Add the "File" menu to the menubar...
        menubar->AppendItem("~File", menu);
        
        menu = new DW::Menu();
        DW::MenuItem *checkable_menuitem = menu->AppendItem("~Checkable Menu Item", 0, TRUE);
        checkable_menuitem->ConnectClicked([this]() -> int 
        {
            this->app->MessageBox("Menu Item Callback", DW_MB_OK | DW_MB_INFORMATION, "\"Checkable Menu Item\" selected");
            return FALSE;
        });
        DW::MenuItem *noncheckable_menuitem = menu->AppendItem("~Non-Checkable Menu Item");
        noncheckable_menuitem->ConnectClicked([this]() -> int 
        {
            this->app->MessageBox("Menu Item Callback", DW_MB_OK | DW_MB_INFORMATION, "\"Non-Checkable Menu Item\" selected");
            return FALSE;
        });
        menuitem = menu->AppendItem("~Disabled menu Item", DW_MIS_DISABLED|DW_MIS_CHECKED, TRUE);
        // separator
        menuitem = menu->AppendItem(DW_MENU_SEPARATOR);
        menuitem = menu->AppendItem("~Menu Items Disabled", 0, TRUE);
        menuitem->ConnectClicked([this, checkable_menuitem, noncheckable_menuitem]() -> int 
        {
            // Toggle the variable
            this->menu_enabled = !this->menu_enabled;
            // Set the ENABLED/DISABLED state on the menu items
            checkable_menuitem->SetStyle(menu_enabled ? DW_MIS_ENABLED : DW_MIS_DISABLED);
            noncheckable_menuitem->SetStyle(menu_enabled ? DW_MIS_ENABLED : DW_MIS_DISABLED);
            return FALSE;
        });
        // Add the "Menu" menu to the menubar...
        menubar->AppendItem("~Menu", menu);

        menu = new DW::Menu();
        menuitem = menu->AppendItem("~About");
        menuitem->ConnectClicked([this]() -> int 
        {
            DWEnv env;

            this->app->GetEnvironment(&env);
            this->app->MessageBox("About dwindows", DW_MB_OK | DW_MB_INFORMATION, "dwindows test\n\nOS: %s %s %s Version: %d.%d.%d.%d\n\nHTML: %s\n\ndwindows Version: %d.%d.%d\n\nScreen: %dx%d %dbpp",
                           env.osName, env.buildDate, env.buildTime,
                           env.MajorVersion, env.MinorVersion, env.MajorBuild, env.MinorBuild,
                           env.htmlEngine,
                           env.DWMajorVersion, env.DWMinorVersion, env.DWSubVersion,
                           this->app->GetScreenWidth(), this->app->GetScreenHeight(), this->app->GetColorDepth());
            return FALSE;
        });
        // Add the "Help" menu to the menubar...
        menubar->AppendItem("~Help", menu);
    }
    
    // Notebook page 1
    void CreateInput(DW::Box *notebookbox)
    {
        DW::Box *lbbox = new DW::Box(DW_VERT, 10);

        notebookbox->PackStart(lbbox, 150, 70, TRUE, TRUE, 0);

        /* Copy and Paste */
        DW::Box *browsebox = new DW::Box(DW_HORZ, 0);
        lbbox->PackStart(browsebox, 0, 0, FALSE, FALSE, 0);

        DW::Entryfield *copypastefield = new DW::Entryfield();
        copypastefield->SetLimit(260);
        browsebox->PackStart(copypastefield, TRUE, FALSE, 4);

        DW::Button *copybutton = new DW::Button("Copy");
        browsebox->PackStart(copybutton, FALSE, FALSE, 0);

        DW::Button *pastebutton = new DW::Button("Paste");
        browsebox->PackStart(pastebutton, FALSE, FALSE, 0);

        /* Archive Name */
        DW::Text *stext = new DW::Text("File to browse");
        stext->SetStyle(DW_DT_VCENTER);
        lbbox->PackStart(stext, 130, 15, TRUE, TRUE, 2);

        browsebox = new DW::Box(DW_HORZ, 0);
        lbbox->PackStart(browsebox, 0, 0, TRUE, TRUE, 0);

        DW::Entryfield *entryfield = new DW::Entryfield();
        entryfield->SetLimit(260);
        browsebox->PackStart(entryfield, 100, 15, TRUE, TRUE, 4);

        DW::Button *browsefilebutton = new DW::Button("Browse File");
        browsebox->PackStart(browsefilebutton, 40, 15, TRUE, TRUE, 0);

        DW::Button *browsefolderbutton = new DW::Button("Browse Folder");
        browsebox->PackStart(browsefolderbutton, 40, 15, TRUE, TRUE, 0);

        browsebox->SetColor(DW_CLR_PALEGRAY, DW_CLR_PALEGRAY);
        stext->SetColor(DW_CLR_BLACK, DW_CLR_PALEGRAY);

        // Buttons
        DW::Box *buttonbox = new DW::Box(DW_HORZ, 10);
        lbbox->PackStart(buttonbox, 0, 0, TRUE, TRUE, 0);

        DW::Button *cancelbutton = new DW::Button("Exit");
        buttonbox->PackStart(cancelbutton, 130, 30, TRUE, TRUE, 2);

        DW::Button *cursortogglebutton = new DW::Button("Set Cursor pointer - CLOCK");
        buttonbox->PackStart(cursortogglebutton, 130, 30, TRUE, TRUE, 2);

        DW::Button *okbutton = new DW::Button("Turn Off Annoying Beep!");
        buttonbox->PackStart(okbutton, 130, 30, TRUE, TRUE, 2);

        cancelbutton->Unpack();
        buttonbox->PackStart(cancelbutton, 130, 30, TRUE, TRUE, 2);
        //this->ClickDefault(cancelbutton);

        DW::Button *colorchoosebutton = new DW::Button("Color Chooser Dialog");
        buttonbox->PackStart(colorchoosebutton, 130, 30, TRUE, TRUE, 2);

        /* Set some nice fonts and colors */
        lbbox->SetColor(DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
        buttonbox->SetColor(DW_CLR_DARKCYAN, DW_CLR_PALEGRAY);
        okbutton->SetColor(DW_CLR_PALEGRAY, DW_CLR_DARKCYAN);
#ifdef COLOR_DEBUG
        copypastefield->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        copybutton->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        // Set a color then clear it to make sure it clears correctly
        entryfield->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        entryfield->SetColor(DW_CLR_DEFAULT, DW_CLR_DEFAULT);
        // Set a color then clear it to make sure it clears correctly... again
        pastebutton->SetColor(DW_CLR_WHITE, DW_CLR_RED);
        pastebutton->SetColor(DW_CLR_DEFAULT, DW_CLR_DEFAULT);
#endif

        // Connect signals
        browsefilebutton->ConnectClicked([this, entryfield, copypastefield]() -> int 
        {
            char *tmp = this->app->FileBrowse("Pick a file", "dwtest.c", "c", DW_FILE_OPEN);
            if(tmp)
            {
#if 0
                char *errors = read_file(tmp);
                char *title = "New file load";
                char *image = "image/test.png";
                DW::Notification *notification;

                if(errors)
                    notification = new DW::Notification(title, image, "dwtest failed to load \"%s\" into the file browser, %s.", tmp, errors);
                else
                    notification = new DW::Notification(title, image, "dwtest loaded \"%s\" into the file browser on the Render tab, with \"File Display\" selected from the drop down list.", tmp);
#endif

                if(current_file)
                    this->app->Free(current_file);
                current_file = tmp;
                entryfield->SetText(current_file);
                current_col = current_row = 0;

#if 0
                render_draw();
                notification->ConnectClicked([this]() -> int {
                    return TRUE;
                });
                notification->Send();
#endif
            }
            copypastefield->SetFocus();
            return FALSE;
        });
        
        browsefolderbutton->ConnectClicked([this]() -> int 
        {
            char *tmp = this->app->FileBrowse("Pick a folder", ".", "c", DW_DIRECTORY_OPEN);
            this->app->Debug("Folder picked: %s\n", tmp ? tmp : "None");
            return FALSE;
        });

        copybutton->ConnectClicked([this, copypastefield, entryfield]() -> int {
            char *test = copypastefield->GetText();

            if(test) {
                this->app->SetClipboard(test);
                this->app->Free(test);
            }
            entryfield->SetFocus();
            return TRUE; 
        });

        pastebutton->ConnectClicked([this, copypastefield]() -> int 
        {
            char *test = this->app->GetClipboard();
            if(test) {
                copypastefield->SetText(test);
                this->app->Free(test);
            }
            return TRUE;
        });

        okbutton->ConnectClicked([this]() -> int 
        {
            if(this->timer) {
                delete this->timer;
                this->timer = DW_NULL;
            }
            return TRUE;
        });

        cancelbutton->ConnectClicked([this] () -> int 
        {
            if(this->app->MessageBox("dwtest", DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit?") != 0) {
                this->app->MainQuit();
            }
            return TRUE;
        });

        cursortogglebutton->ConnectClicked([this, cursortogglebutton] () -> int 
        {
            this->cursor_arrow = !this->cursor_arrow;
            cursortogglebutton->SetText(this->cursor_arrow ? "Set Cursor pointer - ARROW" :
                                        "Set Cursor pointer - CLOCK");
            this->SetPointer(this->cursor_arrow ? DW_POINTER_CLOCK : DW_POINTER_DEFAULT);
            return FALSE;
        });

        colorchoosebutton->ConnectClicked([this]() -> int
        {
            this->current_color = this->app->ColorChoose(this->current_color);
            return FALSE;
        });
    }
public:
    // Constructor creates the application
    DWTest(const char *title) {
        char fileiconpath[1025] = "file";
        char foldericonpath[1025] = "folder";

        // Get our application singleton
        app = DW::App::Init();

        // Add menus to the window
        CreateMenus();
        
        // Create our notebook and add it to the window
        DW::Box *notebookbox = new DW::Box(DW_VERT, 5);
        this->PackStart(notebookbox, 0, 0, TRUE, TRUE, 0);

        /* First try the current directory */
        foldericon = app->LoadIcon(foldericonpath);
        fileicon = app->LoadIcon(fileiconpath);

#ifdef PLATFORMFOLDER
        /* In case we are running from the build directory...
         * also check the appropriate platform subfolder
         */
        if(!foldericon)
        {
            strncpy(foldericonpath, PLATFORMFOLDER "folder", 1024);
            foldericon = app->LoadIcon(foldericonpath);
        }
        if(!fileicon)
        {
            strncpy(fileiconpath, PLATFORMFOLDER "file", 1024);
            fileicon = app->LoadIcon(fileiconpath);
        }
#endif

        /* Finally try from the platform application directory */
        if(!foldericon && !fileicon)
        {
            char *appdir = app->GetDir();
            char pathbuff[1025] = {0};
            int pos = (int)strlen(appdir);

            strncpy(pathbuff, appdir, 1024);
            pathbuff[pos] = DW_DIR_SEPARATOR;
            pos++;
            strncpy(&pathbuff[pos], "folder", 1024-pos);
            foldericon = app->LoadIcon(pathbuff);
            if(foldericon)
                strncpy(foldericonpath, pathbuff, 1025);
            strncpy(&pathbuff[pos], "file", 1024-pos);
            fileicon = app->LoadIcon(pathbuff);
            if(fileicon)
                strncpy(fileiconpath, pathbuff, 1025);
        }

        DW::Notebook *notebook = new DW::Notebook();
        notebookbox->PackStart(notebook, TRUE, TRUE, 0);
        notebook->ConnectSwitchPage([this](unsigned long page_num) -> int 
        {
            this->app->Debug("DW_SIGNAL_SWITCH_PAGE: PageNum: %u\n", page_num);
            return TRUE;
        });

        // Create Notebook page 1
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateInput(notebookbox);
        unsigned long notebookpage = notebook->PageNew(0, TRUE);
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "buttons and entry");

        // Finalize the window
        this->SetSize(640, 550);

        timer = new DW::Timer(2000, [this]() -> int
        {
            this->app->Beep(200, 200);

            // Return TRUE so we get called again
            return TRUE;
        });
    }

    DW::App *app;

    // Page 1
    DW::Timer *timer;
    int cursor_arrow = TRUE;
    unsigned long current_color;

    // Page 2
    HPIXMAP text1pm,text2pm,image;
    int image_x = 20, image_y = 20, image_stretch = 0;

    int font_width = 8, font_height=12;
    int rows=10,width1=6,cols=80;
    int num_lines=0, max_linewidth=0;
    int current_row=0,current_col=0;
    int render_type = SHAPES_DOUBLE_BUFFERED;

    FILE *fp=NULL;
    char **lp;

    // Page 4
    int mle_point=-1;

    // Miscellaneous
    int menu_enabled = 1;
    char *current_file = NULL;
    HICN fileicon,foldericon;

    int OnDelete() override {
        if(app->MessageBox("dwtest", DW_MB_YESNO | DW_MB_QUESTION, "Are you sure you want to exit?") != 0) {
            app->MainQuit();
        }
        return TRUE;
    }
};

/* Pretty list of features corresponding to the DWFEATURE enum in dw.h */
const char *DWFeatureList[] = {
    "Supports the HTML Widget",
    "Supports the DW_SIGNAL_HTML_RESULT callback",
    "Supports custom window border sizes",
    "Supports window frame transparency",
    "Supports Dark Mode user interface",
    "Supports auto completion in Multi-line Edit boxes",
    "Supports word wrapping in Multi-line Edit boxes",
    "Supports striped line display in container widgets",
    "Supports Multiple Document Interface window frame",
    "Supports status text area on notebook/tabbed controls",
    "Supports sending system notifications",
    "Supports UTF8 encoded Unicode text",
    "Supports Rich Edit based MLE control (Windows)",
    "Supports icons in the taskbar or similar system widget",
    "Supports the Tree Widget",
    "Supports arbitrary window placement",
    "Supports alternate container view modes",
    NULL };

/*
 * Let's demonstrate the functionality of this library. :)
 */
int dwmain(int argc, char* argv[]) 
{
    /* Initialize the Dynamic Windows engine */
    DW::App *app = DW::App::Init(argc, argv, "org.dbsoft.dwindows.dwtestoo", "Dynamic Windows Test C++");

    /* Enable full dark mode on platforms that support it */
    if(getenv("DW_DARK_MODE"))
        app->SetFeature(DW_FEATURE_DARK_MODE, DW_DARK_MODE_FULL);

#ifdef DW_MOBILE
    /* Enable multi-line container display on Mobile platforms */
    app->SetFeature(DW_FEATURE_CONTAINER_MODE, DW_CONTAINER_MODE_MULTI);
#endif

    /* Test all the features and display the results */
    for(int intfeat=DW_FEATURE_HTML; intfeat<DW_FEATURE_MAX; intfeat++)
    {
        DWFEATURE feat = static_cast<DWFEATURE>(intfeat);
        int result = dw_feature_get(feat);
        const char *status = "Unsupported";

        if(result == 0)
            status = "Disabled";
        else if(result > 0)
            status = "Enabled";

        app->Debug("%s: %s (%d)\n", DWFeatureList[feat], status, result);
    }

    DWTest *window = new DWTest("dwindows test UTF8 中国語 (繁体) cañón");
    window->Show();

    app->Main();
    app->Exit(0);

    return 0;
}
