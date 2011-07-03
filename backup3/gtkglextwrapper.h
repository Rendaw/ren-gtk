#ifndef gtkglextwrapper_h
#define gtkglextwrapper_h

#include <gtk/gtkgl.h>
#include <general/lifetime.h>
#include "gtkwrapper.h"
#include <video/video.h>
#include <video/texture.h>

class ImageData : public Texture, public Good<String, ImageData>
{
	public: 
		ImageData(const String &Filename);
		void Draw(FlatVector Position, bool Centered = false);
		
		const FlatVector &GetSize(void);
	private: 
		FlatVector Size;
		FlatVector Corners[4];
};

typedef Access<ImageData> Image;

class RenderingArea : public Widget
{
	public:
		RenderingArea(void);
		virtual ~RenderingArea(void);

		void Refresh(void);
		void RefreshOnEvents(bool On);

		void SetFixedScale(const float &NewFixedScale); // Size of "1"
		void SetMinimumAxis(const float &NewMinimumAxis); // Minimum width/height

		Image *LoadImage(const String &Filename);

		float GetScale(void);
		FlatVector GetCursor(void);

		class ContextTag
		{
			protected:
				friend class RenderingArea;
				void Initialize(GtkWidget *Widget);
				~ContextTag(void);
			public:
				void Bind(void);
				void Release(void);

				int GetReferences(void);

			private:
				void Swap(void);

			private:
				GtkWidget *Data;
				int References;

				GdkGLDrawable *Drawable;
		} DrawContext;

	protected:
		// Necessary handlers
		virtual void Setup(void);
		virtual void Render(void);

		// Mouse handlers
		virtual void ClickEvent(const FlatVector &Cursor);
		virtual void DeclickEvent(const FlatVector &Cursor);
		virtual void MoveEvent(const FlatVector &Cursor);

	private:
		bool EventRefresh;

		bool FixedScale;
		float MinimumAxis, Scale;

		FlatVector Translate;

		Pool<String, ImageData> Images;

		void InternalResizeHandler(float NewWidth, float NewHeight);

		static void SetupHandler(GtkWidget *Widget, RenderingArea *This);
		static gboolean ResizeHandler(GtkWidget *Widget, GdkEventConfigure *Event, RenderingArea *This);
		static gboolean DrawHandler(GtkWidget *Widget, GdkEventExpose *Event, RenderingArea *This);
		static gboolean ClickHandler(GtkWidget *Widget, GdkEventButton *Event, RenderingArea *This);
		static gboolean DeclickHandler(GtkWidget *Widget, GdkEventButton *Event, RenderingArea *This);
		static gboolean MoveHandler(GtkWidget *Widget, GdkEventMotion *Event, RenderingArea *This);
};

#endif
