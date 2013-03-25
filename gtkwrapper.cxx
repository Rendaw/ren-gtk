#include "gtkwrapper.h"

#include "../ren-general/region.h"
#include "../ren-general/range.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

const char *ConvertStock(DefaultIcons From)
{
	switch (From)
	{
		case diNone: return nullptr; // Who knows?
		case diUp: return GTK_STOCK_GO_UP;
		case diDown: return GTK_STOCK_GO_DOWN;
		case diAdd: return GTK_STOCK_ADD;
		case diRemove: return GTK_STOCK_REMOVE;
		case diDelete: return GTK_STOCK_DELETE;
		case diNew: return GTK_STOCK_NEW;
		case diOpen: return GTK_STOCK_OPEN;
		case diSave: return GTK_STOCK_SAVE;
		case diRefresh: return GTK_STOCK_REFRESH;
		case diClear: return GTK_STOCK_CLEAR;
		case diFirst: return GTK_STOCK_GOTO_FIRST;
		case diLast: return GTK_STOCK_GOTO_LAST;
		case diTop: return GTK_STOCK_GOTO_TOP;
		case diBottom: return GTK_STOCK_GOTO_BOTTOM;
		case diPlay: return GTK_STOCK_MEDIA_PLAY;
		case diPause: return GTK_STOCK_MEDIA_PAUSE;
		case diRewind: return GTK_STOCK_MEDIA_REWIND;
		case diStop: return GTK_STOCK_MEDIA_STOP;
		case diInfo: return GTK_STOCK_INFO;
		case diProperties: return GTK_STOCK_PROPERTIES;
		case diConfigure: return GTK_STOCK_PREFERENCES;
		case diOkay: return GTK_STOCK_OK;
		case diCancel: return GTK_STOCK_CANCEL;
		case diClose: return GTK_STOCK_CLOSE;
		case diQuit: return GTK_STOCK_QUIT;
		default: assert(false); return nullptr;
	}
}

namespace GTK
{
	void Error(GtkWidget *Anchor, const String &Section, const String &Description)
	{
		g_print("%s: %s\n", Section.c_str(), Description.c_str());

		GtkWidget *Message = gtk_message_dialog_new(
			GTK_WINDOW(gtk_widget_get_toplevel(Anchor)),
			GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
			"%s", Section.c_str());
		gtk_dialog_add_button(GTK_DIALOG(Message), GTK_STOCK_QUIT, 0);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), "%s", Description.c_str());
		gtk_dialog_run(GTK_DIALOG(Message));
		gtk_main_quit();
	}

	void Warning(GtkWidget *Anchor, const String &Section, const String &Description)
	{
		g_print("%s: %s\n", Section.c_str(), Description.c_str());

		GtkWidget *Message = gtk_message_dialog_new(
			GTK_WINDOW(gtk_widget_get_toplevel(Anchor)),
			GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
			"%s", Section.c_str());
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), "%s", Description.c_str());
		gtk_dialog_run(GTK_DIALOG(Message));
		gtk_widget_destroy(Message);
	}

	bool Confirm(GtkWidget *Anchor, const String &Section, const String &Description)
	{
		GtkWidget *Message = gtk_message_dialog_new(
			GTK_WINDOW(gtk_widget_get_toplevel(Anchor)),
			GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"%s", Section.c_str());
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), "%s", Description.c_str());
		int Result = gtk_dialog_run(GTK_DIALOG(Message));
		gtk_widget_destroy(Message);

		return Result == GTK_RESPONSE_YES;
	}
}

///////////////////////////////////////////////////////////
// Non
Widget::Widget(GtkWidget *Data) : Data(Data), Destroy(false) {}
Widget::~Widget() { if (Destroy) gtk_widget_destroy(Data); }
Widget::operator GtkWidget*(void) { return Data; }
void Widget::Hide(void) { gtk_widget_hide(Data); }
void Widget::Show(void) { gtk_widget_show(Data); }
void Widget::Disable(void) { gtk_widget_set_sensitive(Data, false); }
void Widget::Enable(void) { gtk_widget_set_sensitive(Data, true); }
void Widget::DestroyWhenDeleted(void) { Destroy = true; }

// Nun?
TimedEvent::TimedEvent(unsigned int Milliseconds) : Period(Milliseconds), TimerID(0) {}

TimedEvent::~TimedEvent(void)
	{ assert(Handler); StopTimer(); }
		
void TimedEvent::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void TimedEvent::StartTimer(void)
{
	assert(Handler);
	if (!Handler) return;
	if (TimerID == 0)
		TimerID = g_timeout_add(Period, (GSourceFunc)TimeHandler, this);
}

void TimedEvent::StopTimer(void)
{
	if (TimerID != 0)
	{
		g_source_remove(TimerID);
		TimerID = 0;
	}
}

unsigned int TimedEvent::GetTimerPeriod(void) const
	{ return Period; }

gboolean TimedEvent::TimeHandler(TimedEvent *This)
	{ This->Handler(); return TRUE; }

///////////////////////////////////////////////////////////
// Widget extensions
KeyboardWidget::KeyboardWidget(GtkWidget *Data) :
	Data(Data),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "key-press-event", G_CALLBACK(KeyCallback), this)),
	Destroy(false)
{
	gtk_widget_add_events(Data, GDK_KEY_PRESS);
}

KeyboardWidget::~KeyboardWidget(void)
	{ assert(this->Handler); if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID); }
		
void KeyboardWidget::SetHandler(KeyHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void KeyboardWidget::DestroyWhenDeleted(void) { Destroy = true; }

gboolean KeyboardWidget::KeyCallback(GtkWidget *, GdkEventKey *Event, KeyboardWidget *This)
{ 
	assert(This->Handler);
	if (!This->Handler) return false;
	return This->Handler(Event->keyval, Event->state); 
}

//
MenuItem::MenuItem(String const &Text) : Widget(gtk_menu_item_new_with_label(Text.c_str())),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "activate", G_CALLBACK(ClickHandler), this))
	{}

MenuItem::MenuItem(String const &Text, DefaultIcons const Icon) : Widget(gtk_image_menu_item_new_with_label(Text.c_str())),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "activate", G_CALLBACK(ClickHandler), this))
{
	GtkWidget *IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Data), IconWidget);
}

MenuItem::MenuItem(String const &Text, String const &IconFilename) : Widget(gtk_image_menu_item_new_with_label(Text.c_str())),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "activate", G_CALLBACK(ClickHandler), this))
{
	GtkWidget *IconWidget = gtk_image_new_from_file(IconFilename.c_str());
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(Data), IconWidget);
}

MenuItem::~MenuItem(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID); }
		
void MenuItem::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void MenuItem::ClickHandler(GtkMenuItem *, MenuItem *This)
	{ if (This->Handler != nullptr) This->Handler(); }

//
ToolButton::ToolButton(String const &Text) :
	Widget(GTK_WIDGET(gtk_tool_button_new(nullptr, Text.c_str()))),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(ClickHandler), this))
	{}

ToolButton::ToolButton(String const &Text, DefaultIcons const Icon) :
	Widget(GTK_WIDGET(gtk_tool_button_new_from_stock(ConvertStock(Icon)))),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(ClickHandler), this))
	{ if (!Text.empty()) gtk_tool_button_set_label(GTK_TOOL_BUTTON(Data), Text.c_str()); }

ToolButton::~ToolButton(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID); }

void ToolButton::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void ToolButton::SetPrompt(String const &NewPrompt)
	{ gtk_tool_button_set_label(GTK_TOOL_BUTTON(Data), NewPrompt.c_str()); }

void ToolButton::ClickHandler(GtkToolItem *, ToolButton *This)
	{ if (This->Handler != nullptr) This->Handler(); }

///////////////////////////////////////////////////////////
// Window type
Window::Window(const String &Title, unsigned int const &EdgePadding) : Widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
	AttemptCloseConnectionID(g_signal_connect(G_OBJECT(Data), "delete_event", G_CALLBACK(AttemptCloseCallback), this)),
	CloseConnectionID(g_signal_connect(G_OBJECT(Data), "destroy", G_CALLBACK(CloseCallback), this))
{
	gtk_window_set_title(GTK_WINDOW(Data), Title.c_str());
	gtk_container_set_reallocate_redraws(GTK_CONTAINER(Data), true);
	gtk_container_set_border_width(GTK_CONTAINER(Data), EdgePadding);
}

Window::~Window(void)
{
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), AttemptCloseConnectionID);
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), CloseConnectionID);
}
		
void Window::SetAttemptCloseHandler(std::function<bool(void)> const &Handler)
{
	assert(!AttemptCloseHandler);
	AttemptCloseHandler = Handler;
}

void Window::SetCloseHandler(std::function<void(void)> const &Handler)
{
	assert(!CloseHandler);
	CloseHandler = Handler;
}
		
void Window::SetResizeHandler(std::function<void(void)> const &Handler)
{
	assert(!ResizeHandler);
	ResizeHandler = Handler;
	g_signal_connect(G_OBJECT(Data), "configure-event", G_CALLBACK(ResizeCallback), this);
}

void Window::SetIcon(const String &Filename)
	{ gtk_window_set_icon_from_file(GTK_WINDOW(Data), Filename.c_str(), nullptr); }

void Window::SetTitle(const String &NewTitle)
	{ gtk_window_set_title(GTK_WINDOW(Data), NewTitle.c_str()); }

void Window::SetFullscreen(bool On)
{
	if (On) gtk_window_fullscreen(GTK_WINDOW(Data));
	else gtk_window_unfullscreen(GTK_WINDOW(Data));
}

void Window::SetDefaultSize(const FlatVector &Size)
	{ gtk_window_set_default_size(GTK_WINDOW(Data), Size[0], Size[1]); }

void Window::Set(GtkWidget *Addee)
	{ gtk_container_add(GTK_CONTAINER(Data), Addee); gtk_widget_show(Addee); }
		
gboolean Window::AttemptCloseCallback(GtkWidget *, GdkEvent *, Window *This)
	{ if (This->AttemptCloseHandler) return !This->AttemptCloseHandler(); return false; }

void Window::CloseCallback(GtkWidget *, Window *This) 
	{ if (This->CloseHandler) This->CloseHandler(); }

gboolean Window::ResizeCallback(GtkWidget *, GdkEventConfigure *, Window *This)
	{ assert(This->ResizeHandler); This->ResizeHandler(); return FALSE; }

// Dialog window, doesn't appear til' Run is called
Dialog::Dialog(GtkWidget *Parent, const String &Title, FlatVector const &DefaultSize) : Widget(gtk_dialog_new())
{
	gtk_window_set_title(GTK_WINDOW(Data), Title.c_str());
	gtk_window_set_default_size(GTK_WINDOW(Data), DefaultSize[0], DefaultSize[1]);
	gtk_container_set_reallocate_redraws(GTK_CONTAINER(Data), true);

	// NOTE Parent must be realized already and in some container
	gtk_window_set_transient_for(GTK_WINDOW(Data), GTK_WINDOW(gtk_widget_get_toplevel(Parent)));
}

Dialog::~Dialog(void)
	{ gtk_widget_destroy(Data); }

void Dialog::Add(GtkWidget *Widget)
{
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(Data))), Widget, false, true, 0);
	gtk_widget_show(Widget);
}

void Dialog::AddFill(GtkWidget *Widget)
{
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(Data))), Widget, true, true, 0);
	gtk_widget_show(Widget);
}

void Dialog::SetDefaultSize(const FlatVector &DefaultSize)
	{ gtk_window_set_default_size(GTK_WINDOW(Data), DefaultSize[0], DefaultSize[1]); }

void Dialog::AddAction(GtkWidget *Widget)
{
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(Data))), Widget, false, true, 0);
	gtk_widget_show(Widget);
}

void Dialog::AddActionFill(GtkWidget *Widget)
{
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(Data))), Widget, true, true, 0);
	gtk_widget_show(Widget);
}

void Dialog::Run(void)
	{ gtk_dialog_run(GTK_DIALOG(Data)); }

void Dialog::Close(void)
	{ gtk_dialog_response(GTK_DIALOG(Data), GTK_RESPONSE_NONE); }

// Popup menu
PopupMenu::PopupMenu() : MenuData(gtk_menu_new())
	{}

PopupMenu::~PopupMenu(void)
	{ gtk_widget_destroy(MenuData); }
		
void PopupMenu::SetPositionHandler(MenuPositionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void PopupMenu::Clear(void)
{
	gtk_widget_destroy(MenuData);
	MenuData = gtk_menu_new();
}

MenuItem *PopupMenu::Add(MenuItem *NewItem)
{
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), *NewItem);
	gtk_widget_show(*NewItem);
	return NewItem;
}

void PopupMenu::AddSeparator(void)
{
	GtkWidget *Separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), Separator);
	gtk_widget_show(Separator);
}

void PopupMenu::Activate(void)
{
	if (Handler) gtk_menu_popup(GTK_MENU(MenuData),
		nullptr, nullptr, (void (*)(GtkMenu *, gint *, gint *, gboolean *, void *))PositionCallback, this, 0, gtk_get_current_event_time());
	else gtk_menu_popup(GTK_MENU(MenuData),
		nullptr, nullptr, nullptr, this, 0, gtk_get_current_event_time());
}
		
void PopupMenu::PositionCallback(GtkMenu *, gint *X, gint *Y, gboolean *ForceVisible, PopupMenu *This)
{
	This->Handler(*X, *Y);
	*ForceVisible = TRUE;
}

///////////////////////////////////////////////////////////
// Box container wrapper
Layout::Layout(bool Horizontal, int EdgePadding, int ItemPadding) :
	Widget(Horizontal ? gtk_hbox_new(false, ItemPadding) : gtk_vbox_new(false, ItemPadding)),
	Horizontal(Horizontal)
	{ gtk_container_set_border_width(GTK_CONTAINER(Data), EdgePadding); }

void Layout::Add(GtkWidget *Widget)
{
	assert(Widget != nullptr);
	gtk_box_pack_start(GTK_BOX(Data), Widget, false, true, 0);
	gtk_widget_show(Widget);
}

void Layout::AddFill(GtkWidget *Widget)
{
	assert(Widget != nullptr);
	gtk_box_pack_start(GTK_BOX(Data), Widget, true, true, 0);
	gtk_widget_show(Widget);
}

void Layout::AddSpacer(void)
	{ Add(Horizontal ? gtk_vseparator_new() : gtk_hseparator_new()); }

void Layout::AddSpace(void)
	{ AddFill(Horizontal ? gtk_hbox_new(false, 0) : gtk_vbox_new(false, 0)); }

ColorLayout::ColorLayout(bool Horizontal, int EdgePadding, int ItemPadding) : Widget(gtk_event_box_new()), 
	Inner(Horizontal, EdgePadding, ItemPadding)
{ 
	gtk_container_add(GTK_CONTAINER(Data), Inner); 
	gtk_widget_show(Inner);
}

Color ColorLayout::GetDefaultColor(void) const
{
	GdkColor Got;
	if (!gtk_style_lookup_color(gtk_widget_get_style(Data), "background-color", &Got)) return Color(1, 1, 1);
	return Color((float)Got.red / 65535.0f, (float)Got.green / 65535.0f, (float)Got.blue / 65535.0f);
}

void ColorLayout::SetColor(Color const &NewColor)
{
	GdkColor SetColor;
	SetColor.red = 65535 * NewColor.Red;
	SetColor.green = 65535 * NewColor.Green;
	SetColor.blue = 65535 * NewColor.Blue;
	gtk_widget_modify_bg(Data, GTK_STATE_NORMAL, &SetColor);
}

void ColorLayout::Add(GtkWidget *Widget) 
	{ Inner.Add(Widget); }

void ColorLayout::AddFill(GtkWidget *Widget) 
	{ Inner.AddFill(Widget); }

void ColorLayout::AddSpacer(void) 
	{ Inner.AddSpacer(); }

void ColorLayout::AddSpace(void) 
	{ Inner.AddSpace(); }

// LayoutBorder wrapper
LayoutBorder::LayoutBorder(void) : Widget(gtk_frame_new(nullptr))
	{ gtk_frame_set_shadow_type(GTK_FRAME(Data), GTK_SHADOW_IN); }

LayoutBorder::LayoutBorder(const String &Title) : Widget(gtk_frame_new(Title.c_str()))
	{}

void LayoutBorder::Set(GtkWidget *Settee)
	{ gtk_container_add(GTK_CONTAINER(Data), Settee); gtk_widget_show(Settee); }

// Unhidden notebook
/*Notebook::CloseHandler::~CloseHandler(void) 
	{}*/

Notebook::Notebook(void) : Widget(gtk_notebook_new())
	{}

int Notebook::Add(GtkWidget *Addee, String const &Title)
{
	Label LabelText(Title);
	int Page = gtk_notebook_insert_page(GTK_NOTEBOOK(Data), Addee, LabelText, -1);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(Data), Addee, true);
	LabelText.Show();
	gtk_widget_show(Addee);
	return Page;
}

/*int Notebook::Add(Notebook::CloseHandler *Target, GtkWidget *Addee, String const &Title)
{
	Layout LabelLayout(true, 0, 2);
	LabelLayout.AddFill(Label(Title));
	GtkWidget *CloseButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_button_set_relief(GTK_BUTTON(CloseButton), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(CloseButton), false);
	std::pair<Notebook::CloseHandler *, GtkWidget *> *Data = 
		new std::pair<Notebook::CloseHandler *, GtkWidget *>(Target, Addee);
	g_signal_connect(G_OBJECT(CloseButton), "clicked", G_CALLBACK(HandleClose), Data);
	LabelLayout.Add(CloseButton);
	
	int Page = gtk_notebook_insert_page(GTK_NOTEBOOK(Data), Addee, LabelLayout, -1);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(Data), Addee, true);
	LabelLayout.Show();
	gtk_widget_show(Addee);
	return Page;
}*/

void Notebook::SetPage(int Page)
	{ gtk_notebook_set_current_page(GTK_NOTEBOOK(Data), Page); }

void Notebook::Rename(GtkWidget *Addee, const String &NewTitle)
	{ gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(Data), Addee, NewTitle.c_str()); }

int Notebook::GetPage(void)
	{ return gtk_notebook_get_current_page(GTK_NOTEBOOK(Data)); }

int Notebook::Find(GtkWidget *Addee)
	{ return gtk_notebook_page_num(GTK_NOTEBOOK(Data), Addee); }

/*void Notebook::HandleClose(GtkWidget *Widget, std::pair<Notebook::CloseHandler *, GtkWidget *> *Data)
{
	if (Data->first != nullptr)
		Data->first->PageClosed(Data->second);
	gtk_widget_destroy(Data->second);
	delete Data;
}*/

// Hidden notebook (the tabs are hidden)
HiddenNotebook::HiddenNotebook(void) : Widget(gtk_notebook_new())
	{ gtk_notebook_set_show_tabs(GTK_NOTEBOOK(Data), false); }

int HiddenNotebook::Add(GtkWidget *Addee)
{
	int Page = gtk_notebook_insert_page(GTK_NOTEBOOK(Data), Addee, nullptr, -1);
	gtk_widget_show(Addee);
	return Page;
}

void HiddenNotebook::SetPage(int Page)
	{ gtk_notebook_set_current_page(GTK_NOTEBOOK(Data), Page); }

int HiddenNotebook::GetPage(void)
	{ return gtk_notebook_get_current_page(GTK_NOTEBOOK(Data)); }

// A scroller container
Scroller::Scroller(void) : Widget(gtk_scrolled_window_new(nullptr, nullptr))
{
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Data), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	//gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(Data), GTK_CORNER_TOP_RIGHT);
}

void Scroller::Set(GtkWidget *Settee)
{
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Data), Settee);
	gtk_widget_show(Settee);
}

void Scroller::ShowRange(float Start, float End)
{
	GtkAdjustment *Adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data));
	float Base = gtk_adjustment_get_lower(Adjustment);
	float Difference = gtk_adjustment_get_upper(Adjustment) - Base;
	gtk_adjustment_clamp_page(Adjustment, Base + Difference * Start, Base + Difference * End);
}

// A 2D scroller for only what must necessarily be scrolled in 2D
CanvasScroller::CanvasScroller(void) : Widget(gtk_scrolled_window_new(nullptr, nullptr)),
	InitialAdjustmentCompleted(false),
	HorizontalAdjustment(nullptr), VerticalAdjustment(nullptr),
	HorizontalScrollHandler(-1), VerticalScrollHandler(-1)
{ 
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Data), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS); 
	g_signal_connect(G_OBJECT(Data), "map", G_CALLBACK(InitialStateChangeHandler), this);
}

void CanvasScroller::Set(GtkWidget *Settee)
{
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Data), Settee);
	
	HorizontalAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Data));
	g_signal_connect(G_OBJECT(HorizontalAdjustment), "changed", G_CALLBACK(InitialStateChangeHandler), this);
	HorizontalScrollHandler = g_signal_handler_find(HorizontalAdjustment, G_SIGNAL_MATCH_DATA,
		g_signal_lookup("value-changed", G_TYPE_FROM_INSTANCE(HorizontalAdjustment)),
		0, nullptr, nullptr, GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(Data))));
	
	VerticalAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data));
	VerticalScrollHandler = g_signal_handler_find(VerticalAdjustment, G_SIGNAL_MATCH_DATA,
		g_signal_lookup("value-changed", G_TYPE_FROM_INSTANCE(VerticalAdjustment)),
		0, nullptr, nullptr, GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(Data))));
	
	gtk_widget_show(Settee);
}

void CanvasScroller::ShowRange(FlatVector Start, FlatVector End)
{
	assert(VerticalAdjustment != nullptr);
	// May jerk because 
	gtk_adjustment_clamp_page(VerticalAdjustment, Start[1], End[1]);
	gtk_adjustment_clamp_page(HorizontalAdjustment, Start[0], End[0]);
}

void CanvasScroller::GoTo(FlatVector Position)
{
	if (InitialAdjustmentCompleted)
	{
		assert(VerticalAdjustment != nullptr);
		
		Position -= FlatVector(gtk_adjustment_get_page_size(HorizontalAdjustment),
			gtk_adjustment_get_page_size(VerticalAdjustment)) * 0.5f;

		SetAdjustments(Position[0], Position[1]);
	}
	else 
		InitialAdjustmentFunction = [this, Position]() { GoTo(Position); };
}

void CanvasScroller::GoToPercent(FlatVector Percent)
{
	if (InitialAdjustmentCompleted)
	{
		assert(VerticalAdjustment != nullptr);
		
		Percent *= FlatVector(
			gtk_adjustment_get_upper(HorizontalAdjustment) - gtk_adjustment_get_lower(HorizontalAdjustment),
			gtk_adjustment_get_upper(VerticalAdjustment) - gtk_adjustment_get_lower(VerticalAdjustment));
		Percent += FlatVector(gtk_adjustment_get_lower(HorizontalAdjustment), gtk_adjustment_get_lower(VerticalAdjustment));
		Percent -= FlatVector(gtk_adjustment_get_page_size(HorizontalAdjustment), gtk_adjustment_get_page_size(VerticalAdjustment)) * 0.5f;
		/*std::cout << "GoToPercent: " << Percent.AsString() << " -- horiz lower and upper = " <<
			gtk_adjustment_get_upper(HorizontalAdjustment) << ", " << gtk_adjustment_get_lower(HorizontalAdjustment) <<
			" -- vert lower and upper = " << gtk_adjustment_get_upper(VerticalAdjustment) << ", " << 
			gtk_adjustment_get_lower(VerticalAdjustment) << std::endl;*/
		SetAdjustments(Percent[0], Percent[1]);
	}
	else 
		InitialAdjustmentFunction = [this, Percent]() { GoToPercent(Percent); };
}

/*void CanvasScroller::Nudge(int Cardinality)
{
	if ((Cardinality == 0) || (Cardinality == 2))
	{
		float Jump = gtk_adjustment_get_step_increment(VerticalAdjustment);
		if (Cardinality == 0)
			gtk_adjustment_set_value(VerticalAdjustment, gtk_adjustment_get_value(VerticalAdjustment) - Jump);
		else gtk_adjustment_set_value(VerticalAdjustment, gtk_adjustment_get_value(VerticalAdjustment) + Jump);
	}
	else
	{
		float Jump = gtk_adjustment_get_step_increment(HorizontalAdjustment);
		if (Cardinality == 0)
			gtk_adjustment_set_value(HorizontalAdjustment, gtk_adjustment_get_value(HorizontalAdjustment) + Jump);
		else gtk_adjustment_set_value(HorizontalAdjustment, gtk_adjustment_get_value(HorizontalAdjustment) - Jump);
	}
}*/

void CanvasScroller::Nudge(const FlatVector &Offset)
{
	// NOTE manually clips value becase gtk is broken
	assert(VerticalAdjustment != nullptr);
	SetAdjustments(
		RangeF(0, gtk_adjustment_get_upper(HorizontalAdjustment) - gtk_adjustment_get_page_size(HorizontalAdjustment)).Constrain(
			gtk_adjustment_get_value(HorizontalAdjustment) + Offset[0]),
		RangeF(0, gtk_adjustment_get_upper(VerticalAdjustment) - gtk_adjustment_get_page_size(VerticalAdjustment)).Constrain(
			gtk_adjustment_get_value(VerticalAdjustment) + Offset[1]));   
}

void CanvasScroller::SetAdjustments(int NewX, int NewY)
{
	assert(VerticalAdjustment != nullptr);
	int const
		OldX = gtk_adjustment_get_value(HorizontalAdjustment), 
		OldY = gtk_adjustment_get_value(VerticalAdjustment);
		
	if ((OldX != NewX) && (OldY != NewY))
		gtk_signal_handler_block(HorizontalAdjustment, HorizontalScrollHandler);
	gtk_adjustment_set_value(HorizontalAdjustment, NewX);
	if ((OldX != NewX) && (OldY != NewY))
		gtk_signal_handler_unblock(HorizontalAdjustment, HorizontalScrollHandler);
	gtk_adjustment_set_value(VerticalAdjustment, NewY);
}

void CanvasScroller::DoInitialAdjustment(void)
{
	if (InitialAdjustmentCompleted) return;
	if ((HorizontalAdjustment == nullptr) || (VerticalAdjustment == nullptr)) return; 
	if ((gtk_adjustment_get_upper(HorizontalAdjustment) <= 1) ||
		(gtk_adjustment_get_upper(VerticalAdjustment) <= 1)) return; // Apparently GTK displays things before properly calculating sizes sometimes.  There might be a better way to do this, but this is pretty simple.
	
	InitialAdjustmentCompleted = true;
	
	if (!InitialAdjustmentFunction) return;
	
	InitialAdjustmentFunction();
	InitialAdjustmentFunction = decltype(InitialAdjustmentFunction)();
}

void CanvasScroller::InitialStateChangeHandler(void *, CanvasScroller *This)
	{ This->DoInitialAdjustment(); }


///////////////////////////////////////////////////////////
// Label wrapper

// Title label
Title::Title(const String &Text) : Widget(gtk_label_new(nullptr))
{
	char *MarkupString = g_markup_printf_escaped("<span size=\"large\" weight=\"bold\">%s</span>", Text.c_str());
	gtk_label_set_markup(GTK_LABEL(Data), MarkupString);
	g_free(MarkupString);
}

void Title::SetText(const String &NewText)
{
	char *MarkupString = g_markup_printf_escaped("<span size=\"large\" weight=\"bold\">%s</span>", NewText.c_str());
	gtk_label_set_markup(GTK_LABEL(Data), MarkupString);
	g_free(MarkupString);
}

void Title::SetHardSize(bool On)
	{ gtk_label_set_ellipsize(GTK_LABEL(Data), On ? PANGO_ELLIPSIZE_END : PANGO_ELLIPSIZE_NONE); }

// Normal label
Label::Label(const String &Text) : Widget(gtk_label_new(Text.c_str()))
	{}

void Label::SetText(const String &NewText)
	{ gtk_label_set_text(GTK_LABEL(Data), NewText.c_str()); }

void Label::SetHardSize(bool On)
	{ gtk_label_set_ellipsize(GTK_LABEL(Data), On ? PANGO_ELLIPSIZE_END : PANGO_ELLIPSIZE_NONE); }

// URL label/hyperlink
LinkLabel::LinkLabel(const String &URL) : Widget(gtk_link_button_new(URL.c_str()))
	{}

// Progress label (progress bar with label underneath)
ProgressLabel::ProgressLabel(void) : Widget(gtk_progress_bar_new())
	{}

void ProgressLabel::SetText(const String &NewText)
	{ gtk_progress_bar_set_text(GTK_PROGRESS_BAR(Data), NewText.c_str()); }

void ProgressLabel::SetPercent(float NewPercent)
	{ gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(Data), NewPercent); }

void ProgressLabel::SetHardSize(bool On)
	{ gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(Data), On ? PANGO_ELLIPSIZE_END : PANGO_ELLIPSIZE_NONE); }

// The labler
LabelBox::LabelBox(const String &Text) : Layout(true), BoxLabel(Text)
	{ Add(BoxLabel); }

void LabelBox::SetText(const String &NewText)
	{ BoxLabel.SetText(NewText); }

// Block (word wrapped) label
BlockLabel::BlockLabel(const String &Text) : Widget(gtk_label_new(Text.c_str()))
	{ gtk_label_set_line_wrap(GTK_LABEL(Data), true); }
	
void BlockLabel::SetText(const String &NewText)
	{ gtk_label_set_text(GTK_LABEL(Data), NewText.c_str()); }

// Sticker, an image
Sticker::Sticker(const String &Filename) : Widget(gtk_image_new_from_file(Filename.c_str()))
	{}

// Article - a long, wrapped, bordered(?), scrollbarred label
Article::Article(const String &Text) : Widget(gtk_text_view_new())
{
	if (!Text.empty())
		gtk_text_buffer_set_text(
			gtk_text_view_get_buffer(GTK_TEXT_VIEW(Data)),
			Text.c_str(), -1);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(Data), false);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(Data), GTK_WRAP_WORD_CHAR);
}

void Article::SetText(const String &Text)
{
	gtk_text_buffer_set_text(
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(Data)),
		Text.c_str(), -1);
}

// Button wrapper
Button::Button(const String &Text, bool Small) :
	Widget(Text.empty() ? gtk_button_new() : gtk_button_new_with_label(Text.c_str())),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressHandler), this)),
	LastIcon(diNone)
{
	if (Small) gtk_button_set_relief(GTK_BUTTON(Data), GTK_RELIEF_NONE);
}

Button::Button(const String &Text, DefaultIcons Icon, bool Small) :
	Widget(Text.empty() ? gtk_button_new() : gtk_button_new_with_label(Text.c_str())),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressHandler), this)),
	LastIcon(diNone)
{
	GtkWidget *IconWidget = nullptr;
	if (Small) 
	{
		gtk_button_set_relief(GTK_BUTTON(Data), GTK_RELIEF_NONE);
		IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_MENU);
	}
	else IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(Data), IconWidget);
}

Button::~Button(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID); }
		
void Button::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void Button::SetText(const String &NewText)
	{ gtk_button_set_label(GTK_BUTTON(Data), NewText.c_str()); }

void Button::SetIcon(DefaultIcons Icon)
{
	if (Icon == LastIcon) return;
	GtkWidget *IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(Data), IconWidget);
	LastIcon = Icon;
}

void Button::PressHandler(GtkWidget *, Button *This)
	{ if (This->Handler) This->Handler(); }

// Short entry (one line text box) type
ShortEntry::ShortEntry(String const &Prompt, String const &InitialText) : Layout(true),
	EntryData(gtk_entry_new()), ConnectionID(0)
{
	Add(Label(Prompt));
	AddFill(EntryData);

	if (!InitialText.empty())
		gtk_entry_set_text(GTK_ENTRY(EntryData), InitialText.c_str());

	ConnectionID = g_signal_connect(G_OBJECT(EntryData), "changed", G_CALLBACK(EntryCallback), this);
}

ShortEntry::~ShortEntry(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(EntryData), ConnectionID); }
		
void ShortEntry::SetInputHandler(InputHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void ShortEntry::SetEditable(bool Editable)
	{ gtk_entry_set_editable(GTK_ENTRY(EntryData), Editable); }

void ShortEntry::SetValue(const String &NewText)
	{ gtk_entry_set_text(GTK_ENTRY(EntryData), NewText.c_str()); }

String ShortEntry::GetValue(void) const
	{ return gtk_entry_get_text(GTK_ENTRY(EntryData)); }

void ShortEntry::EntryCallback(GtkWidget *, ShortEntry *This)
	{ if (This->Handler) This->Handler(); }

// Long... ?
LongEntry::LongEntry(String const &InitialData) : 
	Widget(gtk_text_view_new()), Buffer(gtk_text_buffer_new(nullptr))
{
	gtk_text_buffer_set_text(Buffer, InitialData.c_str(), InitialData.size());
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(Data), Buffer);
}

void LongEntry::SetEditable(bool Editable)
	{ gtk_text_view_set_editable(GTK_TEXT_VIEW(Data), Editable); }

void LongEntry::SetText(const String &NewText)
	{ gtk_text_buffer_set_text(Buffer, NewText.c_str(), NewText.size()); }

String LongEntry::GetText(void) const
{
	GtkTextIter StartIterator, EndIterator;
	gtk_text_buffer_get_start_iter(Buffer, &StartIterator);
	gtk_text_buffer_get_end_iter(Buffer, &EndIterator);
	return gtk_text_buffer_get_text(Buffer, &StartIterator, &EndIterator, true);
}

// Slida
extern const float SliderFactor = 4, InverseSliderFactor = 1.0f / SliderFactor;
Slider::Slider(const String &Prompt, const RangeF &ValueRange, float Initial) : Layout(true),
	ValueRange(ValueRange),
	SliderData(gtk_hscale_new_with_range(0, 1, 0.01)), EntryData(gtk_entry_new()),
	PromptLabel(Prompt)
{
	Add(PromptLabel);

	gtk_entry_set_width_chars(GTK_ENTRY(EntryData), 6);
	Add(EntryData);
	
	gtk_scale_set_draw_value(GTK_SCALE(SliderData), false);
	gtk_widget_set_size_request(SliderData, 200, 0);
	SetValue(ValueRange.Constrain(Initial));
	AddFill(SliderData);

	SetValue(Initial);
	HandleChangeSliderPosition(nullptr, this);

	// Connect the slide notification last.  We could do it earlier, but then in most cases
	// the text would be updated twice.  Not beautiful!  Not gorgeous!
	SliderHandlerID = g_signal_connect(G_OBJECT(SliderData), "value-changed", G_CALLBACK(HandleChangeSliderPosition), this);

	// For the backward editing path
	EntryHandlerID = g_signal_connect(G_OBJECT(EntryData), "changed", G_CALLBACK(HandleChangeEntryText), this);
}

Slider::~Slider(void)
{ 
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(SliderData), SliderHandlerID); 
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(EntryData), EntryHandlerID); 
}

void Slider::SetInputHandler(InputHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void Slider::SetPrompt(String const &NewPrompt)
	{ PromptLabel.SetText(NewPrompt); }

float Slider::GetValue(void)
	{ return ValueRange.AtPercent(powf(gtk_range_get_value(GTK_RANGE(SliderData)), SliderFactor)); }

void Slider::SetValue(float NewValue)
	{ gtk_range_set_value(GTK_RANGE(SliderData), powf(RangeF(0, 1).Constrain(ValueRange.Percent(NewValue)), InverseSliderFactor)); }

void Slider::HandleChangeSliderPosition(GtkWidget *, Slider *This)
{
	gtk_entry_set_text(GTK_ENTRY(This->EntryData), ((String)(MemoryStream() << OutputStream::Float(This->GetValue()).MaxFractionalDigits(4))).c_str());
	if (This->Handler) This->Handler();
}

void Slider::HandleChangeEntryText(GtkWidget *, Slider *This)
{
	g_signal_handler_block(This->SliderData, This->SliderHandlerID);
	float NewValue;
	MemoryStream(gtk_entry_get_text(GTK_ENTRY(This->EntryData))) >> NewValue;
	This->SetValue(NewValue);
	g_signal_handler_unblock(This->SliderData, This->SliderHandlerID);
	if (This->Handler) This->Handler();
}

// Check button
CheckButton::CheckButton(const String &Text, bool StartState) :
	Widget(gtk_check_button_new_with_label(Text.c_str())),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressCallback), this))
{ 
	g_signal_handler_block(G_OBJECT(Data), ConnectionID);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Data), StartState); 
	g_signal_handler_unblock(G_OBJECT(Data), ConnectionID);
}

CheckButton::CheckButton(bool StartState) :
	Widget(gtk_check_button_new()),
	ConnectionID(g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressCallback), this))
{
	g_signal_handler_block(G_OBJECT(Data), ConnectionID);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Data), StartState); 
	g_signal_handler_unblock(G_OBJECT(Data), ConnectionID);
}

CheckButton::~CheckButton(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID); }

void CheckButton::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void CheckButton::SetPrompt(String const &NewPrompt)
	{ gtk_button_set_label(GTK_BUTTON(Data), NewPrompt.c_str()); }

void CheckButton::SetValue(bool NewValue)
	{ gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Data), NewValue); }

bool CheckButton::GetValue(void)
	{ return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Data)); }

void CheckButton::PressCallback(GtkWidget *, CheckButton *This)
	{ if (This->Handler) This->Handler(); }

// Wheel
Wheel::Wheel(String const &Prompt, RangeF const &ValueRange, float Initial, bool Float) : Layout(true),
	ValueRange(ValueRange),
	WheelData(gtk_spin_button_new_with_range(ValueRange.Min, ValueRange.Max, 1)),
	ConnectionID(g_signal_connect(G_OBJECT(WheelData), "value-changed", G_CALLBACK(SpinCallback), this))
{
	g_signal_handler_block(G_OBJECT(WheelData), ConnectionID); // Or else stuff starts triggering when fiddling with things
	gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(WheelData), false);
	if (Float) 
	{
		float IncrementValue = ValueRange.Length() / 100.0f;
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(WheelData), 2);
		gtk_spin_button_set_increments(GTK_SPIN_BUTTON(WheelData), IncrementValue, IncrementValue * 10);
	}
	else
	{
		int IncrementValue = ValueRange.Length() < 100 ? 1 :
				ValueRange.Length() < 1000 ? (int)ValueRange.Length() / 100 : (int)ValueRange.Length() / 1000 * 10;
		gtk_spin_button_set_increments(GTK_SPIN_BUTTON(WheelData), IncrementValue, IncrementValue * 10);
	}
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(WheelData), ValueRange.Constrain(Initial));
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(WheelData), true);
	g_signal_handler_unblock(G_OBJECT(WheelData), ConnectionID);
	Add(Label(Prompt));
	AddFill(WheelData);
}

Wheel::~Wheel(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(WheelData), ConnectionID); }

void Wheel::SetInputHandler(InputHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

int Wheel::GetInt(void)
	{ return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(WheelData)); }

float Wheel::GetFloat(void)
	{ return gtk_spin_button_get_value(GTK_SPIN_BUTTON(WheelData)); }

float Wheel::GetValue(void)
	{ return gtk_spin_button_get_value(GTK_SPIN_BUTTON(WheelData)); }

void Wheel::SetValue(int NewValue)
	{ gtk_spin_button_set_value(GTK_SPIN_BUTTON(WheelData), NewValue); }

void Wheel::SetValue(float NewValue)
	{ gtk_spin_button_set_value(GTK_SPIN_BUTTON(WheelData), NewValue); }

void Wheel::SpinCallback(GtkWidget *, Wheel *This)
	{ if (This->Handler) This->Handler(); }

// Drop selection
List::List(String const &Prompt, bool Multiline) : Widget(nullptr),
	Multiline(Multiline), Added(0),
	Store(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN))
{
	GtkTreeModel *Model = GTK_TREE_MODEL(Store);
	GtkTreeModel *FilteredModel = gtk_tree_model_filter_new(Model, nullptr);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(FilteredModel), 1);
	GtkCellRenderer *ColumnRenderer = gtk_cell_renderer_text_new();

	if (Multiline)
	{
		ListData = Data = gtk_tree_view_new_with_model(FilteredModel);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(Data), -1, Prompt.c_str(), ColumnRenderer, "text", 0, nullptr);
		ConnectionID = g_signal_connect(G_OBJECT(Data), "cursor-changed", G_CALLBACK(SelectCallback), this);
	}
	else
	{
		Layout PreData(true);
		PreData.Add(Label(Prompt));

		ListData = gtk_combo_box_new_with_model(FilteredModel);
		g_object_ref(G_OBJECT(ListData)); // Who knows why this is necessary.  GTK bug? Otherwise refs get down to 0 and there's a warning.  Version 2.24.10
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ListData), ColumnRenderer, true);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ListData), ColumnRenderer, "text", 0, nullptr);
		ConnectionID = g_signal_connect(G_OBJECT(ListData), "changed", G_CALLBACK(SelectCallback), this);
		PreData.AddFill(ListData);
		Data = PreData;
	}
}

List::~List(void)
{
	if (Destroy) g_object_unref(Store);
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(ListData), ConnectionID);
}
		
void List::SetInputHandler(InputHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

bool List::Empty(void)
	{ return Added == 0; }

unsigned int List::Size(void)
	{ return Added; }

void List::Clear(void)
{
	gtk_list_store_clear(Store);
	Added = 0;
}

int List::Add(const String &NewString)
{
	GtkTreeIter Iterator;
	gtk_list_store_append(Store, &Iterator);
	gtk_list_store_set(Store, &Iterator, 0, NewString.c_str(), 1, true, -1);

	return Added++;
}

int List::Add(const String &NewString, int Position)
{
	assert(Position >= 0);

	GtkTreeIter Iterator;
	gtk_list_store_insert(Store, &Iterator, Position);
	gtk_list_store_set(Store, &Iterator, 0, NewString.c_str(), 1, true, -1);

	Added++;
	if (Position > Added)
		return Added;
	else return Position;
}

void List::Remove(int Item)
{
	bool Result;

	GtkTreeIter Iterator;
	Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, nullptr, Item); assert(Result);
	gtk_list_store_remove(Store, &Iterator);

	Added--;
}

void List::MoveUp(int Item)
{
	assert(Item > 0);
	GtkTreeIter From, To;
	bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &From, nullptr, Item); assert(Result);
	Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &To, nullptr, Item - 1); assert(Result);
	gtk_list_store_swap(Store, &From, &To);
}

void List::MoveDown(int Item)
{
	assert(Item >= 0);
	GtkTreeIter From, To;
	bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &From, nullptr, Item); assert(Result);
	Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &To, nullptr, Item + 1); assert(Result);
	gtk_list_store_swap(Store, &From, &To);
}

void List::Rename(int Item, const String &NewString)
{
	GtkTreeIter Iterator;
	bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, nullptr, Item); assert(Result);
	gtk_list_store_set(Store, &Iterator, 0, NewString.c_str(), -1);
}

void List::Hide(int Item)
{
	GtkTreeIter Iterator;
	bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, nullptr, Item); assert(Result);
	gtk_list_store_set(Store, &Iterator, 1, false, -1);
}

void List::Show(int Item)
{
	GtkTreeIter Iterator;
	bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, nullptr, Item); assert(Result);
	gtk_list_store_set(Store, &Iterator, 1, true, -1);
}

void List::Select(int NewSelection)
{
	if (NewSelection < 0)
	{
		if (Multiline)
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(ListData)));
		else gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ListData), nullptr);
		return;
	}
	
	assert(NewSelection < Added);

	GtkTreeIter Iterator;
	bool Result;

#ifndef NDEBUG
	Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, nullptr, NewSelection); assert(Result);
	bool IsVisible;
	gtk_tree_model_get(GTK_TREE_MODEL(Store), &Iterator, 1, &IsVisible, -1);
	assert(IsVisible);
#endif

	Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, nullptr, NewSelection); assert(Result);

	if (Multiline)
	{
		GtkTreePath *NewPath = gtk_tree_model_get_path(GTK_TREE_MODEL(Store), &Iterator);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(ListData), NewPath, nullptr, false);
		gtk_tree_path_free(NewPath);
	}
	else
	{
		GtkTreeIter FilteredIterator;
		gtk_tree_model_filter_convert_child_iter_to_iter(
			GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model(GTK_COMBO_BOX(ListData))), &FilteredIterator, &Iterator);
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(ListData), &FilteredIterator);
	}
}

int List::GetSelection(void)
{
	GtkTreeIter FilteredIterator;
	GtkTreeModelFilter *FilteredStore = nullptr;
	if (Multiline)
	{
		FilteredStore = GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(ListData)));

		GtkTreePath *FilteredPath;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(ListData), &FilteredPath, nullptr);
		if (FilteredPath == nullptr) return -1;

		gtk_tree_model_get_iter(GTK_TREE_MODEL(FilteredStore), &FilteredIterator, FilteredPath);
		gtk_tree_path_free(FilteredPath);
	}
	else
	{
		FilteredStore = GTK_TREE_MODEL_FILTER(gtk_combo_box_get_model(GTK_COMBO_BOX(ListData)));

		int FilteredSelection = gtk_combo_box_get_active(GTK_COMBO_BOX(ListData));
		if (FilteredSelection < 0) return -1;

		bool Result = gtk_tree_model_iter_nth_child(gtk_combo_box_get_model(GTK_COMBO_BOX(ListData)), &FilteredIterator, nullptr, FilteredSelection); assert(Result);
	}

	GtkTreeIter Iterator;
	gtk_tree_model_filter_convert_iter_to_child_iter(FilteredStore, &Iterator, &FilteredIterator);
	GtkTreePath *RealPath;
	RealPath = gtk_tree_model_get_path(GTK_TREE_MODEL(Store), &Iterator);
	assert(RealPath != nullptr);
	int Out = gtk_tree_path_get_indices(RealPath)[0];
	gtk_tree_path_free(RealPath);
	return Out;
}

void List::SelectCallback(GtkWidget *, List *This)
	{ if (This->Handler) This->Handler(); }

// Toolbar as a menu button
MenuButton::MenuButton(const String &Label, DefaultIcons Icon) : Button(Label, Icon)
{
	MenuData.SetPositionHandler([&](gint &X, gint &Y)
	{
		gdk_window_get_origin(Data->window, &X, &Y);
		X += Data->allocation.x;
		Y += Data->allocation.y + Data->allocation.height;
	});
	SetAction([&](void) { MenuData.Activate(); });
}

MenuItem *MenuButton::Add(MenuItem *NewItem)
	{ return MenuData.Add(NewItem); }

void MenuButton::AddSeparator(void)
	{ MenuData.AddSeparator(); }

// File thing 1 s
DirectorySelect::DirectorySelect(String const &Prompt, const String &InitialDirectory) :
	Layout(true),
	ButtonData(gtk_file_chooser_button_new("Select directory", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)),
	ConnectionID(g_signal_connect(G_OBJECT(ButtonData), "file-set", G_CALLBACK(OpenCallback), this))
{
	if (!Prompt.empty())
		Add(Label(Prompt));
	
	//gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ButtonData), InitialDirectory.c_str());
	if (!InitialDirectory.empty())
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(ButtonData), InitialDirectory.c_str());
	AddFill(ButtonData);
}

DirectorySelect::~DirectorySelect(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(ButtonData), ConnectionID); }

void DirectorySelect::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}
		
void DirectorySelect::SetValue(String const &NewDirectory)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(ButtonData), NewDirectory.c_str());
}

String DirectorySelect::GetValue(void)
{
	char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ButtonData));
	String Out = PreOut;
	g_free(PreOut);
	return Out;
}

void DirectorySelect::OpenCallback(GtkWidget *, DirectorySelect *This)
	{ if (This->Handler) This->Handler(); }

OpenSelect::OpenSelect(String const &Prompt, String const &InitialFile, String const &FilterName) :
	Layout(true),
	ButtonData(gtk_file_chooser_button_new("Select file", GTK_FILE_CHOOSER_ACTION_OPEN)),
	ConnectionID(g_signal_connect(G_OBJECT(ButtonData), "file-set", G_CALLBACK(OpenCallback), this))
{
	gtk_widget_set_size_request(ButtonData, 150, -1);
	
	if (!Prompt.empty())
		Add(Label(Prompt));
	AddFill(ButtonData);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(ButtonData), InitialFile.c_str());

	// Set filter for opening
	SingleFilter = gtk_file_filter_new();
	gtk_file_filter_set_name(SingleFilter, FilterName.c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(ButtonData), SingleFilter);
}

OpenSelect::~OpenSelect(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(ButtonData), ConnectionID); }

void OpenSelect::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

void OpenSelect::AddFilterPass(const String &Filter)
{
	assert(SingleFilter != nullptr);
	gtk_file_filter_add_pattern(SingleFilter, Filter.c_str());
}

String OpenSelect::GetValue(void)
{
	char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ButtonData));
	String Out = PreOut;
	g_free(PreOut);
	return Out;
}

void OpenSelect::OpenCallback(GtkWidget *, OpenSelect *This)
	{ if (This->Handler) This->Handler(); }

OutputSelect::OutputSelect(String const &Prompt, String const &InitialDirectory) : Layout(true),
	Location("", InitialDirectory), SelectButton("Select...")
{
	if (!Prompt.empty())
		Add(Label(Prompt));
	Location.SetEditable(false);
	AddFill(Location);

	SelectButton.SetAction([&]()
	{
		GtkWidget *SaveDialog = gtk_file_chooser_dialog_new("Save...", GTK_WINDOW(gtk_widget_get_toplevel(SelectButton)),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			nullptr);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(SaveDialog), Location.GetValue().c_str());

		if (gtk_dialog_run(GTK_DIALOG(SaveDialog)) == GTK_RESPONSE_ACCEPT)
		{
			char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(SaveDialog));
			Location.SetValue(PreOut);
			g_free(PreOut);
		}

		gtk_widget_destroy(SaveDialog);

		if (Handler) Handler();
	});
	Add(SelectButton);
}
		
void OutputSelect::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

String OutputSelect::GetValue(void)
	{ return Location.GetValue(); }

// Toolbox
Toolbox::Toolbox(void) : Widget(nullptr), IDCounter(0)
{
	Model = gtk_list_store_new(3, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_INT, -1);

	Data = gtk_icon_view_new_with_model(GTK_TREE_MODEL(Model));
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(Data), 1);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(Data), 0);
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(Data), GTK_SELECTION_SINGLE);
	gtk_icon_view_set_orientation(GTK_ICON_VIEW(Data), GTK_ORIENTATION_HORIZONTAL);
	ConnectionID = g_signal_connect(G_OBJECT(Data), "selection-changed", G_CALLBACK(HandleSelect), this);
}

Toolbox::~Toolbox(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID); }

void Toolbox::ForceColumns(int ColumnCount)
	{ gtk_icon_view_set_columns(GTK_ICON_VIEW(Data), ColumnCount); }

int Toolbox::AddItem(const String &Image, const String &Text)
{
	GtkTreeIter NewIterator;
	gtk_list_store_append(GTK_LIST_STORE(Model), &NewIterator);

	GdkPixbuf *NewPixbuf = nullptr;
	if (!Image.empty())
	{
		GError *PixbufError = nullptr;
		NewPixbuf = gdk_pixbuf_new_from_file(Image.c_str(), &PixbufError);
		if (NewPixbuf == nullptr)
			g_print("Error loading toolbox image %s: %s\n",
				Image.c_str(), PixbufError->message);
	}
	if (NewPixbuf == nullptr) NewPixbuf = gtk_widget_render_icon(Data,
		GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR, nullptr);

	gtk_list_store_set(Model, &NewIterator,
		0, Text.c_str(),
		1, NewPixbuf,
		2, IDCounter++,
		-1);

	return IDCounter - 1;
}

int Toolbox::GetSelected(void)
{
	GList *Selection = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(Data));

	if (Selection == nullptr) return -1;

	assert(g_list_length(Selection) == 1);

	GtkTreeIter Selected;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(Model), &Selected, (GtkTreePath *)Selection->data);

	gint ID;
	gtk_tree_model_get(GTK_TREE_MODEL(Model), &Selected, 2, &ID, -1);

	g_list_foreach(Selection, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(Selection);

	return ID;
}

void Toolbox::Unselect(void)
	{ gtk_icon_view_unselect_all(GTK_ICON_VIEW(Data)); }

void Toolbox::Clear(void)
	{ gtk_list_store_clear(GTK_LIST_STORE(Model)); IDCounter = 0; }

void Toolbox::OnSelect(int)
	{}

void Toolbox::HandleSelect(GtkWidget *, Toolbox *This)
	{ This->OnSelect(This->GetSelected()); }

// Toolbar, button menu type
Toolbar::Toolbar(void) : Widget(gtk_toolbar_new())
{
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(Data), true);
	gtk_toolbar_set_style(GTK_TOOLBAR(Data), GTK_TOOLBAR_BOTH_HORIZ);
}

void Toolbar::Add(ToolButton &NewItem, int AtPosition)
{
	gtk_toolbar_insert(GTK_TOOLBAR(Data), GTK_TOOL_ITEM((GtkWidget *)NewItem), AtPosition);
	gtk_tool_item_set_is_important(GTK_TOOL_ITEM((GtkWidget *)NewItem), true);
	gtk_widget_show(NewItem);
}

void Toolbar::Add(GtkWidget *Item, int AtPosition)
{
	GtkToolItem *NewToolItem = gtk_tool_item_new();
	gtk_tool_item_set_is_important(NewToolItem, true);
	gtk_container_add(GTK_CONTAINER(NewToolItem), Item);
	gtk_widget_show(Item);

	gtk_toolbar_insert(GTK_TOOLBAR(Data), NewToolItem, AtPosition);
	gtk_widget_show(GTK_WIDGET(NewToolItem));
}

void Toolbar::AddSpacer(int AtPosition)
{
	GtkToolItem *Separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(Data), Separator, AtPosition);
	gtk_widget_show(GTK_WIDGET(Separator));
}

void Toolbar::AddSpace(int AtPosition)
{
	GtkToolItem *Spacer = gtk_separator_tool_item_new();
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(Spacer), false);
	gtk_tool_item_set_expand(GTK_TOOL_ITEM(Spacer), true);
	gtk_toolbar_insert(GTK_TOOLBAR(Data), Spacer, AtPosition);
	gtk_widget_show(GTK_WIDGET(Spacer));
}

// File open/save dialog
FileDialog::FileDialog(const String &Title, const String &FilterName, Window *Parent, bool SaveMode)
{
	Data = gtk_file_chooser_dialog_new(Title.c_str(), GTK_WINDOW((GtkWidget *)*Parent),
		(SaveMode) ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		(SaveMode) ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		nullptr);

	//if (SaveMode)
	//	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(Data), true);

	// Create the default filter
	SingleFilter = gtk_file_filter_new();
	gtk_file_filter_set_name(SingleFilter, FilterName.c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Data), SingleFilter);
}

void FileDialog::AddFilterPass(const String &Pattern)
{
	assert(Data != nullptr);
	assert(SingleFilter != nullptr);
	gtk_file_filter_add_pattern(SingleFilter, Pattern.c_str());
}

void FileDialog::SetDefaultSuffix(const String &NewSuffix)
	{ Suffix = NewSuffix; }

void FileDialog::SetFile(const String &NewFile)
{
	assert(Data != nullptr);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Data), NewFile.c_str());
}

void FileDialog::SetDirectory(const String &NewDirectory)
{
	assert(Data != nullptr);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(Data), NewDirectory.c_str());
}

String FileDialog::Run(void)
{
	assert(Data != nullptr);

	String Out;
	if (gtk_dialog_run(GTK_DIALOG(Data)) == GTK_RESPONSE_ACCEPT)
	{
		char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Data));
		Out = PreOut;
		g_free(PreOut);

		if ((Out.size() < Suffix.size()) || 
			(Out.substr(Out.size() - Suffix.size(), Suffix.size()) != Suffix))
			Out += Suffix;
	}

	gtk_widget_destroy(Data);
	Data = nullptr;

	return Out;
}

// COlor select button
ColorButton::ColorButton(String const &Prompt, Color const &InitialColor, bool SelectAlpha) : Layout(true),
	Alpha(SelectAlpha),
	ButtonData(gtk_color_button_new()),
	ConnectionID(g_signal_connect(G_OBJECT(ButtonData), "color-set", G_CALLBACK(HandleSelect), this)),
	Prompt(Prompt)
{
	if (Alpha)
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(ButtonData), true);

	SetColor(InitialColor);

	Add(this->Prompt);
	Add(ButtonData);
}

ColorButton::~ColorButton(void)
	{ if (Destroy) g_signal_handler_disconnect(G_OBJECT(ButtonData), ConnectionID); }

void ColorButton::SetAction(ActionHandler const &Handler)
{
	assert(!this->Handler);
	this->Handler = Handler;
}

Color ColorButton::GetColor(void)
{
	GdkColor PreOut;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(ButtonData), &PreOut);
	Color Out(
		(float)PreOut.red / 65535.0f,
		(float)PreOut.green / 65535.0f,
		(float)PreOut.blue / 65535.0f);

	if (Alpha) Out.Alpha = (float)gtk_color_button_get_alpha(GTK_COLOR_BUTTON(ButtonData)) / 65535.0f;

	return Out;
}

void ColorButton::SetColor(const Color &NewColor)
{
	GdkColor ColorConversion;
	ColorConversion.red = std::min(NewColor.Red * 65535, 65535.0f);
	ColorConversion.green = std::min(NewColor.Green * 65535, 65535.0f);
	ColorConversion.blue = std::min(NewColor.Blue * 65535, 65535.0f);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(ButtonData), &ColorConversion);

	if (Alpha) gtk_color_button_set_alpha(GTK_COLOR_BUTTON(ButtonData), std::min(NewColor.Alpha * 65535, 65535.0f));
}

void ColorButton::HandleSelect(GtkWidget *, ColorButton *This)
	{ if (This->Handler) This->Handler(); }

// Color toggle button
const FlatVector ColorToggleButtonSize(54, 48);
ColorToggleButton::ColorToggleButton(bool InitiallyFore, const Color &ForegroundColor, const Color &BackgroundColor) : Widget(gtk_button_new()),
	State(InitiallyFore), Foreground(ForegroundColor), Background(BackgroundColor),
	ColorArea(gtk_drawing_area_new())
{
	gtk_widget_set_size_request(ColorArea, ColorToggleButtonSize[0], ColorToggleButtonSize[1]);
	RefreshConnectionID = g_signal_connect(G_OBJECT(ColorArea), "expose_event", G_CALLBACK(RefreshCallback), this);

	gtk_button_set_image(GTK_BUTTON(Data), ColorArea);
	ClickConnectionID = g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(ClickCallback), this);
}

ColorToggleButton::~ColorToggleButton(void)
{
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), ClickConnectionID);
	if (Destroy) g_signal_handler_disconnect(G_OBJECT(Data), RefreshConnectionID);
}

bool ColorToggleButton::GetState(void)
	{ return State; }

void ColorToggleButton::SetForegroundColor(const Color &NewForegroundColor)
	{ Foreground = NewForegroundColor; if (State) Refresh(); }

void ColorToggleButton::SetBackgroundColor(const Color &NewBackgroundColor)
	{ Background = NewBackgroundColor; if (!State) Refresh(); }

void ColorToggleButton::Refresh()
{
	if (!GDK_IS_WINDOW(ColorArea->window)) return;
	const GdkRectangle RefreshRectangle = {0, 0, ColorArea->allocation.width, ColorArea->allocation.height};
	gdk_window_invalidate_rect(ColorArea->window, &RefreshRectangle, false);
	gdk_window_process_updates(ColorArea->window, false);
}

gboolean ColorToggleButton::RefreshCallback(GtkWidget *, GdkEventExpose *Event, ColorToggleButton *This)
{
	cairo_t *CairoContext = gdk_cairo_create(Event->window);
	Color &FillColor = This->State ? This->Foreground : This->Background;
	cairo_set_source_rgb(CairoContext, FillColor.Red, FillColor.Green, FillColor.Blue);
	cairo_paint(CairoContext);
	cairo_destroy(CairoContext);

	return true;
}

void ColorToggleButton::ClickCallback(GtkWidget *, ColorToggleButton *This)
	{ This->State = !This->State; }

