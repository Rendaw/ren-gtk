#include "gtkcairowrapper.h"

#include <iostream>

FontData::FontData(const std::pair<int, VectorArea *> &Data) :
	PangoFont(pango_font_description_new()), Canvas(Data.second)
{
	pango_font_description_set_family(PangoFont, "sans");
	pango_font_description_set_absolute_size(PangoFont, Data.first * PANGO_SCALE);
}

FontData::~FontData(void)
	{ pango_font_description_free(PangoFont); }

void FontData::Print(String const &Text, TextAlignment Alignment, FlatVector const &Position)
{
	PangoLayout *Layout = pango_cairo_create_layout(Canvas->GetContext());
	pango_layout_set_font_description(Layout, PangoFont);
	pango_layout_set_text(Layout, Text.c_str(), -1);

	int Height, Width;
	pango_layout_get_size(Layout, &Width, &Height);
	FlatVector Size = FlatVector(Width, Height) / PANGO_SCALE;
	if (Alignment == taLeft)
		cairo_move_to(Canvas->GetContext(), Position[0], Position[1]);
	else if (Alignment == taMiddle)
		cairo_move_to(Canvas->GetContext(), Position[0] - Size[0] * 0.5f, Position[1]);
	else if (Alignment == taFullMiddle)
		cairo_move_to(Canvas->GetContext(), Position[0] - Size[0] * 0.5f, Position[1] - Size[1] * 0.5f);
	else cairo_move_to(Canvas->GetContext(), Position[0] - Size[0], Position[1]);

	pango_cairo_show_layout(Canvas->GetContext(), Layout);
	g_object_unref(Layout);
}
void FontData::Print(String const &Text, TextAlignment Alignment, Region const &Limits, bool Wrap)
{
	PangoLayout *Layout = pango_cairo_create_layout(Canvas->GetContext());
	pango_layout_set_font_description(Layout, PangoFont);
	pango_layout_set_text(Layout, Text.c_str(), -1);
	//
	pango_layout_set_width(Layout, Limits.Size[0] * PANGO_SCALE);
	pango_layout_set_alignment(Layout,
		Alignment == taLeft ? PANGO_ALIGN_LEFT : Alignment == taRight ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_CENTER);
	if (Wrap) pango_layout_set_wrap(Layout, PANGO_WRAP_WORD);
	else pango_layout_set_ellipsize(Layout, PANGO_ELLIPSIZE_END);

	/*int Height, Width;
	pango_layout_get_size(Layout, &Width, &Height);
	FlatVector Size = FlatVector(Width, Height) / PANGO_SCALE;
	if (Size[0] > Limits[0])
	{
		pango_layout_set_width(Layout, Limits[0] * PANGO_SCALE);
		pango_layout_set_ellipsize(Layout, PANGO_ELLIPSIZE_END);
		Size[0] = Limits[0];
	}*/
	//if (Alignment == taLeft)
		cairo_move_to(Canvas->GetContext(), Limits.Start[0], Limits.Start[1]);
	/*else if (Alignment == taMiddle)
		cairo_move_to(Canvas->GetContext(), Position[0] - Size[0] * 0.5f, Position[1] - Size[1] * 0.5f);
	else cairo_move_to(Canvas->GetContext(), Position[0] - Size[0], Position[1]);*/

	pango_cairo_show_layout(Canvas->GetContext(), Layout);
	g_object_unref(Layout);
}

VectorArea::VectorArea(void) :
	Left(false), Middle(false), Right(false),
	Data(gtk_drawing_area_new()),
	FillOn(false), Roundedness(3.0f),
	CairoContext(NULL), ShouldPartialRefresh(false)
{
	gtk_widget_set_can_focus(Data, true);
	gtk_widget_add_events(Data, 
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
		GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
	g_signal_connect(G_OBJECT(Data), "configure-event", G_CALLBACK(ResizeHandler), this);
	g_signal_connect(G_OBJECT(Data), "expose-event", G_CALLBACK(DrawHandler), this);
	g_signal_connect(G_OBJECT(Data), "button-press-event", G_CALLBACK(ClickHandler), this);
	g_signal_connect(G_OBJECT(Data), "button-release-event", G_CALLBACK(DeclickHandler), this);
	g_signal_connect(G_OBJECT(Data), "motion-notify-event", G_CALLBACK(MoveHandler), this);
	g_signal_connect(G_OBJECT(Data), "scroll-event", G_CALLBACK(ScrollHandler), this);
	g_signal_connect(G_OBJECT(Data), "enter-notify-event", G_CALLBACK(EnterHandler), this);
	g_signal_connect(G_OBJECT(Data), "leave-notify-event", G_CALLBACK(LeaveHandler), this);
}

VectorArea::~VectorArea(void)
	{}

VectorArea::operator GtkWidget*(void)
	{ return Data; }

void VectorArea::Refresh(void)
{
	if (!GDK_IS_WINDOW(Data->window)) return;
	GdkRectangle Region;
	Region.x = 0;
	Region.y = 0;
	Region.width = Data->allocation.width;
	Region.height = Data->allocation.height;
	gdk_window_invalidate_rect(Data->window, &Region, false);
	gdk_window_process_updates(Data->window, false);
}

void VectorArea::RedrawArea(const FlatVector &Position, const FlatVector &Size)
{
	GdkRectangle Region;
	Region.x = Position[0];
	Region.y = Position[1];
	Region.width = Size[0];
	Region.height = Size[1];
	gdk_window_invalidate_rect(Data->window, &Region, false);
	ShouldPartialRefresh = true;
}

void VectorArea::PartialRefresh(void)
{
	if (!ShouldPartialRefresh) return;
	gdk_window_process_updates(Data->window, false);
	ShouldPartialRefresh = false;
}

void VectorArea::RequestSize(const FlatVector &Size)
	{ gtk_widget_set_size_request(Data, Size[0], Size[1]); }

void VectorArea::SetBackgroundColor(const Color &NewColor)
{
	GdkColor SetColor;
	SetColor.red = 65535 * NewColor.Red;
	SetColor.green = 65535 * NewColor.Green;
	SetColor.blue = 65535 * NewColor.Blue;
	gtk_widget_modify_bg(Data, GTK_STATE_NORMAL, &SetColor);
}

void VectorArea::Translate(const FlatVector &Translation)
	{ cairo_translate(CairoContext, Translation[0], Translation[1]); }

void VectorArea::SetColor(const Color &NewColor)
	{ cairo_set_source_rgba(CairoContext, NewColor.Red, NewColor.Green, NewColor.Blue, NewColor.Alpha); }
void VectorArea::SetWidth(float Width)
	{ cairo_set_line_width(CairoContext, Width); }
void VectorArea::SetCap(bool Rounded)
	{ cairo_set_line_cap(CairoContext, Rounded ? CAIRO_LINE_CAP_ROUND : CAIRO_LINE_CAP_BUTT); }
void VectorArea::SetFill(bool On)
	{ FillOn = On; }
void VectorArea::SetRoundedness(float Roundedness)
	{ this->Roundedness = Roundedness; }

void VectorArea::SetFontSize(unsigned int NewSize)
	{ cairo_set_font_size(CairoContext, NewSize); }

float VectorArea::GetTextWidth(const String &Text)
{
	bool WasNull = false;
	cairo_surface_t *TempSurface = NULL;
	if (CairoContext == NULL)
	{
		WasNull = true;
		TempSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
		CairoContext = cairo_create(TempSurface);
		cairo_set_font_size(CairoContext, 12);
	}

	cairo_text_extents_t Extents;
	cairo_text_extents(CairoContext, Text.c_str(), &Extents);

	if (WasNull)
	{
		cairo_destroy(CairoContext);
		cairo_surface_destroy(TempSurface);
		CairoContext = NULL;
	}

	return Extents.width;
}

void VectorArea::PrintNumber(float Number, TextAlignment Alignment, const FlatVector &Position)
{
	StringStream ToText;
	ToText << Number;
	Print(ToText.str(), Alignment, Position);
}

void VectorArea::PrintHex(unsigned int Number, TextAlignment Alignment, const FlatVector &Position)
{
	StringStream ToText;
	ToText << std::hex << Number;
	Print(ToText.str(), Alignment, Position);
}

void VectorArea::Print(String const &Text, TextAlignment Alignment, FlatVector const &Position)
{
	cairo_text_extents_t Extents;
	cairo_text_extents(CairoContext, Text.c_str(), &Extents);
	cairo_font_extents_t FontExtents;
	cairo_font_extents(CairoContext, &FontExtents);

	FlatVector Start = Position + FlatVector(-Extents.x_bearing, FontExtents.ascent - FontExtents.descent);
	if (Alignment == taMiddle)
		Start[0] -= Extents.width * 0.5f;
	else if (Alignment == taFullMiddle)
		Start += FlatVector(-Extents.width * 0.5f, -FontExtents.ascent * 0.5f);
	else if (Alignment == taRight)
		Start[0] -= Extents.width - Extents.x_bearing;

	cairo_move_to(CairoContext, Start[0], Start[1]);
	cairo_show_text(CairoContext, Text.c_str());
}

void VectorArea::Print(String const &Text, FlatVector const &Alignment, FlatVector const &Position)
{
	cairo_text_extents_t Extents;
	cairo_text_extents(CairoContext, Text.c_str(), &Extents);
	cairo_font_extents_t FontExtents;
	cairo_font_extents(CairoContext, &FontExtents);

	FlatVector Size(Extents.width, FontExtents.descent + FontExtents.ascent * 0.9f);

	FlatVector Start = Position +
		FlatVector(-Extents.x_bearing, -FontExtents.descent) +
		Size * (FlatVector(-0.5f, 0.5f) + 0.5f * Alignment);

	cairo_move_to(CairoContext, Start[0], Start[1]);
	cairo_show_text(CairoContext, Text.c_str());
}

void VectorArea::DrawLine(const FlatVector &Start, const FlatVector &End)
{
	cairo_move_to(CairoContext, Start[0], Start[1]);
	cairo_line_to(CairoContext, End[0], End[1]);
	cairo_stroke(CairoContext);
}

void VectorArea::DrawRectangle(const FlatVector &Start, const FlatVector &Size)
{
	cairo_rectangle(CairoContext, Start[0], Start[1], Size[0], Size[1]);
	if (FillOn) cairo_fill(CairoContext);
	else cairo_stroke(CairoContext);
}

void VectorArea::DrawRoundedRectangle(const FlatVector &Start, const FlatVector &Size)
{
	if ((Size[0] <= Roundedness) || (Size[1] <= Roundedness))
	{
		cairo_move_to(CairoContext, Start[0], Start[1]);
		cairo_line_to(CairoContext, Start[0] + Size[0], Start[1]);
		cairo_line_to(CairoContext, Start[0] + Size[0], Start[1] + Size[1]);
		cairo_line_to(CairoContext, Start[0], Start[1] + Size[1]);
	}
	else
	{
		FlatVector ArcCenters[4] =
		{
			FlatVector(Start[0], Start[1]),
			FlatVector(Start[0] + Size[0], Start[1]),
			FlatVector(Start[0] + Size[0], Start[1] + Size[1]),
			FlatVector(Start[0], Start[1] + Size[1])
		};
		ArcCenters[0] += FlatVector(Roundedness, Roundedness);
		ArcCenters[1] += FlatVector(-Roundedness, Roundedness);
		ArcCenters[2] += FlatVector(-Roundedness, -Roundedness);
		ArcCenters[3] += FlatVector(Roundedness, -Roundedness);

		// Top
		cairo_move_to(CairoContext, ArcCenters[0][0],  Start[1]);
		cairo_line_to(CairoContext, ArcCenters[1][0],  Start[1]);
		cairo_arc(CairoContext, ArcCenters[1][0], ArcCenters[1][1], Roundedness, 1.5f * Pi, 0);

		// Right
		cairo_line_to(CairoContext, Start[0] + Size[0],  ArcCenters[2][1]);
		cairo_arc(CairoContext, ArcCenters[2][0], ArcCenters[2][1], Roundedness, 0, 0.5f * Pi);

		// Bottom
		cairo_line_to(CairoContext, ArcCenters[3][0],  Start[1] + Size[1]);
		cairo_arc(CairoContext, ArcCenters[3][0], ArcCenters[3][1], Roundedness, 0.5f * Pi, Pi);

		// Left
		cairo_line_to(CairoContext, Start[0],  ArcCenters[0][1]);
		cairo_arc(CairoContext, ArcCenters[0][0], ArcCenters[0][1], Roundedness, Pi, 1.5f * Pi);
	}

	if (FillOn)
		cairo_fill(CairoContext);
	else cairo_stroke(CairoContext);
}

void VectorArea::DrawCircle(const FlatVector &Position, float Radius)
{
	cairo_arc(CairoContext, Position[0], Position[1], Radius, 0, 2.0f * Pi);
	if (FillOn) cairo_fill(CairoContext);
	else cairo_stroke(CairoContext);
}

void VectorArea::DrawArc(const FlatVector &Position, float Radius, float AngleStart, float AngleEnd)
{
	cairo_arc(CairoContext, Position[0], Position[1], Radius,
		AngleStart / 180.0f * Pi, AngleEnd / 180.0f * Pi);
	if (FillOn) cairo_fill(CairoContext);
	else cairo_stroke(CairoContext);
}

void VectorArea::DrawImage(const String &Filename, const FlatVector &Position, bool Centered)
{
	// Get the image and image data
	cairo_surface_t *Data =
		cairo_image_surface_create_from_png(Filename.c_str());
	if (cairo_surface_status(Data) != CAIRO_STATUS_SUCCESS) return;
	FlatVector Size = FlatVector(
		cairo_image_surface_get_width(Data),
		cairo_image_surface_get_height(Data));

	// Figure out the draw position
	FlatVector Corner = Position;
	if (Centered) Corner -= Size * 0.5f;

	// Do the drawing
	cairo_set_source_surface(CairoContext, Data, Corner[0], Corner[1]);
	cairo_rectangle(CairoContext, Corner[0], Corner[1], Size[0], Size[1]);
	cairo_fill(CairoContext);

	// Clean up
	cairo_surface_destroy(Data);
}

void VectorArea::DrawImage(const String &Filename, const FlatVector &Position, Angle Rotation)
{
	// Get the image and image data
	cairo_surface_t *Data =
		cairo_image_surface_create_from_png(Filename.c_str());
	if (cairo_surface_status(Data) != CAIRO_STATUS_SUCCESS) return;
	FlatVector Size = FlatVector(
		cairo_image_surface_get_width(Data),
		cairo_image_surface_get_height(Data));

	// Do the drawing
	cairo_matrix_t PreMatrix;
	cairo_get_matrix(CairoContext, &PreMatrix);

	cairo_translate(CairoContext, Position[0], Position[1]);
	cairo_rotate(CairoContext, Rotation * ToRadians);
	cairo_set_source_surface(CairoContext, Data, -Size[0] * 0.5f, -Size[1] * 0.5f);
	cairo_paint(CairoContext);

	cairo_set_matrix(CairoContext, &PreMatrix);

	// Clean up
	cairo_surface_destroy(Data);
}

Font *VectorArea::GetFont(int Size)
{
	Prune();
	return Get(std::pair<int, VectorArea *>(Size, this));
}

FlatVector VectorArea::GetSize(void)
{
	return FlatVector(Data->allocation.width, Data->allocation.height);
}

void VectorArea::ResizeEvent(FlatVector const &NewSize) {}
void VectorArea::Draw(void) {}
void VectorArea::ClickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged) {}
void VectorArea::DeclickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged) {}
void VectorArea::ScrollEvent(FlatVector const &Cursor, int VerticalScroll, int HorizontalScroll) {}
void VectorArea::MoveEvent(FlatVector const &Cursor) {}
void VectorArea::EnterEvent(void) {}
void VectorArea::LeaveEvent(void) {}

cairo_t *VectorArea::GetContext(void)
	{ return CairoContext; }

void VectorArea::DrawInternal(int X, int Y, int Width, int Height)
{
	CairoContext = gdk_cairo_create(Data->window);
	//cairo_select_font_face(CairoContext, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(CairoContext, 12);
	cairo_rectangle(CairoContext, X, Y, Width, Height);
        cairo_clip(CairoContext);

	Draw();

	cairo_destroy(CairoContext);
	CairoContext = NULL;
}

gboolean VectorArea::ResizeHandler(GtkWidget *Widget, GdkEventConfigure *Event, VectorArea *This)
{
	This->ResizeEvent(FlatVector(Widget->allocation.width, Widget->allocation.height));
	return true;
}

gboolean VectorArea::DrawHandler(GtkWidget *Widget, GdkEventExpose *Event, VectorArea *This)
{
	This->DrawInternal(Event->area.x, Event->area.y, Event->area.width, Event->area.height);
	return true;
}

gboolean VectorArea::ClickHandler(GtkWidget *Widget, GdkEventButton *Event, VectorArea *This)
{
	if (Event->type != GDK_BUTTON_PRESS) return false;
	if (Event->button == 1) This->Left = true;
	if (Event->button == 2) This->Middle = true;
	if (Event->button == 3) This->Right = true;

	This->ClickEvent(FlatVector(Event->x, Event->y), Event->button == 1, Event->button == 2, Event->button == 3);

	return false;
}

gboolean VectorArea::DeclickHandler(GtkWidget *Widget, GdkEventButton *Event, VectorArea *This)
{
	gtk_widget_grab_focus(Widget);
	if (Event->button == 1) This->Left = false;
	if (Event->button == 2) This->Middle = false;
	if (Event->button == 3) This->Right = false;

	This->DeclickEvent(FlatVector(Event->x, Event->y),
		Event->button == 1, Event->button == 2, Event->button == 3);

	return false;
}

gboolean VectorArea::ScrollHandler(GtkWidget *Widget, GdkEventScroll *Event, VectorArea *This)
{
	This->ScrollEvent(FlatVector(Event->x, Event->y),
		(Event->direction == GDK_SCROLL_DOWN) ? 1 : (Event->direction == GDK_SCROLL_UP) ? -1 : 0, 
		(Event->direction == GDK_SCROLL_RIGHT) ? 1 : (Event->direction == GDK_SCROLL_LEFT) ? -1 : 0);
	
	return false;
}

gboolean VectorArea::MoveHandler(GtkWidget *Widget, GdkEventMotion *Event, VectorArea *This)
{
	This->MoveEvent(FlatVector(Event->x, Event->y));
	//	Event->state &
	//         ^ for mouse buttonz, maybe laterz?

	return false;
}

gboolean VectorArea::EnterHandler(GtkWidget *Widget, GdkEventCrossing *Event, VectorArea *This)
	{ This->EnterEvent(); return false; }

gboolean VectorArea::LeaveHandler(GtkWidget *Widget, GdkEventCrossing *Event, VectorArea *This)
	{ This->LeaveEvent(); return false; }

// Slate
void Slate::SetResizeHandler(decltype(ResizeHandler) const &Handler) 
	{ ResizeHandler = Handler; }
	
void Slate::SetDrawHandler(decltype(DrawHandler) const &Handler) 
	{ DrawHandler = Handler; }
	
void Slate::SetClickHandler(decltype(ClickHandler) const &Handler)
	{ ClickHandler = Handler; }
	
void Slate::SetDeclickHandler(decltype(DeclickHandler) const &Handler)
	{ DeclickHandler = Handler; }
	
void Slate::SetScrollHandler(decltype(ScrollHandler) const &Handler)
	{ ScrollHandler = Handler; }
	
void Slate::SetMoveHandler(decltype(MoveHandler) const &Handler)
	{ MoveHandler = Handler; }
	
void Slate::SetEnterHandler(decltype(EnterHandler) const &Handler)
	{ EnterHandler = Handler; }
	
void Slate::SetLeaveHandler(decltype(LeaveHandler) const &Handler)
	{ LeaveHandler = Handler; }
	
void Slate::ResizeEvent(FlatVector const &NewSize)
	{ if (ResizeHandler) ResizeHandler(NewSize); }
	
void Slate::Draw(void)
	{ if (DrawHandler) DrawHandler(); }
	
void Slate::ClickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged)
	{ if (ClickHandler) ClickHandler(Cursor, LeftChanged, MiddleChanged, RightChanged); }

void Slate::DeclickEvent(FlatVector const &Cursor, bool LeftChanged, bool MiddleChanged, bool RightChanged)
	{ if (DeclickHandler) DeclickHandler(Cursor, LeftChanged, MiddleChanged, RightChanged); }
	
void Slate::ScrollEvent(FlatVector const &Cursor, int VerticalScroll, int HorizontalScroll)
	{ if (ScrollHandler) ScrollHandler(Cursor, VerticalScroll, HorizontalScroll); }
	
void Slate::MoveEvent(FlatVector const &Cursor)
	{ if (MoveHandler) MoveHandler(Cursor); }
	
void Slate::EnterEvent(void)
	{ if (EnterHandler) EnterHandler(); }
	
void Slate::LeaveEvent(void)
	{ if (LeaveHandler) LeaveHandler(); }
