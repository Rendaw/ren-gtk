#ifndef gtkwrapper_h
#define gtkwrapper_h

#include "../general/auxinclude.h"
#include "../general/vector.h"
#include "../general/color.h"

#include <gtk/gtk.h>
#include <vector>

enum DefaultIcons
{
	diNone,
	diUp, diDown,
	diAdd, diRemove, diDelete,
	diNew, diOpen, diSave, diRefresh,
	diFirst, diLast, diTop, diBottom,
	diPlay, diPause, diRewind, diStop,
	diInfo, diProperties, diConfigure, diOkay, diCancel, diClose, diQuit
};

////////////////////////////////////////////////////////////////
// Nun widgets
class TimedEvent
{
	public:
		TimedEvent(unsigned int Milliseconds = 50);
		virtual ~TimedEvent(void);

		void StartTimer(void);
		void StopTimer(void);

	protected:
		virtual void TickEvent(void) = 0;

	private:
		static gboolean TimeHandler(TimedEvent *This);

		unsigned int Period;
		int TimerID;
};

////////////////////////////////////////////////////////////////
// Base type
class Widget
{
	public:
		Widget(GtkWidget *NewData);
		virtual ~Widget(void);
		void Show(void);
		void Hide(void);

	protected:
		GtkWidget *GetData(Widget *Other);
		GtkWidget *GetWindow(Widget *Other);

		GtkWidget *Data;
};

// Widget extensions
class KeyboardWidget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual bool OnKey(unsigned int KeyCode, unsigned int Modifier) = 0;
		};
		KeyboardWidget(GtkWidget *Data, Handler *Target);

	private:
		Handler *Target;
		static gboolean KeyHandler(GtkWidget *Widget, GdkEventKey *Event, KeyboardWidget *This);
};

////////////////////////////////////////////////////////////////
// Window types
class Window : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual bool ConfirmClose(Window *Source) = 0;
				virtual void OnClose(Window *Source) = 0;
		};

		Window(const String &Title, Handler *Target);

		void SetIcon(const String &Filename);
		void SetTitle(const String &NewTitle);
		void SetFullscreen(bool On);
		void SetDefaultSize(const FlatVector &Size);

		void Set(Widget *Addee);
		void SetAndShow(Widget *Addee);

		// Popups
		void Error(const String &Section, const String &Description);
		void Warning(const String &Section, const String &Description);
		bool Confirm(const String &Section, const String &Description);

	protected:
		friend class FileDialog;
		GtkWidget *GetWidget(void);

	private:
		Handler *Target;

		static gboolean DeleteHandler(GtkWidget *Source, GdkEvent *Event, Window *This);
		static void DestroyHandler(GtkWidget *Source, Window *This);
};

class Dialog : private Widget
{
	public:
		Dialog(Window *Parent, const String &Title);

		void AddAndShow(Widget *NewWidget, bool Fill = true);
		void AddAndShowAction(Widget *NewWidget, bool Fill = true);

		// Popups
		void Error(const String &Section, const String &Description);
		void Warning(const String &Section, const String &Description);
		bool Confirm(const String &Section, const String &Description);

		// Control
		void Run(void);

	protected:
		void Close(void);
};

class PopupMenu
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSelect(PopupMenu *Source, int NewSelection) = 0;
		};

		PopupMenu(Handler *Target);
		~PopupMenu(void);

		void Clear(void);

		int Add(const String &NewString);
		int Add(const String &NewString, const DefaultIcons Icon);
		int Add(const String &NewString, const String &IconFile);

		void Show(void);

	protected:
		//virtual void OnSelect(int NewSelection);

	private:
		Handler *Target;

		static void ClickHandler(GtkMenuItem *MenuWidget, PopupMenu *This);

		GtkWidget *MenuData;

		int OptionCount;
		std::map<String, int> OptionMap;
};

////////////////////////////////////////////////////////////////
// Containers
class Box : public Widget
{
	public:
		Box(bool Horizontal);
		void Add(Widget *Addee, bool Fill = true);
		void Add(GtkWidget *Addee, bool Fill = true);
		void AddAndShow(Widget *Addee, bool Fill = true);

		void AddSpacer(void);
		void AddSpace(void);

	private:
		bool IsHorizontal;
};

inline Box *HorizontalBox(void) { return new Box(true); }
inline Box *VerticalBox(void) { return new Box(false); }

class Table : public Widget
{
	public:
		Table(void);
		void Resize(unsigned int Width, unsigned int Height);
		void Add(Widget *Addee, int X, int Y, bool Fill = true);
		void AddAndShow(Widget *Addee, int X, int Y, bool Fill = true);
};

class Frame : public Widget
{
	public:
		Frame(void);
		Frame(const String &Title);

		void Set(Widget *Settee);
		void SetAndShow(Widget *Settee);
		void Set(GtkWidget *Settee);
};

class RatioFrame : public Widget
{
	public:
		RatioFrame(const String &Title, float Ratio);
		void SetAndShow(Widget *Settee);
		void SetAndShow(GtkWidget *Settee);
};

class Notebook : public Widget
{
	public:
		Notebook(void);

		int Add(Widget *Addee, const String &Title);
		void SetPage(int Page);

		void Rename(Widget *Addee, const String &NewTitle);

		int GetPage(void);
		int Find(Widget *Addee);
};

class HiddenNotebook : public Widget
{
	public:
		HiddenNotebook(void);

		int Add(Widget *Addee);
		void SetPage(int Page);

		int GetPage(void);
};

class Scroller : public Widget
{
	public:
		Scroller(void);
		void SetAndShow(Widget *Settee);

		void ShowRange(float Start, float End);
};

class CanvasScroller : public Widget
{
	public:
		CanvasScroller(void);
		void SetAndShow(Widget *Settee);

		void ShowRange(FlatVector Start, FlatVector End);
		void GoTo(FlatVector Position);
		void GoToPercent(FlatVector Percent);
		void Nudge(int Cardinality); // 0 = up, 1 = right
		void Nudge(const FlatVector &Offset);
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

class LabelBox : public Box
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
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnPress(Button *Source) = 0;
		};

		Button(Handler *Target, const String &Text);
		Button(Handler *Target, const String &Text, DefaultIcons Icon);

		// Manipulation
		void Grey(bool On = true);
		void SetText(const String &NewText);
		void SetIcon(DefaultIcons Icon);

	private:
		static void PressHandler(GtkWidget *Widget, Button *This);

		Handler *Target;
		DefaultIcons LastIcon;
};

class ShortEntry : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnEntry(ShortEntry *Source) = 0;
		};

		ShortEntry(Handler *Target, const String &InitialText);

		void SetEditable(bool Editable);

		void SetValue(const String &NewText);
		String GetValue(void);

	private:
		static void EntryHandler(GtkWidget *Data, ShortEntry *This);

		Handler *Target;
};

class Slider : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSlide(Slider *Source) = 0;
		};

		Slider(Handler *Target, float Minimum, float Maximum, float InitialValue);

		void SetValue(const float &NewValue);
		float GetValue(void);

	private:
		static void SlideHandler(GtkWidget *Widget, Slider *This);

		Handler *Target;
};

class Wheel : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSpin(Wheel *Source) = 0;
		};

		Wheel(Handler *Target, int Min, int Max, int Initial);

		int GetInt(void);
		void SetValue(int NewValue);
	private:
		static void SpinHandler(GtkWidget *Widget, Wheel *This);

		Handler *Target;
};

class CheckButton : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnToggle(CheckButton *Source) = 0;
		};

		CheckButton(Handler *Target, const String &Text, bool StartState);
		CheckButton(Handler *Target, bool StartState);

		void SetValue(bool NewValue);
		bool GetValue(void);

	private:
		static void PressHandler(GtkWidget *Widget, CheckButton *This);

		Handler *Target;
};

class List : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSelect(List *Source) = 0;
		};

		List(Handler *Target, bool Multiline = false);
		virtual ~List(void);

		bool Empty(void);
		void Clear(void);
		int Add(const String &NewString);
		void Remove(int Item);
		void Rename(int Item, const String &NewString);
		void Select(int NewSelection);

		int GetSelection(void);

	private:
		static void SelectHandler(GtkWidget *Widget, List *This);

		Handler *Target;

		bool Multiline;

		int Added;

		//std::vector<GtkTreeIter *> ItemIterators;
		GtkListStore *Store;
};

class MenuButton : public Button, public Button::Handler
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSelect(MenuButton *Source, int NewSelection) = 0;
		};

		MenuButton(Handler *Target, const String &Label, DefaultIcons Icon);
		virtual ~MenuButton(void);

		int Add(const String &NewString);
		int Add(const String &NewString, const DefaultIcons Icon);
		int Add(const String &NewString, const String &IconFile);
		void AddSpacer(void);

	protected:
		void OnPress(Button *Source);

	private:
		static void ClickHandler(GtkMenuItem *MenuWidget, MenuButton *This);
		static void Positioning(GtkMenu *Widget, gint *X, gint *Y, gboolean *ShouldPushFit, MenuButton *This);

		Handler *Target;

		GtkWidget *MenuData;

		int OptionCount;
		std::map<String, int> OptionMap;
};

class OpenSelect : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSelect(OpenSelect *Source) = 0;
		};

		OpenSelect(Handler *Target, const String &InitialDirectory);
		OpenSelect(Handler *Target, const String &InitialFile, const String &FilterName);

		void AddFilterPass(const String &Filter);

		String GetValue(void);

	private:
		Handler *Target;

		static void OpenHandler(GtkWidget *Widget, OpenSelect *This);

		GtkFileFilter *SingleFilter;
};

class OutputSelect : public Box, public Button::Handler
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnSelect(OutputSelect *Source) = 0;
		};

		OutputSelect(Handler *Target, const String &InitialDirectory);

		String GetValue(void);

	protected:
		void OnPress(Button *Source);

	private:
		Handler *Target;

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
		virtual void OnSelect(int NewSelection);

	private:
		static void HandleSelect(Widget *Data, Toolbox *This);

		int IDCounter;

		GtkListStore *Model;
};

class Toolbar : public Widget
{
	public:
		class Handler
		{
			public:
				virtual ~Handler(void);
				virtual void OnPress(void *Source) = 0;
		};

		Toolbar(void);

		void *AddButton(Handler *Target, const String &Text);
		void *AddButton(Handler *Target, const String &Text, DefaultIcons Icon);
	private:
		static void HandleSelect(Widget *Data, Toolbar *This);

		std::map<void *, Handler *> ButtonMap;
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
class ColorButton : public Widget
{
	public:
		ColorButton(const Color &InitialColor, bool SelectAlpha);
		virtual ~ColorButton(void);

		Color GetColor(void);
		void SetColor(const Color &NewColor);

	protected:
		virtual void OnSelect(const Color &NewColor);

	private:
		bool Alpha;
		static void HandleSelect(GtkWidget *Widget, ColorButton *This);
};

#endif
