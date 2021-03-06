#ifndef gtkwrapper_h
#define gtkwrapper_h

#include "../ren-general/auxinclude.h"
#include "../ren-general/vector.h"
#include "../ren-general/color.h"
#include "../ren-general/range.h"

#include <gtk/gtk.h>
#include <vector>
#include <utility>
#include <functional>

enum DefaultIcons
{
	diNone,
	diUp, diDown,
	diAdd, diRemove, diDelete,
	diNew, diOpen, diSave, diRefresh, diClear,
	diFirst, diLast, diTop, diBottom,
	diPlay, diPause, diRewind, diStop,
	diInfo, diProperties, diConfigure, diOkay, diCancel, diClose, diQuit
};

namespace GTK
{
	void Error(GtkWidget *Anchor, const String &Section, const String &Description);
	void Warning(GtkWidget *Anchor, const String &Section, const String &Description);
	bool Confirm(GtkWidget *Anchor, const String &Section, const String &Description);
}

////////////////////////////////////////////////////////////////
// Events
typedef std::function<void(void)> ActionHandler;
typedef std::function<void(void)> InputHandler;
typedef std::function<bool(unsigned int KeyCode, unsigned int Modifier)> KeyHandler;

////////////////////////////////////////////////////////////////
// Base widget
class Widget
{
	public:
		Widget(GtkWidget *Data);
		virtual ~Widget(void);
		operator GtkWidget*(void);

		void Hide(void);
		void Show(void);
		void Disable(void);
		void Enable(void);

		void DestroyWhenDeleted(void);
	protected:
		GtkWidget *Data;
		bool Destroy;
};

// Widget helpers
class ConnectionAnchor
{
	public:
		template <typename CallbackType>
		ConnectionAnchor(GtkWidget *Data, String const &ConnectionName, CallbackType const &Callback, void *This) : 
			Data(Data), 
			ConnectionID(g_signal_connect(Data, ConnectionName.c_str(), G_CALLBACK(Callback), This))
		{
		}

		ConnectionAnchor(void) : Data(nullptr) {}

		ConnectionAnchor &operator =(ConnectionAnchor &&Other)
		{
			Data = Other.Data;
			ConnectionID = Other.ConnectionID;
			Other.Data = nullptr;
			return *this;
		}

		~ConnectionAnchor(void)
		{
			if (Data != nullptr)
				g_signal_handler_disconnect(G_OBJECT(Data), ConnectionID);
		}
	private:
		GtkWidget *Data;
		gulong ConnectionID;
};

// Nun widgets
class TimedEvent
{
	public:
		TimedEvent(unsigned int Milliseconds = 50);
		virtual ~TimedEvent(void);
		
		void SetAction(ActionHandler const &Handler);

		void StartTimer(void);
		void StopTimer(void);
		
		unsigned int GetTimerPeriod(void) const;

	private:
		static gboolean TimeHandler(TimedEvent *This);

		ActionHandler Handler;

		unsigned int Period;
		int TimerID;
};

////////////////////////////////////////////////////////////////
// Widget extensions
class KeyboardWidget
{
	public:
		KeyboardWidget(GtkWidget *Data);
		~KeyboardWidget(void);

		void SetHandler(KeyHandler const &Handler);
		void DestroyWhenDeleted(void);

	private:
		GtkWidget *Data;
		KeyHandler Handler;

		static gboolean KeyCallback(GtkWidget *, GdkEventKey *Event, KeyboardWidget *This);
		gulong ConnectionID;
		
		bool Destroy;
};

////////////////////////////////////////////////////////////////
// Stuff
class MenuItem : public Widget
{
	public:
		MenuItem(String const &Text);
		MenuItem(String const &Text, DefaultIcons const Icon);
		MenuItem(String const &Text, String const &IconFilename);
		~MenuItem(void);

		void SetAction(ActionHandler const &Handler);

	private:
		ActionHandler Handler;

		static void ClickHandler(GtkMenuItem *, MenuItem *This);
		gulong ConnectionID;
};

class ToolButton : public Widget
{
	public:
		ToolButton(String const &Text);
		ToolButton(String const &Text, DefaultIcons const Icon);
		~ToolButton(void);
		
		void SetAction(ActionHandler const &Handler);
		void SetPrompt(String const &NewPrompt);

	private:
		ActionHandler Handler;

		static void ClickHandler(GtkToolItem *, ToolButton *This);
		gulong ConnectionID;
};

////////////////////////////////////////////////////////////////
// Window types
class Window : public Widget
{
	public:
		Window(const String &Title, unsigned int const &EdgePadding = 6);
		~Window(void);

		void SetAttemptCloseHandler(std::function<bool(void)> const &Handler); // return true to allow close
		void SetCloseHandler(std::function<void(void)> const &Handler);
		void SetResizeHandler(std::function<void(void)> const &Handler);
		void SetIcon(const String &Filename);
		void SetTitle(const String &NewTitle);
		void SetFullscreen(bool On);
		void SetDefaultSize(const FlatVector &Size);

		void Set(GtkWidget *Addee);

	private:
		static gboolean AttemptCloseCallback(GtkWidget *, GdkEvent *, Window *This);
		gulong AttemptCloseConnectionID;

		static void CloseCallback(GtkWidget *, Window *This);
		gulong CloseConnectionID;

		static gboolean ResizeCallback(GtkWidget *, GdkEventConfigure *, Window *This);

		std::function<bool(void)> AttemptCloseHandler;
		std::function<void(void)> CloseHandler;
		std::function<void(void)> ResizeHandler;
};

class Dialog : public Widget
{
	public:
		Dialog(GtkWidget *Parent, const String &Title, FlatVector const &DefaultSize = FlatVector());
		~Dialog(void);

		void Add(GtkWidget *Widget);
		void AddFill(GtkWidget *Widget);
		void AddAction(GtkWidget *Widget);
		void AddActionFill(GtkWidget *Widget);
		void SetDefaultSize(const FlatVector &DefaultSize);

		// Control
		void Run(void);

	protected:
		void Close(void);
};

typedef std::function<void(gint &X, gint &Y)> MenuPositionHandler;
class PopupMenu
{
	public:
		PopupMenu(void);
		~PopupMenu(void);

		void SetPositionHandler(MenuPositionHandler const &Handler);
		void Clear(void);
		MenuItem *Add(MenuItem *NewItem);
		void AddSeparator(void);

		void Activate(void);

	private:
		static void PositionCallback(GtkMenu *, gint *X, gint *Y, gboolean *ForceVisible, PopupMenu *This);
		GtkWidget *MenuData;
		MenuPositionHandler Handler;
};

////////////////////////////////////////////////////////////////
// Containers
class Layout : public Widget
{
	public:
		Layout(bool Horizontal, int EdgePadding = 3, int ItemPadding = 3);

		void Add(GtkWidget *Widget);
		void AddFill(GtkWidget *Widget);
		void AddSpacer(void);
		void AddSpace(void);
	private:
		bool Horizontal;
};

/* TODO
class ZebraLayout : public Layout
{
	public:
		Layout(bool Horizontal, int EdgePadding = 3, int ItemPadding = 3);
		void SetLightColor(Color const &LightColor);
		void SetDarkColor(Color const &DarkColor);
	private:
		Color LightColor;
		Color DarkColor;
};*/

class ColorLayout : public Widget
{
	public:
		ColorLayout(bool Horizontal, int EdgePadding = 3, int ItemPadding = 3);
		
		Color GetDefaultColor(void) const;
		void SetColor(Color const &NewColor);
		
		void Add(GtkWidget *Widget);
		void AddFill(GtkWidget *Widget);
		void AddSpacer(void);
		void AddSpace(void);
	private:
		Layout Inner;
};

class LayoutBorder : public Widget
{
	public:
		LayoutBorder(void);
		LayoutBorder(const String &Title);
		void Set(GtkWidget *Settee);
};

class RatioFrame : public Widget
{
	public:
		RatioFrame(const String &Title, float Ratio);
		void Set(GtkWidget *Settee);
};

class Notebook : public Widget
{
	public:
		Notebook(void);

		int Add(GtkWidget *Addee, const String &Title);
		void SetPage(int Page);

		void Rename(GtkWidget *Addee, const String &NewTitle);

		int GetPage(void);
		int Find(GtkWidget *Addee);
};

class HiddenNotebook : public Widget
{
	public:
		HiddenNotebook(void);

		int Add(GtkWidget *Addee);
		void SetPage(int Page);

		int GetPage(void);
};

class Scroller : public Widget
{
	public:
		Scroller(void);

		void Set(GtkWidget *Settee);

		void ShowRange(float Start, float End);
};

class CanvasScroller : public Widget
{
	public:
		CanvasScroller(void);

		void Set(GtkWidget *Settee);

		void ShowRange(FlatVector Start, FlatVector End);
		void GoTo(FlatVector Position);
		void GoToPercent(FlatVector Percent);
		void Nudge(int Cardinality); // 0 = up, 1 = right
		void Nudge(const FlatVector &Offset);
		
	private:
		void SetAdjustments(int NewX, int NewY);
		void DoInitialAdjustment(void);
		static void InitialStateChangeHandler(void *, CanvasScroller *This);
		
		bool InitialAdjustmentCompleted;
		std::function<void(void)> InitialAdjustmentFunction;
		
		GtkAdjustment *HorizontalAdjustment, *VerticalAdjustment;
		gulong HorizontalScrollHandler;
		gulong VerticalScrollHandler;
};

////////////////////////////////////////////////////////////////
// Elements/non containers
class Title : public Widget
{
	public:
		Title(const String &Text);

		void SetText(const String &NewText);
		void SetHardSize(bool On);
};

class Label : public Widget
{
	public:
		Label(const String &Text);

		void SetText(const String &NewText);
		void SetHardSize(bool On);
};

class LinkLabel : public Widget
{
	public:
		LinkLabel(const String &URL);
};

class ProgressLabel : public Widget
{
	public:
		ProgressLabel(void);

		void SetText(const String &NewText);
		void SetPercent(float NewPercent);
		void SetHardSize(bool On);
};

class LabelBox : public Layout
{
	public:
		LabelBox(const String &Text);
		void SetText(const String &NewText);
	private:
		Label BoxLabel;
};

class BlockLabel : public Widget
{
	public:
		BlockLabel(const String &Text);
		void SetText(const String &NewText);
};

class Sticker : public Widget
{
	public:
		Sticker(const String &Filename);
};

class Article : public Widget
{
	public:
		Article(const String &Text = String());

		void SetText(const String &Text);
};

class Button : public Widget
{
	public:
		Button(const String &Text, bool Small = false);
		Button(const String &Text, DefaultIcons Icon, bool Small = false);
		~Button(void);

		void SetAction(ActionHandler const &Handler);
		void SetText(const String &NewText);
		void SetIcon(DefaultIcons Icon);

	private:
		ActionHandler Handler;

		static void PressHandler(GtkWidget *, Button *This);
		gulong ConnectionID;

		DefaultIcons LastIcon;
};

class ShortEntry : public Layout
{
	public:
		ShortEntry(String const &Prompt, String const &InitialText);
		~ShortEntry(void);

		void SetInputHandler(InputHandler const &Handler);
		void SetEditable(bool Editable);

		void SetValue(const String &NewText);
		String GetValue(void) const;

	private:
		GtkWidget *EntryData;
		InputHandler Handler;

		static void EntryCallback(GtkWidget *, ShortEntry *This);
		gulong ConnectionID;
};

class LongEntry : public Widget
{
	public:
		LongEntry(String const &InitialData);
		
		void SetEditable(bool Editable);
		
		void SetText(const String &NewText);
		String GetText(void) const;
	private:
		GtkTextBuffer *Buffer;
};

class Slider : public Layout
{
	public:
		Slider(String const &Prompt, RangeF const &ValueRange, float Initial);
		~Slider(void);
		
		void SetInputHandler(InputHandler const &Handler);
		void SetPrompt(String const &NewPrompt);

		float GetValue(void);
		void SetValue(float NewValue);
	private:
		static void HandleChangeSliderPosition(GtkWidget *, Slider *This);
		static void HandleChangeEntryText(GtkWidget *, Slider *This);

		const RangeF ValueRange;
		GtkWidget *SliderData, *EntryData;
		Label PromptLabel;
		InputHandler Handler;

		gulong SliderHandlerID;
		gulong EntryHandlerID;
};

class CheckButton : public Widget
{
	public:
		CheckButton(const String &Text, bool StartState);
		CheckButton(bool StartState);
		~CheckButton(void);

		void SetAction(ActionHandler const &Handler);

		void SetPrompt(String const &NewPrompt);
		void SetValue(bool NewValue);
		bool GetValue(void);

	private:
		ActionHandler Handler;

		static void PressCallback(GtkWidget *, CheckButton *This);
		gulong ConnectionID;
};

class Wheel : public Layout
{
	public:
		Wheel(String const &Prompt, RangeF const &ValueRange, float Initial, bool Float = false);
		~Wheel(void);
		
		void SetInputHandler(InputHandler const &Handler);

		int GetInt(void);
		float GetFloat(void);
		float GetValue(void);
		void SetValue(int NewValue);
		void SetValue(float NewValue);
	private:
		const RangeF ValueRange;
		GtkWidget *WheelData;
		InputHandler Handler;

		static void SpinCallback(GtkWidget *, Wheel *This);
		gulong ConnectionID;
};

class List : public Widget
{
	public:
		List(String const &Prompt, bool Multiline = false);
		~List(void);

		using Widget::Hide;
		using Widget::Show;
		
		void SetInputHandler(InputHandler const &Handler);

		bool Empty(void);
		unsigned int Size(void);

		void Clear(void);
		int Add(const String &NewString);
		int Add(const String &NewString, int Position);
		void Remove(int Item);
		void MoveUp(int Item);
		void MoveDown(int Item);
		void Rename(int Item, const String &NewString);
		void Hide(int Item);
		void Show(int Item);
		void Select(int NewSelection);
		void Deselect(void) { Select(-1); }

		int GetSelection(void);
	private:
		GtkWidget *ListData;
		InputHandler Handler;

		static void SelectCallback(GtkWidget *, List *This);
		gulong ConnectionID;

		bool Multiline;

		int Added;

		GtkListStore *Store;
};

class MenuButton : public Button
{
	public:
		MenuButton(const String &Label, DefaultIcons Icon);

		MenuItem *Add(MenuItem *NewItem);
		void AddSeparator(void);

	private:
		PopupMenu MenuData;
};

class DirectorySelect : public Layout
{
	public:
		DirectorySelect(String const &Prompt, String const &InitialDirectory);
		~DirectorySelect(void);

		void SetAction(ActionHandler const &Handler);

		void SetValue(String const &NewDirectory);
		String GetValue(void);

	private:
		GtkWidget *ButtonData;
		ActionHandler Handler;

		static void OpenCallback(GtkWidget *, DirectorySelect *This);
		gulong ConnectionID;
};

class OpenSelect : public Layout
{
	public:
		OpenSelect(String const &Prompt, const String &InitialFile, const String &FilterName);
		~OpenSelect(void);

		void SetAction(ActionHandler const &Handler);
		void AddFilterPass(const String &Filter);

		String GetValue(void);

	private:
		GtkWidget *ButtonData;
		ActionHandler Handler;

		static void OpenCallback(GtkWidget *, OpenSelect *This);
		gulong ConnectionID;

		GtkFileFilter *SingleFilter;
};

class OutputSelect : public Layout
{
	public:
		OutputSelect(String const &Prompt, String const &InitialDirectory);
		
		void SetAction(ActionHandler const &Handler);

		String GetValue(void);

	private:
		ActionHandler Handler;

		ShortEntry Location;
		Button SelectButton;
};

class Toolbox : public Widget
{
	public:
		Toolbox(void);
		virtual ~Toolbox(void);

		void ForceColumns(int ColumnCount);

		int AddItem(const String &Image, const String &Text);
		int GetSelected(void);

		void Unselect(void);

		void Clear(void);

	protected:
		virtual void OnSelect(int);

	private:
		static void HandleSelect(GtkWidget *, Toolbox *This);
		gulong ConnectionID;

		int IDCounter;

		GtkListStore *Model;
};

class Toolbar : public Widget
{
	public:
		Toolbar(void);

		void Add(ToolButton &NewItem, int AtPosition = -1);
		void Add(GtkWidget *Item, int AtPosition = -1);
		void AddSpacer(int AtPosition = -1);
		void AddSpace(int AtPosition = -1);
};

////////////////////////////////////////////////////////////////
// Dialogs
class FileDialog
{
	public:
		FileDialog(const String &Title, const String &FilterName, Window *Parent, bool SaveMode);
		FileDialog(const String &Title, const String &FilterName, GtkWidget *Parent, bool SaveMode);

		void AddFilterPass(const String &Pattern);
		void SetDefaultSuffix(const String &NewSuffix);
		void SetFile(const String &NewFile);
		void SetDirectory(const String &NewDirectory);

		String Run(void);

	private:
		String Suffix;

		GtkWidget *Data;
		GtkFileFilter *SingleFilter;
};

////////////////////////////////////////////////////////////////
// Very special case items
class ColorButton : public Layout
{
	public:
		ColorButton(String const &Prompt, Color const &InitialColor, bool SelectAlpha);
		~ColorButton(void);
		
		void SetAction(ActionHandler const &Handler);

		Color GetColor(void);
		void SetColor(const Color &NewColor);

	private:
		bool Alpha;

		GtkWidget *ButtonData;

		static void HandleSelect(GtkWidget *, ColorButton *This);
		gulong ConnectionID;

		Label Prompt;
		
		ActionHandler Handler;
};

class ColorToggleButton : public Widget
{
	public:
		ColorToggleButton(bool InitiallyFore, Color const &ForegroundColor, Color const &BackgroundColor);
		~ColorToggleButton(void);

		void SetState(bool Fore);
		bool GetState(void);

		void SetForegroundColor(Color const &NewForegroundColor);
		void SetBackgroundColor(Color const &NewBackgroundColor);
	private:
		void Refresh(void);
		static gboolean RefreshCallback(GtkWidget *, GdkEventExpose *Event, ColorToggleButton *This);
		static void ClickCallback(GtkWidget *, ColorToggleButton *This);

		bool State;
		Color Foreground, Background;
		GtkWidget *ColorArea;

		gulong RefreshConnectionID, ClickConnectionID;
};

#endif
