#include "gtkwrapper.h"

#include "../general/region.h"
#include "../general/range.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

const char *ConvertStock(DefaultIcons From)
{
	switch (From)
	{
		case diNone: return NULL; // Who knows?
		case diUp: return GTK_STOCK_GO_UP;
		case diDown: return GTK_STOCK_GO_DOWN;
		case diAdd: return GTK_STOCK_ADD;
		case diRemove: return GTK_STOCK_REMOVE;
		case diDelete: return GTK_STOCK_DELETE;
		case diNew: return GTK_STOCK_NEW;
		case diOpen: return GTK_STOCK_OPEN;
		case diSave: return GTK_STOCK_SAVE;
		case diRefresh: return GTK_STOCK_REFRESH;
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
		default: assert(0);
	}
}

const int Spacing = 6;
const int HalfSpacing = 3;

///////////////////////////////////////////////////////////

TimedEvent::TimedEvent(unsigned int Milliseconds) : Period(Milliseconds), TimerID(0) {}
TimedEvent::~TimedEvent(void)
	{ StopTimer(); }

void TimedEvent::StartTimer(void)
{
	if (TimerID == 0)
		TimerID = g_timeout_add(Period, (GSourceFunc)TimeHandler, this);
	TickEvent();
}

void TimedEvent::StopTimer(void)
{
	if (TimerID != 0)
	{
		g_source_remove(TimerID);
		TimerID = 0;
	}
}

gboolean TimedEvent::TimeHandler(TimedEvent *This)
	{ This->TickEvent(); return true; }

///////////////////////////////////////////////////////////
Widget::Widget(GtkWidget *NewData) : Data(NewData)
	{}

Widget::~Widget(void)
	{ gtk_widget_destroy(Data); }

void Widget::Show(void)
	{ gtk_widget_show(Data); }

void Widget::Hide(void)
	{ gtk_widget_hide(Data); }

GtkWidget *Widget::GetData(Widget *Other)
	{ return Other->Data; }

GtkWidget *Widget::GetWindow(Widget *Other)
	{ return gtk_widget_get_toplevel(Other->Data); }

///////////////////////////////////////////////////////////
// Widget extensions
KeyboardWidget::Handler::~Handler(void) {}
KeyboardWidget::KeyboardWidget(GtkWidget *Data, Handler *Target) :
	Target(Target)
{
	gtk_widget_add_events(Data, GDK_KEY_PRESS);
	g_signal_connect(G_OBJECT(Data), "key-press-event", G_CALLBACK(KeyHandler), this);
}

gboolean KeyboardWidget::KeyHandler(GtkWidget *Widget, GdkEventKey *Event, KeyboardWidget *This)
{
	if (This->Target != NULL)
		return This->Target->OnKey(Event->keyval, Event->state);
	return true;
}

///////////////////////////////////////////////////////////
// Window type
Window::Handler::~Handler(void) {}

Window::Window(const String &Title, Handler *Target) : Widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
	Target(Target)
{
	gtk_window_set_title(GTK_WINDOW(Data), Title.c_str());
	gtk_container_set_reallocate_redraws(GTK_CONTAINER(Data), true);
	g_signal_connect(G_OBJECT(Data), "delete_event", G_CALLBACK(DeleteHandler), this);
	g_signal_connect(G_OBJECT(Data), "destroy", G_CALLBACK(DestroyHandler), this);
	gtk_container_set_border_width(GTK_CONTAINER(Data), Spacing);
}

void Window::SetIcon(const String &Filename)
	{ gtk_window_set_icon_from_file(GTK_WINDOW(Data), Filename.c_str(), NULL); }

void Window::SetTitle(const String &NewTitle)
	{ gtk_window_set_title(GTK_WINDOW(Data), NewTitle.c_str()); }

void Window::SetFullscreen(bool On)
{
	if (On) gtk_window_fullscreen(GTK_WINDOW(Data));
	else gtk_window_unfullscreen(GTK_WINDOW(Data));
}

void Window::SetDefaultSize(const FlatVector &Size)
{
	gtk_window_set_default_size(GTK_WINDOW(Data), Size[0], Size[1]);
}

void Window::Set(Widget *Addee)
	{ gtk_container_add(GTK_CONTAINER(Data), GetData(Addee)); }

void Window::SetAndShow(Widget *Addee)
	{ gtk_container_add(GTK_CONTAINER(Data), GetData(Addee)); Addee->Show(); }
// Dialogs
void Window::Error(const String &Section, const String &Description)
{
	g_print("%s: %s\n", Section.c_str(), Description.c_str());

	GtkWidget *Message = gtk_message_dialog_new(
		GTK_WINDOW(Data),
		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
		Section.c_str());
	gtk_dialog_add_button(GTK_DIALOG(Message), GTK_STOCK_QUIT, 0);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), Description.c_str());
	gtk_dialog_run(GTK_DIALOG(Message));
	gtk_main_quit();
}

void Window::Warning(const String &Section, const String &Description)
{
	g_print("%s: %s\n", Section.c_str(), Description.c_str());

	GtkWidget *Message = gtk_message_dialog_new(
		GTK_WINDOW(Data),
		GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
		Section.c_str());
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), Description.c_str());
	gtk_dialog_run(GTK_DIALOG(Message));
	gtk_widget_destroy(Message);
}

bool Window::Confirm(const String &Section, const String &Description)
{
	GtkWidget *Message = gtk_message_dialog_new(
		GTK_WINDOW(Data),
		GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
		Section.c_str());
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), Description.c_str());
	int Result = gtk_dialog_run(GTK_DIALOG(Message));
	gtk_widget_destroy(Message);

	return Result == GTK_RESPONSE_YES;
}

GtkWidget *Window::GetWidget(void)
	{ return Data; }

gboolean Window::DeleteHandler(GtkWidget *Source, GdkEvent *Event, Window *This)
{
	if (This->Target == NULL)
		return false;
	else return !This->Target->ConfirmClose(This);
}

void Window::DestroyHandler(GtkWidget *Source, Window *This)
{
	if (This->Target == NULL)
		gtk_main_quit();
	else This->Target->OnClose(This);
}

// Dialog window, doesn't appear til' Run is called
Dialog::Dialog(Window *Parent, const String &Title) : Widget(gtk_dialog_new())
{
	gtk_window_set_title(GTK_WINDOW(Data), Title.c_str());
	gtk_container_set_reallocate_redraws(GTK_CONTAINER(Data), true);

	gtk_window_set_transient_for(GTK_WINDOW(Data), GTK_WINDOW(GetData(Parent)));
}

void Dialog::AddAndShow(Widget *NewWidget, bool Fill)
{
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(Data))),
		GetData(NewWidget), Fill, true, HalfSpacing);
	NewWidget->Show();
}

void Dialog::AddAndShowAction(Widget *NewWidget, bool Fill)
{
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(Data))),
		GetData(NewWidget), Fill, true, HalfSpacing);
	NewWidget->Show();
}

void Dialog::Error(const String &Section, const String &Description)
{
	g_print("%s: %s\n", Section.c_str(), Description.c_str());

	GtkWidget *Message = gtk_message_dialog_new(
		GTK_WINDOW(Data),
		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE,
		Section.c_str());
	gtk_dialog_add_button(GTK_DIALOG(Message), GTK_STOCK_QUIT, 0);
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), Description.c_str());
	gtk_dialog_run(GTK_DIALOG(Message));
	gtk_main_quit();
}

void Dialog::Warning(const String &Section, const String &Description)
{
	g_print("%s: %s\n", Section.c_str(), Description.c_str());

	GtkWidget *Message = gtk_message_dialog_new(
		GTK_WINDOW(Data),
		GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
		Section.c_str());
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), Description.c_str());
	gtk_dialog_run(GTK_DIALOG(Message));
	gtk_widget_destroy(Message);
}

bool Dialog::Confirm(const String &Section, const String &Description)
{
	GtkWidget *Message = gtk_message_dialog_new(
		GTK_WINDOW(Data),
		GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
		Section.c_str());
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(Message), Description.c_str());
	int Result = gtk_dialog_run(GTK_DIALOG(Message));
	gtk_widget_destroy(Message);

	return Result == GTK_RESPONSE_YES;
}

void Dialog::Run(void)
{
	gtk_dialog_run(GTK_DIALOG(Data));
}

void Dialog::Close(void)
{
	gtk_dialog_response(GTK_DIALOG(Data), GTK_RESPONSE_NONE);
}

// Popup menu
PopupMenu::Handler::~Handler(void) {}

PopupMenu::PopupMenu(Handler *Target) :
	Target(Target), OptionCount(0)
{
	MenuData = gtk_menu_new();
}

PopupMenu::~PopupMenu(void)
{
	gtk_widget_destroy(MenuData);
}

void PopupMenu::Clear(void)
{
	gtk_widget_destroy(MenuData);
	MenuData = gtk_menu_new();
}

int PopupMenu::Add(const String &NewString)
{
	GtkWidget *NewItem = gtk_menu_item_new_with_label(NewString.c_str());
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	g_signal_connect(G_OBJECT(NewItem), "activate", G_CALLBACK(ClickHandler), this);
	gtk_widget_show(GTK_WIDGET(NewItem));

	int Out = OptionCount++;
	OptionMap[NewString] = Out;
	return Out;
}

int PopupMenu::Add(const String &NewString, const DefaultIcons Icon)
{
	GtkWidget *NewItem = gtk_image_menu_item_new_with_label(NewString.c_str());
	GtkWidget *IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(NewItem), IconWidget);

	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	g_signal_connect(G_OBJECT(NewItem), "activate", G_CALLBACK(ClickHandler), this);
	gtk_widget_show(GTK_WIDGET(NewItem));

	int Out = OptionCount++;
	OptionMap[NewString] = Out;
	return Out;
}

int PopupMenu::Add(const String &NewString, const String &IconFile)
{
	GtkWidget *NewItem = gtk_image_menu_item_new_with_label(NewString.c_str());
	GtkWidget *IconWidget = gtk_image_new_from_file(IconFile.c_str());
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(NewItem), IconWidget);

	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	g_signal_connect(G_OBJECT(NewItem), "activate", G_CALLBACK(ClickHandler), this);
	gtk_widget_show(GTK_WIDGET(NewItem));

	int Out = OptionCount++;
	OptionMap[NewString] = Out;
	return Out;
}

void PopupMenu::Show(void)
{
	gtk_menu_popup(GTK_MENU(MenuData),
		NULL, NULL, NULL, this, 0, gtk_get_current_event_time());
}

void PopupMenu::ClickHandler(GtkMenuItem *MenuWidget, PopupMenu *This)
{
	if (This->Target != NULL)
		This->Target->OnSelect(This,
			This->OptionMap[String(gtk_menu_item_get_label(GTK_MENU_ITEM(MenuWidget)))]);
}

///////////////////////////////////////////////////////////
// Box container wrapper
Box::Box(bool Horizontal) : Widget(NULL),
	IsHorizontal(Horizontal)
{
	if (Horizontal) Data = gtk_hbox_new(false, Spacing);
	else Data = gtk_vbox_new(false, Spacing);
}

void Box::Add(Widget *Addee, bool Fill)
	{ gtk_box_pack_start(GTK_BOX(Data), GetData(Addee), Fill, true, HalfSpacing); }

void Box::Add(GtkWidget *Addee, bool Fill)
	{ gtk_box_pack_start(GTK_BOX(Data), Addee, Fill, true, HalfSpacing); }

void Box::AddAndShow(Widget *Addee, bool Fill)
	{ gtk_box_pack_start(GTK_BOX(Data), GetData(Addee), Fill, true, HalfSpacing); Addee->Show(); }

void Box::AddSpacer(void)
{
	GtkWidget *Separator = (IsHorizontal) ? gtk_hseparator_new() : gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(Data), Separator, false, true, HalfSpacing);
	gtk_widget_show(Separator);
}

void Box::AddSpace(void)
{
	GtkWidget *SpacerBox = (IsHorizontal) ? gtk_hbox_new(false, 0) : gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(Data), SpacerBox, true, true, 0);
	gtk_widget_show(SpacerBox);
}

// Tabla widget yuh
Table::Table(void) : Widget(gtk_table_new(0, 0, false))
	{}

void Table::Resize(unsigned int Width, unsigned int Height)
	{ gtk_table_resize(GTK_TABLE(Data), Width, Height); }

void Table::Add(Widget *Addee, int X, int Y, bool Fill)
{
	gtk_table_attach(GTK_TABLE(Data), GetData(Addee), X, X + 1, Y, Y + 1,
		GtkAttachOptions(GTK_SHRINK | (Fill ? (GTK_EXPAND | GTK_FILL) : 0)),
		GtkAttachOptions(GTK_SHRINK | (Fill ? (GTK_EXPAND | GTK_FILL) : 0)),
		3, 0);
}

void Table::AddAndShow(Widget *Addee, int X, int Y, bool Fill)
{
	gtk_table_attach(GTK_TABLE(Data), GetData(Addee), X, X + 1, Y, Y + 1,
		GtkAttachOptions(GTK_SHRINK | (Fill ? (GTK_EXPAND | GTK_FILL) : 0)),
		GtkAttachOptions(GTK_SHRINK | (Fill ? (GTK_EXPAND | GTK_FILL) : 0)),
		3, 0);
	Addee->Show();
}

// Frame wrapper
Frame::Frame(void) : Widget(gtk_frame_new(NULL))
{
	gtk_frame_set_shadow_type(GTK_FRAME(Data), GTK_SHADOW_IN);
}

Frame::Frame(const String &Title) : Widget(gtk_frame_new(Title.c_str()))
	{}

void Frame::Set(Widget *Settee)
	{ gtk_container_add(GTK_CONTAINER(Data), GetData(Settee)); }

void Frame::SetAndShow(Widget *Settee)
	{ gtk_container_add(GTK_CONTAINER(Data), GetData(Settee)); Settee->Show(); }

void Frame::Set(GtkWidget *Settee)
	{ gtk_container_add(GTK_CONTAINER(Data), Settee); }

// Aspect ratio controlling frame
RatioFrame::RatioFrame(const String &Title, float Ratio) :
	Widget(gtk_aspect_frame_new(Title.c_str(), 0.5f, 0.5f, Ratio, false))
	{}

void RatioFrame::SetAndShow(Widget *Settee)
	{ gtk_container_add(GTK_CONTAINER(Data), GetData(Settee)); Settee->Show(); }

void RatioFrame::SetAndShow(GtkWidget *Settee)
	{ gtk_container_add(GTK_CONTAINER(Data), Settee); gtk_widget_show(Settee); }

// Unhidden notebook
Notebook::Notebook(void) : Widget(gtk_notebook_new())
	{}

int Notebook::Add(Widget *Addee, const String &Title)
{
	Label *TabLabel = new Label(Title);
	int Page = gtk_notebook_insert_page(GTK_NOTEBOOK(Data), GetData(Addee), GetData(TabLabel), -1);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(Data), GetData(Addee), true);
	TabLabel->Show();
	Addee->Show();
	return Page;
}

void Notebook::SetPage(int Page)
	{ gtk_notebook_set_current_page(GTK_NOTEBOOK(Data), Page); }

void Notebook::Rename(Widget *Addee, const String &NewTitle)
	{ gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(Data), GetData(Addee), NewTitle.c_str()); }

int Notebook::GetPage(void)
	{ return gtk_notebook_get_current_page(GTK_NOTEBOOK(Data)); }

int Notebook::Find(Widget *Addee)
	{ return gtk_notebook_page_num(GTK_NOTEBOOK(Data), GetData(Addee)); }

// Hidden notebook (the tabs are hidden)
HiddenNotebook::HiddenNotebook(void) : Widget(gtk_notebook_new())
{
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(Data), false);
}

int HiddenNotebook::Add(Widget *Addee)
{
	int Page = gtk_notebook_insert_page(GTK_NOTEBOOK(Data), GetData(Addee), NULL, -1);
	Addee->Show();
	return Page;
}

void HiddenNotebook::SetPage(int Page)
	{ gtk_notebook_set_current_page(GTK_NOTEBOOK(Data), Page); }

int HiddenNotebook::GetPage(void)
	{ return gtk_notebook_get_current_page(GTK_NOTEBOOK(Data)); }

// A scroller container
Scroller::Scroller(void) : Widget(gtk_scrolled_window_new(NULL, NULL))
{
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Data), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(Data), GTK_CORNER_TOP_RIGHT);
}

void Scroller::SetAndShow(Widget *Settee)
{
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Data), GetData(Settee));
	Settee->Show();
}

void Scroller::ShowRange(float Start, float End)
{
	GtkAdjustment *Adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data));
	float Base = gtk_adjustment_get_lower(Adjustment);
	float Difference = gtk_adjustment_get_upper(Adjustment) - Base;
	gtk_adjustment_clamp_page(Adjustment, Base + Difference * Start, Base + Difference * End);
}

// A 2D scroller for only what must necessarily be scrolled in 2D
CanvasScroller::CanvasScroller(void) : Widget(gtk_scrolled_window_new(NULL, NULL))
{
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(Data), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	//gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(Data), G);
}

void CanvasScroller::SetAndShow(Widget *Settee)
{
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(Data), GetData(Settee));
	Settee->Show();
}

void CanvasScroller::ShowRange(FlatVector Start, FlatVector End)
{
	gtk_adjustment_clamp_page(
		gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data)),
		Start[0], End[0]);

	gtk_adjustment_clamp_page(
		gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Data)),
		Start[1], End[1]);
}

void CanvasScroller::GoTo(FlatVector Position)
{
	GtkAdjustment *VAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data)),
		*HAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Data));

	Position -= FlatVector(gtk_adjustment_get_page_size(HAdjustment),
		gtk_adjustment_get_page_size(VAdjustment)) * 0.5f;

	gtk_adjustment_set_value(HAdjustment, Position[0]);
	gtk_adjustment_set_value(VAdjustment, Position[1]);
}

void CanvasScroller::GoToPercent(FlatVector Percent)
{
	GtkAdjustment *VAdjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data)),
		*HAdjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Data));

	std::cout << "Percent 1: " << Percent.AsString() << std::endl;
	Percent *= FlatVector(gtk_adjustment_get_upper(HAdjustment) - gtk_adjustment_get_upper(HAdjustment),
		gtk_adjustment_get_upper(VAdjustment) - gtk_adjustment_get_upper(VAdjustment));
	std::cout << "Percent 2: " << Percent.AsString() << std::endl;
	Percent += FlatVector(gtk_adjustment_get_lower(HAdjustment), gtk_adjustment_get_lower(VAdjustment));
	std::cout << "Percent 3: " << Percent.AsString() << std::endl;
	Percent -= FlatVector(gtk_adjustment_get_page_size(HAdjustment), gtk_adjustment_get_page_size(VAdjustment)) * 0.5f;
	std::cout << "Percent 4: " << Percent.AsString() << std::endl;

	gtk_adjustment_set_value(HAdjustment, Percent[0]);
	gtk_adjustment_set_value(VAdjustment, Percent[1]);
}

void CanvasScroller::Nudge(int Cardinality)
{
	if ((Cardinality == 0) || (Cardinality == 2))
	{
		GtkAdjustment *Adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data));
		float Jump = gtk_adjustment_get_step_increment(Adjustment);
		if (Cardinality == 0)
			gtk_adjustment_set_value(Adjustment, gtk_adjustment_get_value(Adjustment) - Jump);
		else gtk_adjustment_set_value(Adjustment, gtk_adjustment_get_value(Adjustment) + Jump);
	}
	else
	{
		GtkAdjustment *Adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Data));
		float Jump = gtk_adjustment_get_step_increment(Adjustment);
		if (Cardinality == 0)
			gtk_adjustment_set_value(Adjustment, gtk_adjustment_get_value(Adjustment) + Jump);
		else gtk_adjustment_set_value(Adjustment, gtk_adjustment_get_value(Adjustment) - Jump);
	}

}

void CanvasScroller::Nudge(const FlatVector &Offset)
{
	// NOTE manually clips value becase gtk is broken
	GtkAdjustment *Adjustment;

	Adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(Data));
	gtk_adjustment_set_value(Adjustment,
		RangeF(0, gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_page_size(Adjustment)).Constrain(
			gtk_adjustment_get_value(Adjustment) + Offset[0]));

	Adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(Data));
	gtk_adjustment_set_value(Adjustment,
		RangeF(0, gtk_adjustment_get_upper(Adjustment) - gtk_adjustment_get_page_size(Adjustment)).Constrain(
			gtk_adjustment_get_value(Adjustment) + Offset[1]));
}

///////////////////////////////////////////////////////////
// Label wrapper

// Title label
Title::Title(const String &Text) : Widget(gtk_label_new(NULL))
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
LabelBox::LabelBox(const String &Text) : Box(true), BoxLabel(Text)
	{ AddAndShow(&BoxLabel, false); }

void LabelBox::SetText(const String &NewText)
	{ BoxLabel.SetText(NewText); }

// Block (word wrapped) label
BlockLabel::BlockLabel(const String &Text) : Widget(gtk_label_new(Text.c_str()))
{
	gtk_label_set_line_wrap(GTK_LABEL(Data), true);
}

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
Button::Handler::~Handler(void) {}

Button::Button(Handler *Target, const String &Text) : Widget(NULL),
	Target(Target), LastIcon(diNone)
{
	if (Text.empty()) Data = gtk_button_new();
	else Data = gtk_button_new_with_label(Text.c_str());

	if (Target != NULL) // Null handlers for layout mockup purposes
		g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressHandler), this);
}

Button::Button(Handler *Target, const String &Text, DefaultIcons Icon) : Widget(NULL),
	Target(Target), LastIcon(Icon)
{
	if (Text.empty()) Data = gtk_button_new();
	else Data = gtk_button_new_with_label(Text.c_str());

	GtkWidget *IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(Data), IconWidget);

	if (Target != NULL) // Null handlers for layout mockup purposes
		g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressHandler), this);
}

void Button::Grey(bool On)
	{ gtk_widget_set_sensitive(Data, !On); }

void Button::SetText(const String &NewText)
	{ gtk_button_set_label(GTK_BUTTON(Data), NewText.c_str()); }

void Button::SetIcon(DefaultIcons Icon)
{
	if (Icon == LastIcon) return;
	GtkWidget *IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_button_set_image(GTK_BUTTON(Data), IconWidget);
	LastIcon = Icon;
}

void Button::PressHandler(GtkWidget *Widget, Button *This)
	{ This->Target->OnPress(This); }

// Short entry (one line text box) type
ShortEntry::Handler::~Handler(void) {}

ShortEntry::ShortEntry(Handler *Target, const String &InitialText) : Widget(gtk_entry_new()),
	Target(Target)
{
	if (!InitialText.empty()) gtk_entry_set_text(GTK_ENTRY(Data), InitialText.c_str());

	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "changed", G_CALLBACK(EntryHandler), this);
}

void ShortEntry::SetEditable(bool Editable)
	{ gtk_entry_set_editable(GTK_ENTRY(Data), Editable); }

void ShortEntry::SetValue(const String &NewText)
	{ gtk_entry_set_text(GTK_ENTRY(Data), NewText.c_str()); }

String ShortEntry::GetValue(void)
	{ return gtk_entry_get_text(GTK_ENTRY(Data)); }

void ShortEntry::EntryHandler(GtkWidget *Data, ShortEntry *This)
	{ This->Target->OnEntry(This); }

// Slida
Slider::Handler::~Handler(void) {}

Slider::Slider(Handler *Target, float Minimum, float Maximum, float InitialValue) :
	Widget(gtk_hscale_new_with_range(Minimum, Maximum, (Maximum - Minimum) / 10.0f)),
	Target(Target)
{
	gtk_range_set_value(GTK_RANGE(Data), InitialValue);
	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "value-changed", G_CALLBACK(SlideHandler), this);
}

void Slider::SetValue(const float &NewValue)
	{ gtk_range_set_value(GTK_RANGE(Data), NewValue); }

float Slider::GetValue(void)
	{ return gtk_range_get_value(GTK_RANGE(Data)); }

void Slider::SlideHandler(GtkWidget *Widget, Slider *This)
	{ This->Target->OnSlide(This); }

// Check button
CheckButton::Handler::~Handler(void) {}

CheckButton::CheckButton(Handler *Target, const String &Text, bool StartState) :
	Widget(gtk_check_button_new_with_label(Text.c_str())),
	Target(Target)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Data), StartState);
	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressHandler), this);
}

CheckButton::CheckButton(Handler *Target, bool StartState) :
	Widget(gtk_check_button_new()), Target(Target)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Data), StartState);
	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "clicked", G_CALLBACK(PressHandler), this);
}

void CheckButton::SetValue(bool NewValue)
	{ gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Data), NewValue); }

bool CheckButton::GetValue(void)
	{ return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Data)); }

void CheckButton::PressHandler(GtkWidget *Widget, CheckButton *This)
	{ This->Target->OnToggle(This); }

// Wheel
Wheel::Handler::~Handler(void) {}

Wheel::Wheel(Handler *Target, int Min, int Max, int Initial) : Widget(gtk_spin_button_new_with_range(Min, Max, Initial)),
	Target(Target)
{
	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "value-changed", G_CALLBACK(SpinHandler), this);
}

int Wheel::GetInt(void)
	{ return gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(Data)); }

void Wheel::SetValue(int NewValue)
	{ gtk_spin_button_set_value(GTK_SPIN_BUTTON(Data), NewValue); }

void Wheel::SpinHandler(GtkWidget *Widget, Wheel *This)
	{ This->Target->OnSpin(This); }

// Drop selection
List::Handler::~Handler(void) {}

List::List(List::Handler *Target, bool Multiline) : Widget(NULL),
	Target(Target), Multiline(Multiline), Added(0), Store(gtk_list_store_new(1, G_TYPE_STRING))
{
	GtkTreeModel *Model = GTK_TREE_MODEL(Store);
	GtkCellRenderer *ColumnRenderer = gtk_cell_renderer_text_new();

	if (Multiline)
	{
		Data = gtk_tree_view_new_with_model(Model);

		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(Data), -1, "Name", ColumnRenderer, "text", 0, NULL);

		if (Target != NULL)
			g_signal_connect(G_OBJECT(Data), "cursor-changed", G_CALLBACK(SelectHandler), this);
	}
	else
	{
		Data = gtk_combo_box_new_with_model(Model);

		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(Data), ColumnRenderer, true);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(Data), ColumnRenderer, "text", 0, NULL);

		if (Target != NULL)
			g_signal_connect(G_OBJECT(Data), "changed", G_CALLBACK(SelectHandler), this);
	}

	//g_object_unref(Model);
}

List::~List(void)
{
	/*for (std::vector<GtkTreeIter *>::iterator CurrentLink = ItemIterators.begin();
		CurrentLink != ItemIterators.end(); CurrentLink++)
		delete *CurrentLink;*/
	g_object_unref(Store);
}

bool List::Empty(void)
	{ return Added == 0; }

void List::Clear(void)
{
	/*for (std::vector<GtkTreeIter *>::iterator CurrentLink = ItemIterators.begin();
		CurrentLink != ItemIterators.end(); CurrentLink++)
		delete *CurrentLink;
	ItemIterators.clear();*/

	/*GtkListStore *Store = NULL;
	if (Multiline) Store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Data)));
	else Store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(Data)));*/

	gtk_list_store_clear(Store);

	Added = 0;
}

int List::Add(const String &NewString)
{
	/*if (Multiline)
	{
		GtkListStore *Model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Data)));*/

		GtkTreeIter Iterator;
		gtk_list_store_append(Store, &Iterator);
		gtk_list_store_set(Store, &Iterator, 0, NewString.c_str(), -1);

		//ItemIterators.push_back(Iterator);
	/*}
	else gtk_combo_box_append_text(GTK_COMBO_BOX(Data), NewString.c_str());*/

	return Added++;
}

void List::Remove(int Item)
{
	/*if (Multiline)
	{*/
		//GtkListStore *Model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(Data)));

		bool Result;

		GtkTreeIter Iterator;
		Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, NULL, Item); assert(Result);
		Result = gtk_list_store_remove(Store, &Iterator); assert(Result);
		//ItemIterators.erase(ItemIterators.begin() + Item);
	/*}
	else gtk_combo_box_remove_text(GTK_COMBO_BOX(Data), Item);*/

	Added--;
}

void List::Rename(int Item, const String &NewString)
{
	/*GtkTreeModel *Model;
	if (Multiline)
		Model = gtk_tree_view_get_model(GTK_TREE_VIEW(Data));
	else gtk_combo_box_get_model(GTK_COMBO_BOX(Data));*/
	GtkTreeIter Iterator;
	bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, NULL, Item); assert(Result);
	gtk_list_store_set(Store, &Iterator, 0, NewString.c_str(), -1);

}

void List::Select(int NewSelection)
{
	assert(NewSelection >= 0);
	assert(NewSelection < Added);

	if (Multiline)
	{
		//GtkTreeModel *Model = gtk_tree_view_get_model(GTK_TREE_VIEW(Data));
		GtkTreeIter Iterator;
		bool Result = gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(Store), &Iterator, NULL, NewSelection); assert(Result);
		GtkTreePath *NewPath = gtk_tree_model_get_path(GTK_TREE_MODEL(Store), &Iterator);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(Data), NewPath, NULL, false);
		gtk_tree_path_free(NewPath);
	}
	else gtk_combo_box_set_active(GTK_COMBO_BOX(Data), NewSelection);
}

int List::GetSelection(void)
{
	if (Multiline)
	{
		GtkTreePath *NewPath;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(Data), &NewPath, NULL);
		if (NewPath == NULL) return -1;
		int *Indexes = gtk_tree_path_get_indices(NewPath);
		int Out = Indexes[0];
		gtk_tree_path_free(NewPath);
		return Out;
	}
	else return gtk_combo_box_get_active(GTK_COMBO_BOX(Data));
}

void List::SelectHandler(GtkWidget *Widget, List *This)
	{ This->Target->OnSelect(This); }

// Toolbar as a menu button
MenuButton::Handler::~Handler(void) {}

MenuButton::MenuButton(MenuButton::Handler *Target, const String &Label, DefaultIcons Icon) : Button(this, Label, Icon),
	Target(Target), MenuData(gtk_menu_new()), OptionCount(0)
	{}

MenuButton::~MenuButton(void)
{
	gtk_widget_destroy(MenuData);
}

int MenuButton::Add(const String &NewString)
{
	GtkWidget *NewItem = gtk_menu_item_new_with_label(NewString.c_str());
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	if (Target != NULL)
		g_signal_connect(G_OBJECT(NewItem), "activate", G_CALLBACK(ClickHandler), this);
	gtk_widget_show(GTK_WIDGET(NewItem));

	int Out = OptionCount++;
	OptionMap[NewString] = Out;
	return Out;
}

int MenuButton::Add(const String &NewString, const DefaultIcons Icon)
{
	GtkWidget *NewItem = gtk_image_menu_item_new_with_label(NewString.c_str());
	GtkWidget *IconWidget = gtk_image_new_from_stock(ConvertStock(Icon), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(NewItem), IconWidget);

	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	if (Target != NULL)
		g_signal_connect(G_OBJECT(NewItem), "activate", G_CALLBACK(ClickHandler), this);
	gtk_widget_show(GTK_WIDGET(NewItem));

	int Out = OptionCount++;
	OptionMap[NewString] = Out;
	return Out;
}

int MenuButton::Add(const String &NewString, const String &IconFile)
{
	GtkWidget *NewItem = gtk_image_menu_item_new_with_label(NewString.c_str());
	GtkWidget *IconWidget = gtk_image_new_from_file(IconFile.c_str());
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(NewItem), IconWidget);

	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	if (Target != NULL)
		g_signal_connect(G_OBJECT(NewItem), "activate", G_CALLBACK(ClickHandler), this);
	gtk_widget_show(GTK_WIDGET(NewItem));

	int Out = OptionCount++;
	OptionMap[NewString] = Out;
	return Out;
}

void MenuButton::AddSpacer(void)
{
	GtkWidget *NewItem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(MenuData), NewItem);
	gtk_widget_show(GTK_WIDGET(NewItem));
}

void MenuButton::OnPress(Button *Source)
	{ gtk_menu_popup(GTK_MENU(MenuData), NULL, NULL, (GtkMenuPositionFunc)Positioning, this, 0, gtk_get_current_event_time()); }

void MenuButton::ClickHandler(GtkMenuItem *MenuWidget, MenuButton *This)
	{ This->Target->OnSelect(This, This->OptionMap[String(gtk_menu_item_get_label(GTK_MENU_ITEM(MenuWidget)))]); }

void MenuButton::Positioning(GtkMenu *Widget, gint *X, gint *Y, gboolean *ShouldPushFit, MenuButton *This)
{
	gdk_window_get_origin(This->Data->window, X, Y);

	*X += This->Data->allocation.x;
	*Y += This->Data->allocation.y + This->Data->allocation.height;

	*ShouldPushFit = true;
}

// File things
OpenSelect::Handler::~Handler(void) {}

OpenSelect::OpenSelect(Handler *Target, const String &InitialDirectory) :
	Widget(gtk_file_chooser_button_new("Select directory", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER)),
	Target(Target)
{
	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "file-set", G_CALLBACK(OpenHandler), this);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(Data), InitialDirectory.c_str());

	SingleFilter = NULL;
}

OpenSelect::OpenSelect(Handler *Target, const String &InitialFile, const String &FilterName) :
	Widget(gtk_file_chooser_button_new("Select file", GTK_FILE_CHOOSER_ACTION_OPEN)),
	Target(Target)
{
	if (Target != NULL)
		g_signal_connect(G_OBJECT(Data), "file-set", G_CALLBACK(OpenHandler), this);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Data), InitialFile.c_str());

	// Set filter for opening
	SingleFilter = gtk_file_filter_new();
	gtk_file_filter_set_name(SingleFilter, FilterName.c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Data), SingleFilter);
}

void OpenSelect::AddFilterPass(const String &Filter)
{
	assert(SingleFilter != NULL);
	gtk_file_filter_add_pattern(SingleFilter, Filter.c_str());
}

String OpenSelect::GetValue(void)
{
	char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Data));
	String Out = PreOut;
	g_free(PreOut);
	return Out;
}

void OpenSelect::OpenHandler(GtkWidget *Widget, OpenSelect *This)
	{ This->Target->OnSelect(This); }

OutputSelect::Handler::~Handler(void) {}

OutputSelect::OutputSelect(Handler *Target, const String &InitialDirectory) : Box(true),
	Target(Target), Location(NULL, InitialDirectory), SelectButton(this, "Select...")
{
	Location.SetEditable(false);
	AddAndShow(&Location, true);
	AddAndShow(&SelectButton, false);
}

String OutputSelect::GetValue(void)
	{ return Location.GetValue(); }

void OutputSelect::OnPress(Button *Source)
{
	GtkWidget *SaveDialog = gtk_file_chooser_dialog_new("Save...", GTK_WINDOW(GetWindow(Source)),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(SaveDialog), Location.GetValue().c_str());

	if (gtk_dialog_run(GTK_DIALOG(SaveDialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(SaveDialog));
		Location.SetValue(PreOut);
		g_free(PreOut);
	}

	gtk_widget_destroy(SaveDialog);

	if (Target != NULL) Target->OnSelect(this);
}

// Toolbox
Toolbox::Toolbox(void) : Widget(NULL),
	IDCounter(0)
{
	Model = gtk_list_store_new(3, G_TYPE_STRING, GDK_TYPE_PIXBUF, G_TYPE_INT, -1);

	Data = gtk_icon_view_new_with_model(GTK_TREE_MODEL(Model));
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(Data), 1);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(Data), 0);
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(Data), GTK_SELECTION_SINGLE);
	gtk_icon_view_set_orientation(GTK_ICON_VIEW(Data), GTK_ORIENTATION_HORIZONTAL);
	g_signal_connect(G_OBJECT(Data), "selection-changed", G_CALLBACK(HandleSelect), this);
}

Toolbox::~Toolbox(void)
	{}

void Toolbox::ForceColumns(int ColumnCount)
	{ gtk_icon_view_set_columns(GTK_ICON_VIEW(Data), ColumnCount); }

int Toolbox::AddItem(const String &Image, const String &Text)
{
	GtkTreeIter NewIterator;
	gtk_list_store_append(GTK_LIST_STORE(Model), &NewIterator);

	GdkPixbuf *NewPixbuf = NULL;
	if (!Image.empty())
	{
		GError *PixbufError = NULL;
		NewPixbuf = gdk_pixbuf_new_from_file(Image.c_str(), &PixbufError);
		if (NewPixbuf == NULL)
			g_print("Error loading toolbox image %s: %s\n",
				Image.c_str(), PixbufError->message);
	}
	if (NewPixbuf == NULL) NewPixbuf = gtk_widget_render_icon(Data,
		GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

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

	if (Selection == NULL) return -1;

	assert(g_list_length(Selection) == 1);

	GtkTreeIter Selected;
	gtk_tree_model_get_iter(GTK_TREE_MODEL(Model), &Selected, (GtkTreePath *)Selection->data);

	gint ID;
	gtk_tree_model_get(GTK_TREE_MODEL(Model), &Selected, 2, &ID, -1);

	g_list_foreach(Selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(Selection);

	return ID;
}

void Toolbox::Unselect(void)
	{ gtk_icon_view_unselect_all(GTK_ICON_VIEW(Data)); }

void Toolbox::Clear(void)
	{ gtk_list_store_clear(GTK_LIST_STORE(Model)); IDCounter = 0; }

void Toolbox::OnSelect(int NewSelection)
	{}

void Toolbox::HandleSelect(Widget *Data, Toolbox *This)
	{ This->OnSelect(This->GetSelected()); }

// Toolbar, button menu type
Toolbar::Handler::~Handler(void) {}

Toolbar::Toolbar(void) : Widget(gtk_toolbar_new())
{
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(Data), true);
	gtk_toolbar_set_style(GTK_TOOLBAR(Data), GTK_TOOLBAR_BOTH_HORIZ);
}

void *Toolbar::AddButton(Handler *Target, const String &Text)
{
	GtkToolItem *NewButton = gtk_tool_button_new(NULL, Text.c_str());
	g_signal_connect(G_OBJECT(NewButton), "clicked", G_CALLBACK(HandleSelect), this);
	gtk_toolbar_insert(GTK_TOOLBAR(Data), NewButton, -1);
	gtk_widget_show(GTK_WIDGET(NewButton));

	ButtonMap[NewButton] = Target;

	return NewButton;
}

void *Toolbar::AddButton(Handler *Target, const String &Text, DefaultIcons Icon)
{
	GtkToolItem *NewButton = gtk_tool_button_new_from_stock(ConvertStock(Icon));
	if (!Text.empty()) gtk_tool_button_set_label(GTK_TOOL_BUTTON(NewButton), Text.c_str());
	g_signal_connect(G_OBJECT(NewButton), "clicked", G_CALLBACK(HandleSelect), this);
	gtk_toolbar_insert(GTK_TOOLBAR(Data), NewButton, -1);
	gtk_widget_show(GTK_WIDGET(NewButton));

	ButtonMap[NewButton] = Target;

	return NewButton;
}

void Toolbar::HandleSelect(Widget *Data, Toolbar *This)
{
	std::map<void *, Handler *>::iterator Found = This->ButtonMap.find(Data);
	assert(Found != This->ButtonMap.end());

	if (Found->second == NULL) return;

	Found->second->OnPress(Data);
}

// File open/save dialog
FileDialog::FileDialog(const String &Title, const String &FilterName, Window *Parent, bool SaveMode)
{
	Data = gtk_file_chooser_dialog_new(Title.c_str(), GTK_WINDOW(Parent->GetWidget()),
		(SaveMode) ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		(SaveMode) ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	//if (SaveMode)
	//	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(Data), true);

	// Create the default filter
	SingleFilter = gtk_file_filter_new();
	gtk_file_filter_set_name(SingleFilter, FilterName.c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(Data), SingleFilter);
}

void FileDialog::AddFilterPass(const String &Pattern)
{
	assert(Data != NULL);
	assert(SingleFilter != NULL);
	gtk_file_filter_add_pattern(SingleFilter, Pattern.c_str());
}

void FileDialog::SetDefaultSuffix(const String &NewSuffix)
	{ Suffix = NewSuffix; }

void FileDialog::SetFile(const String &NewFile)
{
	assert(Data != NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(Data), NewFile.c_str());
}

void FileDialog::SetDirectory(const String &NewDirectory)
{
	assert(Data != NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(Data), NewDirectory.c_str());
}

String FileDialog::Run(void)
{
	assert(Data != NULL);

	String Out;
	if (gtk_dialog_run(GTK_DIALOG(Data)) == GTK_RESPONSE_ACCEPT)
	{
		char *PreOut = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(Data));
		Out = PreOut;
		g_free(PreOut);

		if (Right(Out, Suffix.size()) != Suffix)
			Out += Suffix;
	}

	gtk_widget_destroy(Data);
	Data = NULL;

	return Out;
}

// COlor select button
ColorButton::ColorButton(const Color &InitialColor, bool SelectAlpha) :
	Widget(gtk_color_button_new()), Alpha(SelectAlpha)
{
	if (Alpha)
		gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(Data), true);

	SetColor(InitialColor);

	g_signal_connect(G_OBJECT(Data), "color-set", G_CALLBACK(HandleSelect), this);
}

ColorButton::~ColorButton(void)
	{}

Color ColorButton::GetColor(void)
{
	GdkColor PreOut;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(Data), &PreOut);
	Color Out(
		(float)PreOut.red / 65535.0f,
		(float)PreOut.green / 65535.0f,
		(float)PreOut.blue / 65535.0f);

	if (Alpha) Out.Alpha = (float)gtk_color_button_get_alpha(GTK_COLOR_BUTTON(Data)) / 65535.0f;

	return Out;
}

void ColorButton::SetColor(const Color &NewColor)
{
	GdkColor ColorConversion;
	ColorConversion.red = std::min(NewColor.Red * 65535, 65535.0f);
	ColorConversion.green = std::min(NewColor.Green * 65535, 65535.0f);
	ColorConversion.blue = std::min(NewColor.Blue * 65535, 65535.0f);
	gtk_color_button_set_color(GTK_COLOR_BUTTON(Data), &ColorConversion);

	if (Alpha) gtk_color_button_set_alpha(GTK_COLOR_BUTTON(Data), NewColor.Alpha);
}

void ColorButton::OnSelect(const Color &NewColor) {}

void ColorButton::HandleSelect(GtkWidget *Widget, ColorButton *This)
	{ This->OnSelect(This->GetColor()); }
