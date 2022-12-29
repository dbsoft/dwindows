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

#define APP_TITLE "Dynamic Windows C++"
#define APP_EXIT "Are you sure you want to exit?"

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

    char *ReadFile(char *filename)
    {
        char *errors = NULL;
        FILE *fp=NULL;
#ifdef __ANDROID__
        int fd = -1;

        // Special way to open for URIs on Android
        if(strstr(filename, "://"))
        {
            fd = dw_file_open(filename, O_RDONLY);
            fp = fdopen(fd, "r");
        }
        else
#endif
            fp = fopen(filename, "r");
        if(!fp)
            errors = strerror(errno);
        else
        {
            int i,len;

            lp = (char **)calloc(1000,sizeof(char *));
            // should test for out of memory
            max_linewidth=0;
            for(i=0; i<1000; i++)
            {
               lp[i] = (char *)calloc(1, 1025);
               if (fgets(lp[i], 1024, fp) == NULL)
                   break;
               len = (int)strlen(lp[i]);
               if (len > max_linewidth)
                   max_linewidth = len;
               if(lp[i][len - 1] == '\n')
                   lp[i][len - 1] = '\0';
            }
            num_lines = i;
            fclose(fp);

            hscrollbar->SetRange(max_linewidth, cols);
            hscrollbar->SetPos(0);
            vscrollbar->SetRange(num_lines, rows);
            vscrollbar->SetPos(0);
        }
#ifdef __ANDROID__
        if(fd != -1)
            close(fd);
#endif
        return errors;
    }

    // When hpm is not NULL we are printing.. so handle things differently
    void DrawFile(int row, int col, int nrows, int fheight, DW::Pixmap *hpm)
    {
        DW::Pixmap *pixmap = hpm ? hpm : pixmap2;
        char buf[16] = {0};
        int i,y,fileline;
        char *pLine;

        if(current_file)
        {
            pixmap->SetForegroundColor(DW_CLR_WHITE);
            if(!hpm)
                pixmap1->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)pixmap1->GetWidth(), (int)pixmap1->GetHeight());
            pixmap->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)pixmap->GetWidth(), (int)pixmap->GetHeight());

            for(i = 0;(i < nrows) && (i+row < num_lines); i++)
            {
                fileline = i + row - 1;
                y = i*fheight;
                pixmap->SetColor(fileline < 0 ? DW_CLR_WHITE : fileline % 16, 1 + (fileline % 15));
                if(!hpm)
                {
                    snprintf(buf, 15, "%6.6d", i+row);
                    pixmap1->DrawText(0, y, buf);
                }
                pLine = lp[i+row];
                pixmap->DrawText(0, y, pLine+col);
            }
        }
    }

    // When hpm is not NULL we are printing.. so handle things differently 
    void DrawShapes(int direct, DW::Pixmap *hpm)
    {
        DW::Pixmap *pixmap = hpm ? hpm : pixmap2;
        int width = (int)pixmap->GetWidth(), height = (int)pixmap->GetHeight();
        int x[7] = { 20, 180, 180, 230, 180, 180, 20 };
        int y[7] = { 50, 50, 20, 70, 120, 90, 90 };
        DW::Drawable *drawable = direct ? static_cast<DW::Drawable *>(render2) : static_cast<DW::Drawable *>(pixmap);

        drawable->SetForegroundColor(DW_CLR_WHITE);
        drawable->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, width, height);
        drawable->SetForegroundColor(DW_CLR_DARKPINK);
        drawable->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 10, 10, width - 20, height - 20);
        drawable->SetColor(DW_CLR_GREEN, DW_CLR_DARKRED);
        drawable->DrawText(10, 10, "This should be aligned with the edges.");
        drawable->SetForegroundColor(DW_CLR_YELLOW);
        drawable->DrawLine(width - 10, 10, 10, height - 10);
        drawable->SetForegroundColor(DW_CLR_BLUE);
        drawable->DrawPolygon(DW_DRAW_FILL, 7, x, y);
        drawable->SetForegroundColor(DW_CLR_BLACK);
        drawable->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 80, 80, 80, 40);
        drawable->SetForegroundColor(DW_CLR_CYAN);
        // Bottom right corner
        drawable->DrawArc(0, width - 30, height - 30, width - 10, height - 30, width - 30, height - 10);
        // Top right corner
        drawable->DrawArc(0, width - 30, 30, width - 30, 10, width - 10, 30);
        // Bottom left corner
        drawable->DrawArc(0, 30, height - 30, 30, height - 10, 10, height - 30);
        // Full circle in the left top area
        drawable->DrawArc(DW_DRAW_FULL, 120, 100, 80, 80, 160, 120);
        if(image && image->GetHPIXMAP())
        {
            if(imagestretchcheck->Get())
                drawable->BitBltStretch(0, 10, width - 20, height - 20, image, 0, 0, (int)image->GetWidth(), (int)image->GetHeight());
            else
                drawable->BitBlt(imagexspin->GetPos(), imageyspin->GetPos(), (int)image->GetWidth(), (int)image->GetHeight(), image, 0, 0);
        }
    }

    void UpdateRender(void)
    {
        switch(render_type)
        {
            case SHAPES_DOUBLE_BUFFERED:
                DrawShapes(FALSE, NULL);
                break;
            case SHAPES_DIRECT:
                DrawShapes(TRUE, NULL);
                break;
            case DRAW_FILE:
                DrawFile(current_row, current_col, rows, font_height, NULL);
                break;
        }
    }

    // Request that the render widgets redraw...
    // If not using direct rendering, call update_render() to
    // redraw the in memory pixmaps. Then trigger the expose events.
    // Expose will call update_render() to draw directly or bitblt the pixmaps.
    void RenderDraw() {
        // If we are double buffered, draw to the pixmaps
        if(render_type != SHAPES_DIRECT)
            UpdateRender();
        // Trigger expose event
        render1->Redraw();
        render2->Redraw();
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
            if(this->app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
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
            this->app->MessageBox("About dwindows", DW_MB_OK | DW_MB_INFORMATION,
                            "dwindows test\n\nOS: %s %s %s Version: %d.%d.%d.%d\n\nHTML: %s\n\ndwindows Version: %d.%d.%d\n\nScreen: %dx%d %dbpp",
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
                char *errors = ReadFile(tmp);
                const char *title = "New file load";
                const char *image = "image/test.png";
                DW::Notification *notification;

                if(errors)
                    notification = new DW::Notification(title, image, APP_TITLE " failed to load the file into the file browser.");
                else
                    notification = new DW::Notification(title, image, APP_TITLE " loaded the file into the file browser on the Render tab, with \"File Display\" selected from the drop down list.");

                if(current_file)
                    this->app->Free(current_file);
                current_file = tmp;
                entryfield->SetText(current_file);
                current_col = current_row = 0;

                RenderDraw();
                notification->ConnectClicked([this]() -> int {
                    this->app->Debug("Notification clicked\n");
                    return TRUE;
                });
                notification->Send();
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
            if(this->app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
                this->app->MainQuit();
            }
            return TRUE;
        });

        cursortogglebutton->ConnectClicked([this, cursortogglebutton] () -> int 
        {
            cursortogglebutton->SetText(this->cursor_arrow ? "Set Cursor pointer - ARROW" :
                                        "Set Cursor pointer - CLOCK");
            this->SetPointer(this->cursor_arrow ? DW_POINTER_CLOCK : DW_POINTER_DEFAULT);
            this->cursor_arrow = !this->cursor_arrow;
            return FALSE;
        });

        colorchoosebutton->ConnectClicked([this]() -> int
        {
            this->current_color = this->app->ColorChoose(this->current_color);
            return FALSE;
        });
    }

    void CreateRender(DW::Box *notebookbox) {
        int vscrollbarwidth, hscrollbarheight;
        wchar_t widestring[100] = L"DWTest Wide";
        char *utf8string = dw_wchar_to_utf8(widestring);

        // create a box to pack into the notebook page
        DW::Box *pagebox = new DW::Box(DW_HORZ, 2);
        notebookbox->PackStart(pagebox, 0, 0, TRUE, TRUE, 0);

        // now a status area under this box
        DW::Box *hbox = new DW::Box(DW_HORZ, 1);
        notebookbox->PackStart(hbox, 100, 20, TRUE, FALSE, 1);

        DW::StatusText *status1 = new DW::StatusText();
        hbox->PackStart(status1, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);

        DW::StatusText *status2 = new DW::StatusText();
        hbox->PackStart(status2, 100, DW_SIZE_AUTO, TRUE, FALSE, 1);
        // a box with combobox and button
        hbox = new DW::Box(DW_HORZ, 1 );
        notebookbox->PackStart(hbox, 100, 25, TRUE, FALSE, 1);

        DW::ComboBox *rendcombo = new DW::ComboBox( "Shapes Double Buffered");
        hbox->PackStart(rendcombo, 80, 25, TRUE, TRUE, 0);
        rendcombo->Append("Shapes Double Buffered");
        rendcombo->Append("Shapes Direct");
        rendcombo->Append("File Display");

        DW::Text *label = new DW::Text("Image X:");
        label->SetStyle(DW_DT_VCENTER | DW_DT_CENTER);
        hbox->PackStart(label, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        imagexspin = new DW::SpinButton("20");
        hbox->PackStart(imagexspin, 25, 25, TRUE, TRUE, 0);

        label = new DW::Text("Y:");
        label->SetStyle(DW_DT_VCENTER | DW_DT_CENTER);
        hbox->PackStart(label, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        imageyspin = new DW::SpinButton("20");
        hbox->PackStart(imageyspin, 25, 25, TRUE, TRUE, 0);
        imagexspin->SetLimits(2000, 0);
        imageyspin->SetLimits(2000, 0);
        imagexspin->SetPos(20);
        imageyspin->SetPos(20);

        imagestretchcheck = new DW::CheckBox("Stretch");
        hbox->PackStart(imagestretchcheck, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        DW::Button *refreshbutton = new DW::Button("Refresh");
        hbox->PackStart(refreshbutton, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        DW::Button *printbutton = new DW::Button("Print");
        hbox->PackStart(printbutton, DW_SIZE_AUTO, 25, FALSE, TRUE, 0);

        // Pre-create the scrollbars so we can query their sizes
        vscrollbar = new DW::ScrollBar(DW_VERT, 50);
        hscrollbar = new DW::ScrollBar(DW_HORZ, 50);
        vscrollbar->GetPreferredSize(&vscrollbarwidth, NULL);
        hscrollbar->GetPreferredSize(NULL, &hscrollbarheight);

        // On GTK with overlay scrollbars enabled this returns us 0...
        // so in that case we need to give it some real values.
        if(!vscrollbarwidth)
            vscrollbarwidth = 8;
        if(!hscrollbarheight)
            hscrollbarheight = 8;

        // Create render box for line number pixmap
        render1 = new DW::Render();
        render1->SetFont(FIXEDFONT);
        render1->GetTextExtents("(g", &font_width, &font_height);
        font_width = font_width / 2;

        DW::Box *vscrollbox = new DW::Box(DW_VERT, 0);
        vscrollbox->PackStart(render1, font_width*width1, font_height*rows, FALSE, TRUE, 0);
        vscrollbox->PackStart(DW_NOHWND, (font_width*(width1+1)), hscrollbarheight, FALSE, FALSE, 0);
        pagebox->PackStart(vscrollbox, 0, 0, FALSE, TRUE, 0);

        // pack empty space 1 character wide
        pagebox->PackStart(DW_NOHWND, font_width, 0, FALSE, TRUE, 0);

        // create box for filecontents and horz scrollbar
        DW::Box *textbox = new DW::Box(DW_VERT,0 );
        pagebox->PackStart(textbox, 0, 0, TRUE, TRUE, 0);

        // create render box for filecontents pixmap
        render2 = new DW::Render();
        textbox->PackStart(render2, 10, 10, TRUE, TRUE, 0);
        render2->SetFont(FIXEDFONT);
        // create horizonal scrollbar
        textbox->PackStart(hscrollbar, TRUE, FALSE, 0);

        // create vertical scrollbar
        vscrollbox = new DW::Box(DW_VERT, 0);
        vscrollbox->PackStart(vscrollbar, FALSE, TRUE, 0);
        // Pack an area of empty space of the scrollbar dimensions
        vscrollbox->PackStart(DW_NOHWND, vscrollbarwidth, hscrollbarheight, FALSE, FALSE, 0);
        pagebox->PackStart(vscrollbox, 0, 0, FALSE, TRUE, 0);

        pixmap1 = new DW::Pixmap(render1, font_width*width1, font_height*rows);
        pixmap2 = new DW::Pixmap(render2, font_width*cols, font_height*rows);
        image = new DW::Pixmap(render1, "image/test");
        if(!image || !image->GetHPIXMAP())
            image = new DW::Pixmap(render1, "~/test");
        if(!image || !image->GetHPIXMAP())
        {
            char *appdir = app->GetDir();
            char pathbuff[1025] = {0};
            int pos = (int)strlen(appdir);
            
            strncpy(pathbuff, appdir, 1024);
            pathbuff[pos] = DW_DIR_SEPARATOR;
            pos++;
            strncpy(&pathbuff[pos], "test", 1024-pos);
            image = new DW::Pixmap(render1, pathbuff);
        }
        if(image)
            image->SetTransparentColor(DW_CLR_WHITE);

        app->MessageBox(utf8string ? utf8string : "DWTest", DW_MB_OK|DW_MB_INFORMATION, "Width: %d Height: %d\n", font_width, font_height);
        if(utf8string)
            app->Free(utf8string);
        pixmap1->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, font_width*width1, font_height*rows);
        pixmap2->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, font_width*cols, font_height*rows);

        // Signal handler lambdas
        render1->ConnectButtonPress([this](int x, int y, int buttonmask) -> int
        {
            DW::Menu *menu = new DW::Menu();
            DW::MenuItem *menuitem = menu->AppendItem("~Quit");
            long px, py;

            menuitem->ConnectClicked([this] () -> int 
            {
                if(this->app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
                    this->app->MainQuit();
                }
                return TRUE;
            });


            menu->AppendItem(DW_MENU_SEPARATOR);
            menuitem = menu->AppendItem("~Show Window");
            menuitem->ConnectClicked([this]() -> int
            {
                this->Show();
                this->Raise();
                return TRUE;
            });

            this->app->GetPointerPos(&px, &py);
            menu->Popup(this, (int)px, (int)py);
            return TRUE;
        });

        render1->ConnectExpose([this](DWExpose *exp) -> int 
        {
            if(render_type != SHAPES_DIRECT)
            {
                this->render1->BitBlt(0, 0, (int)pixmap1->GetWidth(), (int)pixmap1->GetHeight(), pixmap1, 0, 0);
                render1->Flush();
            }
            else
            {
                UpdateRender();
            }
            return TRUE;
        });

        render2->ConnectExpose([this](DWExpose *exp) -> int 
        {
            if(render_type != SHAPES_DIRECT)
            {
                this->render2->BitBlt(0, 0, (int)pixmap2->GetWidth(), (int)pixmap2->GetHeight(), pixmap2, 0, 0);
                render2->Flush();
            }
            else
            {
                UpdateRender();
            }
            return TRUE;
        });

        render2->ConnectKeyPress([this, status1](char ch, int vk, int state, char *utf8) -> int
        {
            char tmpbuf[101] = {0};
            if(ch)
                snprintf(tmpbuf, 100, "Key: %c(%d) Modifiers: %s(%d) utf8 %s", ch, ch, this->ResolveKeyModifiers(state), state,  utf8);
            else
                snprintf(tmpbuf, 100, "Key: %s(%d) Modifiers: %s(%d) utf8 %s", this->ResolveKeyName(vk), vk, ResolveKeyModifiers(state), state, utf8);
            status1->SetText(tmpbuf);
            return FALSE;
        });

        hscrollbar->ConnectValueChanged([this, status1](int value) -> int
        {
            char tmpbuf[101] = {0};

            this->current_col = value;
            snprintf(tmpbuf, 100, "Row:%d Col:%d Lines:%d Cols:%d", current_row,current_col,num_lines,max_linewidth);
            status1->SetText(tmpbuf);
            this->RenderDraw();
            return TRUE;
        });

        vscrollbar->ConnectValueChanged([this, status1](int value) -> int
        {
            char tmpbuf[101] = {0};

            this->current_row = value;
            snprintf(tmpbuf, 100, "Row:%d Col:%d Lines:%d Cols:%d", current_row,current_col,num_lines,max_linewidth);
            status1->SetText(tmpbuf);
            this->RenderDraw();
            return TRUE;
        });

        render2->ConnectMotionNotify([status2](int x, int y, int buttonmask) -> int
        {
            char buf[201] = {0};

            snprintf(buf, 200, "motion_notify: %dx%d buttons %d", x, y, buttonmask);
            status2->SetText(buf);
            return FALSE;
        });

        render2->ConnectButtonPress([status2](int x, int y, int buttonmask) -> int
        {
            char buf[201] = {0};

            snprintf(buf, 200, "button_press: %dx%d buttons %d", x, y, buttonmask);
            status2->SetText(buf);
            return FALSE;
        });

        render2->ConnectConfigure([this](int width, int height) -> int
        {
            DW::Pixmap *old1 = this->pixmap1, *old2 = this->pixmap2;

            rows = height / font_height;
            cols = width / font_width;

            // Create new pixmaps with the current sizes
            this->pixmap1 = new DW::Pixmap(this->render1, (unsigned long)(font_width*(width1)), (unsigned long)height);
            this->pixmap2 = new DW::Pixmap(this->render2, (unsigned long)width, (unsigned long)height);

            // Make sure the side area is cleared
            this->pixmap1->SetForegroundColor(DW_CLR_WHITE);
            this->pixmap1->DrawRect(DW_DRAW_FILL | DW_DRAW_NOAA, 0, 0, (int)this->pixmap1->GetWidth(), (int)this->pixmap1->GetHeight());

            // Destroy the old pixmaps
            delete old1;
            delete old2;

            // Update scrollbar ranges with new values
            this->hscrollbar->SetRange(max_linewidth, cols);
            this->vscrollbar->SetRange(num_lines, rows);

            // Redraw the render widgets
            this->RenderDraw();
            return TRUE;
        });

        imagestretchcheck->ConnectClicked([this]() -> int
        {
            this->RenderDraw();
            return TRUE;
        });

        refreshbutton->ConnectClicked([this]() -> int
        {
            this->RenderDraw();
            return TRUE;
        });

        printbutton->ConnectClicked([this]() -> int
        {
            DW::Print *print = new DW::Print("DWTest Job", 0, 2, [this](DW::Pixmap *pixmap, int page_num) -> int
            {
               pixmap->SetFont(FIXEDFONT);
               if(page_num == 0)
               {
                   this->DrawShapes(FALSE, pixmap);
               }
               else if(page_num == 1)
               {
                   /* Get the font size for this printer context... */
                   int fheight, fwidth;

                   /* If we have a file to display... */
                   if(current_file)
                   {
                       int nrows;

                       /* Calculate new dimensions */
                       pixmap->GetTextExtents("(g", NULL, &fheight);
                       nrows = (int)(pixmap->GetHeight() / fheight);

                       /* Do the actual drawing */
                       this->DrawFile(0, 0, nrows, fheight, pixmap);
                   }
                   else
                   {
                       /* We don't have a file so center an error message on the page */
                       const char *text = "No file currently selected!";
                       int posx, posy;

                       pixmap->GetTextExtents(text, &fwidth, &fheight);

                       posx = (int)(pixmap->GetWidth() - fwidth)/2;
                       posy = (int)(pixmap->GetHeight() - fheight)/2;

                       pixmap->SetColor(DW_CLR_BLACK, DW_CLR_WHITE);
                       pixmap->DrawText(posx, posy, text);
                   }
               }
               return TRUE;
            });
            print->Run(0);
            return TRUE;
        });

        rendcombo->ConnectListSelect([this](int index) -> int 
        {
            if(index != this->render_type)
            {
                if(index == DRAW_FILE)
                {
                    this->hscrollbar->SetRange(max_linewidth, cols);
                    this->hscrollbar->SetPos(0);
                    this->vscrollbar->SetRange(num_lines, rows);
                    this->vscrollbar->SetPos(0);
                    this->current_col = this->current_row = 0;
                }
                else
                {
                    this->hscrollbar->SetRange(0, 0);
                    this->hscrollbar->SetPos(0);
                    this->vscrollbar->SetRange(0, 0);
                    this->vscrollbar->SetPos(0);
                }
                this->render_type = index;
                this->RenderDraw();
            }
            return FALSE;
        });

        app->TaskBarInsert(render1, fileicon, "DWTest");
    }
public:
    // Constructor creates the application
    DWTest(const char *title): DW::Window(title) {
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

        // Create Notebook Page 1 - Input
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateInput(notebookbox);
        unsigned long notebookpage = notebook->PageNew(0, TRUE);
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "buttons and entry");

        // Create Notebook Page 2 - Render
        notebookbox = new DW::Box(DW_VERT, 5);
        CreateRender(notebookbox);
        notebookpage = notebook->PageNew();
        notebook->Pack(notebookpage, notebookbox);
        notebook->PageSetText(notebookpage, "render");

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
    DW::Render *render1, *render2;
    DW::Pixmap *pixmap1, *pixmap2, *image;
    DW::ScrollBar *hscrollbar, *vscrollbar;
    DW::SpinButton *imagexspin, *imageyspin;
    DW::CheckBox *imagestretchcheck;

    int font_width = 8, font_height=12;
    int rows=10,width1=6,cols=80;
    int num_lines=0, max_linewidth=0;
    int current_row=0,current_col=0;
    int render_type = SHAPES_DOUBLE_BUFFERED;

    char **lp;
    char *current_file = NULL;

    // Page 4
    int mle_point=-1;

    // Miscellaneous
    int menu_enabled = TRUE;
    HICN fileicon,foldericon;

    int OnDelete() override {
        if(app->MessageBox(APP_TITLE, DW_MB_YESNO | DW_MB_QUESTION, APP_EXIT) != 0) {
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
