#ifndef gtkcairowrapper_h
#define gtkcairowrapper_h

#include <functional>

#include "gtkwrapper.h"

#include <ren-general/rotation.h>
#include <ren-general/pool.h>
#include <ren-general/region.h>

enum TextAlignment { taLeft, taMiddle, taFullMiddle, taRight };

class VectorArea;
class FontData : public PoolItem<std::pair<int, VectorArea *>, FontData>
{
	public:
		FontData(const std::pair<int, VectorArea *> &Data);
		~FontData(void);

		void Print(String const &Text, TextAlignment Alignment, FlatVector const &Position);
		void Print(String const &Text, TextAlignment Alignment, Region const &Limits, bool Wrap = false);
	protected:
		friend class VectorArea;
		PangoFontDescription *PangoFont;

	private:
		VectorArea *Canvas;
};

typedef Access<FontData> Font;

class VectorArea : private Pool<std::pair<int, VectorArea *>, FontData> // Deprecated, use Slate below
{
	public:
		VectorArea(void);
		~VectorArea(void);
		operator GtkWidget*(void);

		void Refresh(void);
		void PartialRefresh(void);
		void RedrawArea(const FlatVector &Position, const FlatVector &Size);

		void RequestSize(const FlatVector &Size);
		void SetBackgroundColor(const Color &NewColor);

		// Cairo tools - valid only within Draw()
		void Translate(const FlatVector &Translation);
		void SetColor(const Color &NewColor);
		void SetWidth(float Width); // For line things
		void SetCap(bool Rounded);
		void SetFill(bool On); // For fillable things
		void SetRoundedness(float NewRoundedness); // ... For rounded things

		void SetFontSize(unsigned int NewSize);
		float GetTextWidth(const String &Text);
		void PrintNumber(float Number, TextAlignment Alignment, const FlatVector &Position);
		void PrintHex(unsigned int Number, TextAlignment Alignment, const FlatVector &Position);
		void Print(const String &Text, TextAlignment Alignment, const FlatVector &Position);
		void Print(const String &Text, const FlatVector &Alignment, const FlatVector &Position);
		void DrawLine(const FlatVector &Start, const FlatVector &End);

		void DrawRectangle(const FlatVector &Start, const FlatVector &Size);
		void DrawRoundedRectangle(const FlatVector &Start, const FlatVector &Size);
		void DrawCircle(const FlatVector &Position, float Radius);
		void DrawArc(const FlatVector &Position, float Radius, float AngleStart, float AngleEnd);

		void DrawImage(const String &Filename, const FlatVector &Position, bool Centered = true);
		void DrawImage(const String &Filename, const FlatVector &Position, Angle Rotation);

		// Pango text stuff - above is toy api
		Font *GetFont(int Size);

		// Queries
		FlatVector GetSize(void);

	protected:
		virtual void ResizeEvent(FlatVector const &NewSize);

		virtual void Draw(void);

		virtual void ClickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged);
		virtual void DeclickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged);
		virtual void ScrollEvent(FlatVector const &Cursor, int VerticalScroll, int HorizontalScroll);
		virtual void MoveEvent(FlatVector const &Cursor);
		virtual void EnterEvent(void);
		virtual void LeaveEvent(void);

		bool Left, Middle, Right;

		friend class FontData;
		cairo_t *GetContext(void);

	private:
		GtkWidget *Data;

		bool FillOn;
		float Roundedness;

		cairo_t *CairoContext; // Valid only within draw function
		bool ShouldPartialRefresh;

		void DrawInternal(int X, int Y, int Width, int Height);
		static gboolean ResizeHandler(GtkWidget *Widget, GdkEventConfigure *Event, VectorArea *This);
		static gboolean DrawHandler(GtkWidget *Widget, GdkEventExpose *Event, VectorArea *This);
		static gboolean ClickHandler(GtkWidget *Widget, GdkEventButton *Event, VectorArea *This);
		static gint ScrollHandler(GtkWidget *Widget, GdkEventScroll *Event, VectorArea *This);
		static gboolean DeclickHandler(GtkWidget *Widget, GdkEventButton *Event, VectorArea *This);
		static gboolean MoveHandler(GtkWidget *Widget, GdkEventMotion *Event, VectorArea *This);
		static gboolean EnterHandler(GtkWidget *Widget, GdkEventCrossing *Event, VectorArea *This);
		static gboolean LeaveHandler(GtkWidget *Widget, GdkEventCrossing *Event, VectorArea *This);
};

class Slate : public VectorArea
{
	private:
		std::function<void (FlatVector const &/*NewSize*/)> ResizeHandler;
		std::function<void (void)> DrawHandler;
		std::function<void (FlatVector const &/*Cursor*/, bool /*LeftChanged*/, bool /*MiddleChanged*/, bool /*RightChanged*/)> ClickHandler;
		std::function<void (FlatVector const &/*Cursor*/, bool /*LeftChanged*/, bool /*MiddleChanged*/, bool /*RightChanged*/)> DeclickHandler;
		std::function<void (FlatVector const &/*Cursor*/, int /*VerticalScroll*/, int /*HorizontalScroll*/)> ScrollHandler;
		std::function<void (FlatVector const &/*Cursor*/)> MoveHandler;
		std::function<void (void)> EnterHandler;
		std::function<void (void)> LeaveHandler;
	
	public:
		// Construction
		void SetResizeHandler(decltype(ResizeHandler) const &Handler);
		void SetDrawHandler(decltype(DrawHandler) const &Handler);
		void SetClickHandler(decltype(ClickHandler) const &Handler);
		void SetDeclickHandler(decltype(DeclickHandler) const &Handler);
		void SetScrollHandler(decltype(ScrollHandler) const &Handler);
		void SetMoveHandler(decltype(MoveHandler) const &Handler);
		void SetEnterHandler(decltype(EnterHandler) const &Handler);
		void SetLeaveHandler(decltype(LeaveHandler) const &Handler);
		
	private:
		void ResizeEvent(FlatVector const &NewSize);
		void Draw(void);
		void ClickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged);
		void DeclickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged);
		void ScrollEvent(FlatVector const &Cursor, int VerticalScroll, int HorizontalScroll);
		void MoveEvent(FlatVector const &Cursor);
		void EnterEvent(void);
		void LeaveEvent(void);
};

#endif
