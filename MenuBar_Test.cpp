/*
menubar owner drawing.
*/
#include<windows.h>
#include<windowsx.h>
#include<math.h>
#include<stdio.h>
#include<commctrl.h>
#include<Richedit.h>
#include<gdiplus.h>
using namespace Gdiplus; //not use yet.
 
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"gdiplus.lib")

#define WINDOW_CLASS_NAME "MENUBAR NOT WORK WELL"
#define RWIDTH(A) abs(A.right - A.left)   
#define RHEIGHT(A) abs(A.bottom - A.top)  

HINSTANCE instance;
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

#define STEP_COLOR(C1,C2,LEN,IDX) \
RGB(GetRValue(C1)+(BYTE)((double)(GetRValue(C2)-GetRValue(C1))*1.0/(LEN)*(IDX)),\
    GetGValue(C1)+(BYTE)((double)(GetGValue(C2)-GetGValue(C1))*1.0/(LEN)*(IDX)),\
    GetBValue(C1)+(BYTE)((double)(GetBValue(C2)-GetBValue(C1))*1.0/(LEN)*(IDX)))

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
int Frame_Test(HWND parent);

#define ICONMARGIN 5 //icon margin
#define ICONPIXLS 25 //icon pixls
#define BTNMARGIN 5 //btn margin
#define BTNPIXLSX 25 //btn pixls
#define BTNPIXLSY 20
#define MENUBARY 20 //menubar height
#define BORDERPIXLS 2//border pixls

typedef enum FRAME_AREA_TYPE {
    ZCLOSE,
    ZMAX,
    ZMIN,
    ZICON,
    ZMENUBAR,
    ZCAPTION,
    ZTEXTTITLE,
    ZNOCARE
} EFNCZone;
typedef struct _STRUCT_FRAME_STYLE_ {
   EFNCZone czone;
   HFONT font;
   COLORREF color_mibk;//menu item back color
   HBRUSH brush_mibk;
   COLORREF color_border;
   HBRUSH brush_border;
   
   COLORREF color_active;
   COLORREF color_deactive;
   HBRUSH brush_cap_active;
   HBRUSH brush_cap_deactive;
   
   COLORREF color_title;
   COLORREF color_text;
   COLORREF color_bk;
   
   HBRUSH brush_btnclose;
   HBRUSH brush_btn;
   
   HDC memdc;//mem dc double buffering flicker.
   HDC hdc;
   HBITMAP bmp;
   HPEN pen;
   
   WNDPROC proc;
   WNDPROC pre_proc;
}RFrameStyle,*pFrameStyle;
int Frame_InitialSettings(HWND hwnd);
int Frame_ClearSettings(HWND hwnd);
pFrameStyle Frame_GetSettings(HWND hwnd);

int Frame_PopMenu(HWND hwnd,POINT pt);
BOOL Frame_IsMenuBarRect(HWND hwnd,POINT pt);
int Frame_GetMenuItemRect(HWND hwnd,int index,LPRECT prc);
int Frame_DrawMenuItem(LPDRAWITEMSTRUCT draw);
int Frame_NCDraw(HWND hwnd,LPVOID param);
int Frame_CopyBtnRect(HWND hwnd,LPRECT pbtn_rc,int btn_count);
int Frame_GetNCZoneRect(HWND hwnd,EFNCZone type,LPRECT prc,BOOL allign_topleft);
int Frame_NCCalcSize(HWND hwnd,WPARAM wParam,LPARAM lParam);
int Frame_InitialMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam);
int Frame_NCHitTest(HWND hwnd,WPARAM wParam,LPARAM lParam);

void gradient_rect(HDC hdc,RECT rect,COLORREF rgb_1,COLORREF rgb_2,int verical_or_horizen);

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, PSTR szcmdLine, int icmdshow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASSEX winclass;
	InitCommonControls();
	instance = hinstance;

	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = hinstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = WINDOW_CLASS_NAME;
	winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&winclass)) return 0;

	if (!(hwnd = CreateWindowEx(NULL,
								WINDOW_CLASS_NAME,
								"Frame Test.",
								WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								240, 262,
								780,400,
								NULL,
								NULL,
								hinstance,
								NULL)))
		return 0;

	while (GetMessage(&msg, NULL, 0, 0)) {
	    if(!IsDialogMessage(hwnd,&msg)) { //tabstop
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
	    }
	}

	return(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE: {
	    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	    InitCommonControls();
        Frame_Test(hwnd);
	} break;
	case WM_NCCALCSIZE: {
        return Frame_NCCalcSize(hwnd,wParam,lParam);
	} break;
	case WM_NCACTIVATE:
	case WM_NCPAINT: {
        BOOL active_flag=(BOOL)wParam;
        Frame_NCDraw(hwnd,&active_flag);
        return !active_flag;	    
	} break;
    case WM_NCMOUSEMOVE: {
        UINT HITPOS = wParam;
        POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
        BOOL active_flag=(GetActiveWindow()==hwnd);
        Frame_NCDraw(hwnd,&active_flag);
                
        return 0;
    } break;
    case WM_INITMENUPOPUP: {
        Frame_InitialMenuPopup(hwnd,wParam,lParam);
    } break;    
    case WM_NCHITTEST: {
        return Frame_NCHitTest(hwnd,wParam,lParam);
    } break;
    case WM_NCLBUTTONDOWN: {
        UINT HITPOS = wParam;
        POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
        
        if(HITPOS==HTMENU) {
            Frame_PopMenu(hwnd,pt);
            return 0;
        }
        else if(HITPOS==HTMINBUTTON||
                HITPOS==HTMAXBUTTON||
                HITPOS==HTCLOSE) return 0;
    } break;
    case WM_NCLBUTTONUP: {
        UINT HITPOS = wParam;
        
        //写得有点别扭
        if(HITPOS==HTMINBUTTON) ShowWindow(hwnd,SW_MINIMIZE);
        else if(HITPOS==HTMAXBUTTON) {
            if(IsZoomed(hwnd)) ShowWindow(hwnd,SW_RESTORE);
            else ShowWindow(hwnd,SW_MAXIMIZE);
        }
        else if(HITPOS==HTCLOSE) SendMessage(hwnd,WM_CLOSE,0,0);                
    } break;
    case WM_SIZE: {		
        int height = HIWORD(lParam);
	    int width = LOWORD(lParam);
        
        RECT window_rect;
        HRGN window_rgn;
        
        //Rectangle the window      
        GetWindowRect(hwnd,&window_rect);
        window_rgn=CreateRectRgn(0,0,window_rect.right-window_rect.left,window_rect.bottom-window_rect.top);
        SetWindowRgn(hwnd,window_rgn,TRUE);
        DeleteObject(window_rgn);        
    } break;
    case WM_NCDESTROY: {
        Frame_ClearSettings(hwnd);
    } break;
	case WM_DESTROY: {
	    GdiplusShutdown(gdiplusToken);
		PostQuitMessage(0);
		return (0);
	} break;
	case WM_COMMAND: {
	    HWND ctrl=(HWND)lParam;
	    UINT code=HIWORD(wParam);
	} break;
	case WM_NOTIFY: {
	    LPNMHDR phdr=(LPNMHDR)lParam;
	    UINT ctrlid=wParam;
	    
	    if(phdr->code==DTN_DATETIMECHANGE) {
	        //DateTimePicker Alter Notify.
	    }
	} break;
    case WM_MEASUREITEM: {
        LPMEASUREITEMSTRUCT lpmis;
        lpmis = (LPMEASUREITEMSTRUCT)lParam;
        
        if(lpmis->CtlType == ODT_MENU) {
            UINT menu_item_id = lpmis->itemID;
            lpmis->itemWidth = 200;
            lpmis->itemHeight = ((menu_item_id==0x0)?5:28);//menu item & menu spliter
        }
    } break;
	case WM_DRAWITEM: {
	    UINT ctrl_id=(UINT)wParam;
	    LPDRAWITEMSTRUCT pDraw=(LPDRAWITEMSTRUCT)lParam;
	    
	    if(pDraw->CtlType==ODT_MENU) {
	        Frame_DrawMenuItem(pDraw);
	    }
	} break;
    case WM_CTLCOLORLISTBOX:
    {
        HDC dc=(HDC)wParam;
        HWND ctrl=(HWND)lParam;
    } break;
    case WM_CTLCOLOREDIT:
    {
        HDC dc=(HDC)wParam;
        HWND ctrl=(HWND)lParam;
    } break;
    case WM_SYSKEYDOWN: return 0;
	default:
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}


int Frame_ClearSettings(HWND hwnd)
{
    pFrameStyle fs=Frame_GetSettings(hwnd);
    if(!fs) return -1;
    
    DeleteObject(fs->font);
    DeleteObject(fs->brush_mibk);
    DeleteObject(fs->brush_cap_active);
    DeleteObject(fs->brush_cap_deactive);
    DeleteObject(fs->brush_border);
    DeleteObject(fs->pen);
    DeleteObject(fs->bmp);
    DeleteObject(fs->brush_btnclose);
    DeleteObject(fs->brush_btn);
    DeleteDC(fs->memdc);
    ReleaseDC(hwnd,fs->hdc);
    
    free(fs);
    
    return 0;
}

pFrameStyle Frame_GetSettings(HWND hwnd)
{
    return (pFrameStyle)GetWindowLongPtr(hwnd,GWLP_USERDATA);
}

int Frame_InitialSettings(HWND hwnd)
{
    pFrameStyle fs=(pFrameStyle)calloc(sizeof(RFrameStyle),1);
    if(!fs) return -1;
    
    fs->font=CreateFont(17,0,0,0,
                        FW_MEDIUM,//FW_SEMIBOLD,
                        FALSE,FALSE,FALSE,
                        DEFAULT_CHARSET,
                        OUT_OUTLINE_PRECIS,
                        CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, 
                        VARIABLE_PITCH,
                        "Courier New");
    SendMessage(hwnd,WM_SETFONT,(WPARAM)fs->font,0);
    SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)fs);
    
    fs->color_mibk=RGB(43,43,43);
    fs->brush_mibk=CreateSolidBrush(fs->color_mibk);
    
    fs->color_active=RGB(30,30,30);
    fs->brush_cap_active=CreateSolidBrush(fs->color_active);
    fs->color_deactive=RGB(20,20,40);
    fs->brush_cap_deactive=CreateSolidBrush(fs->color_deactive);
    
    fs->color_border=RGB(53,53,53);
    fs->brush_border=CreateSolidBrush(fs->color_border);
    
    fs->color_text=RGB(255,255,255);
    fs->color_bk=RGB(16,16,16);
    
    fs->pen=CreatePen(PS_SOLID,BORDERPIXLS,fs->color_border);
    
    fs->brush_btnclose=CreateSolidBrush(RGB(255,0,0));
    fs->brush_btn=CreateSolidBrush(RGB(200,200,200));
    
    fs->hdc=GetWindowDC(hwnd);
    fs->memdc=CreateCompatibleDC(fs->hdc);
    fs->bmp=CreateCompatibleBitmap(fs->hdc,2500,1500);//GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));//when full screen , something was wrong.
    SelectObject(fs->memdc,fs->bmp);
    SelectObject(fs->memdc,fs->font);
    SelectObject(fs->memdc,fs->pen);
    
    return 0;
}

int Frame_Test(HWND frame)
{
    Frame_InitialSettings(frame);
    
    {
        HMENU menu=LoadMenu((HINSTANCE)GetWindowLongPtr(frame,GWLP_HINSTANCE),"IDR_FRMENU");
        SetMenu(frame,menu);
        //Try to adjust the height of the caption zone. not work.
        //RECT rc={0};
        //GetWindowRect(frame,&rc);
        //SetWindowPos(frame,NULL,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
    }
    
    HWND btn=CreateWindowEx(NULL,
             			   WC_EDIT, "",
             			   WS_TABSTOP|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,
             			   15,15,150,17,
             			   frame, (HMENU)0x0001,
             			   (HINSTANCE)GetWindowLongPtr(frame, GWLP_HINSTANCE), NULL);
    SendMessage(btn,WM_SETFONT,(WPARAM)Frame_GetSettings(frame)->font,0);
    return 0;
}

int Frame_PopMenu(HWND hwnd,POINT pt)
{
    int item_count;
    HMENU menu;
    HMENU sub_menu;
    RECT mi_rc,win_rc;
    POINT pos;

    GetWindowRect(hwnd,&win_rc);
    pos=pt;
    pt.x=pt.x-win_rc.left;
    pt.y=pt.y-win_rc.top;
    
    menu=GetMenu(hwnd);
    item_count=GetMenuItemCount(menu);
    for(int index=0;index<item_count;index++) {
        Frame_GetMenuItemRect(hwnd,index,&mi_rc);
        if(PtInRect(&mi_rc,pt)) {
            sub_menu=GetSubMenu(menu,index);
            TrackPopupMenu(sub_menu,TPM_LEFTALIGN|TPM_TOPALIGN,mi_rc.left+win_rc.left,mi_rc.bottom+win_rc.top,0,hwnd,NULL);                                
            return 0;
        }
    }
    return 0;
}

BOOL Frame_IsMenuBarRect(HWND hwnd,POINT pt)
{
    int item_count;
    RECT mb_rc;
    MENUBARINFO mb_if;
    
    Frame_GetMenuItemRect(hwnd,0,&mb_rc);
    mb_if.cbSize=sizeof(mb_if);
    GetMenuBarInfo(hwnd,OBJID_MENU,0,&mb_if);
    OffsetRect(&(mb_if.rcBar),-mb_if.rcBar.left,-mb_if.rcBar.top);
    mb_rc.left=mb_if.rcBar.left;
    mb_rc.right=mb_if.rcBar.right;
    
    return PtInRect(&mb_rc,pt);
}

int Frame_GetMenuItemRect(HWND hwnd,int index,LPRECT prc)
{
    HMENU menu;
    MENUBARINFO mb_if;
    POINT pt_offset;
    RECT rc_bar;
    
    Frame_GetNCZoneRect(hwnd,ZMENUBAR,&rc_bar,TRUE);
    menu=GetMenu(hwnd);
    mb_if.cbSize=sizeof(mb_if);
    GetMenuBarInfo(hwnd,OBJID_MENU,1,&mb_if);
    pt_offset={rc_bar.left-mb_if.rcBar.left,rc_bar.top-mb_if.rcBar.top};
    
    GetMenuBarInfo(hwnd,OBJID_MENU,index+1,&mb_if);
    CopyRect(prc,&(mb_if.rcBar));
    OffsetRect(prc,pt_offset.x+index*BTNMARGIN+BORDERPIXLS,pt_offset.y);
    
    return 0;
}

int Frame_NCDraw(HWND hwnd,LPVOID param)//BOOL active_flag)
{
    BOOL active_flag=*(BOOL*)param;
    pFrameStyle fs=Frame_GetSettings(hwnd);
    HICON icon;
    RECT rc={0},rc_mem={0},rc_cap,rc_ico,rc_title,rc_bar,rc_min,rc_max,rc_x;
    POINT pt_cursor;
    char text[256]="";
    COLORREF color_trans=RGB(0,0,1);
    HBRUSH brush_trans=CreateSolidBrush(color_trans),pre_brush;
    
    if(!fs) return 1;
    
    GetWindowRect(hwnd,&rc);
    CopyRect(&rc_mem,&rc);
    OffsetRect(&rc_mem,-rc_mem.left,-rc_mem.top);
    
    //获取鼠标位置
    GetCursorPos(&pt_cursor);
    pt_cursor.x-=rc.left;
    pt_cursor.y-=rc.top;
    
    //刷新画板
    pre_brush=(HBRUSH)SelectObject(fs->memdc,brush_trans);
    SelectObject(fs->memdc,fs->pen);
    Rectangle(fs->memdc,rc_mem.left,rc_mem.top,rc_mem.right,rc_mem.bottom);
    DeleteObject(SelectObject(fs->memdc,pre_brush));
    
    //填充标题
    Frame_GetNCZoneRect(hwnd,ZCAPTION,&rc_cap,TRUE);
    FillRect(fs->memdc,&rc_cap,active_flag?fs->brush_cap_active:fs->brush_cap_deactive);
    
    //绘制图标
    Frame_GetNCZoneRect(hwnd,ZICON,&rc_ico,TRUE);
    icon=(HICON)SendMessage(hwnd,WM_GETICON,(WPARAM)ICON_SMALL,0);//ICON_BIG 大图标
    if(icon==NULL) icon=(HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
    if(icon == NULL) {
        HWND hwnd_owner=GetWindow(hwnd,GW_OWNER);
        if(hwnd_owner) icon=(HICON)GetClassLongPtrW(hwnd_owner, GCLP_HICONSM);
        if(icon==NULL) icon=(HICON)LoadIcon(NULL,IDI_APPLICATION);
    }
    DrawIconEx(fs->memdc,rc_ico.left,rc_ico.top,icon,ICONPIXLS,ICONPIXLS,0,0,DI_NORMAL);//是否需要释放?
    
    //绘制标题文本
    Frame_GetNCZoneRect(hwnd,ZTEXTTITLE,&rc_title,TRUE);
    GetWindowText(hwnd,text,sizeof(text));
    SetBkMode(fs->memdc,TRANSPARENT);
    SetTextColor(fs->memdc,fs->color_text);
    DrawText(fs->memdc,text,-1,&rc_title,DT_SINGLELINE|DT_VCENTER);
    
    //绘制MenuBar
    if(Frame_GetNCZoneRect(hwnd,ZMENUBAR,&rc_bar,TRUE)>0) {
        HMENU menu;
        MENUBARINFO mb_if;
        MENUITEMINFO mi_if;
        int count;
        char menu_text[256]="";
        RECT rc_mi;//菜单项区域
        POINT pt_offset;
        
        gradient_rect(fs->memdc,rc_bar,active_flag?fs->color_active:fs->color_deactive,fs->color_bk,1);
        //绘制菜单项.
        menu=GetMenu(hwnd);
        count=GetMenuItemCount(menu);
        
        mb_if.cbSize=sizeof(mb_if);
        for(int index=1;index<=count;index++) {
            GetMenuBarInfo(hwnd,OBJID_MENU,index,&mb_if);//菜单项的index从1开始
            
            mi_if.cbSize=sizeof(mi_if);
            mi_if.fMask=MIIM_STRING|MIIM_STATE;
            mi_if.dwTypeData=menu_text;
            mi_if.cch=sizeof(menu_text);
            GetMenuItemInfo(menu,index-1,TRUE,&mi_if);
            
            if(index==1) pt_offset={rc_bar.left-mb_if.rcBar.left,rc_bar.top-mb_if.rcBar.top};
            
            CopyRect(&rc_mi,&(mb_if.rcBar));
            OffsetRect(&rc_mi,pt_offset.x+index*BTNMARGIN+BORDERPIXLS,pt_offset.y);
            if(PtInRect(&rc_mi,pt_cursor)) gradient_rect(fs->memdc,rc_mi,RGB(15,15,15),RGB(55,55,55),1);
            DrawText(fs->memdc,menu_text,-1,&rc_mi,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        }
    }
    
    //绘制关闭
    Frame_GetNCZoneRect(hwnd,ZCLOSE,&rc_x,TRUE);
    if(PtInRect(&rc_x,pt_cursor)) FillRect(fs->memdc,&rc_x,fs->brush_btnclose);//关闭
    DrawText(fs->memdc,"×",-1,&rc_x,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    
    //绘制最大化
    Frame_GetNCZoneRect(hwnd,ZMAX,&rc_max,TRUE);
    if(PtInRect(&rc_max,pt_cursor)) FillRect(fs->memdc,&rc_max,fs->brush_btn);
    if(IsZoomed(hwnd)) {
        //最大化 
        DrawText(fs->memdc,"□",-1,&rc_max,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        OffsetRect(&rc_max,2,-2);
        DrawText(fs->memdc,"□",-1,&rc_max,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    }
    else {
        //恢复
        DrawText(fs->memdc,"□",-1,&rc_max,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
    }
    
    //绘制最小化
    Frame_GetNCZoneRect(hwnd,ZMIN,&rc_min,TRUE);
    if(PtInRect(&rc_min,pt_cursor)) FillRect(fs->memdc,&rc_min,fs->brush_btn);
    DrawText(fs->memdc,"-",-1,&rc_min,DT_SINGLELINE|DT_VCENTER|DT_CENTER);

    for(int index=0;index<BORDERPIXLS;index++) {
         FrameRect(fs->memdc,&rc_mem,fs->brush_border);
         InflateRect(&rc_mem,-1,-1);
    }
    
    OffsetRect(&rc,-rc.left,-rc.top);
    TransparentBlt(fs->hdc,rc.left,rc.top,(rc.right-rc.left),(rc.bottom-rc.top),
                   fs->memdc,0,0,(rc.right-rc.left),(rc.bottom-rc.top),color_trans);
                   
    return 0;
}

int Frame_CopyBtnRect(HWND hwnd,LPRECT pbtn_rc,int btn_count)
{
    RECT rc_frame;
    int rc_length;
    int btn_height=20,btn_width=25;
    int btn_margin=10;//最按钮距离右侧边框的pixs
    
    if(hwnd==NULL) return -1;
    if(pbtn_rc==NULL) return -1;
    
    GetWindowRect(hwnd,&rc_frame);
    OffsetRect(&rc_frame,-rc_frame.left,-rc_frame.top);
    rc_length=rc_frame.right-rc_frame.left;
    
    for(int index=btn_count;index>0;index--) {
        pbtn_rc[index-1]={rc_length-btn_margin-btn_width*index,btn_margin,rc_length-btn_margin-btn_width*(index-1),btn_margin+btn_height};
    }
    
    return 0;
}

void gradient_rect(HDC hdc,RECT rect,COLORREF rgb_1,COLORREF rgb_2,int verical_or_horizen)
{
    int length_pixs;
    int index_pixs = 0;
    COLORREF step_color;
    HPEN step_pen=NULL,pre_pen=NULL,tmp_pen=NULL;
    POINT pre_point;
    
    if(verical_or_horizen == 0) {
        length_pixs = rect.right - rect.left;
    }
    else length_pixs = rect.bottom - rect.top;
    
    //最好在memdc上操作
    while(index_pixs < length_pixs) {
        step_color = STEP_COLOR(rgb_1,rgb_2,length_pixs,index_pixs);
        step_pen = CreatePen(PS_SOLID,1,step_color);
        tmp_pen=(HPEN)SelectObject(hdc,step_pen);
        if(index_pixs == 0) pre_pen = tmp_pen;
        
        if(verical_or_horizen == 0) {
            MoveToEx(hdc,rect.left+index_pixs,rect.top,&pre_point);
            LineTo(hdc,rect.left+index_pixs,rect.bottom);
        }
        else {
            MoveToEx(hdc,rect.left,rect.top+index_pixs,&pre_point);
            LineTo(hdc,rect.right,rect.top+index_pixs);
        }
        
        DeleteObject(step_pen);
        index_pixs++;
    }
    SelectObject(hdc,pre_pen);
}


int Frame_DrawMenuItem(LPDRAWITEMSTRUCT draw)
{
    UINT menu_item_id=draw->itemID;
    char text[256]="";
    RECT rect;
    HDC hdc;
    HMENU menu;
    HWND menu_hwnd;
    MENUITEMINFO mi_if={0}/*,itemtype={0}*/;
    COLORREF rgb_bk=RGB(43,43,43);//RGB(70,70,70);
    COLORREF rgb_disabled=RGB(150,150,150);
    COLORREF rgb_text_disabled=RGB(100,100,100);
    COLORREF rgb_check=RGB(65,65,65);
    COLORREF rgb_text_check=RGB(0,128,255);
    COLORREF rgb_text=RGB(255,255,255);
    HBRUSH brush,pre_brush;
    HPEN pen,pre_pen;
    
    hdc=draw->hDC;
    InflateRect(&(draw->rcItem),1,1);
    CopyRect(&rect,&(draw->rcItem));
    menu=(HMENU)draw->hwndItem;
    menu_hwnd=draw->hwndItem;
    
    mi_if.cbSize=sizeof(mi_if);
    mi_if.fMask=MIIM_STRING|MIIM_STATE|MIIM_ID;
    mi_if.dwTypeData=text;
    mi_if.cch=sizeof(text);
    GetMenuItemInfo(menu,menu_item_id,FALSE,&mi_if);
    
    if(draw->itemState&ODS_CHECKED==ODS_CHECKED) {
        if(mi_if.fState&MFS_DISABLED==MFS_DISABLED) {
            brush=CreateSolidBrush(rgb_bk);
            pre_brush=(HBRUSH)SelectObject(hdc,brush);
            SetTextColor(hdc,rgb_text_disabled);
            pen=CreatePen(PS_SOLID,1,rgb_text_disabled);
            pre_pen=(HPEN)SelectObject(hdc,pen);
        }
        else {
            brush=CreateSolidBrush(rgb_check);
            pre_brush=(HBRUSH)SelectObject(hdc,brush);
            SetTextColor(hdc,rgb_text_check);
            pen=CreatePen(PS_SOLID,1,rgb_check);
            pre_pen=(HPEN)SelectObject(hdc,pen);
        }
    }
    else {
        brush=CreateSolidBrush(rgb_bk);
        pre_brush=(HBRUSH)SelectObject(hdc,brush);
        if(mi_if.fState&MFS_DISABLED==MFS_DISABLED) {
            SetTextColor(hdc,rgb_text_disabled);
            pen=CreatePen(PS_SOLID,1,rgb_text_disabled);
            pre_pen=(HPEN)SelectObject(hdc,pen);
        }
        else {
            SetTextColor(hdc,rgb_text);
            pen=CreatePen(PS_SOLID,1,rgb_bk);
            pre_pen=(HPEN)SelectObject(hdc,pen);
        }
    }

    SetBkMode(hdc,TRANSPARENT);
    FillRect(hdc,&rect,brush);
    if(strlen(text)>0) {
        RECT check_rect;
        CopyRect(&check_rect,&rect);
        check_rect.right=rect.left+(rect.bottom-rect.top);

        if((UINT)(mi_if.fState&MFS_CHECKED)==(UINT)MFS_CHECKED) {
            DrawText(hdc,"√",-1,&check_rect,DT_SINGLELINE|DT_VCENTER|DT_CENTER);
        }
        rect.left = rect.left+(rect.bottom-rect.top);
        DrawText(hdc,text,-1,&rect,DT_SINGLELINE|DT_VCENTER|DT_EXPANDTABS);//DT_EXPANDTABS识别\t
    }
    else {
        //绘制菜单分隔条
        POINT pt[5]={{rect.left+5,(rect.top+rect.bottom)/2},
                     {rect.right-5,(rect.top+rect.bottom)/2},
                     {rect.left+5,(rect.top+rect.bottom)/2+1},
                     {rect.right-5,(rect.top+rect.bottom)/2+1}};
        
        COLORREF color_1=RGB(19,19,19),color_2=RGB(68,68,68);
        HPEN sp_line1=CreatePen(PS_SOLID,1,color_1);
        HPEN sp_line2=CreatePen(PS_SOLID,1,color_2);
        HPEN pre_sp;
        
        pre_sp=(HPEN)SelectObject(hdc,sp_line1);
        MoveToEx(hdc,pt[0].x,pt[0].y,&(pt[4]));
        LineTo(hdc,pt[1].x,pt[1].y);
        
        SelectObject(hdc,sp_line2);
        MoveToEx(hdc,pt[2].x,pt[2].y,&(pt[4]));
        LineTo(hdc,pt[3].x,pt[3].y);
        SelectObject(hdc,pre_sp);
        DeleteObject(sp_line1);
        DeleteObject(sp_line2);
    }
    
    DeleteObject(SelectObject(hdc,pre_brush));
    DeleteObject(SelectObject(hdc,pre_pen));
    return 0;
}

/*
allign_topleft:TRUE 相对于窗口左上角的坐标
               FALSE 屏幕坐标
 */
int Frame_GetNCZoneRect(HWND hwnd,EFNCZone type,LPRECT prc,BOOL allign_topleft)
{
    RECT rc={0};
    BOOL menubar_exist=(NULL!=GetMenu(hwnd));
    int assign_flag=-1;
    GetWindowRect(hwnd,&rc);
    
    switch(type) {
    case ZCLOSE: {
        prc->top=rc.top+BTNMARGIN;
        prc->bottom=prc->top+BTNPIXLSY;
        prc->right=rc.right-BTNMARGIN;
        prc->left=prc->right-BTNPIXLSY;
        assign_flag=1;
    } break;
    case ZMAX: {
        prc->top=rc.top+BTNMARGIN;
        prc->bottom=prc->top+BTNPIXLSY;
        prc->right=rc.right-BTNMARGIN-BTNPIXLSY;
        prc->left=prc->right-BTNPIXLSY;
        assign_flag=1;
    } break;
    case ZMIN: {
        prc->top=rc.top+BTNMARGIN;
        prc->bottom=prc->top+BTNPIXLSY;
        prc->right=rc.right-BTNMARGIN-BTNPIXLSY*2;
        prc->left=prc->right-BTNPIXLSY;
        assign_flag=1;
    } break;
    case ZICON: {
        prc->top=rc.top+ICONMARGIN;
        prc->bottom=prc->top+ICONPIXLS;
        prc->left=rc.left+ICONMARGIN;
        prc->right=prc->left+ICONPIXLS;
        assign_flag=1;
    } break;
    case ZMENUBAR: {
        if(!menubar_exist) break;
        prc->top=rc.top+ICONMARGIN*2+ICONPIXLS;
        prc->bottom=prc->top+MENUBARY;
        prc->left=rc.left;
        prc->right=rc.right;
        assign_flag=1;
    } break;
    case ZCAPTION: {
        prc->top=rc.top+BORDERPIXLS;
        prc->bottom=rc.top+ICONMARGIN*2+ICONPIXLS;
        prc->left=rc.left+BORDERPIXLS;
        prc->right=rc.right-BORDERPIXLS;
        assign_flag=1;
    } break;
    case ZTEXTTITLE: {
        prc->left=rc.left+ICONMARGIN*2+ICONPIXLS;
        prc->right=rc.right-BTNMARGIN-BTNPIXLSX*3;
        prc->top=rc.top;
        prc->bottom=rc.top+ICONMARGIN*2+ICONPIXLS;
        assign_flag=1;
    } break;
    }
    
    if(assign_flag==1&&allign_topleft) {
        OffsetRect(prc,-rc.left,-rc.top);
    }
    return assign_flag;
}

int Frame_NCCalcSize(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    RECT rect_new;
    RECT rect_old;
    RECT client_rect_new;
    RECT client_rect_old;
    BOOL HasMenu=(GetMenu(hwnd)!=NULL);//失效原因应该与WM_NCCALCSIZE的触发时机有关.在创建时触发
    {
        char text[256]="";
        sprintf(text,"has Menu:%s",HasMenu?"True":"False");
        MessageBox(hwnd,text,"NCCalcSize",MB_OK);
    }
    if(wParam == TRUE) {
        LPNCCALCSIZE_PARAMS calc_param = (LPNCCALCSIZE_PARAMS)lParam;
        
        CopyRect(&rect_new,&(calc_param->rgrc[0]));
        CopyRect(&rect_old,&(calc_param->rgrc[1]));
        CopyRect(&client_rect_old,&(calc_param->rgrc[2]));
        
        client_rect_new = {rect_new.left+BORDERPIXLS,
                           rect_new.top+ICONMARGIN*2+ICONPIXLS+(HasMenu?MENUBARY:0),
                           rect_new.right-BORDERPIXLS,
                           rect_new.bottom-BORDERPIXLS};
        CopyRect(&(calc_param->rgrc[0]),&client_rect_new);
        CopyRect(&(calc_param->rgrc[1]),&rect_new);
        CopyRect(&(calc_param->rgrc[2]),&rect_old);
        
        return WVR_VALIDRECTS;
    }
    else {
        RECT* prect = (RECT*)lParam;
        
        prect->left+=BORDERPIXLS; 
        prect->right-=BORDERPIXLS; 
        prect->top+=ICONMARGIN*2+ICONPIXLS+(HasMenu?MENUBARY:0); 
        prect->bottom-=BORDERPIXLS;
        return 0;
    }
}

int Frame_InitialMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    HMENU menu=(HMENU)wParam;
    int pos=(UINT)LOWORD(lParam);
    BOOL sysmenu_flag=(BOOL)HIWORD(lParam);
    int item_count;
    MENUINFO mif={0};
    MENUITEMINFO miif={0};
    
    pFrameStyle fs=Frame_GetSettings(hwnd);
    if(!fs) return 0;
    
    item_count=GetMenuItemCount(menu);
    for(int index=0;index<item_count;index++) {
        miif.cbSize=sizeof(miif);
        miif.fMask=MIIM_STATE|MIIM_ID;
        GetMenuItemInfo(menu,index,TRUE,&miif);
        
        miif.cbSize=sizeof(miif);
        miif.fMask=MIIM_ID|MIIM_FTYPE;
        miif.fType=MFT_OWNERDRAW;//Menu Item Owner drawing
        SetMenuItemInfo(menu,index,TRUE,&miif);
    }
    //menu item backgnd color settings.
    mif.cbSize=sizeof(MENUINFO);
    mif.fMask=MIM_BACKGROUND;
    mif.hbrBack=fs->brush_mibk;
    SetMenuInfo(menu,&mif);
    return 0;
}

int Frame_NCHitTest(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    RECT rc,rc_cap,rc_bar;
    RECT rc_min,rc_max,rc_x,rc_ico;
    char text[512]="";
    POINT hit={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
    LRESULT result=DefWindowProc(hwnd,WM_NCHITTEST,wParam,lParam);
    
    Frame_GetNCZoneRect(hwnd,ZCAPTION,&rc_cap,FALSE);
    Frame_GetNCZoneRect(hwnd,ZMENUBAR,&rc_bar,FALSE);
    Frame_GetNCZoneRect(hwnd,ZCLOSE,&rc_x,FALSE);
    Frame_GetNCZoneRect(hwnd,ZMAX,&rc_max,FALSE);
    Frame_GetNCZoneRect(hwnd,ZMIN,&rc_min,FALSE);
    Frame_GetNCZoneRect(hwnd,ZICON,&rc_ico,FALSE);
    
    if(PtInRect(&rc_bar,hit)) return HTMENU;
    else if(result==HTMENU) return HTCAPTION;
    else if(PtInRect(&rc_min,hit)) result=HTMINBUTTON;
    else if(PtInRect(&rc_max,hit)) result=HTMAXBUTTON;
    else if(PtInRect(&rc_x,hit)) result=HTCLOSE;
    else if(PtInRect(&rc_cap,hit)) result=HTCAPTION;
    
    return result;
}