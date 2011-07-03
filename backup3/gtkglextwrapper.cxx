#include "gtkglextwrapper.h"

#include <GL/glew.h>
#include <GL/gl.h>

#include <gtk/gtkgl.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <video/videoaux.h>

// A class for managing the image context
ImageData::ImageData(const String &Filename) : Texture(), Good<String, ImageData>(),
	Size()
{
	// Make a pixbuf of the texture first
	GdkPixbuf *NewPixbuf = NULL;
	if (!Filename.empty())
	{
		GError *PixbufError = NULL;
		NewPixbuf = gdk_pixbuf_new_from_file(Filename.c_str(), &PixbufError);
		if (NewPixbuf == NULL)
		{
			g_print("Error loading rendering area image %s: %s\n",
				Filename.c_str(), PixbufError->message);
			return;
		}
	}
	if (NewPixbuf == NULL) return;

	// Check the data
	assert(gdk_pixbuf_get_colorspace(NewPixbuf) == GDK_COLORSPACE_RGB);
	assert(gdk_pixbuf_get_bits_per_sample(NewPixbuf) == 8);
	//assert(gdk_pixbuf_get_has_alpha(pixbuf));

	unsigned int Channels = gdk_pixbuf_get_n_channels(NewPixbuf);
	assert(Channels >= 3);
	assert(gdk_pixbuf_get_bits_per_sample(NewPixbuf) == 8);

	FlatVector PixelSize(gdk_pixbuf_get_width(NewPixbuf), gdk_pixbuf_get_height(NewPixbuf));

	// Copy the data to a new array, cleaning out the junk data
	unsigned char *PreData = gdk_pixbuf_get_pixels(NewPixbuf);
	int Stride = gdk_pixbuf_get_rowstride(NewPixbuf);

	unsigned char *NewData = (unsigned char *)malloc((int)(PixelSize[0] * PixelSize[1] * Channels * sizeof(unsigned char)));

	assert(sizeof(unsigned char) == sizeof(guchar));
	for (int CurrentRow = 0; CurrentRow < PixelSize[1]; CurrentRow++)
	{
		memcpy(
			NewData + (int)PixelSize[0] * CurrentRow * Channels,
			PreData + Stride * CurrentRow,
			Channels * (int)PixelSize[0]);
	}

	// Extract the data, create the GL texture
	unsigned int NewTexture;
	glGenTextures(1, &NewTexture);
	glBindTexture(GL_TEXTURE_2D, NewTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, Channels,
		(int)PixelSize[0], (int)PixelSize[1], 0,
		(Channels == 3 ? GL_RGB : GL_RGBA), GL_UNSIGNED_BYTE, NewData);
		
	free(NewData);

	// Error the heck out
	unsigned int ErrorID;
	switch (ErrorID = glGetError())
	{
		case 0: break;
		case GL_INVALID_VALUE: g_print("Error creating texture - check the texture size?  Does your card support NPOT textures?\n"); break;
		default: g_print("Error creating texture; unknown cause. (%u)\n", ErrorID); assert(false); break;
	}

	// Set the mad props
	Size = PixelSize;
	
	FlatVector HalfTexel = 0.5f * FlatVector(1, 1) / PixelSize;
	
	Corners[0] = FlatVector(0 + HalfTexel[0], 0 + HalfTexel[1]);
	Corners[1] = FlatVector(1 - HalfTexel[0], 0 + HalfTexel[1]);
	Corners[2] = FlatVector(1 - HalfTexel[0], 1 - HalfTexel[1]);
	Corners[3] = FlatVector(0 + HalfTexel[0], 1 - HalfTexel[1]);
	
	SetID(NewTexture);
}

void ImageData::Draw(FlatVector Position, bool Centered)
{
	if (IsBroken()) return;

	if (Centered) Position -= Size * 0.5f;

	Bind();
	vaDrawQuads();
		UV(Corners[0]); vaVertex(Position);
		UV(Corners[1]); vaVertex(Position + FlatVector(Size[0], 0));
		UV(Corners[2]); vaVertex(Position + FlatVector(Size[0], Size[1]));
		UV(Corners[3]); vaVertex(Position + FlatVector(0, Size[1]));
	vaEndDrawing();
	Unbind();
}

const FlatVector &ImageData::GetSize(void)
	{ return Size; }

// A class for managing the drawing context
void RenderingArea::ContextTag::Initialize(GtkWidget *Widget)
	{ Data = Widget; References = 0; }

RenderingArea::ContextTag::~ContextTag(void)
	{ assert(References == 0); }

void RenderingArea::ContextTag::Bind(void)
{
	References++;

	if (References == 1)
	{
		GdkGLContext *Context = gtk_widget_get_gl_context(Data);
		Drawable = gtk_widget_get_gl_drawable(Data);

		bool Result = gdk_gl_drawable_gl_begin(Drawable, Context);
		assert(Result);
	}
}

void RenderingArea::ContextTag::Release(void)
{
	assert(References > 0);
	References--;

	if (References == 0)
	{
		gdk_gl_drawable_gl_end(Drawable);
	}
}

int RenderingArea::ContextTag::GetReferences(void)
	{ return References; }

void RenderingArea::ContextTag::Swap(void)
{
	if (gdk_gl_drawable_is_double_buffered(Drawable))
		gdk_gl_drawable_swap_buffers (Drawable);
	else glFlush();
}

// The GL drawing area widget
RenderingArea::RenderingArea(void) : Widget(gtk_drawing_area_new()),
	EventRefresh(false), FixedScale(false), MinimumAxis(1.0f), Scale(1.0f)
{
	GdkGLConfig *GLConfig = gdk_gl_config_new_by_mode
		(static_cast<GdkGLConfigMode>(GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_ALPHA | GDK_GL_MODE_DOUBLE));

	gtk_widget_set_size_request(Data, 400, 400);
	gtk_widget_set_gl_capability(Data, GLConfig, NULL, true, GDK_GL_RGBA_TYPE);
	gtk_widget_add_events(Data, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
	g_signal_connect_after(G_OBJECT(Data), "realize", G_CALLBACK(SetupHandler), this);
	g_signal_connect(G_OBJECT(Data), "configure_event", G_CALLBACK(ResizeHandler), this);
	g_signal_connect(G_OBJECT(Data), "expose_event", G_CALLBACK(DrawHandler), this);
	g_signal_connect(G_OBJECT(Data), "button_press_event", G_CALLBACK(ClickHandler), this);
	g_signal_connect(G_OBJECT(Data), "button_release_event", G_CALLBACK(DeclickHandler), this);
	g_signal_connect(G_OBJECT(Data), "motion_notify_event", G_CALLBACK(MoveHandler), this);

	DrawContext.Initialize(Data);
}

RenderingArea::~RenderingArea(void)
{
}

void RenderingArea::Refresh(void)
{
	gdk_window_invalidate_rect(Data->window, &Data->allocation, false);
	gdk_window_process_updates(Data->window, false);
}

void RenderingArea::RefreshOnEvents(bool On)
{
	EventRefresh = On;
}

void RenderingArea::SetFixedScale(const float &NewFixedScale)
{
	FixedScale = true;
	Scale = 1.0f / NewFixedScale;
}

void RenderingArea::SetMinimumAxis(const float &NewMinimumAxis)
{
	FixedScale = false;
	MinimumAxis = NewMinimumAxis;
}

Image *RenderingArea::LoadImage(const String &Filename)
{
	// This is a decent place to put it, the texture purging.
	// Some texture loading goes with texture deleting, and
	// time isn't made out of texture deleting, you know.
	Images.Prune();
	
	return Images.Get(Filename);
}

float RenderingArea::GetScale(void)
{
	return Scale;
}

FlatVector RenderingArea::GetCursor(void)
{
	int PreX, PreY;
	gtk_widget_get_pointer(Data, &PreX, &PreY);
	return FlatVector(PreX, PreY) * Scale + Translate;
}

void RenderingArea::Setup(void) {}
void RenderingArea::Render(void) {}
void RenderingArea::ClickEvent(const FlatVector &Location) {}
void RenderingArea::DeclickEvent(const FlatVector &Location) {}
void RenderingArea::MoveEvent(const FlatVector &Location) {}

void RenderingArea::InternalResizeHandler(float NewWidth, float NewHeight)
{
	glViewport(0, 0, (int)NewWidth, (int)NewHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (!FixedScale)
	{
		// Calculate new scale by view area
		if (NewWidth > NewHeight)
		{
			float AspectRatio = NewWidth / NewHeight;
			glOrtho(
				-MinimumAxis * 0.5f * AspectRatio,
				MinimumAxis * 0.5f * AspectRatio,
				MinimumAxis * 0.5f,
				-MinimumAxis * 0.5f, -5, 5);

			Scale = MinimumAxis / NewHeight;
		}
		else
		{
			float AspectRatio = NewHeight / NewWidth;
			glOrtho(
				-MinimumAxis * 0.5f,
				MinimumAxis * 0.5f,
				MinimumAxis * 0.5f * AspectRatio,
				-MinimumAxis * 0.5f * AspectRatio, -5, 5);

			Scale = MinimumAxis / NewWidth;
		}
	}
	else
	{
		// Scale already set, but set the view transformation.
		glOrtho(
			-NewWidth * 0.5f * Scale,
			NewWidth * 0.5f * Scale,
			NewHeight * 0.5f * Scale,
			-NewHeight * 0.5f * Scale, -5, 5);
	}

	// Update offset to screen center based on new scale
	Translate = FlatVector(-NewWidth * Scale * 0.5f, -NewHeight * Scale * 0.5f);

	glMatrixMode(GL_MODELVIEW);
}

void RenderingArea::SetupHandler(GtkWidget *Widget, RenderingArea *This)
{
	This->DrawContext.Bind();
	
	GLenum GLEWResult = glewInit();
	if(GLEWResult != GLEW_OK)
	{
		std::cout << "ERROR: GLEW initialization failed with message, \"" << glewGetErrorString(GLEWResult) << "\"" << std::endl;
		exit(1);
	}
	
	This->Setup();
	This->DrawContext.Release();
}

gboolean RenderingArea::ResizeHandler(GtkWidget *Widget, GdkEventConfigure *Event, RenderingArea *This)
{
	This->DrawContext.Bind();
	This->InternalResizeHandler(Widget->allocation.width, Widget->allocation.height);
	This->DrawContext.Release();

	return true;
}

gboolean RenderingArea::DrawHandler(GtkWidget *Widget, GdkEventExpose *Event, RenderingArea *This)
{
	This->DrawContext.Bind();

	// Do drawing
	This->Render();

	This->DrawContext.Swap();
	This->DrawContext.Release();

	return true;
}

gboolean RenderingArea::ClickHandler(GtkWidget *Widget, GdkEventButton *Event, RenderingArea *This)
{
	This->ClickEvent(FlatVector(Event->x, Event->y) * This->Scale + This->Translate);

	if (This->EventRefresh)
		This->Refresh();

	return false;
}

gboolean RenderingArea::DeclickHandler(GtkWidget *Widget, GdkEventButton *Event, RenderingArea *This)
{
	This->DeclickEvent(FlatVector(Event->x, Event->y) * This->Scale + This->Translate);

	if (This->EventRefresh)
		This->Refresh();

	return false;
}

gboolean RenderingArea::MoveHandler(GtkWidget *Widget, GdkEventMotion *Event, RenderingArea *This)
{
	This->MoveEvent(FlatVector(Event->x, Event->y) * This->Scale + This->Translate);
	//	Event->state &
	//         ^ for mouse buttonz, maybe laterz?

	if (This->EventRefresh)
		This->Refresh();

	return false;
}
